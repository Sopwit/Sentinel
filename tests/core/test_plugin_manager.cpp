// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginDependencyResolver.h"
#include "sentinel/core/plugin/PluginManager.h"
#include "sentinel/core/plugin/PluginManifest.h"
#include "sentinel/core/plugin/PluginPermissions.h"
#include "sentinel/core/plugin/PluginSandbox.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace sentinel::core::plugin;

class PluginManagerTest : public QObject {
    Q_OBJECT

private slots:
    void testVersionRequirementCheck();
    void testManifestParsing();
    void testPermissionsAndSandbox();
    void testDependencyResolverSuccess();
    void testDependencyResolverMissingDep();
    void testDependencyResolverCircularDep();
    void testPluginManagerLifecycle();
    void testSamplePluginsLoading();
};

void PluginManagerTest::testVersionRequirementCheck() {
    QVERIFY(checkVersionRequirement(QStringLiteral("1.0.0"), QStringLiteral(">=1.0.0")));
    QVERIFY(checkVersionRequirement(QStringLiteral("1.2.3"), QStringLiteral(">=1.0.0")));
    QVERIFY(!checkVersionRequirement(QStringLiteral("0.9.9"), QStringLiteral(">=1.0.0")));

    QVERIFY(checkVersionRequirement(QStringLiteral("2.0.0"), QStringLiteral(">1.5.0")));
    QVERIFY(!checkVersionRequirement(QStringLiteral("1.5.0"), QStringLiteral(">1.5.0")));

    QVERIFY(checkVersionRequirement(QStringLiteral("1.0.0"), QStringLiteral("<=1.0.0")));
    QVERIFY(!checkVersionRequirement(QStringLiteral("1.0.1"), QStringLiteral("<=1.0.0")));

    QVERIFY(checkVersionRequirement(QStringLiteral("1.0.0"), QStringLiteral("==1.0.0")));
    QVERIFY(!checkVersionRequirement(QStringLiteral("1.0.1"), QStringLiteral("==1.0.0")));
}

void PluginManagerTest::testManifestParsing() {
    QJsonObject json;
    json[QStringLiteral("id")] = QStringLiteral("test.plugin");
    json[QStringLiteral("name")] = QStringLiteral("Test Plugin");
    json[QStringLiteral("version")] = QStringLiteral("1.0.0");
    json[QStringLiteral("entry_point")] = QStringLiteral("test_plugin");

    QJsonArray perms;
    perms.append(Permissions::NetworkLoopback);
    perms.append(Permissions::FileSystemRead);
    json[QStringLiteral("permissions")] = perms;

    QJsonObject deps;
    deps[QStringLiteral("dev.sentinel.core")] = QStringLiteral(">=1.0.0");
    json[QStringLiteral("dependencies")] = deps;

    QString error;
    PluginManifest manifest = PluginManifest::parseJson(json, &error);
    QVERIFY2(manifest.isValid(), qPrintable(error));
    QCOMPARE(manifest.id, QStringLiteral("test.plugin"));
    QCOMPARE(manifest.name, QStringLiteral("Test Plugin"));
    QCOMPARE(manifest.version, QStringLiteral("1.0.0"));
    QVERIFY(manifest.permissions.has(Permissions::NetworkLoopback));
    QVERIFY(manifest.permissions.has(Permissions::FileSystemRead));
    QVERIFY(manifest.isCompatibleWithCore(QStringLiteral("1.0.0")));
    QVERIFY(!manifest.isCompatibleWithCore(QStringLiteral("0.5.0")));
}

void PluginManagerTest::testPermissionsAndSandbox() {
    PluginSandbox sandbox;
    PluginPermissions perms;
    perms.grant(Permissions::ToolExecution);
    perms.grant(Permissions::ModelConfigRead);

    sandbox.registerPluginPermissions(QStringLiteral("plugin.a"), perms);

    QVERIFY(sandbox.checkPermission(QStringLiteral("plugin.a"), Permissions::ToolExecution));
    QVERIFY(sandbox.checkPermission(QStringLiteral("plugin.a"), Permissions::ModelConfigRead));
    QVERIFY(!sandbox.checkPermission(QStringLiteral("plugin.a"), Permissions::FileSystemWrite));

    sandbox.grantPermission(QStringLiteral("plugin.a"), Permissions::FileSystemWrite);
    QVERIFY(sandbox.checkPermission(QStringLiteral("plugin.a"), Permissions::FileSystemWrite));

    sandbox.revokePermission(QStringLiteral("plugin.a"), Permissions::ToolExecution);
    QVERIFY(!sandbox.checkPermission(QStringLiteral("plugin.a"), Permissions::ToolExecution));
}

void PluginManagerTest::testDependencyResolverSuccess() {
    PluginManifest m1;
    m1.id = QStringLiteral("plugin.base");
    m1.name = QStringLiteral("Base");
    m1.version = QStringLiteral("1.0.0");
    m1.entryPoint = QStringLiteral("base");

    PluginManifest m2;
    m2.id = QStringLiteral("plugin.dependent");
    m2.name = QStringLiteral("Dependent");
    m2.version = QStringLiteral("1.0.0");
    m2.entryPoint = QStringLiteral("dependent");
    m2.dependencies[QStringLiteral("plugin.base")] = QStringLiteral(">=1.0.0");

    QList<PluginManifest> manifests = {m2, m1};
    ResolutionResult res = PluginDependencyResolver::resolve(manifests);

    QVERIFY(res.success);
    QCOMPARE(res.loadOrder.size(), 2);
    QCOMPARE(res.loadOrder.first(), QStringLiteral("plugin.base"));
    QCOMPARE(res.loadOrder.last(), QStringLiteral("plugin.dependent"));
}

