#include "sentinel/core/plugin/PluginHookRegistry.h"
#include <QtTest>

using sentinel::core::plugin::PluginHookRegistry;

class PluginHookRegistryTest final : public QObject {
    Q_OBJECT
private slots:
    void registersAndInvokes();
};

void PluginHookRegistryTest::registersAndInvokes() {
    PluginHookRegistry registry;
    QVERIFY(registry.registerHook(QStringLiteral("transform"), [](const QJsonObject& input) {
        QJsonObject output = input;
        output.insert(QStringLiteral("done"), true);
        return output;
    }));
    QVERIFY(registry.contains(QStringLiteral("transform")));
    QVERIFY(
        registry.invoke(QStringLiteral("transform"), {}).value(QStringLiteral("done")).toBool());
    QVERIFY(registry.unregisterHook(QStringLiteral("transform")));
    QVERIFY(!registry.contains(QStringLiteral("transform")));
}

QTEST_MAIN(PluginHookRegistryTest)
#include "test_plugin_hook_registry.moc"
