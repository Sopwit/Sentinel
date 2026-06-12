#include "sentinel/core/CompanionService.h"

#include <QtTest>

using sentinel::core::CompanionService;

class CompanionServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesReadinessOnlyMetadata();
    void exposesSafeActionModel();
    void exposesNativeActionReadinessAndPausedState();
};

void CompanionServiceTest::exposesReadinessOnlyMetadata() {
    const CompanionService service;

    const auto disabled = service.summary(false);
    QCOMPARE(disabled.available, false);
    QCOMPARE(disabled.enabledPreference, false);
    QCOMPARE(disabled.status, QStringLiteral("Disabled"));
    QCOMPARE(disabled.availability, QStringLiteral("Unavailable"));
    QVERIFY(disabled.platformCapability.contains(QStringLiteral("native integration unavailable")));
    QVERIFY(disabled.permissionPostureSummary.contains(QStringLiteral("foreground-safe shell")));
    QVERIFY(disabled.safetyBoundarySummary.contains(QStringLiteral("no background daemon")));
    QVERIFY(disabled.quickCaptureSummary.contains(QStringLiteral("no note")));
    QCOMPARE(disabled.platformSummaries.size(), 3);
    QCOMPARE(disabled.traceSummaries.size(), 6);

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
    QVERIFY(actions.last().summary.contains(QStringLiteral("normal application quit")));

    for (const auto& action : actions) {
        QVERIFY(!action.available);
        QVERIFY(!action.executionEnabled);
        QVERIFY(sentinel::core::companionActionSummary(action)
                    .contains(QStringLiteral("execution=disabled")));
    }

    const auto summary = service.summary(true).actionSummaries.join(QStringLiteral("\n"));
    QVERIFY(summary.contains(QStringLiteral("Quick Note")));
    QVERIFY(summary.contains(QStringLiteral("no filesystem")));
    QVERIFY(summary.contains(QStringLiteral("permission=Disabled")));
}

void CompanionServiceTest::exposesNativeActionReadinessAndPausedState() {
    const CompanionService service;
    const auto summary = service.summary(true, true, false);

    QVERIFY(summary.available);
    QCOMPARE(summary.status, QStringLiteral("Active"));
    QCOMPARE(summary.availability, QStringLiteral("Native Available"));
    QVERIFY(summary.platformCapability.contains(QStringLiteral("native integration available")));

    const auto actions = service.actions(true, true);
    QCOMPARE(actions.at(0).label, QStringLiteral("Open Sentinel"));
    QVERIFY(actions.at(0).available);
    QVERIFY(actions.at(0).executionEnabled);
    QVERIFY(!actions.at(2).available);
    QVERIFY(!actions.at(2).executionEnabled);
    QCOMPARE(actions.at(3).label, QStringLiteral("Resume Companion"));

    const auto pausedSummary = service.summary(true, true, true);
    QCOMPARE(pausedSummary.status, QStringLiteral("Paused"));
    QVERIFY(pausedSummary.traceSummaries.join(QStringLiteral("\n"))
                .contains(QStringLiteral("pause: paused")));
}

QTEST_MAIN(CompanionServiceTest)

#include "test_companion_service.moc"
