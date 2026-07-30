// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ApplicationBootstrapper.h"

#include "GraphicsInitializer.h"
#include "LoggingInitializer.h"
#include "PlatformInitializer.h"

#include "sentinel/core/app/AppMetadata.h"
#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/platform/DpapiEncryptedSettingsStore.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/runtime/LocalInference.h"
#include "sentinel/core/app/ModeManager.h"
#include "sentinel/core/platform/WinProtocolHandler.h"
#include "sentinel/core/platform/WinTaskbarIntegration.h"
#include "sentinel/desktop/NativeCompanionAdapter.h"

#include <QCommandLineOption>
#include <QIcon>
#include <QNetworkProxy>
#include <QQmlContext>
#include <QQuickWindow>

namespace sentinel::desktop {

ApplicationBootstrapper::ApplicationBootstrapper(int argc, char* argv[], QObject* parent)
    : QObject(parent), m_argc(argc), m_argv(argv) {
    QGuiApplication::setApplicationName(sentinel::core::AppMetadata::displayName());
    QGuiApplication::setOrganizationName(sentinel::core::AppMetadata::organizationName());
    QGuiApplication::setApplicationVersion(sentinel::core::AppMetadata::version());
    QGuiApplication::setDesktopFileName(sentinel::core::AppMetadata::appId());
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/dev.sentinel.Sentinel.png")));

    m_parser.setApplicationDescription(QStringLiteral("Sentinel AI Desktop Assistant"));
    m_parser.addHelpOption();
    m_parser.addVersionOption();

    QCommandLineOption verboseOption(
        {QStringLiteral("verbose")},
        QCoreApplication::translate("main", "Enable verbose diagnostic logging in terminal."));
    m_parser.addOption(verboseOption);

    QCommandLineOption quietOption(
        {QStringLiteral("quiet")},
        QCoreApplication::translate("main", "Suppress diagnostic terminal output."));
    m_parser.addOption(quietOption);

    QCommandLineOption safeModeOption(
        {QStringLiteral("safe-mode")},
        QCoreApplication::translate("main", "Start with all extensions disabled and factory defaults."));
    m_parser.addOption(safeModeOption);

    if (QCoreApplication::instance()) {
        m_parser.process(*QCoreApplication::instance());
        m_verbose = m_parser.isSet(verboseOption);
        m_quiet = m_parser.isSet(quietOption);
        m_safeMode = m_parser.isSet(safeModeOption);
    }
}

ApplicationBootstrapper::~ApplicationBootstrapper() = default;

bool ApplicationBootstrapper::ensureSingleInstance() {
    return m_singleInstanceGuard.tryLockAndSetupIpc();
}

void ApplicationBootstrapper::initializeLogging() {
    configureLogging(m_verbose, m_quiet, m_pathProvider.logDirectoryPath());
}

void ApplicationBootstrapper::initializeGraphics() {
    configureGraphicsBackend();
}

void ApplicationBootstrapper::initializePlatformIntegrations() {
    sentinel::desktop::initializePlatformIntegrations(m_pathProvider.crashDumpDirectoryPath());

    if (m_safeMode) {
        qInfo().noquote() << "Safe mode enabled: using factory defaults.";
    }

    configureDefaultUiFont();
}

bool ApplicationBootstrapper::setupQmlEngine(QApplication& app) {
    app.setQuitOnLastWindowClosed(false);

    m_settings = std::make_unique<sentinel::core::AppSettings>(
        std::make_unique<sentinel::core::DpapiEncryptedSettingsStore>(
            std::make_unique<sentinel::core::JsonSettingsStore>(m_pathProvider.settingsFilePath())));

    installStartupTranslator(app, *m_settings, m_translator);

    if (m_settings->proxyEnabled()) {
        QNetworkProxy proxy;
        const QString type = m_settings->proxyType().toLower();
        if (type == QStringLiteral("socks5")) {
            proxy.setType(QNetworkProxy::Socks5Proxy);
        } else {
            proxy.setType(QNetworkProxy::HttpProxy);
        }
        proxy.setHostName(m_settings->proxyHost());
        proxy.setPort(m_settings->proxyPort());
        if (!m_settings->proxyUser().isEmpty()) {
            proxy.setUser(m_settings->proxyUser());
            proxy.setPassword(m_settings->proxyPassword());
        }
        QNetworkProxy::setApplicationProxy(proxy);
        qInfo().noquote()
            << "Proxy enabled:" << type << m_settings->proxyHost() << QString::number(m_settings->proxyPort());
    }

    // Clean dependency injection using ApplicationControllerBuilder
    sentinel::core::ApplicationControllerBuilder builder;
    m_controller = builder.withStandardDefaults(m_pathProvider, *m_settings).build();
    m_controller->setConversationExportDirectory(m_pathProvider.conversationExportDirectoryPath());

    m_modeManager = std::make_unique<sentinel::core::ModeManager>();
    m_controller->setRoutingModeByName(m_settings->routingModeName());

    m_taskbarIntegration = std::make_unique<sentinel::core::WinTaskbarIntegration>();

    m_shellViewModel = std::make_unique<DesktopShellViewModel>(
        *m_controller, *m_modeManager, *m_settings, m_taskbarIntegration.get());

    m_singleInstanceGuard.bindShellViewModel(m_shellViewModel.get());

    const QString ownUrl = sentinel::core::extractSentinelUrl(QCoreApplication::arguments());
    if (!ownUrl.isEmpty()) {
        qInfo().noquote() << "Deep link from command line:" << ownUrl;
    }

    auto* ollamaPuller = new OllamaModelPuller(this);
    QObject::connect(ollamaPuller, &OllamaModelPuller::activeModelChanged, m_shellViewModel.get(),
                     [ollamaPuller, this]() {
                         const QString active = ollamaPuller->activeModel();
                         if (ollamaPuller->pulling() && !active.isEmpty()) {
                             m_shellViewModel->addNotification(
                                 QStringLiteral("Models"), QStringLiteral("Downloading Model"),
                                 QStringLiteral("Retrieving '%1' from registry. You can monitor "
                                                "progress in the modelfiles panel.")
                                     .arg(active));
                         }
                     });
    QObject::connect(
        ollamaPuller, &OllamaModelPuller::pullFinished, m_controller.get(),
        [this](const QString& modelId, bool success) {
            if (success) {
                m_controller->refreshOllamaStatus();
                m_shellViewModel->addNotification(
                    QStringLiteral("Models"), QStringLiteral("Model Installed"),
                    QStringLiteral(
                        "'%1' has been successfully downloaded and is ready for local inference.")
                        .arg(modelId));
            } else {
                m_shellViewModel->addNotification(
                    QStringLiteral("Models"), QStringLiteral("Installation Failed"),
                    QStringLiteral("Could not retrieve '%1'. Please ensure your server is active "
                                   "and try again.")
                        .arg(modelId));
            }
        });
    QObject::connect(
        ollamaPuller, &OllamaModelPuller::removeFinished, m_controller.get(),
        [this](const QString& modelId, bool success) {
            if (success) {
                m_controller->refreshOllamaStatus();
                m_shellViewModel->addNotification(
                    QStringLiteral("Models"), QStringLiteral("Model Removed"),
                    QStringLiteral("'%1' has been deleted. Disk space has been reclaimed.")
                        .arg(modelId));
            }
        });

    auto* ollamaLibraryFetcher = new OllamaLibraryFetcher(this);
    auto* ollamaModelDetailFetcher = new OllamaModelDetailFetcher(this);
    auto* lmStudioLibraryFetcher = new LMStudioLibraryFetcher(this);

    m_engine.rootContext()->setContextProperty(QStringLiteral("shellViewModel"), m_shellViewModel.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("ollamaPuller"), ollamaPuller);
    m_engine.rootContext()->setContextProperty(QStringLiteral("ollamaLibraryFetcher"),
                                             ollamaLibraryFetcher);
    m_engine.rootContext()->setContextProperty(QStringLiteral("ollamaModelDetailFetcher"),
                                             ollamaModelDetailFetcher);
    m_engine.rootContext()->setContextProperty(QStringLiteral("lmStudioLibraryFetcher"),
                                             lmStudioLibraryFetcher);

    QObject::connect(m_settings.get(), &sentinel::core::AppSettings::appLanguageChanged, &app,
                     [this, &app]() {
                         const auto lang = effectiveLanguageCode(*m_settings);
                         installTranslator(app, m_translator, lang);
                         m_engine.retranslate();
                     });

    QObject::connect(
        &m_engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    m_engine.loadFromModule(QStringLiteral("Sentinel.Desktop"), QStringLiteral("Main"));

    QObject* rootWindow = m_engine.rootObjects().isEmpty() ? nullptr : m_engine.rootObjects().first();
    if (auto* quickWindow = qobject_cast<QQuickWindow*>(rootWindow)) {
        installGraphicsDiagnostics(*quickWindow);
    } else {
        qWarning()
            << "Sentinel graphics diagnostics unavailable: root object is not a QQuickWindow";
    }

    new NativeCompanionAdapter(*m_shellViewModel, *m_settings, rootWindow, this);

    return true;
}

} // namespace sentinel::desktop