void PluginManagerTest::testDependencyResolverMissingDep() {
    PluginManifest m;
    m.id = QStringLiteral("plugin.orphan");
    m.name = QStringLiteral("Orphan");
    m.version = QStringLiteral("1.0.0");
    m.entryPoint = QStringLiteral("orphan");
    m.dependencies[QStringLiteral("plugin.nonexistent")] = QStringLiteral(">=1.0.0");

    ResolutionResult res = PluginDependencyResolver::resolve({m});
    QVERIFY(!res.success);
    QVERIFY(res.errorMessage.contains(QStringLiteral("Missing plugin dependency")));
}

void PluginManagerTest::testDependencyResolverCircularDep() {
    PluginManifest m1;
    m1.id = QStringLiteral("plugin.a");
    m1.name = QStringLiteral("A");
    m1.version = QStringLiteral("1.0.0");
    m1.entryPoint = QStringLiteral("a");
    m1.dependencies[QStringLiteral("plugin.b")] = QStringLiteral(">=1.0.0");

    PluginManifest m2;
    m2.id = QStringLiteral("plugin.b");
    m2.name = QStringLiteral("B");
    m2.version = QStringLiteral("1.0.0");
    m2.entryPoint = QStringLiteral("b");
    m2.dependencies[QStringLiteral("plugin.a")] = QStringLiteral(">=1.0.0");

    ResolutionResult res = PluginDependencyResolver::resolve({m1, m2});
    QVERIFY(!res.success);
    QVERIFY(res.errorMessage.contains(QStringLiteral("Circular dependency")));
}

void PluginManagerTest::testPluginManagerLifecycle() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Create a mock plugin directory structure
    QString pluginDir = tempDir.path() + QStringLiteral("/mock_plugin");
    QDir().mkpath(pluginDir);

    QJsonObject json;
    json[QStringLiteral("id")] = QStringLiteral("dev.sentinel.mock");
    json[QStringLiteral("name")] = QStringLiteral("Mock Plugin");
    json[QStringLiteral("version")] = QStringLiteral("1.0.0");
    json[QStringLiteral("entry_point")] = QStringLiteral("nonexistent_binary");

    QFile manifestFile(pluginDir + QStringLiteral("/plugin.json"));
    QVERIFY(manifestFile.open(QIODevice::WriteOnly));
    manifestFile.write(QJsonDocument(json).toJson());
    manifestFile.close();

    PluginManager manager(QStringLiteral("1.0.0"), tempDir.path());
    int discovered = manager.discoverPlugins(tempDir.path());
    QCOMPARE(discovered, 1);
    QVERIFY(manager.registeredPluginIds().contains(QStringLiteral("dev.sentinel.mock")));

    const auto* desc = manager.descriptor(QStringLiteral("dev.sentinel.mock"));
    QVERIFY(desc != nullptr);
    QCOMPARE(desc->state, PluginState::Unloaded);

    // Attempting to load binary that does not exist should update state to Error
    QVERIFY(!manager.loadPlugin(QStringLiteral("dev.sentinel.mock")));
    QCOMPARE(manager.pluginState(QStringLiteral("dev.sentinel.mock")), PluginState::Error);
}

void PluginManagerTest::testSamplePluginsLoading() {
    QString binDir = QCoreApplication::applicationDirPath();
    QDir samplesDir(binDir + QStringLiteral("/../plugins/samples"));
    if (!samplesDir.exists()) {
        samplesDir = QDir(binDir + QStringLiteral("/plugins/samples"));
    }

    if (!samplesDir.exists()) {
        QSKIP(
            "Sample plugins build directory not found, skipping live binary plugin loading test.");
    }

    QTemporaryDir dataDir;
    PluginManager manager(QStringLiteral("1.0.0"), dataDir.path());
    int discovered = manager.discoverPlugins(samplesDir.absolutePath());
    QVERIFY(discovered >= 2);

    QVERIFY(manager.registeredPluginIds().contains(
        QStringLiteral("dev.sentinel.plugin.ollama-extended")));
    QVERIFY(
        manager.registeredPluginIds().contains(QStringLiteral("dev.sentinel.plugin.custom-tool")));

    QVERIFY(manager.initializeAll());
    QVERIFY(manager.startAll());

    QCOMPARE(manager.pluginState(QStringLiteral("dev.sentinel.plugin.ollama-extended")),
             PluginState::Active);
    QCOMPARE(manager.pluginState(QStringLiteral("dev.sentinel.plugin.custom-tool")),
             PluginState::Active);

    // Check plugin instance queries
    auto* ollamaPlugin =
        manager.pluginInstance(QStringLiteral("dev.sentinel.plugin.ollama-extended"));
    QVERIFY(ollamaPlugin != nullptr);
    QCOMPARE(ollamaPlugin->displayName(), QStringLiteral("Ollama Extended Provider"));

    // Check sandbox permissions
    QVERIFY(manager.sandbox().checkPermission(QStringLiteral("dev.sentinel.plugin.ollama-extended"),
                                              Permissions::NetworkLoopback));
    QVERIFY(manager.sandbox().checkPermission(QStringLiteral("dev.sentinel.plugin.custom-tool"),
                                              Permissions::ToolExecution));

    QVERIFY(manager.stopAll());
    manager.unloadAll();

    QCOMPARE(manager.pluginState(QStringLiteral("dev.sentinel.plugin.ollama-extended")),
             PluginState::Unloaded);
    QCOMPARE(manager.pluginState(QStringLiteral("dev.sentinel.plugin.custom-tool")),
             PluginState::Unloaded);
}

QTEST_MAIN(PluginManagerTest)
#include "test_plugin_manager.moc"
