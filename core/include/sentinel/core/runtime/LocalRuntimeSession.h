#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

struct LocalRuntimeSessionId {
    QString value = QStringLiteral("local-runtime-session-1");
};

enum class LocalRuntimeSessionStatus : std::uint8_t {
    NotStarted,
    Reserved,
    Active,
    Suspended,
    Released,
};

inline QString localRuntimeSessionStatusName(LocalRuntimeSessionStatus status) {
    switch (status) {
    case LocalRuntimeSessionStatus::NotStarted:
        return QStringLiteral("Not Started");
    case LocalRuntimeSessionStatus::Reserved:
        return QStringLiteral("Reserved");
    case LocalRuntimeSessionStatus::Active:
        return QStringLiteral("Active");
    case LocalRuntimeSessionStatus::Suspended:
        return QStringLiteral("Suspended");
    case LocalRuntimeSessionStatus::Released:
        return QStringLiteral("Released");
    }

    return QStringLiteral("Not Started");
}

enum class LocalRuntimeSessionHealth : std::uint8_t {
    PlaceholderOnly,
    Ready,
    Unavailable,
};

inline QString localRuntimeSessionHealthName(LocalRuntimeSessionHealth health) {
    switch (health) {
    case LocalRuntimeSessionHealth::PlaceholderOnly:
        return QStringLiteral("Placeholder Only");
    case LocalRuntimeSessionHealth::Ready:
        return QStringLiteral("Ready");
    case LocalRuntimeSessionHealth::Unavailable:
        return QStringLiteral("Unavailable");
    }

    return QStringLiteral("Unavailable");
}

struct LocalRuntimeAllocation {
    QString runtimeId;
    QString profile;
    QStringList capabilityIds;
    QString summary;
};

struct LocalRuntimeReservation {
    QString id;
    QString reason;
    LocalRuntimeSessionStatus status = LocalRuntimeSessionStatus::NotStarted;
    QString summary;
};

struct LocalRuntimeSession {
    LocalRuntimeSessionId id;
    LocalRuntimeSessionStatus status = LocalRuntimeSessionStatus::NotStarted;
    LocalRuntimeSessionHealth health = LocalRuntimeSessionHealth::Unavailable;
    LocalRuntimeAllocation allocation;
    LocalRuntimeReservation reservation;
    int revision = 1;
    QString summary;
};

QString safeLocalRuntimeAllocationSummary(const LocalRuntimeAllocation& allocation);
QString safeLocalRuntimeReservationSummary(const LocalRuntimeReservation& reservation);
QString safeLocalRuntimeSessionSummary(const LocalRuntimeSession& session);
QStringList localRuntimeSessionSummaries(const QList<LocalRuntimeSession>& sessions);

class ILocalRuntimeSessionManager {
public:
    virtual ~ILocalRuntimeSessionManager() = default;

    virtual QList<LocalRuntimeSession> sessions() const = 0;
    virtual LocalRuntimeSession currentSession() const = 0;
};

class NullLocalRuntimeSessionManager final : public ILocalRuntimeSessionManager {
public:
    QList<LocalRuntimeSession> sessions() const override;
    LocalRuntimeSession currentSession() const override;
};

} // namespace sentinel::core
