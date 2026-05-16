#include "sentinel/core/LocalRuntimeSession.h"

namespace sentinel::core {

QString safeLocalRuntimeAllocationSummary(const LocalRuntimeAllocation& allocation) {
    if (!allocation.summary.isEmpty()) {
        return allocation.summary;
    }

    if (allocation.runtimeId.isEmpty()) {
        return QStringLiteral("No local runtime allocation metadata.");
    }

    return QStringLiteral("%1 allocation for %2")
        .arg(allocation.profile.isEmpty() ? QStringLiteral("Placeholder") : allocation.profile,
             allocation.runtimeId);
}

QString safeLocalRuntimeReservationSummary(const LocalRuntimeReservation& reservation) {
    if (!reservation.summary.isEmpty()) {
        return reservation.summary;
    }

    if (reservation.id.isEmpty()) {
        return QStringLiteral("No local runtime reservation metadata.");
    }

    return QStringLiteral("%1: %2").arg(reservation.id,
                                        localRuntimeSessionStatusName(reservation.status));
}

QString safeLocalRuntimeSessionSummary(const LocalRuntimeSession& session) {
    if (!session.summary.isEmpty()) {
        return session.summary;
    }

    return QStringLiteral("%1: %2 / %3")
        .arg(session.id.value, localRuntimeSessionStatusName(session.status),
             localRuntimeSessionHealthName(session.health));
}

QStringList localRuntimeSessionSummaries(const QList<LocalRuntimeSession>& sessions) {
    QStringList summaries;
    for (const auto& session : sessions) {
        summaries.append(safeLocalRuntimeSessionSummary(session));
    }
    return summaries;
}

QList<LocalRuntimeSession> NullLocalRuntimeSessionManager::sessions() const {
    return {currentSession()};
}

LocalRuntimeSession NullLocalRuntimeSessionManager::currentSession() const {
    LocalRuntimeSession session;
    session.id = LocalRuntimeSessionId{QStringLiteral("local-runtime-session-1")};
    session.status = LocalRuntimeSessionStatus::Reserved;
    session.health = LocalRuntimeSessionHealth::PlaceholderOnly;
    session.allocation = LocalRuntimeAllocation{
        QStringLiteral("null-local-runtime"),
        QStringLiteral("Metadata Only"),
        {
            QStringLiteral("local-runtime.metadata"),
        },
        QStringLiteral("Metadata-only local runtime allocation; no model or process is started."),
    };
    session.reservation = LocalRuntimeReservation{
        QStringLiteral("local-runtime-reservation-1"),
        QStringLiteral("Reserve deterministic placeholder runtime ownership metadata."),
        LocalRuntimeSessionStatus::Reserved,
        QStringLiteral("Placeholder reservation is held for metadata visibility only."),
    };
    session.summary =
        QStringLiteral("local-runtime-session-1: Reserved placeholder local runtime metadata.");
    return session;
}

} // namespace sentinel::core
