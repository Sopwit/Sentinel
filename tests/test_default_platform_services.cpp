// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/platform/DefaultPlatformService.h"

#include <QtTest>

class TestDefaultPlatformServices : public QObject {
    Q_OBJECT

private slots:
    void testPlatformService();
    void testNotificationService();
    void testSystemIntegrationService();
    void testIntegration();
    void testPlugin();
};

void TestDefaultPlatformServices::testPlatformService() {
    sentinel::core::DefaultPlatformService service;
    QVERIFY(!service.platformName().isEmpty());
}

void TestDefaultPlatformServices::testNotificationService() {
    sentinel::core::DefaultNotificationService service;
    QVERIFY(service.isAvailable());
}

void TestDefaultPlatformServices::testSystemIntegrationService() {
    sentinel::core::DefaultSystemIntegrationService service;
    QVERIFY(service.isAvailable());
}

void TestDefaultPlatformServices::testIntegration() {
    sentinel::core::DefaultIntegration integration("test_id", "Test Integration", true);
    QCOMPARE(integration.id(), QStringLiteral("test_id"));
    QCOMPARE(integration.displayName(), QStringLiteral("Test Integration"));
    QVERIFY(integration.isAvailable());
}

void TestDefaultPlatformServices::testPlugin() {
    sentinel::core::DefaultPlugin plugin("test_plugin", "Test Plugin");
    QCOMPARE(plugin.id(), QStringLiteral("test_plugin"));
    QCOMPARE(plugin.displayName(), QStringLiteral("Test Plugin"));
    QVERIFY(!plugin.isInitialized());

    plugin.initialize();
    QVERIFY(plugin.isInitialized());

    plugin.shutdown();
    QVERIFY(!plugin.isInitialized());
}

QTEST_MAIN(TestDefaultPlatformServices)
#include "test_default_platform_services.moc"
