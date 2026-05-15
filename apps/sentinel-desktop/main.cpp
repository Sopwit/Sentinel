#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/FakeProvider.h"
#include "sentinel/core/InMemoryStore.h"
#include "sentinel/core/ModeManager.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <memory>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Sentinel Desktop Alpha"));
    QGuiApplication::setOrganizationName(QStringLiteral("Sentinel"));

    sentinel::core::ApplicationController controller(
        std::make_unique<sentinel::core::FakeProvider>(),
        std::make_unique<sentinel::core::InMemoryStore>());
    sentinel::core::ModeManager modeManager;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("modeManager"), &modeManager);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("Sentinel.Desktop"), QStringLiteral("Main"));

    return QGuiApplication::exec();
}
