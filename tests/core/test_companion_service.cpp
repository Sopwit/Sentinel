#include "sentinel/core/CompanionService.h"

#include <QtTest>

using sentinel::core::CompanionService;

class CompanionServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesReadinessOnlyMetadata();
    void exposesSafeActionModel();
};

void CompanionServiceTest::exposesReadinessOnlyMetadata() {
    const CompanionService service;

    const auto disabled = service.summary(false);
    QCOMPARE(disabled.available, false);
    QCOMPARE(disabled.enabledPreference, false);
    QCOMPARE(disabled.status, QStringLiteral("Disabled"));
    QCOMPARE(disabled.availability, QStringLiteral("Readiness Only"));
    QVERIFY(disabled.platformCapability.contains(QStringLiteral("readiness only")));
    QVERIFY(disabled.permissionPostureSummary.contains(QStringLiteral("Disabled by default")));
    QVERIFY(disabled.safetyBoundarySummary.contains(QStringLiteral("no background daemon")));
    QVERIFY(disabled.quickCaptureSummary.contains(QStringLiteral("no note")));
    QCOMPARE(disabled.platformSummaries.size(), 3);
    QCOMPARE(disabled.traceSummaries.size(), 5);

    const auto enabled = service.summary(true);
    QCOMPARE(enabled.available, false);
    QCOMPARE(enabled.enabledPreference, true);
    QCOMPARE(enabled.status, QStringLiteral("Readiness Only"));
    QVERIFY(enabled.traceSummaries.join(QStringLiteral("\n"))
                .contains(QStringLiteral("enabled preference")));
}

void CompanionServiceTest::exposesSafeActionModel() {
    const CompanionService service;
    const auto actions = service.actions();

    QCOMPARE(actions.size(), 6);
    QCOMPARE(actions.first().label, QStringLiteral("Open Sentinel"));
    QVERIFY(actions.last().summary.contains(QStringLiteral("no process exit")));

    for (const auto& action : actions) {
        QVERIFY(!action.available);
        QVERIFY(!action.executionEnabled);
        QVERIFY(sentinel::core::companionActionSummary(action)
                    .contains(QStringLiteral("execution=disabled")));
    }

    const auto summary = service.summary(true).actionSummaries.join(QStringLiteral("\n"));
    QVERIFY(summary.contains(QStringLiteral("Quick note")));
    QVERIFY(summary.contains(QStringLiteral("no filesystem")));
    QVERIFY(summary.contains(QStringLiteral("permission=Disabled")));
}

QTEST_MAIN(CompanionServiceTest)

#include "test_companion_service.moc"
