// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
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

QTEST_MAIN(TestDefaultPlatformServices)
#include "test_default_platform_services.moc"
