#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class LocalRuntimeStatus : std::uint8_t {
    MetadataOnly,
    Disabled,
    Unavailable,
};

inline QString localRuntimeStatusName(LocalRuntimeStatus status) {
    switch (status) {
    case LocalRuntimeStatus::MetadataOnly:
        return QStringLiteral("Metadata Only");
    case LocalRuntimeStatus::Disabled:
        return QStringLiteral("Disabled");
    case LocalRuntimeStatus::Unavailable:
        return QStringLiteral("Unavailable");
    }

    return QStringLiteral("Unavailable");
}

enum class LocalRuntimeHealth : std::uint8_t {
    Ready,
    NotExecutable,
    Unavailable,
};

inline QString localRuntimeHealthName(LocalRuntimeHealth health) {
    switch (health) {
    case LocalRuntimeHealth::Ready:
        return QStringLiteral("Ready");
    case LocalRuntimeHealth::NotExecutable:
        return QStringLiteral("Not Executable");
    case LocalRuntimeHealth::Unavailable:
        return QStringLiteral("Unavailable");
    }

    return QStringLiteral("Unavailable");
}

struct LocalRuntimeCapability {
    QString id;
    QString name;
    QString summary;
    bool enabled = false;
};

struct LocalRuntimeDescriptor {
    QString id;
    QString name;
    QString summary;
    LocalRuntimeStatus status = LocalRuntimeStatus::Unavailable;
    LocalRuntimeHealth health = LocalRuntimeHealth::Unavailable;
    QList<LocalRuntimeCapability> capabilities;
};

struct LocalRuntimeRequest {
    QString prompt;
};

struct LocalRuntimeResponse {
    bool accepted = false;
    QString status;
    QString summary = QStringLiteral("No local runtime request was evaluated.");
};

QString localRuntimeCapabilitySummary(const LocalRuntimeCapability& capability);
QString safeLocalRuntimeSummary(const LocalRuntimeDescriptor& descriptor);
QStringList localRuntimeCapabilitySummaries(const QList<LocalRuntimeCapability>& capabilities);
QString safeLocalRuntimeResponseSummary(const LocalRuntimeResponse& response);

class ILocalRuntime {
public:
    virtual ~ILocalRuntime() = default;

    virtual LocalRuntimeDescriptor descriptor() const = 0;
    virtual LocalRuntimeResponse evaluate(const LocalRuntimeRequest& request) const = 0;
};

class NullLocalRuntime final : public ILocalRuntime {
public:
    LocalRuntimeDescriptor descriptor() const override;
    LocalRuntimeResponse evaluate(const LocalRuntimeRequest& request) const override;
};

} // namespace sentinel::core
