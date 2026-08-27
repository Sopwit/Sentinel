// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ApplicationBootstrapper.h"

#include "GraphicsInitializer.h"
#include "LoggingInitializer.h"
#include "PlatformInitializer.h"

#include "sentinel/core/app/AppMetadata.h"
#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/app/ModeManager.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/platform/DpapiEncryptedSettingsStore.h"
#include "sentinel/core/platform/WinProtocolHandler.h"
#include "sentinel/core/platform/WinTaskbarIntegration.h"
#include "sentinel/core/runtime/LocalInference.h"
#include "sentinel/desktop/DaemonClient.h"
#include "sentinel/desktop/NativeCompanionAdapter.h"

#include <QCommandLineOption>
#include <QCoreApplication>
#include <QFileInfo>
#include <QIcon>
#include <QNetworkProxy>
#include <QProcess>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStandardPaths>

namespace sentinel::desktop {

namespace {

QString discoverDaemonBinary() {
    // Prefer a sibling binary next to the running desktop app (dev/build layout),
    // then fall back to PATH lookup for packaged installs.
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString sibling = appDir + QStringLiteral("/sentinel-daemon");
    if (QFileInfo::exists(sibling)) {
        return sibling;
    }
#ifdef Q_OS_WIN
    const QString siblingExe = appDir + QStringLiteral("/sentinel-daemon.exe");
    if (QFileInfo::exists(siblingExe)) {
        return siblingExe;
    }
#endif
    const QString byPath = QStandardPaths::findExecutable(QStringLiteral("sentinel-daemon"));
    if (!byPath.isEmpty()) {
        return byPath;
    }
    return {};
}

} // namespace

void ApplicationBootstrapper::ensureBackgroundDaemon() {
    if (m_safeMode) {
        qInfo().noquote() << "Safe mode active: skipping background daemon launch.";
        return;
    }

    const QString daemonBinary = discoverDaemonBinary();
    if (daemonBinary.isEmpty()) {
        qInfo().noquote() << "sentinel-daemon binary not found; skipping background launch.";
        return;
    }

    m_daemonProcess = std::make_unique<QProcess>(this);
    m_daemonProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    QObject::connect(m_daemonProcess.get(), &QProcess::started, this,
                     []() { qInfo().noquote() << "sentinel-daemon background process started."; });
    QObject::connect(m_daemonProcess.get(), &QProcess::finished, this,
                     [this](int exitCode, QProcess::ExitStatus exitStatus) {
                         qInfo().noquote()
                             << "sentinel-daemon exited:" << exitCode
                             << (exitStatus == QProcess::NormalExit ? "normal" : "crash");
                         m_daemonProcess.release()->deleteLater();
                     });
    m_daemonProcess->start(daemonBinary, QStringList());
}

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
        QCoreApplication::translate("main",
                                    "Start with all extensions disabled and factory defaults."));
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
            std::make_unique<sentinel::core::JsonSettingsStore>(
                m_pathProvider.settingsFilePath())));

    installStartupTranslator(app, *m_settings, m_translator);

    QObject::connect(m_settings.get(), &sentinel::core::AppSettings::appLanguageChanged, &app,
                     [this, &app]() {
                         const auto lang = effectiveLanguageCode(*m_settings);
                         installTranslator(app, m_translator, lang);
                         m_engine.retranslate();
                     });

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
        qInfo().noquote() << "Proxy enabled:" << type << m_settings->proxyHost()
                          << QString::number(m_settings->proxyPort());
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
    ollamaPuller->setEndpoint(m_settings->ollamaEndpoint());
    QObject::connect(ollamaPuller, &OllamaModelPuller::activeModelChanged, m_shellViewModel.get(),
                     [ollamaPuller, this]() {
                         const QString active = ollamaPuller->activeModel();
                         if (ollamaPuller->pulling() && !active.isEmpty()) {
                             m_shellViewModel->addNotification(
                                 tr("Models"), tr("Downloading Model"),
                                 tr("Retrieving '%1' from registry. You can monitor "
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
                    tr("Models"), tr("Model Installed"),
                    tr("'%1' has been successfully downloaded and is ready for local inference.")
                        .arg(modelId));
            } else {
                m_shellViewModel->addNotification(
                    tr("Models"), tr("Installation Failed"),
                    tr("Could not retrieve '%1'. Please ensure your server is active "
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
                    tr("Models"), tr("Model Removed"),
                    tr("'%1' has been deleted. Disk space has been reclaimed.").arg(modelId));
            } else {
                m_shellViewModel->addNotification(
                    tr("Models"), tr("Model Removal Failed"),
                    tr("Could not delete '%1'. Please ensure the Ollama server is active and "
                       "try again.")
                        .arg(modelId));
            }
        });

    auto* ollamaLibraryFetcher = new OllamaLibraryFetcher(this);
    auto* ollamaModelDetailFetcher = new OllamaModelDetailFetcher(this);
    auto* lmStudioLibraryFetcher = new LMStudioLibraryFetcher(this);

    ensureBackgroundDaemon();
    auto* daemonClient = new DaemonClient(this);

    m_engine.rootContext()->setContextProperty(QStringLiteral("shellViewModel"),
                                               m_shellViewModel.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("ollamaPuller"), ollamaPuller);
    m_engine.rootContext()->setContextProperty(QStringLiteral("ollamaLibraryFetcher"),
                                               ollamaLibraryFetcher);
    m_engine.rootContext()->setContextProperty(QStringLiteral("ollamaModelDetailFetcher"),
                                               ollamaModelDetailFetcher);
    m_engine.rootContext()->setContextProperty(QStringLiteral("lmStudioLibraryFetcher"),
                                               lmStudioLibraryFetcher);
    m_engine.rootContext()->setContextProperty(QStringLiteral("daemonClient"), daemonClient);

    QObject::connect(
        &m_engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    m_engine.loadFromModule(QStringLiteral("Sentinel.Desktop"), QStringLiteral("Main"));

    QObject* rootWindow =
        m_engine.rootObjects().isEmpty() ? nullptr : m_engine.rootObjects().first();
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
