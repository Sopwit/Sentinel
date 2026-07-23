#include "sentinel/desktop/DesktopShellViewModel.h"
#include "sentinel/desktop/NativeCompanionAdapter.h"

#include "sentinel/core/AppMetadata.h"
#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/JsonSettingsStore.h"
#include "sentinel/core/LocalEchoProvider.h"
#include "sentinel/core/LocalInference.h"
#include "sentinel/core/ModeManager.h"
#include "sentinel/core/NullAgentRuntime.h"
#include "sentinel/core/OllamaRuntime.h"
#include "sentinel/core/RealToolExecutor.h"
#include "sentinel/core/RuntimePermissions.h"
#include "sentinel/core/SQLiteChatHistoryStore.h"
#include "sentinel/core/SQLiteConversationStore.h"
#include "sentinel/core/SQLiteMemoryStore.h"
#include "sentinel/core/StandardPathProvider.h"
#include "sentinel/core/StaticSandboxPolicy.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTranslator>

#if defined(Q_OS_LINUX) && QT_CONFIG(vulkan)
#include <QVulkanInstance>
#endif

#include <memory>

namespace {

QString graphicsApiName(QSGRendererInterface::GraphicsApi api) {
    if (api == QSGRendererInterface::Unknown) {
        return QStringLiteral("automatic");
    }
    if (api == QSGRendererInterface::Software) {
        return QStringLiteral("software");
    }
    if (api == QSGRendererInterface::OpenVG) {
        return QStringLiteral("OpenVG");
    }
    if (api == QSGRendererInterface::OpenGL) {
        return QStringLiteral("OpenGL");
    }
    if (api == QSGRendererInterface::Direct3D11) {
        return QStringLiteral("Direct3D 11");
    }
    if (api == QSGRendererInterface::Vulkan) {
        return QStringLiteral("Vulkan");
    }
    if (api == QSGRendererInterface::Metal) {
        return QStringLiteral("Metal");
    }
    if (api == QSGRendererInterface::Null) {
        return QStringLiteral("null");
    }

    return QStringLiteral("unknown");
}

void configureGraphicsBackend() {
#if defined(Q_OS_MACOS)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Metal);
    qInfo().noquote() << "Sentinel graphics backend requested:"
                      << graphicsApiName(QSGRendererInterface::Metal);
#elif defined(Q_OS_WIN)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
    qInfo().noquote() << "Sentinel graphics backend requested:"
                      << graphicsApiName(QSGRendererInterface::Direct3D11);
#elif defined(Q_OS_LINUX)
#if QT_CONFIG(vulkan)
    QVulkanInstance vulkanInstance;
    if (vulkanInstance.create()) {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
        qInfo().noquote() << "Sentinel graphics backend requested:"
                          << graphicsApiName(QSGRendererInterface::Vulkan);
        return;
    }
#endif
    qInfo().noquote() << "Sentinel graphics backend requested:"
                      << graphicsApiName(QSGRendererInterface::Unknown);
#endif
}

QString preferredUiFontFamily() {
    const QStringList availableFamilies = QFontDatabase::families();
    const QStringList preferredFamilies = {
        QStringLiteral("Noto Sans"), QStringLiteral("DejaVu Sans"),    QStringLiteral("Ubuntu"),
        QStringLiteral("Segoe UI"),  QStringLiteral("Helvetica Neue"), QStringLiteral("Arial")};

    for (const QString& family : preferredFamilies) {
        if (availableFamilies.contains(family, Qt::CaseInsensitive)) {
            return family;
        }
    }

    return QString();
}

void configureDefaultUiFont() {
    const QString family = preferredUiFontFamily();
    if (family.isEmpty()) {
        return;
    }

    QFont font = QGuiApplication::font();
    if (font.family().compare(family, Qt::CaseInsensitive) == 0) {
        return;
    }

    font.setFamily(family);
    QGuiApplication::setFont(font);
}

QString effectiveLanguageCode(const sentinel::core::AppSettings& settings) {
    const auto configured = settings.appLanguage();
    const auto systemLanguage = QLocale::system().name().left(2).toLower();
    if (configured == QStringLiteral("tr") || configured == QStringLiteral("en")) {
        return configured;
    }
    return systemLanguage == QStringLiteral("tr") ? QStringLiteral("tr") : QStringLiteral("en");
}

void installTranslator(QGuiApplication& app, QTranslator& translator, const QString& language) {
    app.removeTranslator(&translator);
    if (language == QStringLiteral("en")) {
        // English is the source language; no translation file needed.
        return;
    }
    if (translator.load(QStringLiteral(":/i18n/sentinel_%1.qm").arg(language))) {
        app.installTranslator(&translator);
    }
}

void installStartupTranslator(QGuiApplication& app, const sentinel::core::AppSettings& settings,
                              QTranslator& translator) {
    installTranslator(app, translator, effectiveLanguageCode(settings));
}

} // namespace

