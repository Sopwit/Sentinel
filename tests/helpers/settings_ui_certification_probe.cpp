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
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTranslator>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Sentinel Settings Certification");
    QStandardPaths::setTestModeEnabled(true);
    const bool focusSmoke = argc == 2 && QString::fromLocal8Bit(argv[1]) == "--focus-smoke";
    QTemporaryDir temporaryProfile;
    if (argc != 2 || (!focusSmoke && !QDir(QString::fromLocal8Bit(argv[1])).exists()))
        return 2;
    QDir profile(focusSmoke ? temporaryProfile.path() : QString::fromLocal8Bit(argv[1]));
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
    engine.load(QUrl::fromLocalFile(QStringLiteral(CERTIFICATION_QML_WRAPPER)));
    if (engine.rootObjects().isEmpty())
        return 3;
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
