// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/app/CompanionService.h"

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

    const auto summary = service.summary(false);
    QCOMPARE(summary.available, false);
    QCOMPARE(summary.enabledPreference, true);
    QCOMPARE(summary.status, QStringLiteral("Readiness Only"));
    QCOMPARE(summary.availability, QStringLiteral("Unavailable"));
    QVERIFY(summary.platformCapability.contains(QStringLiteral("native integration unavailable")));
    QVERIFY(summary.permissionPostureSummary.contains(QStringLiteral("foreground-safe shell")));
    QVERIFY(summary.safetyBoundarySummary.contains(QStringLiteral("no background daemon")));
    QVERIFY(summary.quickCaptureSummary.contains(QStringLiteral("writes a short note")));
    QCOMPARE(summary.platformSummaries.size(), 3);
    QCOMPARE(summary.traceSummaries.size(), 6);
    QVERIFY(summary.traceSummaries.join(QStringLiteral("\n"))
                .contains(QStringLiteral("permanently enabled")));
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
        QVERIFY(sentinel::core::companionActionSummary(action).contains(
            QStringLiteral("execution=disabled")));
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

    const auto activeActions = service.actions(true, false);
    QCOMPARE(activeActions.at(0).label, QStringLiteral("Open Sentinel"));
    QVERIFY(activeActions.at(0).available);
    QVERIFY(activeActions.at(0).executionEnabled);
    QVERIFY(activeActions.at(2).available);
    QVERIFY(activeActions.at(2).executionEnabled);
    QCOMPARE(activeActions.at(3).label, QStringLiteral("Pause Companion"));

    const auto pausedActions = service.actions(true, true);
    QVERIFY(!pausedActions.at(2).available);
    QCOMPARE(pausedActions.at(3).label, QStringLiteral("Resume Companion"));

    const auto captured = service.captureQuickNote(QStringLiteral("remember this idea"), true, false);
    QVERIFY(captured.captured);
    QCOMPARE(captured.capturedCharacterCount, 18);
    QVERIFY(captured.summary.contains(QStringLiteral("Quick capture succeeded")));

    const auto refused = service.captureQuickNote(QStringLiteral("note while paused"), true, true);
    QVERIFY(!refused.captured);
    QVERIFY(refused.summary.contains(QStringLiteral("not active")));

    const auto empty = service.captureQuickNote(QStringLiteral("   "), true, false);
    QVERIFY(!empty.captured);
    QVERIFY(empty.summary.contains(QStringLiteral("empty")));

    const auto pausedSummary = service.summary(true, true, true);
    QCOMPARE(pausedSummary.status, QStringLiteral("Paused"));
    QVERIFY(pausedSummary.traceSummaries.join(QStringLiteral("\n"))
                .contains(QStringLiteral("pause: paused")));
}

QTEST_MAIN(CompanionServiceTest)

#include "test_companion_service.moc"
