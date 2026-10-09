// SPDX-License-Identifier: GPL-3.0-or-later
// Opt-in UI harness: production QML and viewmodel, disposable disk stores,
// explicitly injected in-memory credential backend. No provider success is faked.
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/app/ModeManager.h"
#include "sentinel/core/chat/SQLiteConversationStore.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/memory/SQLiteMemoryStore.h"
#include "sentinel/desktop/DesktopShellViewModel.h"
#include "sentinel/desktop/NativeCompanionAdapter.h"
#include <QApplication>
#include <QAccessible>
#include <cmath>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickItem>
#include <QJSValue>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTranslator>

// Deterministic catalog data only; the harness does not simulate provider success.
class CatalogUiFixture final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList models MEMBER models NOTIFY changed)
    Q_PROPERTY(QVariantList catalog MEMBER catalog NOTIFY changed)
    Q_PROPERTY(QVariantList tags MEMBER tags NOTIFY changed)
    Q_PROPERTY(bool fetching MEMBER fetching NOTIFY changed)
    Q_PROPERTY(bool pulling MEMBER pulling NOTIFY changed)
    Q_PROPERTY(bool daemonReachable MEMBER daemonReachable NOTIFY changed)
    Q_PROPERTY(QString errorText MEMBER errorText NOTIFY changed)
    Q_PROPERTY(QString statusText MEMBER statusText NOTIFY changed)
    Q_PROPERTY(QString activeModel MEMBER activeModel NOTIFY changed)
    Q_PROPERTY(QString readme MEMBER readme NOTIFY changed)
    Q_PROPERTY(QString query MEMBER query NOTIFY changed)
    Q_PROPERTY(int installedCount MEMBER installedCount NOTIFY changed)
    Q_PROPERTY(bool setupBusy MEMBER setupBusy NOTIFY changed)
    Q_PROPERTY(bool runtimeInstalled MEMBER runtimeInstalled NOTIFY changed)
    Q_PROPERTY(QString setupStatus MEMBER setupStatus NOTIFY changed)
    Q_PROPERTY(QString catalogStatus MEMBER catalogStatus NOTIFY changed)
    Q_PROPERTY(bool hasMore MEMBER hasMore NOTIFY changed)
    Q_PROPERTY(bool hasPrevious MEMBER hasPrevious NOTIFY changed)
    Q_PROPERTY(double progress MEMBER progress NOTIFY changed)
public:
    QVariantList models, catalog, tags;
    bool fetching = false, pulling = false, daemonReachable = false, hasMore = false, hasPrevious = false;
    QString errorText, statusText, activeModel, readme, catalogStatus, query, setupStatus;
    int installedCount = 0;
    bool setupBusy = false, runtimeInstalled = true;
    double progress = 0;
    Q_INVOKABLE void fetch(const QString& = {}) {}
    Q_INVOKABLE void refreshCatalog(const QString&) {}
    Q_INVOKABLE void fetchDetails(const QString&) {}
    Q_INVOKABLE void cancel() {}
    void notify() { emit changed(); }
signals:
    void changed();
};