int main(int argc, char* argv[]) {
    configureGraphicsBackend();

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    configureDefaultUiFont();
    QGuiApplication::setApplicationName(sentinel::core::AppMetadata::displayName());
    QGuiApplication::setOrganizationName(sentinel::core::AppMetadata::organizationName());
    QGuiApplication::setApplicationVersion(sentinel::core::AppMetadata::version());
    QGuiApplication::setDesktopFileName(sentinel::core::AppMetadata::appId());
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/dev.sentinel.Sentinel.png")));

    sentinel::core::StandardPathProvider pathProvider;
    sentinel::core::AppSettings settings(
        std::make_unique<sentinel::core::JsonSettingsStore>(pathProvider.settingsFilePath()));
    QTranslator translator;
    installStartupTranslator(app, settings, translator);

    const auto ollamaConfig = sentinel::core::OllamaConfig::fromEndpoint(settings.ollamaEndpoint());
    sentinel::core::ApplicationController controller(
        std::make_unique<sentinel::core::LocalEchoProvider>(),
        std::make_unique<sentinel::core::SQLiteMemoryStore>(pathProvider.memoryDatabasePath()),
        nullptr,
        std::make_unique<sentinel::core::SQLiteChatHistoryStore>(
            pathProvider.chatHistoryDatabasePath()),
        std::make_unique<sentinel::core::NullAgentRuntime>(
            sentinel::core::NullAgentRuntime::standardTools()),
        nullptr,
        std::make_unique<sentinel::core::StaticSandboxPolicy>(
            QSet<QString>{QStringLiteral("tool.metadata.read"), QStringLiteral("tool.risk.medium"),
                          QStringLiteral("tool.risk.high")}),
        std::make_unique<sentinel::core::RealToolExecutor>(), nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<sentinel::core::LocalOnlyRuntimePermissionPolicy>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr,
        std::make_unique<sentinel::core::OllamaHttpRuntimeClient>(ollamaConfig),
        std::make_unique<sentinel::core::OllamaLocalInferenceClient>(ollamaConfig),
        std::make_unique<sentinel::core::OllamaLocalInferenceStreamClient>(ollamaConfig), nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        std::make_unique<sentinel::core::SQLiteConversationStore>(
            pathProvider.conversationDatabasePath()));
    controller.setConversationExportDirectory(pathProvider.conversationExportDirectoryPath());
    sentinel::core::ModeManager modeManager;
    controller.setRoutingModeByName(settings.routingModeName());
    sentinel::desktop::DesktopShellViewModel shellViewModel(controller, modeManager, settings);
    OllamaModelPuller ollamaPuller;
    QObject::connect(&ollamaPuller, &OllamaModelPuller::activeModelChanged, &shellViewModel,
                     [&ollamaPuller, &shellViewModel]() {
                         const QString active = ollamaPuller.activeModel();
                         if (ollamaPuller.pulling() && !active.isEmpty()) {
                             shellViewModel.addNotification(
                                 QStringLiteral("Models"), QStringLiteral("Downloading Model"),
                                 QStringLiteral("Retrieving '%1' from registry. You can monitor "
                                                "progress in the modelfiles panel.")
                                     .arg(active));
                         }
                     });
    QObject::connect(
        &ollamaPuller, &OllamaModelPuller::pullFinished, &controller,
        [&controller, &shellViewModel](const QString& modelId, bool success) {
            if (success) {
                controller.refreshOllamaStatus();
                shellViewModel.addNotification(
                    QStringLiteral("Models"), QStringLiteral("Model Installed"),
                    QStringLiteral(
                        "'%1' has been successfully downloaded and is ready for local inference.")
                        .arg(modelId));
            } else {
                shellViewModel.addNotification(
                    QStringLiteral("Models"), QStringLiteral("Installation Failed"),
                    QStringLiteral("Could not retrieve '%1'. Please ensure your server is active "
                                   "and try again.")
                        .arg(modelId));
            }
        });
    QObject::connect(
        &ollamaPuller, &OllamaModelPuller::removeFinished, &controller,
        [&controller, &shellViewModel](const QString& modelId, bool success) {
            if (success) {
                controller.refreshOllamaStatus();
                shellViewModel.addNotification(
                    QStringLiteral("Models"), QStringLiteral("Model Removed"),
                    QStringLiteral("'%1' has been deleted. Disk space has been reclaimed.")
                        .arg(modelId));
            }
        });
    OllamaLibraryFetcher ollamaLibraryFetcher;
    OllamaModelDetailFetcher ollamaModelDetailFetcher;
    LMStudioLibraryFetcher lmStudioLibraryFetcher;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("shellViewModel"), &shellViewModel);
    engine.rootContext()->setContextProperty(QStringLiteral("ollamaPuller"), &ollamaPuller);
    engine.rootContext()->setContextProperty(QStringLiteral("ollamaLibraryFetcher"),
                                             &ollamaLibraryFetcher);
    engine.rootContext()->setContextProperty(QStringLiteral("ollamaModelDetailFetcher"),
                                             &ollamaModelDetailFetcher);
    engine.rootContext()->setContextProperty(QStringLiteral("lmStudioLibraryFetcher"),
                                             &lmStudioLibraryFetcher);

    // Runtime language switching: swap the translator and notify all QML objects.
    QObject::connect(&settings, &sentinel::core::AppSettings::appLanguageChanged, &app,
                     [&app, &settings, &translator, &engine]() {
                         const auto lang = effectiveLanguageCode(settings);
                         installTranslator(app, translator, lang);
                         engine.retranslate();
                     });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("Sentinel.Desktop"), QStringLiteral("Main"));

    QObject* rootWindow = engine.rootObjects().isEmpty() ? nullptr : engine.rootObjects().first();
    sentinel::desktop::NativeCompanionAdapter companionAdapter(shellViewModel, settings,
                                                               rootWindow);

    return QApplication::exec();
}
