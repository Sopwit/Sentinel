#include "sentinel/core/ExecutionLifecycle.h"

#include <QtTest>

using sentinel::core::ExecutionCoordinator;
using sentinel::core::ExecutionLifecycleState;
using sentinel::core::executionLifecycleStateName;
using sentinel::core::ExecutionLifecycleStatus;
using sentinel::core::executionLifecycleStatusName;
using sentinel::core::executionLifecycleTraceSummaries;
using sentinel::core::ExecutionRequest;
using sentinel::core::ExecutionSessionStatus;
using sentinel::core::executionSessionStatusName;
using sentinel::core::safeExecutionCoordinationSnapshotSummary;
using sentinel::core::safeExecutionLifecycleSummary;
using sentinel::core::safeExecutionSessionSummary;
using sentinel::core::StaticExecutionLifecycle;

class StaticExecutionLifecycleTest final : public QObject {
    Q_OBJECT

private slots:
    void namesLifecycleMetadata();
    void evaluatesDeterministicBlockedLifecycle();
    void rejectsInvalidTransitionsSafely();
    void recordsSessionOwnershipMetadata();
    void exposesReadOnlyCoordinationSnapshot();
};

void StaticExecutionLifecycleTest::namesLifecycleMetadata() {
    QCOMPARE(executionLifecycleStateName(ExecutionLifecycleState::Requested),
             QStringLiteral("Requested"));
    QCOMPARE(executionLifecycleStateName(ExecutionLifecycleState::PermissionCheck),
             QStringLiteral("Permission Check"));
    QCOMPARE(executionLifecycleStateName(ExecutionLifecycleState::ReadyPlaceholder),
             QStringLiteral("Ready Placeholder"));
    QCOMPARE(executionLifecycleStatusName(ExecutionLifecycleStatus::Blocked),
             QStringLiteral("Blocked"));
    QCOMPARE(executionSessionStatusName(ExecutionSessionStatus::Reserved),
             QStringLiteral("Reserved"));
}

void StaticExecutionLifecycleTest::evaluatesDeterministicBlockedLifecycle() {
    const StaticExecutionLifecycle lifecycle;
    const auto result = lifecycle.evaluate(ExecutionRequest{});

    QCOMPARE(result.status, ExecutionLifecycleStatus::Blocked);
    QCOMPARE(result.state, ExecutionLifecycleState::Blocked);
    QVERIFY(!result.executable);
    QCOMPARE(result.traces.size(), 7);
    for (int i = 0; i < result.traces.size(); ++i) {
        QCOMPARE(result.traces[i].sequence, i + 1);
    }
    QCOMPARE(result.traces[0].state, ExecutionLifecycleState::Requested);
    QCOMPARE(result.traces[1].state, ExecutionLifecycleState::Validating);
    QCOMPARE(result.traces[2].state, ExecutionLifecycleState::PermissionCheck);
    QCOMPARE(result.traces[3].state, ExecutionLifecycleState::SafetyCheck);
    QCOMPARE(result.traces[4].state, ExecutionLifecycleState::Coordination);
    QCOMPARE(result.traces[5].state, ExecutionLifecycleState::ReadyPlaceholder);
    QCOMPARE(result.traces[6].state, ExecutionLifecycleState::Blocked);
    QCOMPARE(safeExecutionLifecycleSummary(result),
             QStringLiteral("Execution lifecycle reached blocked metadata state; no execution is "
                            "permitted."));
    QVERIFY(executionLifecycleTraceSummaries(result.traces)
                .contains(QStringLiteral(
                    "7. Blocked [Blocked]: Execution remains intentionally blocked.")));
}

void StaticExecutionLifecycleTest::rejectsInvalidTransitionsSafely() {
    const StaticExecutionLifecycle lifecycle;
    const auto accepted =
        lifecycle.transition(ExecutionLifecycleState::Requested,
                             ExecutionLifecycleState::Validating, ExecutionRequest{});
    QCOMPARE(accepted.status, ExecutionLifecycleStatus::MetadataOnly);
    QVERIFY(accepted.transitionAccepted);
    QVERIFY(!accepted.executable);

    const auto rejected =
        lifecycle.transition(ExecutionLifecycleState::Requested,
                             ExecutionLifecycleState::SafetyCheck, ExecutionRequest{});
    QCOMPARE(rejected.status, ExecutionLifecycleStatus::Rejected);
    QCOMPARE(rejected.state, ExecutionLifecycleState::Requested);
    QVERIFY(!rejected.transitionAccepted);
    QVERIFY(!rejected.executable);
    QCOMPARE(rejected.summary,
             QStringLiteral("Execution lifecycle transition rejected: Requested to Safety "
                            "Check."));
}

void StaticExecutionLifecycleTest::recordsSessionOwnershipMetadata() {
    const ExecutionCoordinator coordinator;
    const auto& session = coordinator.session();

    QCOMPARE(session.id.value, QStringLiteral("execution-session-1"));
    QCOMPARE(session.status, ExecutionSessionStatus::Reserved);
    QCOMPARE(safeExecutionSessionSummary(session),
             QStringLiteral("Execution session is reserved for metadata only."));
}

void StaticExecutionLifecycleTest::exposesReadOnlyCoordinationSnapshot() {
    const StaticExecutionLifecycle lifecycle;
    const ExecutionCoordinator coordinator;
    const auto snapshot = coordinator.coordinate(lifecycle.evaluate(ExecutionRequest{}));

    QVERIFY(snapshot.readOnly);
    QVERIFY(!snapshot.executable);
    QCOMPARE(snapshot.lifecycle.status, ExecutionLifecycleStatus::Blocked);
    QCOMPARE(safeExecutionCoordinationSnapshotSummary(snapshot),
             QStringLiteral("Execution coordination snapshot is read-only for "
                            "execution-session-1; lifecycle is Blocked and execution is "
                            "blocked."));
}

QTEST_MAIN(StaticExecutionLifecycleTest)

#include "test_static_execution_lifecycle.moc"