QVariant qmlVariant(const QVariant& value) {
    return value.metaType() == QMetaType::fromType<QJSValue>() ? value.value<QJSValue>().toVariant() : value;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Sentinel Settings Certification");
    QStandardPaths::setTestModeEnabled(true);
    const bool phase1Smoke = argc == 2 && QString::fromLocal8Bit(argv[1]) == "--phase1-smoke";
    const bool traySmoke = argc == 2 && QString::fromLocal8Bit(argv[1]) == "--tray-smoke";
    const bool focusSmoke = argc == 2 && QString::fromLocal8Bit(argv[1]) == "--focus-smoke";
    const bool navigationSmoke = argc == 2 && QString::fromLocal8Bit(argv[1]) == "--navigation-smoke";
    const bool chatSmoke = argc == 2 && QString::fromLocal8Bit(argv[1]) == "--chat-smoke";
    const bool settingsSmoke = argc == 2 && QString::fromLocal8Bit(argv[1]) == "--settings-smoke";
    const bool modelsSmoke = argc == 2 && QString::fromLocal8Bit(argv[1]) == "--models-smoke";
    QTemporaryDir temporaryProfile;
    if (argc != 2 || (!phase1Smoke && !focusSmoke && !navigationSmoke && !chatSmoke && !modelsSmoke && !settingsSmoke && !traySmoke && !QDir(QString::fromLocal8Bit(argv[1])).exists()))
        return 2;
    QDir profile(phase1Smoke || focusSmoke || navigationSmoke || chatSmoke || modelsSmoke || settingsSmoke || traySmoke ? temporaryProfile.path() : QString::fromLocal8Bit(argv[1]));
    sentinel::core::AppSettings settings(
        std::make_unique<sentinel::core::JsonSettingsStore>(profile.filePath("settings.json")),
        sentinel::core::inMemoryTestCredentialStore());
    sentinel::core::ApplicationControllerBuilder builder;
    builder.withModelService(std::make_unique<sentinel::core::ModelService>(&settings))
        .withMemoryStore(
            std::make_unique<sentinel::core::SQLiteMemoryStore>(profile.filePath("memory.sqlite3")))
        .withConversationStore(std::make_unique<sentinel::core::SQLiteConversationStore>(
            profile.filePath("chat.sqlite3")));
    auto controller = builder.build();
    sentinel::core::ModeManager modes;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("navigationSmoke", navigationSmoke);
    QObject::connect(&engine, &QQmlApplicationEngine::exit, &app, &QApplication::exit);
    if (phase1Smoke || navigationSmoke || chatSmoke || modelsSmoke || settingsSmoke || traySmoke) settings.setOnboardingComplete(true);
    engine.addImportPath(QStringLiteral(CERTIFICATION_QML_IMPORT));
    QTranslator translator;
    auto translate = [&] {
        app.removeTranslator(&translator);
        if (translator.load(QStringLiteral(CERTIFICATION_TRANSLATIONS) + "/sentinel_" +
                            settings.appLanguage() + ".qm"))
            app.installTranslator(&translator);
        engine.retranslate();
    };
    translate();
    QObject::connect(&settings, &sentinel::core::AppSettings::appLanguageChanged, &app, translate);
    // Match production bootstrap: translator changes precede viewmodel language signals.
    sentinel::desktop::DesktopShellViewModel viewModel(*controller, modes, settings);
    sentinel::desktop::NativeCompanionAdapter native(viewModel, settings, nullptr);
    engine.rootContext()->setContextProperty("nativeDesktop", &native);
    engine.rootContext()->setContextProperty("shellViewModel", &viewModel);
    if (qEnvironmentVariableIsSet("SENTINEL_UI_NOTIFICATION_FIXTURE")) {
        viewModel.setDndEnabled(true); // Synthetic history only; do not request an OS banner.
        viewModel.addNotificationWithPriority("Security", "Disposable test notice", "Synthetic content for notification UI verification.", "High");
    }
    CatalogUiFixture library, emptyLibrary;
    library.models = {
        QVariantMap{{"id", "qwen-small"}, {"name", "Qwen 2.5 7B"}, {"ollamaId", "qwen2.5:7b"}, {"category", "LLM"}, {"provider", "Ollama"}, {"description", "Small variant"}, {"badge", "Open"}, {"badgeColor", "#4f8ef7"}},
        QVariantMap{{"id", "qwen-large"}, {"name", "Qwen 2.5 14B"}, {"ollamaId", "qwen2.5:14b"}, {"category", "LLM"}, {"provider", "Ollama"}, {"description", "Large variant"}, {"badge", "Open"}, {"badgeColor", "#4f8ef7"}}
    };
    engine.rootContext()->setContextProperty("ollamaLibraryFetcher", &library);
    for (const auto* name : {"ggufLibraryFetcher", "lmStudioLibraryFetcher", "ollamaModelDetailFetcher", "ollamaPuller", "daemonClient"})
        engine.rootContext()->setContextProperty(name, &emptyLibrary);
    engine.load(QUrl::fromLocalFile(QStringLiteral(CERTIFICATION_QML_WRAPPER)));
    if (engine.rootObjects().isEmpty())
        return 3;
    if (phase1Smoke) {
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QTimer::singleShot(250, &app, [&, window] {
            auto find = [](auto&& self, QQuickItem* item, const QString& name) -> QQuickItem* {
                if (item->objectName() == name) return item;
                for (auto* child : item->childItems()) if (auto* found = self(self, child, name)) return found;
                return nullptr;
            };
            auto check = [&](bool ok, const char* detail) {
                if (!ok) qFatal("Phase 1 acceptance failed: %s", detail);
            };
            auto* page = find(find, window->contentItem(), "settingsPage");
            auto* popup = window->findChild<QObject*>("notificationCenterPopup");
            auto* palette = window->findChild<QObject*>("certificationPalette");
            check(page && popup && palette, "mounted shared surfaces");
            page->setProperty("activeCategory", "Notifications");
            QApplication::processEvents();
            auto* history = find(find, page, "notificationHistoryButton");
            check(history, "Settings history entry");
            history->forceActiveFocus();
            QMetaObject::invokeMethod(history, "clicked"); QTest::qWait(100);
            check(popup->property("visible").toBool(), "history opens");
            auto* empty = find(find, window->contentItem(), "notificationHistoryEmpty");
            check(empty && empty->isVisible(), "empty history");
            QTest::keyClick(window, Qt::Key_Escape); QTest::qWait(100);
            check(!viewModel.notificationCenterVisible() && !popup->property("visible").toBool(), "Escape synchronizes visibility");
            check(history->hasActiveFocus(), "focus returns to history entry");
            viewModel.addNotificationWithPriority("Security", "Disposable phase 1 notice", "Synthetic test content", "High");
            viewModel.setNotificationCenterVisible(true); QTest::qWait(100);
            auto* list = find(find, window->contentItem(), "notificationHistoryList");
            check(list && list->property("count").toInt() == 2 && !empty->isVisible(), "group and populated history");
            viewModel.setDndEnabled(true); QApplication::processEvents();
            check(list->property("count").toInt() == 2, "DND retains history");
            auto* markRead = find(find, window->contentItem(), "notificationMarkReadButton");
            auto* archive = find(find, window->contentItem(), "notificationArchiveButton");
            check(markRead && archive, "notification row actions");
            QMetaObject::invokeMethod(markRead, "clicked"); QApplication::processEvents();
            const auto readRow = QJsonDocument::fromJson(viewModel.notificationFilteredSummaries().first().toUtf8()).object();
            check(readRow.value("read").toBool(), "row marks notification read");
            archive = find(find, window->contentItem(), "notificationArchiveButton");
            check(archive, "archive action survives refresh");
            QMetaObject::invokeMethod(archive, "clicked"); QApplication::processEvents();
            check(QJsonDocument::fromJson(viewModel.notificationFilteredSummaries().first().toUtf8()).object().value("archived").toBool(), "row archives notification");
            check(viewModel.notificationFilteredSummaries().join(" ").contains("Disposable phase 1 notice"), "archive remains in history");
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5)); QTest::qWait(100);
            check(!popup->property("visible").toBool() && !viewModel.notificationCenterVisible(), "outside dismissal");
            check(viewModel.clearArchivedNotifications(), "clear archived");
            viewModel.setDndEnabled(false);
            const auto actions = qmlVariant(palette->property("actions")).toList();
            for (const auto& value : actions) {
                const auto action = value.toMap();
                const auto name = action.value("action").toString();
                if (name == "settings" || name == "notifications" || name == "search-chats" || name == "universal-search" || name == "theme") {
                    const auto oldTheme = viewModel.themeName();
                    QMetaObject::invokeMethod(palette, "runAction", Q_ARG(QVariant, value));
                    QApplication::processEvents();
                    if (name == "settings") check(page->property("activeCategory").toString() == action.value("category", "Interface").toString(), "category route");
                    if (name == "notifications") { check(viewModel.notificationCenterVisible(), "palette history route"); viewModel.setNotificationCenterVisible(false); }
                    if (name.startsWith("search") || name == "universal-search") check(!action.value("enabled").toBool(), "unsupported search disabled");
                    if (name == "theme") check(viewModel.themeName() != oldTheme, "theme cycle");
                }
            }
            for (int width : {1100, 900, 780}) {
                window->setWidth(width); window->setHeight(width == 780 ? 640 : 780); QTest::qWait(60);
                auto* sidebar = find(find, page, "settingsCategorySidebar");
                check(sidebar && sidebar->isVisible() == !page->property("compact").toBool(), "responsive breakpoint");
                auto inspect = [&](auto&& self, QQuickItem* item) -> void {
                    if (item->isVisible()) check(!item->property("truncated").toBool(), "complete sidebar labels");
                    for (auto* child : item->childItems()) self(self, child);
                };
                inspect(inspect, sidebar);
            }
            auto* status = find(find, window->contentItem(), "daemonStatusButton");
            check(status, "focusable daemon indicator"); status->forceActiveFocus();
            const auto* accessible = QAccessible::queryAccessibleInterface(status);
            check(accessible && !accessible->text(QAccessible::Name).isEmpty(), "status accessible name");
            QTest::keyClick(window, Qt::Key_Space); QTest::qWait(60);
            auto* statusPopup = window->findChild<QObject*>("daemonStatusPopup");
            check(statusPopup && statusPopup->property("visible").toBool(), "keyboard opens daemon details");
            check(window->activeFocusItem() && window->activeFocusItem() != status, "status popup receives focus");
            QTest::keyClick(window, Qt::Key_Escape); QTest::qWait(40);
            check(status->hasActiveFocus(), "status popup restores focus");
            window->setProperty("surface", 2); viewModel.startVoiceCapture(true); QTest::qWait(60);
            check(!viewModel.voiceInputStatus().isEmpty() && !viewModel.voiceInputMessage().contains("ModelUnavailable"), "voice prerequisite guidance");
            auto* voiceSetup = find(find, window->contentItem(), "chatVoiceSetupButton");
            check(voiceSetup && voiceSetup->isVisible(), "voice setup action");
            QMetaObject::invokeMethod(voiceSetup, "clicked"); QApplication::processEvents();
            check(page->property("activeCategory").toString() == "Voice" && window->property("surface").toInt() == 0, "voice settings route");
            QQmlComponent finishComponent(&engine);
            finishComponent.setData(R"(
                import QtQuick
                import Sentinel.Desktop
                FinishStep {
                    width: 900; height: 680
                    viewModel: QtObject {
                        property bool daemonConnected: false
                        property string activeRuntimeReadinessState: "ready"
                        property string currentModeName: "Calm"
                        property string onboardingProcessingMode: "local"
                        property var onboardingSnapshot: ({providerId: "fixture", modelId: "fixture"})
                        property string appLanguage: "en"
                        property string themeName: "Liquid Glass Light"
                        function languageDisplayName(code) { return code }
                    }
                }
            )", QUrl("qrc:/phase1-finish-test.qml"));
            std::unique_ptr<QObject> finish(finishComponent.create());
            check(bool(finish), "finish component fixture");
            auto* readiness = qvariant_cast<QObject*>(finish->property("viewModel"));
            check(readiness && !finish->property("inferenceReady").toBool(), "offline readiness is false");
            readiness->setProperty("daemonConnected", true);
            check(finish->property("inferenceReady").toBool() && finish->property("modelRunnable").toBool(), "authoritative ready presentation");
            readiness->setProperty("activeRuntimeReadinessState", "busy");
            check(!finish->property("inferenceReady").toBool() && finish->property("modelRunnable").toBool(), "busy is not ready to send");
            for (const char* state : {"missingModel", "unavailable", "unknown"}) {
                readiness->setProperty("activeRuntimeReadinessState", state);
                check(!finish->property("inferenceReady").toBool() && !finish->property("modelRunnable").toBool(), "unconfirmed model is not runnable");
            }
            window->setProperty("surface", 0);
            auto luminance = [](const QColor& color) {
                auto linear = [](double v) { return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
                return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
            };
            for (const char* theme : {"Liquid Glass Light", "Liquid Glass Dark"}) {
                viewModel.setThemeName(theme);
                for (bool contrast : {false, true}) {
                    viewModel.setHighContrastEnabled(contrast); QTest::qWait(100);
                    const auto screenshot = window->grabWindow();
                    check(!screenshot.isNull(), "renderer returns icon pixels");
                    const auto background = window->property("settingsSurface").value<QColor>();
                    for (const char* buttonName : {"DashboardNavigationButton", "ModelsNavigationButton", "InspectorNavigationButton"}) {
                        auto* button = find(find, window->contentItem(), buttonName);
                        check(button, "navigation icon button");
                        const auto origin = button->mapToScene(QPointF(button->width() / 2 - 10, button->height() / 2 - 10));
                        const double scale = screenshot.devicePixelRatio();
                        int contrastingPixels = 0;
                        for (int y = int(origin.y() * scale); y < int((origin.y() + 20) * scale); ++y)
                            for (int x = int(origin.x() * scale); x < int((origin.x() + 20) * scale); ++x) {
                                const double a = luminance(screenshot.pixelColor(x, y)), b = luminance(background);
                                if ((qMax(a, b) + 0.05) / (qMin(a, b) + 0.05) >= 3.0) ++contrastingPixels;
                            }
                        check(contrastingPixels >= 10, "icon strokes visible at >= 3:1 against rail surface");
                        qInfo() << "Icon contrast pixels" << theme << contrast << buttonName << contrastingPixels;
                    }
                }
            }
            qInfo("Onboarding readiness state matrix and rendered rail stroke samples passed; not full WCAG certification");
            qInfo("Phase 1 notification lifecycle/dismissal/focus, palette routes, labels, status and missing-STT flow passed (isolated harness)");
            app.exit(0);
        });
    }
    if (traySmoke) {
        auto* panel = engine.rootObjects().first()->findChild<QQuickWindow*>("sentinelQuickPanel");
        auto* fixture = engine.rootObjects().first()->findChild<QObject*>("quickPanelFixture");
        if (!panel || !fixture) return 70;
        panel->show();
        QTimer::singleShot(200, &app, [&, panel, fixture] {
            auto find = [](auto&& self, QQuickItem* parent, const QString& name) -> QQuickItem* {
                if (parent->objectName() == name) return parent;
                for (auto* child : parent->childItems()) if (auto* result = self(self, child, name)) return result;
                return nullptr;
            };
            auto* prompt = find(find, panel->contentItem(), "quickPanelPrompt");
            auto* send = find(find, panel->contentItem(), "quickPanelSend");
            auto* provider = find(find, panel->contentItem(), "quickPanelProvider");
            if (!prompt || !send || !provider || prompt->isEnabled() || send->isEnabled()) { app.exit(71); return; }
            QVariantMap state{{"ready", true}, {"busy", false}, {"connection", "Connected"}, {"state", "idle"}, {"voiceState", "idle"}, {"voiceAvailable", false}, {"voiceTranscript", ""}, {"approvalCount", 0}, {"approval", QVariantMap{}}, {"preview", ""}};
            fixture->setProperty("state", state);
            QApplication::processEvents();
            prompt->setProperty("text", "Quick question");
            prompt->forceActiveFocus();
            QTest::keyClick(panel, Qt::Key_Return);
            if (fixture->property("submissions").toInt() != 1 || !prompt->property("text").toString().isEmpty()) { app.exit(72); return; }
            prompt->setProperty("text", "line one");
            prompt->forceActiveFocus();
            QTest::keyClick(panel, Qt::Key_Return, Qt::ShiftModifier);
            if (fixture->property("submissions").toInt() != 1 || !prompt->property("text").toString().contains('\n')) { app.exit(78); return; }
            auto* statusButton = find(find, panel->contentItem(), "quickPanelStatus");
            auto* statusPopup = panel->findChild<QObject*>("quickPanelStatusDetails");
            if (!statusButton || !statusPopup) { app.exit(79); return; }
            QMetaObject::invokeMethod(statusButton, "clicked");
            QTest::qWait(40);
            if (!statusPopup->property("visible").toBool()) { app.exit(80); return; }
            QTest::keyClick(panel, Qt::Key_Escape);
            QTest::qWait(80);
            if (statusPopup->property("visible").toBool() || !panel->isVisible()) { app.exit(81); return; }
            panel->setProperty("agentMode", true);
            prompt->setProperty("text", "Agent question");
            QTest::keyClick(panel, Qt::Key_Return);
            if (fixture->property("submissions").toInt() != 2 || !fixture->property("lastAgent").toBool()) { app.exit(73); return; }
            prompt->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyClick(panel, Qt::Key_Tab);
            if (!panel->activeFocusItem() || !panel->activeFocusItem()->isVisible()) { app.exit(74); return; }
            state["preview"] = "Fixture response preview — no inference performed.";
            fixture->setProperty("state", state);
            QTest::qWait(80);
            panel->grabWindow().save(QDir::tempPath() + "/sentinel-tray-panel.png");
            for (int width : {420, 320}) {
                panel->setWidth(width);
                QApplication::processEvents();
                if (provider->width() <= 0 || send->mapToScene(QPointF()).x() + send->width() > panel->width()) { app.exit(75); return; }
            }
            state["busy"] = true;
            fixture->setProperty("state", state);
            QApplication::processEvents();
            if (prompt->isEnabled() || provider->isEnabled() || !send->isEnabled()) { app.exit(76); return; }
            fixture->setProperty("error", "Fixture connection error");
            QApplication::processEvents();
            auto* error = find(find, panel->contentItem(), "quickPanelError");
            if (!error || !error->isVisible() || error->property("text").toString() != "Fixture connection error") { app.exit(77); return; }
            fixture->setProperty("error", "request-timeout");
            QApplication::processEvents();
            if (!error->property("text").toString().contains("daemon did not reply")) { app.exit(82); return; }
            const auto evidence = qEnvironmentVariable("SENTINEL_TRAY_EVIDENCE_DIR");
            if (!evidence.isEmpty()) {
                QDir().mkpath(evidence);
                panel->setWidth(420);
                for (const auto& theme : {QStringLiteral("Paper"), QStringLiteral("Liquid Glass Dark")}) {
                    viewModel.setThemeName(theme);
                    QTest::qWait(100);
                    panel->grabWindow().save(evidence + (theme == "Paper" ? "/quick-error-light.png" : "/quick-error-dark.png"));
                }
                fixture->setProperty("error", "");
                state["busy"] = false; state["preview"] = "";
                fixture->setProperty("state", state); prompt->setProperty("text", "");
                QTest::qWait(100); panel->grabWindow().save(evidence + "/quick-idle-dark.png");
                state["busy"] = true; state["approvalCount"] = 1;
                state["approval"] = QVariantMap{{"tool", "Fixture read-only operation"}, {"risk", 0}, {"detail", "Synthetic test approval; no task executed."}};
                fixture->setProperty("state", state);
                QTest::qWait(100); panel->grabWindow().save(evidence + "/quick-approval-fixture-dark.png");
            }
            qInfo("Tray loading/empty/response/error, Chat/Agent keyboard submission, focus and compact layout passed");
            app.exit(0);
        });
    }
    if (modelsSmoke) {
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        window->setProperty("surface", 3);
        QTimer::singleShot(350, &app, [&, window] {
            auto find = [](auto&& self, QQuickItem* parent, const QString& name) -> QQuickItem* {
                if (parent->objectName() == name) return parent;
                for (auto* child : parent->childItems()) if (auto* found = self(self, child, name)) return found;
                return nullptr;
            };
            auto* page = find(find, window->contentItem(), "modelsPage");
            auto* grid = find(find, window->contentItem(), "modelGrid");
            auto* sidebar = find(find, window->contentItem(), "modelSidebar");
            auto* selector = find(find, window->contentItem(), "modelVersionSelector");
            if (!page || !grid || !sidebar || !selector || grid->property("count").toInt() != 1) { app.exit(40); return; }
            selector->setProperty("currentIndex", 1);
            QMetaObject::invokeMethod(selector, "activated", Q_ARG(int, 1));
            auto* delegate = selector->parentItem()->parentItem()->parentItem();
            if (qmlVariant(delegate->property("selectedModel")).toMap().value("ollamaId") != "qwen2.5:14b") { qWarning() << "Variant selection" << delegate << delegate->property("selectedModel") << selector->property("currentIndex"); app.exit(41); return; }
            auto* detailsButton = find(find, window->contentItem(), "modelDetailsButton");
            auto* popup = page->findChild<QObject*>("modelDetailsPopup");
            if (!detailsButton || !popup || !QMetaObject::invokeMethod(detailsButton, "clicked")) { app.exit(49); return; }
            if (qmlVariant(popup->property("modelInfo")).toMap().value("ollamaId") != "qwen2.5:14b") { app.exit(50); return; }
            QMetaObject::invokeMethod(popup, "close");
            page->setProperty("sidebarCollapsed", true);
            window->setWidth(780); window->setHeight(640);
            QTimer::singleShot(250, &app, [&, page, sidebar, window, find] {
                if (sidebar->width() > 65 || sidebar->property("labelsVisible").toBool()) { app.exit(42); return; }
                page->setProperty("sidebarCollapsed", false);
                QTimer::singleShot(70, &app, [&, sidebar] {
                    if (sidebar->property("labelsVisible").toBool() && sidebar->width() < sidebar->property("expandedWidth").toReal() - 1) app.exit(43);
                });
                QTimer::singleShot(250, &app, [&, page, sidebar, window, find] {
                    if (!sidebar->property("labelsVisible").toBool()) { app.exit(44); return; }
                    auto* label = find(find, sidebar, "modelSidebarLabel");
                    auto* icon = find(find, sidebar, "modelSidebarIcon");
                    if (!label || !icon || label->x() < icon->x() + icon->width()) { app.exit(45); return; }
                    page->setProperty("searchQuery", "no-match-fixture");
                    QApplication::processEvents();
                    if (!qmlVariant(page->property("currentModels")).toList().isEmpty()) { app.exit(46); return; }
                    library.fetching = true; library.notify();
                    QApplication::processEvents();
                    if (!page->property("catalogFetching").toBool()) { app.exit(47); return; }
                    library.fetching = false; library.errorText = "Fixture catalog unavailable"; library.notify();
                    page->setProperty("searchQuery", "");
                    QApplication::processEvents();
                    sidebar->forceActiveFocus(Qt::TabFocusReason);
                    QTest::keyClick(window, Qt::Key_Tab);
                    if (!window->activeFocusItem()) { app.exit(48); return; }
                    for (int i = 0; i < 90; ++i)
                        library.models.append(QVariantMap{{"id", QString("fixture/page%1").arg(i)}, {"name", QString("Page model %1").arg(i)}, {"category", "LLM"}});
                    library.notify();
                    QApplication::processEvents();
                    if (qmlVariant(page->property("currentModels")).toList().size() != 40 || page->property("pageCount").toInt() != 3) { app.exit(51); return; }
                    auto* next = find(find, window->contentItem(), "modelsNextPage");
                    auto* previous = find(find, window->contentItem(), "modelsPreviousPage");
                    if (!next || !previous || !QMetaObject::invokeMethod(next, "clicked")) { app.exit(52); return; }
                    QApplication::processEvents();
                    if (page->property("currentPage").toInt() != 1 || qmlVariant(page->property("currentModels")).toList().size() != 40) { app.exit(53); return; }
                    QMetaObject::invokeMethod(next, "clicked"); QApplication::processEvents();
                    if (qmlVariant(page->property("currentModels")).toList().size() != 11 || next->isEnabled()) { app.exit(54); return; }
                    QMetaObject::invokeMethod(previous, "clicked"); QApplication::processEvents();
                    if (page->property("currentPage").toInt() != 1) { app.exit(55); return; }
                    page->setProperty("searchQuery", "Page model 89"); QApplication::processEvents();
                    if (page->property("currentPage").toInt() != 0 || qmlVariant(page->property("currentModels")).toList().size() != 1) { app.exit(56); return; }
                    qInfo("Model pagination, grouping, variants, responsive sidebar, empty/loading/error and keyboard flow passed");
                    app.exit(0);
                });
            });
        });
    }
    if (settingsSmoke) {
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QTimer::singleShot(200, &app, [&, window] {
            auto findItem = [](auto&& self, QQuickItem* parent, const QString& name) -> QQuickItem* {
                if (parent->objectName() == name) return parent;
                for (auto* child : parent->childItems())
                    if (auto* found = self(self, child, name)) return found;
                return nullptr;
            };
            auto* page = findItem(findItem, window->contentItem(), "settingsPage");
            if (!page) { app.exit(50); return; }
            const QStringList categories{"Interface", "Appearance", "AI", "Voice", "Memory", "Security", "Notifications", "System"};
            for (int width : {1320, 780, 560}) {
                window->setWidth(width);
                for (const auto& category : categories) {
                    page->setProperty("activeCategory", category);
                    QApplication::processEvents();
                    auto* scroll = findItem(findItem, page, "settingsContentScroll");
                    if (!scroll || scroll->width() < 250 || scroll->height() < 300) { app.exit(51); return; }
                }
            }
            QJsonArray layoutReport;
            for (const auto& language : settings.availableLanguages()) {
                settings.setAppLanguage(language);
                for (int width : {1320, 780, 560}) {
                    window->setWidth(width);
                    for (const auto& category : categories) {
                        page->setProperty("activeCategory", category);
                        QApplication::processEvents();
                        auto* scroll = findItem(findItem, page, "settingsContentScroll");
                        if (!scroll || scroll->width() < 250 || scroll->height() < 300) { app.exit(61); return; }
                        QJsonArray elisions;
                        auto inspectText = [&](auto&& self, QQuickItem* item) -> void {
                            if (!item->isVisible()) return;
                            if (item->property("truncated").toBool() && elisions.size() < 40)
                                elisions.append(QJsonObject{{"name", item->objectName()}, {"text", item->property("text").toString().left(240)}, {"width", item->width()}});
                            for (auto* child : item->childItems()) self(self, child);
                        };
                        inspectText(inspectText, page);
                        layoutReport.append(QJsonObject{{"language", language}, {"width", width}, {"category", category}, {"contentWidth", scroll->width()}, {"contentHeight", scroll->height()}, {"elisionsForReview", elisions}});
                    }
                }
            }
            window->setWidth(1320);
            settings.setAppLanguage("ar");
            QApplication::processEvents();
            auto* sidebar = findItem(findItem, page, "settingsCategorySidebar");
            auto* rtlContent = findItem(findItem, page, "settingsContentScroll");
            if (!sidebar || !rtlContent || sidebar->mapToScene(QPointF()).x() <= rtlContent->mapToScene(QPointF()).x()) { app.exit(62); return; }
            QTest::qWait(40);
            window->grabWindow().save(QDir::tempPath() + "/sentinel-settings-rtl.png");
            QFile layoutFile(QDir::tempPath() + "/sentinel-settings-language-layout.json");
            if (layoutFile.open(QIODevice::WriteOnly)) layoutFile.write(QJsonDocument(layoutReport).toJson());
            settings.setAppLanguage("en");
            window->setWidth(1320);
            page->setProperty("activeCategory", "Notifications");
            QApplication::processEvents();
            auto* dnd = findItem(findItem, page, "settingsDndSwitch");
            if (!dnd || !dnd->isVisible()) { app.exit(52); return; }
            const auto* accessibleDnd = QAccessible::queryAccessibleInterface(dnd);
            if (!accessibleDnd || accessibleDnd->text(QAccessible::Name).isEmpty()) { app.exit(60); return; }
            dnd->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyClick(window, Qt::Key_Space);
            if (!viewModel.dndEnabled()) { app.exit(53); return; }
            QTest::keyClick(window, Qt::Key_Space);
            if (viewModel.dndEnabled()) { app.exit(54); return; }
            page->setProperty("activeCategory", "Appearance");
            for (const auto& theme : QStringList{"Liquid Glass Light", "Paper", "Linen", "Sage Light", "Sky Light", "Solarized Light"}) {
                viewModel.setThemeName(theme);
                QApplication::processEvents();
                if (viewModel.themeName() != theme) { app.exit(57); return; }
                const auto background = window->property("settingsBackground").value<QColor>();
                const auto text = window->property("settingsText").value<QColor>();
                if (background.lightnessF() < 0.8 || text.lightnessF() > 0.4) { app.exit(55); return; }
            }
            QJsonArray contrastReport;
            auto luminance = [](const QColor& color) {
                auto linear = [](double v) { return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
                return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
            };
            for (const auto& theme : viewModel.availableThemes()) {
                viewModel.setThemeName(theme);
                for (bool highContrast : {false, true}) {
                    viewModel.setHighContrastEnabled(highContrast);
                    QApplication::processEvents();
                    for (const auto& backgroundKey : {"settingsBackground", "settingsSurface"}) {
                        const auto background = window->property(backgroundKey).value<QColor>();
                        for (const auto& textKey : {"settingsText", "settingsMuted"}) {
                            const auto text = window->property(textKey).value<QColor>();
                            const double a = luminance(background), b = luminance(text);
                            const double ratio = (qMax(a, b) + 0.05) / (qMin(a, b) + 0.05);
                            contrastReport.append(QJsonObject{{"theme", theme}, {"highContrast", highContrast}, {"background", backgroundKey}, {"text", textKey}, {"ratio", ratio}, {"AA_normal_text", ratio >= 4.5}});
                        }
                    }
                }
            }
            QFile contrastFile(QDir::tempPath() + "/sentinel-settings-contrast.json");
            if (!contrastFile.open(QIODevice::WriteOnly)) { app.exit(58); return; }
            contrastFile.write(QJsonDocument(contrastReport).toJson());
            contrastFile.close();
            for (const auto& pair : contrastReport) {
                if (!pair.toObject().value("AA_normal_text").toBool()) {
                    qWarning() << "Contrast below AA" << pair; app.exit(59); return;
                }
            }
            viewModel.setReducedMotionEnabled(true);
            viewModel.setReducedTransparencyEnabled(true);
            viewModel.setHighContrastEnabled(true);
            QApplication::processEvents();
            if (window->property("settingsMotionDuration").toInt() != 0 ||
                window->property("settingsPanel").value<QColor>().alpha() != 255) { app.exit(56); return; }
            viewModel.setHighContrastEnabled(false);
            viewModel.setThemeName("Paper");
            QTimer::singleShot(200, &app, [&, window] {
                const auto screenshotPath = QDir::tempPath() + "/sentinel-settings-appearance.png";
                const bool saved = window->grabWindow().save(screenshotPath);
                auto* page = window->findChild<QQuickItem*>("settingsPage");
                if (page) {
                    page->setProperty("activeCategory", "System");
                    QTest::qWait(80);
                    window->grabWindow().save(QDir::tempPath() + "/sentinel-settings-backup.png");
                    page->setProperty("activeCategory", "Memory");
                    QTest::qWait(80);
                    window->grabWindow().save(QDir::tempPath() + "/sentinel-settings-profile.png");
                }
                qInfo() << "Settings screenshot" << screenshotPath << saved;
                qInfo("Settings categories, responsive layout, keyboard DND, light palettes, motion and opacity smoke passed");
                app.exit(0);
            });
        });
    }
    if (chatSmoke) {
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        window->setProperty("surface", 2);
        QTimer::singleShot(300, &app, [&, window] {
            auto findItem = [](auto&& self, QQuickItem* parent, const QString& name) -> QQuickItem* {
                if (parent->objectName() == name) return parent;
                for (auto* child : parent->childItems()) if (auto* found = self(self, child, name)) return found;
                return nullptr;
            };
            auto* greeting = findItem(findItem, window->contentItem(), "chatGreeting");
            auto* composer = findItem(findItem, window->contentItem(), "chatComposer");
            auto* input = findItem(findItem, window->contentItem(), "chatPromptInput");
            if (!greeting || !composer || !input || !greeting->isVisible() || composer->height() < 100) { app.exit(30); return; }
            const auto first = viewModel.createConversation("Empty one");
            const auto second = viewModel.createConversation("Empty two");
            viewModel.switchConversation(first);
            QApplication::processEvents();
            if (!greeting->isVisible()) { app.exit(31); return; }
            viewModel.switchConversation(second);
            QApplication::processEvents();
            if (!greeting->isVisible()) { app.exit(32); return; }
            // No model is configured in this disposable profile: exercise the real failed-response state.
            viewModel.sendMessage("Test message for alignment");
            QApplication::processEvents();
            QTimer::singleShot(200, &app, [&, window, findItem, greeting, composer, input] {
                auto* user = findItem(findItem, window->contentItem(), "userMessageBubble");
                auto* assistant = findItem(findItem, window->contentItem(), "assistantMessageBubble");
                if (!user || !assistant || greeting->isVisible() || user->x() <= assistant->x()) { qWarning() << "Alignment failure" << user << assistant << greeting->isVisible() << viewModel.conversationHistoryMessageCount() << (user ? user->x() : -1) << (assistant ? assistant->x() : -1); app.exit(33); return; }
                window->setWidth(780); window->setHeight(640);
                QApplication::processEvents();
                auto* attach = findItem(findItem, window->contentItem(), "chatAttachButton");
                auto* send = findItem(findItem, window->contentItem(), "chatSendButton");
                if (!attach || !send || attach->mapToScene(QPointF()).x() >= send->mapToScene(QPointF()).x() || composer->height() < 100) { app.exit(34); return; }
                input->forceActiveFocus(Qt::TabFocusReason);
                QTest::keyClick(window, Qt::Key_Tab);
                if (!window->activeFocusItem() || !window->activeFocusItem()->isVisible()) { app.exit(35); return; }
                qInfo("Empty conversation, alignment, failed response, responsive composer and keyboard focus smoke passed");
                app.exit(0);
            });
        });
    }
    if (navigationSmoke) {
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QTimer::singleShot(100, &app, [&, window] {
            auto findItem = [](auto&& self, QQuickItem* parent, const QString& name) -> QQuickItem* {
                if (parent->objectName() == name) return parent;
                for (auto* child : parent->childItems())
                    if (auto* found = self(self, child, name)) return found;
                return nullptr;
            };
            auto* home = findItem(findItem, window->contentItem(), "DashboardNavigationButton");
            auto* preferences = findItem(findItem, window->contentItem(), "settingsNavigationButton");
            if (!home || !preferences) { app.exit(23); return; }
            home->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyClick(window, Qt::Key_Space);
            QApplication::processEvents();
            if (window->property("surface").toInt() != 2) { app.exit(24); return; }
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                             preferences->mapToScene(QPointF(preferences->width() / 2,
                                                            preferences->height() / 2)).toPoint());
            QApplication::processEvents();
            if (window->property("surface").toInt() != 0) { app.exit(25); return; }
        });
    }
    if (focusSmoke) {
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        auto* onboarding = window->findChild<QQuickItem*>("sentinelOnboarding");
        auto* next = window->findChild<QQuickItem*>("onboardingNext");
        auto* back = window->findChild<QQuickItem*>("onboardingBack");
        QTimer::singleShot(200, &app, [&, window, onboarding, next, back] {
            auto contained = [&] {
                auto* item = window->activeFocusItem();
                if (!item || !item->isVisible() || !item->isEnabled())
                    return false;
                while (item && item != onboarding)
                    item = item->parentItem();
                return item == onboarding;
            };
            auto key = [&](int code, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
                QTest::keyClick(window, Qt::Key(code), modifiers);
                QApplication::processEvents();
            };
            if (!onboarding || !next || !back || !contained()) {
                app.exit(10);
                return;
            }
            for (int step = 0; step < 7; ++step) {
                if (step > 0 && !viewModel.advanceOnboarding(true)) {
                    app.exit(11);
                    return;
                }
                QApplication::processEvents();
                window->requestActivate();
                QApplication::processEvents();
                next->forceActiveFocus(Qt::TabFocusReason);
                QSet<QQuickItem*> visited;
                for (int i = 0; i < 24; ++i) {
                    key(Qt::Key_Tab);
                    if (!contained()) {
                        qWarning() << "Tab escaped" << step << i << window->activeFocusItem();
                        app.exit(12);
                        return;
                    }
                    visited.insert(window->activeFocusItem());
                    key(Qt::Key_Backtab, Qt::ShiftModifier);
                    if (!contained()) {
                        qWarning() << "Backtab escaped" << step << i << window->activeFocusItem();
                        app.exit(13);
                        return;
                    }
                    key(Qt::Key_Tab);
                }
                if (step > 0 && !visited.contains(back)) {
                    app.exit(14);
                    return;
                }
            }
            viewModel.backOnboarding();
            QApplication::processEvents();
            next->forceActiveFocus();
            key(Qt::Key_Space);
            if (viewModel.onboardingStepIndex() != 6 || !contained()) {
                app.exit(15);
                return;
            }
            qInfo("Onboarding initial/forward/reverse/Back/Space focus smoke passed across seven "
                  "steps");
            app.exit(0);
        });
    }
    QTimer::singleShot(15 * 60 * 1000, &app, &QApplication::quit);
    const auto code = app.exec();
    // Tear down production QML before its stack-owned viewmodel disappears.
    for (auto* root : engine.rootObjects())
        delete root;
    return code;
}

#include "settings_ui_certification_probe.moc"
