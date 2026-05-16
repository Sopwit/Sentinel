#include "sentinel/core/LocalRuntimeSession.h"

#include <QtTest>

using sentinel::core::LocalRuntimeSessionHealth;
using sentinel::core::localRuntimeSessionHealthName;
using sentinel::core::LocalRuntimeSessionStatus;
using sentinel::core::localRuntimeSessionStatusName;
using sentinel::core::localRuntimeSessionSummaries;
using sentinel::core::NullLocalRuntimeSessionManager;
using sentinel::core::safeLocalRuntimeAllocationSummary;
using sentinel::core::safeLocalRuntimeReservationSummary;
using sentinel::core::safeLocalRuntimeSessionSummary;

class LocalRuntimeSessionTest final : public QObject {
    Q_OBJECT

private slots:
    void namesSessionStatusAndHealth();
    void exposesDeterministicPlaceholderSession();
    void preservesSessionSummaryOrdering();
};

void LocalRuntimeSessionTest::namesSessionStatusAndHealth() {
    QCOMPARE(localRuntimeSessionStatusName(LocalRuntimeSessionStatus::NotStarted),
             QStringLiteral("Not Started"));
    QCOMPARE(localRuntimeSessionStatusName(LocalRuntimeSessionStatus::Reserved),
             QStringLiteral("Reserved"));
    QCOMPARE(localRuntimeSessionStatusName(LocalRuntimeSessionStatus::Active),
             QStringLiteral("Active"));
    QCOMPARE(localRuntimeSessionStatusName(LocalRuntimeSessionStatus::Suspended),
             QStringLiteral("Suspended"));
    QCOMPARE(localRuntimeSessionStatusName(LocalRuntimeSessionStatus::Released),
             QStringLiteral("Released"));
    QCOMPARE(localRuntimeSessionHealthName(LocalRuntimeSessionHealth::PlaceholderOnly),
             QStringLiteral("Placeholder Only"));
    QCOMPARE(localRuntimeSessionHealthName(LocalRuntimeSessionHealth::Ready),
             QStringLiteral("Ready"));
    QCOMPARE(localRuntimeSessionHealthName(LocalRuntimeSessionHealth::Unavailable),
             QStringLiteral("Unavailable"));
}

void LocalRuntimeSessionTest::exposesDeterministicPlaceholderSession() {
    const NullLocalRuntimeSessionManager manager;
    const auto session = manager.currentSession();

    QCOMPARE(session.id.value, QStringLiteral("local-runtime-session-1"));
    QCOMPARE(session.status, LocalRuntimeSessionStatus::Reserved);
    QCOMPARE(session.health, LocalRuntimeSessionHealth::PlaceholderOnly);
    QCOMPARE(session.revision, 1);
    QCOMPARE(session.allocation.runtimeId, QStringLiteral("null-local-runtime"));
    QCOMPARE(session.allocation.capabilityIds,
             QStringList{QStringLiteral("local-runtime.metadata")});
    QCOMPARE(safeLocalRuntimeSessionSummary(session),
             QStringLiteral("local-runtime-session-1: Reserved placeholder local runtime "
                            "metadata."));
    QCOMPARE(safeLocalRuntimeAllocationSummary(session.allocation),
             QStringLiteral("Metadata-only local runtime allocation; no model or process is "
                            "started."));
    QCOMPARE(safeLocalRuntimeReservationSummary(session.reservation),
             QStringLiteral("Placeholder reservation is held for metadata visibility only."));
}

void LocalRuntimeSessionTest::preservesSessionSummaryOrdering() {
    const NullLocalRuntimeSessionManager manager;

    QCOMPARE(manager.sessions().size(), 1);
    QCOMPARE(localRuntimeSessionSummaries(manager.sessions()),
             QStringList{QStringLiteral("local-runtime-session-1: Reserved placeholder local "
                                        "runtime metadata.")});
}

QTEST_MAIN(LocalRuntimeSessionTest)

#include "test_local_runtime_session.moc"
