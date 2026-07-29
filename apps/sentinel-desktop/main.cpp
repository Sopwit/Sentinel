#include "sentinel/desktop/DesktopShellViewModel.h"
#include "sentinel/desktop/GraphicsBackend.h"
#include "sentinel/desktop/NativeCompanionAdapter.h"

#include "sentinel/core/AppMetadata.h"
#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/FileLogger.h"
#include "sentinel/core/DpapiEncryptedSettingsStore.h"
#include "sentinel/core/JsonSettingsStore.h"
#include "sentinel/core/WinTaskbarIntegration.h"
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
#include "sentinel/core/WinCrashHandler.h"
#include "sentinel/core/WinProtocolHandler.h"

#include <QApplication>
#include <QCommandLineOption>
#include <cstdio>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QNetworkProxy>
#include <QLocale>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QStandardPaths>
#include <QTranslator>

#if defined(Q_OS_WIN)
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <shellapi.h>
#endif

#include <memory>

namespace {

void configureLogging(bool verbose, bool quiet, const QString& logDir) {
    if (quiet) {
        QLoggingCategory::setFilterRules(
            QStringLiteral("*.debug=false\n*.info=false\n*.warning=false"));
        return;
    }

    if (!verbose && qgetenv("QT_LOGGING_RULES").isEmpty()) {
        QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false\n"
                                                        "*.info=false\n"
                                                        "qt.*=false\n"
                                                        "qt.qml.*=false\n"
                                                        "qt.scenegraph.*=false\n"
                                                        "qt.rhi.*=false\n"
                                                        "qt.pointer.*=false\n"
                                                        "qt.qpa.*=false"));
    }

    sentinel::core::FileLogger::instance().initialize(logDir);
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
        sentinel::core::FileLogger::instance().handleMessage(type, ctx, msg);

        // Also forward to stderr (stdout for info)
        const auto prefix = [](QtMsgType t) -> const char* {
            switch (t) {
            case QtDebugMsg:    return "[DEBUG]";
            case QtInfoMsg:     return "[INFO]";
            case QtWarningMsg:  return "[WARN]";
            case QtCriticalMsg: return "[ERROR]";
            case QtFatalMsg:    return "[FATAL]";
            }
            return "[?]";
        };
        fprintf(type == QtInfoMsg ? stdout : stderr, "%s %s\n", prefix(type),
                msg.toUtf8().constData());
        if (type == QtFatalMsg) {
            fflush(stderr);
            abort();
        }
    });
}

void configureGraphicsBackend() {
    qInfo().noquote() << "Sentinel Qt version:" << qVersion();
    const auto api = sentinel::desktop::defaultGraphicsApiForPlatform();
    QQuickWindow::setGraphicsApi(api);
    qInfo().noquote() << "Sentinel graphics backend requested:"
                      << sentinel::desktop::graphicsApiName(api);
}

void installGraphicsDiagnostics(QQuickWindow& window) {
    const auto logSelectedBackend = [&window]() {
        qInfo().noquote() << "Sentinel graphics backend selected:"
                          << sentinel::desktop::graphicsApiName(
                                 window.rendererInterface()->graphicsApi());
    };

    QObject::connect(&window, &QQuickWindow::sceneGraphInitialized, &window, logSelectedBackend,
                     Qt::DirectConnection);
    QObject::connect(&window, &QQuickWindow::sceneGraphError, &window,
                     [](QQuickWindow::SceneGraphError error, const QString& message) {
                         qCritical().noquote()
                             << "Sentinel scene graph initialization failed; error:" << error
                             << "message:" << message;
                     });
}

