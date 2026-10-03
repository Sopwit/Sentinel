// SPDX-License-Identifier: GPL-3.0-or-later
// Opt-in UI harness: production QML and viewmodel, disposable disk stores,
// explicitly injected in-memory credential backend. No provider success is faked.
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/app/ModeManager.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/memory/SQLiteMemoryStore.h"
#include "sentinel/core/chat/SQLiteConversationStore.h"
#include "sentinel/desktop/DesktopShellViewModel.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTranslator>
#include <QTimer>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Sentinel Settings Certification");
    QStandardPaths::setTestModeEnabled(true);
    if (argc != 2 || !QDir(QString::fromLocal8Bit(argv[1])).exists()) return 2;
    QDir profile(QString::fromLocal8Bit(argv[1]));
    sentinel::core::AppSettings settings(
        std::make_unique<sentinel::core::JsonSettingsStore>(profile.filePath("settings.json")),
        sentinel::core::inMemoryTestCredentialStore());
    sentinel::core::ApplicationControllerBuilder builder;
    builder.withModelService(std::make_unique<sentinel::core::ModelService>(&settings))
        .withMemoryStore(std::make_unique<sentinel::core::SQLiteMemoryStore>(profile.filePath("memory.sqlite3")))
        .withConversationStore(std::make_unique<sentinel::core::SQLiteConversationStore>(profile.filePath("chat.sqlite3")));
    auto controller = builder.build();
    sentinel::core::ModeManager modes;
    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral(CERTIFICATION_QML_IMPORT));
    QTranslator translator;
    auto translate = [&] {
        app.removeTranslator(&translator);
        if (translator.load(QStringLiteral(CERTIFICATION_TRANSLATIONS) + "/sentinel_" + settings.appLanguage() + ".qm"))
            app.installTranslator(&translator);
        engine.retranslate();
    };
    translate();
    QObject::connect(&settings, &sentinel::core::AppSettings::appLanguageChanged, &app, translate);
    // Match production bootstrap: translator changes precede viewmodel language signals.
    sentinel::desktop::DesktopShellViewModel viewModel(*controller, modes, settings);
    engine.rootContext()->setContextProperty("shellViewModel", &viewModel);
    engine.load(QUrl::fromLocalFile(QStringLiteral(CERTIFICATION_QML_WRAPPER)));
    if (engine.rootObjects().isEmpty()) return 3;
    QTimer::singleShot(15 * 60 * 1000, &app, &QApplication::quit);
    const auto code = app.exec();
    // Tear down production QML before its stack-owned viewmodel disappears.
    for (auto* root : engine.rootObjects()) delete root;
    return code;
}
