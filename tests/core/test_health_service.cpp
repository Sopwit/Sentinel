#include "sentinel/core/observability/HealthService.h"
#include <QtTest>

using namespace sentinel::core;

class HealthServiceTest final : public QObject {
    Q_OBJECT
private slots:
    void reportsCriticalReadiness();
};

void HealthServiceTest::reportsCriticalReadiness() {
    HealthService service;
    service.setCheck({QStringLiteral("database"), true, true, QStringLiteral("ok")});
    service.setCheck({QStringLiteral("provider"), false, false, QStringLiteral("offline")});
    QVERIFY(service.report().healthy == false);
    QVERIFY(service.report().ready);
    service.setCheck({QStringLiteral("provider"), true, true, QStringLiteral("ok")});
    QVERIFY(service.report().healthy);
    QVERIFY(service.report().ready);
}

QTEST_MAIN(HealthServiceTest)
#include "test_health_service.moc"
