#include "sentinel/desktop/DesktopShellViewModel.h"

#include "sentinel/core/AppMetadata.h"
#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/JsonSettingsStore.h"
#include "sentinel/core/LocalEchoProvider.h"
#include "sentinel/core/ModeManager.h"
#include "sentinel/core/NullAgentRuntime.h"
#include "sentinel/core/SQLiteChatHistoryStore.h"
#include "sentinel/core/SQLiteMemoryStore.h"
#include "sentinel/core/StandardPathProvider.h"

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <memory>

namespace {

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

} // namespace

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    configureDefaultUiFont();
    QGuiApplication::setApplicationName(sentinel::core::AppMetadata::displayName());
    QGuiApplication::setOrganizationName(sentinel::core::AppMetadata::organizationName());
    QGuiApplication::setApplicationVersion(sentinel::core::AppMetadata::version());
    QGuiApplication::setDesktopFileName(sentinel::core::AppMetadata::appId());
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/dev.sentinel.Sentinel.png")));

    sentinel::core::StandardPathProvider pathProvider;
    sentinel::core::ApplicationController controller(
        std::make_unique<sentinel::core::LocalEchoProvider>(),
        std::make_unique<sentinel::core::SQLiteMemoryStore>(pathProvider.memoryDatabasePath()),
        nullptr,
        std::make_unique<sentinel::core::SQLiteChatHistoryStore>(
            pathProvider.chatHistoryDatabasePath()),
        std::make_unique<sentinel::core::NullAgentRuntime>());
    sentinel::core::ModeManager modeManager;
    sentinel::core::AppSettings settings(
        std::make_unique<sentinel::core::JsonSettingsStore>(pathProvider.settingsFilePath()));
    controller.setRoutingModeByName(settings.routingModeName());
    sentinel::desktop::DesktopShellViewModel shellViewModel(controller, modeManager, settings);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("shellViewModel"), &shellViewModel);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("Sentinel.Desktop"), QStringLiteral("Main"));

    return QGuiApplication::exec();
}
