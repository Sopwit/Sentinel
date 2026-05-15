#include "sentinel/desktop/DesktopShellViewModel.h"

#include "sentinel/core/AppMetadata.h"
#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/FakeProvider.h"
#include "sentinel/core/InMemoryStore.h"
#include "sentinel/core/JsonSettingsStore.h"
#include "sentinel/core/ModeManager.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>

#include <memory>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(sentinel::core::AppMetadata::displayName());
    QGuiApplication::setOrganizationName(sentinel::core::AppMetadata::organizationName());
    QGuiApplication::setApplicationVersion(sentinel::core::AppMetadata::version());
    QGuiApplication::setDesktopFileName(sentinel::core::AppMetadata::appId());

    sentinel::core::ApplicationController controller(
        std::make_unique<sentinel::core::FakeProvider>(),
        std::make_unique<sentinel::core::InMemoryStore>());
    sentinel::core::ModeManager modeManager;
    const auto configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    sentinel::core::AppSettings settings(std::make_unique<sentinel::core::JsonSettingsStore>(
        configDir + QStringLiteral("/settings.json")));
    sentinel::desktop::DesktopShellViewModel shellViewModel(controller, modeManager, settings);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("shellViewModel"), &shellViewModel);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("Sentinel.Desktop"), QStringLiteral("Main"));

    return QGuiApplication::exec();
}
