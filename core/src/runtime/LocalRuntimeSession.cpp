// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/LocalRuntimeSession.h"

#include <utility>

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

OllamaRuntimeSessionManager::OllamaRuntimeSessionManager(OllamaConfig config)
    : config_(std::move(config)) {}

QList<LocalRuntimeSession> OllamaRuntimeSessionManager::sessions() const {
    return {currentSession()};
}

LocalRuntimeSession OllamaRuntimeSessionManager::currentSession() const {
    const OllamaHttpRuntimeClient client(config_, config_.healthCheckTimeoutMs);
    const auto health = client.healthCheck();
    const auto models = client.installedModels();
    const bool ready = health.healthStatus == OllamaHealthStatus::Healthy && !models.isEmpty();
    LocalRuntimeSession session;
    session.id = {QStringLiteral("ollama-runtime-session-1")};
    session.status =
        ready ? LocalRuntimeSessionStatus::Active : LocalRuntimeSessionStatus::Reserved;
    session.health =
        ready ? LocalRuntimeSessionHealth::Ready : LocalRuntimeSessionHealth::Unavailable;
    session.allocation = {
        QStringLiteral("ollama-local-runtime"),
        QStringLiteral("Ollama"),
        {QStringLiteral("local-runtime.metadata"), QStringLiteral("local-runtime.inference")},
        ready
            ? QStringLiteral("Ollama runtime active with %1 installed model(s).").arg(models.size())
            : safeOllamaHealthSummary(health),
    };
    session.reservation = {
        QStringLiteral("ollama-runtime-reservation-1"),
        QStringLiteral("Local Ollama runtime ownership."),
        session.status,
        session.allocation.summary,
    };
    session.summary = QStringLiteral("ollama-runtime-session-1: %1 / %2")
                          .arg(localRuntimeSessionStatusName(session.status),
                               localRuntimeSessionHealthName(session.health));
    return session;
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
