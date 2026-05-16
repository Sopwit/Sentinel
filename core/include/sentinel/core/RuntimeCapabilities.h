#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class RuntimeCapabilityState : std::uint8_t {
    Enabled,
    Disabled,
    Unavailable,
};

inline QString runtimeCapabilityStateName(RuntimeCapabilityState state) {
    switch (state) {
    case RuntimeCapabilityState::Enabled:
        return QStringLiteral("Enabled");
    case RuntimeCapabilityState::Disabled:
        return QStringLiteral("Disabled");
    case RuntimeCapabilityState::Unavailable:
        return QStringLiteral("Unavailable");
    }

    return QStringLiteral("Unavailable");
}

enum class RuntimeCapabilityGroup : std::uint8_t {
    Inference,
    Memory,
    Integration,
    Security,
    Platform,
};

inline QString runtimeCapabilityGroupName(RuntimeCapabilityGroup group) {
    switch (group) {
    case RuntimeCapabilityGroup::Inference:
        return QStringLiteral("Inference");
    case RuntimeCapabilityGroup::Memory:
        return QStringLiteral("Memory");
    case RuntimeCapabilityGroup::Integration:
        return QStringLiteral("Integration");
    case RuntimeCapabilityGroup::Security:
        return QStringLiteral("Security");
    case RuntimeCapabilityGroup::Platform:
        return QStringLiteral("Platform");
    }

    return QStringLiteral("Platform");
}

struct RuntimeCapabilityDescriptor {
    QString id;
    QString name;
    RuntimeCapabilityGroup group = RuntimeCapabilityGroup::Platform;
    RuntimeCapabilityState state = RuntimeCapabilityState::Unavailable;
    QString summary;
};

struct RuntimeNegotiationProfile {
    QString id = QStringLiteral("runtime-negotiation-profile-1");
    QString name = QStringLiteral("Metadata-Only Runtime Negotiation");
    bool localOnlyEnforced = true;
    QString summary;
};

struct RuntimeNegotiationResult {
    RuntimeNegotiationProfile profile;
    QList<RuntimeCapabilityDescriptor> capabilities;
    QString summary;
};

QString runtimeCapabilitySummary(const RuntimeCapabilityDescriptor& capability);
QString safeRuntimeNegotiationProfileSummary(const RuntimeNegotiationProfile& profile);
QString safeRuntimeNegotiationSummary(const RuntimeNegotiationResult& result);
QString localOnlyRuntimeEnforcementSummary(const RuntimeNegotiationResult& result);
QStringList runtimeCapabilitySummaries(const QList<RuntimeCapabilityDescriptor>& capabilities);
QStringList
enabledRuntimeCapabilitySummaries(const QList<RuntimeCapabilityDescriptor>& capabilities);
QStringList
disabledRuntimeCapabilitySummaries(const QList<RuntimeCapabilityDescriptor>& capabilities);

class IRuntimeCapabilityRegistry {
public:
    virtual ~IRuntimeCapabilityRegistry() = default;

    virtual QList<RuntimeCapabilityDescriptor> capabilities() const = 0;
    virtual RuntimeNegotiationResult negotiate() const = 0;
};

class StaticRuntimeCapabilityRegistry final : public IRuntimeCapabilityRegistry {
public:
    StaticRuntimeCapabilityRegistry();
    explicit StaticRuntimeCapabilityRegistry(QList<RuntimeCapabilityDescriptor> capabilities);

    QList<RuntimeCapabilityDescriptor> capabilities() const override;
    RuntimeNegotiationResult negotiate() const override;

private:
    QList<RuntimeCapabilityDescriptor> capabilities_;
};

} // namespace sentinel::core