QString preferredUiFontFamily() {
    const QStringList availableFamilies = QFontDatabase::families();
#if defined(Q_OS_WIN)
    const QStringList preferredFamilies = {
        QStringLiteral("Segoe UI Variable"), QStringLiteral("Segoe UI"), QStringLiteral("Arial"),
        QStringLiteral("Noto Sans"), QStringLiteral("Helvetica Neue")};
#elif defined(Q_OS_MACOS)
    const QStringList preferredFamilies = {
        QStringLiteral("SF Pro Display"), QStringLiteral("SF Pro Text"),
        QStringLiteral("Helvetica Neue"), QStringLiteral("Noto Sans"), QStringLiteral("Arial")};
#else
    const QStringList preferredFamilies = {
        QStringLiteral("Noto Sans"), QStringLiteral("DejaVu Sans"), QStringLiteral("Ubuntu"),
        QStringLiteral("Segoe UI"),  QStringLiteral("Helvetica Neue"), QStringLiteral("Arial")};
#endif

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
    QApplication app(argc, argv);
    QGuiApplication::setApplicationName(sentinel::core::AppMetadata::displayName());
    QGuiApplication::setOrganizationName(sentinel::core::AppMetadata::organizationName());
    QGuiApplication::setApplicationVersion(sentinel::core::AppMetadata::version());
    QGuiApplication::setDesktopFileName(sentinel::core::AppMetadata::appId());
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/dev.sentinel.Sentinel.png")));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Sentinel AI Desktop Assistant"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption verboseOption(
        {QStringLiteral("verbose")},
        QCoreApplication::translate("main", "Enable verbose diagnostic logging in terminal."));
    parser.addOption(verboseOption);

    QCommandLineOption quietOption(
        {QStringLiteral("quiet")},
        QCoreApplication::translate("main", "Suppress diagnostic terminal output."));
    parser.addOption(quietOption);

    QCommandLineOption safeModeOption(
        {QStringLiteral("safe-mode")},
        QCoreApplication::translate("main", "Start with all extensions disabled and factory defaults."));
    parser.addOption(safeModeOption);

    parser.process(app);

    const bool verbose = parser.isSet(verboseOption);
    const bool quiet = parser.isSet(quietOption);
    const bool safeMode = parser.isSet(safeModeOption);

    sentinel::core::StandardPathProvider pathProvider;
    configureLogging(verbose, quiet, pathProvider.logDirectoryPath());

    // ── Windows Crash Handler ────────────────────────────────────────────
    sentinel::core::installWinCrashHandler(pathProvider.crashDumpDirectoryPath());

    // ── Single Instance Guard + IPC ────────────────────────────────────────
    const QString lockPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                             + QStringLiteral("/sentinel-desktop.lock");
    QLockFile lockFile(lockPath);
    lockFile.setStaleLockTime(0);
    if (!lockFile.tryLock(100)) {
        const QString incomingUrl =
            sentinel::core::extractSentinelUrl(QCoreApplication::arguments());
        if (!incomingUrl.isEmpty()) {
            QLocalSocket socket;
            socket.connectToServer(sentinel::core::sentinelIpcServerName);
            if (socket.waitForConnected(500)) {
                socket.write(incomingUrl.toUtf8());
                socket.waitForBytesWritten(500);
                socket.disconnectFromServer();
            }
        }
        qWarning().noquote() << "Sentinel is already running. Only one instance is allowed.";
        return 1;
    }

    // Start IPC server for protocol handler URL forwarding
    QLocalServer ipcServer;
    QLocalServer::removeServer(sentinel::core::sentinelIpcServerName);
    ipcServer.listen(sentinel::core::sentinelIpcServerName);

    // ── Protocol Handler Registration (Windows) ───────────────────────────
#if defined(Q_OS_WIN)
    sentinel::core::registerSentinelProtocol();
#endif

    // ── Safe Mode ─────────────────────────────────────────────────────────
    if (safeMode) {
        qInfo().noquote() << "Safe mode enabled: using factory defaults.";
    }

#if defined(Q_OS_WIN)
    // ── AppUserModelID ────────────────────────────────────────────────────
    // Use dynamic loading for compatibility with MinGW and older SDKs
    using SetCurrentProcessExplicitAppUserModelIDProc =
        HRESULT(WINAPI*)(PCWSTR AppID);
    auto setAppUserModelId =
        reinterpret_cast<SetCurrentProcessExplicitAppUserModelIDProc>(
            ::GetProcAddress(::GetModuleHandleW(L"shell32.dll"),
                             "SetCurrentProcessExplicitAppUserModelID"));
    if (setAppUserModelId) {
        HRESULT hr = setAppUserModelId(L"dev.sentinel.Sentinel");
        if (FAILED(hr)) {
            qWarning().noquote() << "Failed to set AppUserModelID:" << hr;
        }
    } else {
        qWarning().noquote() << "SetCurrentProcessExplicitAppUserModelID not available";
    }

    // ── Auto-Start (Registry) ─────────────────────────────────────────────
    HKEY hKey;
    LSTATUS regStatus = RegOpenKeyExW(HKEY_CURRENT_USER,
                                       L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                       0, KEY_SET_VALUE, &hKey);
    if (regStatus == ERROR_SUCCESS) {
        const QString appPath = QCoreApplication::applicationFilePath();
        const std::wstring appPathW = appPath.toStdWString();
        RegSetValueExW(hKey, L"Sentinel Desktop", 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(appPathW.c_str()),
                       static_cast<DWORD>((appPathW.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }
#endif

    configureGraphicsBackend();
    app.setQuitOnLastWindowClosed(false);
    configureDefaultUiFont();

    sentinel::core::AppSettings settings(
        std::make_unique<sentinel::core::DpapiEncryptedSettingsStore>(
            std::make_unique<sentinel::core::JsonSettingsStore>(pathProvider.settingsFilePath())));
    QTranslator translator;
    installStartupTranslator(app, settings, translator);

    // ── Proxy Configuration ────────────────────────────────────────────────
    if (settings.proxyEnabled()) {
        QNetworkProxy proxy;
        const QString type = settings.proxyType().toLower();
        if (type == QStringLiteral("socks5")) {
            proxy.setType(QNetworkProxy::Socks5Proxy);
        } else {
            proxy.setType(QNetworkProxy::HttpProxy);
        }
        proxy.setHostName(settings.proxyHost());
        proxy.setPort(settings.proxyPort());
        if (!settings.proxyUser().isEmpty()) {
            proxy.setUser(settings.proxyUser());
            proxy.setPassword(settings.proxyPassword());
        }
        QNetworkProxy::setApplicationProxy(proxy);
        qInfo().noquote()
            << "Proxy enabled:" << type << settings.proxyHost() << QString::number(settings.proxyPort());
    }

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
    sentinel::core::WinTaskbarIntegration taskbarIntegration;
    sentinel::desktop::DesktopShellViewModel shellViewModel(controller, modeManager, settings,
                                                           &taskbarIntegration);

    // ── IPC: Forward sentinel:// URLs from secondary instances ─────────────
    QObject::connect(&ipcServer, &QLocalServer::newConnection, [&]() {
        while (auto* socket = ipcServer.nextPendingConnection()) {
            socket->waitForReadyRead(1000);
            const QString url = QString::fromUtf8(socket->readAll());
            if (!url.isEmpty()) {
                qInfo().noquote() << "Deep link received:" << url;
                shellViewModel.addNotification(
                    QStringLiteral("System"), QStringLiteral("Deep Link"),
                    QStringLiteral("Received: %1").arg(url));
            }
            socket->deleteLater();
        }
    });

    // Also process sentinel:// URL from this instance's own command line
    const QString ownUrl = sentinel::core::extractSentinelUrl(QCoreApplication::arguments());
    if (!ownUrl.isEmpty()) {
        qInfo().noquote() << "Deep link from command line:" << ownUrl;
    }

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
    if (auto* quickWindow = qobject_cast<QQuickWindow*>(rootWindow)) {
        installGraphicsDiagnostics(*quickWindow);
    } else {
        qWarning()
            << "Sentinel graphics diagnostics unavailable: root object is not a QQuickWindow";
    }
    sentinel::desktop::NativeCompanionAdapter companionAdapter(shellViewModel, settings,
                                                               rootWindow);

    return QApplication::exec();
}
