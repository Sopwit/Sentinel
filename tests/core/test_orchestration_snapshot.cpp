#include "sentinel/core/OrchestrationSnapshot.h"

#include <QtTest>

using sentinel::core::OrchestrationHealthStatus;
using sentinel::core::orchestrationHealthStatusName;
using sentinel::core::OrchestrationSignal;
using sentinel::core::orchestrationSignalSummaries;
using sentinel::core::OrchestrationSnapshot;
using sentinel::core::safeOrchestrationSnapshotSummary;
using sentinel::core::WorkspaceStateSummary;

class OrchestrationSnapshotTest final : public QObject {
    Q_OBJECT

private slots:
    void namesHealthStatus();
    void exposesDeterministicSignalSummaries();
    void keepsSnapshotExecutionDisabledByDefault();
};

void OrchestrationSnapshotTest::namesHealthStatus() {
    QCOMPARE(orchestrationHealthStatusName(OrchestrationHealthStatus::Unknown),
             QStringLiteral("Unknown"));
    QCOMPARE(orchestrationHealthStatusName(OrchestrationHealthStatus::Ready),
             QStringLiteral("Ready"));
    QCOMPARE(orchestrationHealthStatusName(OrchestrationHealthStatus::Degraded),
             QStringLiteral("Degraded"));
}

void OrchestrationSnapshotTest::exposesDeterministicSignalSummaries() {
    OrchestrationSnapshot snapshot;
    snapshot.healthStatus = OrchestrationHealthStatus::Ready;
    snapshot.workspace = WorkspaceStateSummary{
        QStringLiteral("Local Only"),
        QStringLiteral("Routed"),
        QStringLiteral("Local Metadata Provider / Sentinel Local Placeholder"),
        QStringLiteral("Fallback Planned"),
        QStringLiteral("Unknown task uses safe local metadata fallback."),
        QStringLiteral("Atlas (Coordinator, Available, Local)"),
        QStringLiteral("Ambient (Available, Public Metadata, Session)"),
        QStringLiteral("Empty"),
        QStringLiteral("No runtime context yet."),
        QStringLiteral("No agent activity yet."),
        4,
        6,
        5,
        0,
    };
    snapshot.signalList = {
        OrchestrationSignal{QStringLiteral("routing"), QStringLiteral("Routing"),
                            QStringLiteral("Local Only / Routed")},
        OrchestrationSignal{QStringLiteral("catalogs"), QStringLiteral("Catalogs"),
                            QStringLiteral("4 providers / 6 agents / 5 memory")},
    };
    snapshot.summary = QStringLiteral("Ready orchestration snapshot.");

    QCOMPARE(safeOrchestrationSnapshotSummary(snapshot),
             QStringLiteral("Ready orchestration snapshot."));
    QCOMPARE(orchestrationSignalSummaries(snapshot),
             QStringList({QStringLiteral("Routing: Local Only / Routed"),
                          QStringLiteral("Catalogs: 4 providers / 6 agents / 5 memory")}));
}

void OrchestrationSnapshotTest::keepsSnapshotExecutionDisabledByDefault() {
    const OrchestrationSnapshot snapshot;

    QVERIFY(!snapshot.executionEnabled);
    QCOMPARE(safeOrchestrationSnapshotSummary(snapshot), QStringLiteral("Unknown"));
}

QTEST_MAIN(OrchestrationSnapshotTest)

#include "test_orchestration_snapshot.moc"
