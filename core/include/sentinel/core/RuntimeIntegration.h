#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class LocalRuntimeAdapterStatus : std::uint8_t {
    Placeholder,
    NotConfigured,
    Unavailable,
};

inline QString localRuntimeAdapterStatusName(LocalRuntimeAdapterStatus status) {
    switch (status) {
    case LocalRuntimeAdapterStatus::Placeholder:
        return QStringLiteral("Placeholder");
    case LocalRuntimeAdapterStatus::NotConfigured:
        return QStringLiteral("Not Configured");
    case LocalRuntimeAdapterStatus::Unavailable:
        return QStringLiteral("Unavailable");
    }

    return QStringLiteral("Unavailable");
}

enum class LocalRuntimeAdapterHealth : std::uint8_t {
    MetadataOnly,
    NotConnected,
    NotExecutable,
};

inline QString localRuntimeAdapterHealthName(LocalRuntimeAdapterHealth health) {
    switch (health) {
    case LocalRuntimeAdapterHealth::MetadataOnly:
        return QStringLiteral("Metadata Only");
    case LocalRuntimeAdapterHealth::NotConnected:
        return QStringLiteral("Not Connected");
    case LocalRuntimeAdapterHealth::NotExecutable:
        return QStringLiteral("Not Executable");
    }

    return QStringLiteral("Not Executable");
}

struct LocalRuntimeAdapterCapabilitySummary {
    QString id;
    QString name;
    QString summary;
    bool available = false;
    bool executable = false;
};

struct LocalRuntimeAdapterDescriptor {
    QString id = QStringLiteral("ollama-local-runtime-adapter-placeholder");
    QString name = QStringLiteral("Ollama Local Runtime Adapter Placeholder");
    LocalRuntimeAdapterStatus status = LocalRuntimeAdapterStatus::Placeholder;
    LocalRuntimeAdapterHealth health = LocalRuntimeAdapterHealth::MetadataOnly;
    QString summary = QStringLiteral("Ollama local runtime adapter contract is metadata-only; no "
                                     "runtime connection is configured.");
    QList<LocalRuntimeAdapterCapabilitySummary> capabilities;
};

QString
localRuntimeAdapterCapabilitySummary(const LocalRuntimeAdapterCapabilitySummary& capability);
QStringList localRuntimeAdapterCapabilitySummaries(
    const QList<LocalRuntimeAdapterCapabilitySummary>& capabilities);
QString safeLocalRuntimeAdapterSummary(const LocalRuntimeAdapterDescriptor& descriptor);

class ILocalRuntimeAdapter {
public:
    virtual ~ILocalRuntimeAdapter() = default;

    virtual LocalRuntimeAdapterDescriptor descriptor() const = 0;
};

class StaticLocalRuntimeAdapter final : public ILocalRuntimeAdapter {
public:
    LocalRuntimeAdapterDescriptor descriptor() const override;
};

enum class ProviderRuntimeBridgeStatus : std::uint8_t {
    NotConnected,
    NotExecutable,
    Unavailable,
};

inline QString providerRuntimeBridgeStatusName(ProviderRuntimeBridgeStatus status) {
    switch (status) {
    case ProviderRuntimeBridgeStatus::NotConnected:
        return QStringLiteral("Not Connected");
    case ProviderRuntimeBridgeStatus::NotExecutable:
        return QStringLiteral("Not Executable");
    case ProviderRuntimeBridgeStatus::Unavailable:
        return QStringLiteral("Unavailable");
    }

    return QStringLiteral("Unavailable");
}

struct ProviderRuntimeBridgeSummary {
    QString id = QStringLiteral("provider-runtime-bridge-placeholder");
    ProviderRuntimeBridgeStatus status = ProviderRuntimeBridgeStatus::NotConnected;
    QString summary = QStringLiteral("Provider runtime bridge is not connected and cannot execute "
                                     "provider requests.");
    bool connected = false;
    bool executable = false;
};

struct ProviderRuntimeBridgeRequest {
    QString id = QStringLiteral("provider-runtime-bridge-request-1");
    QString providerId = QStringLiteral("ollama-local");
    QString adapterId = QStringLiteral("ollama-local-runtime-adapter-placeholder");
    QString summary = QStringLiteral("Metadata-only provider runtime bridge request.");
};

struct ProviderRuntimeBridgeResponse {
    ProviderRuntimeBridgeRequest request;
    ProviderRuntimeBridgeStatus status = ProviderRuntimeBridgeStatus::NotConnected;
    QString summary = QStringLiteral("Provider runtime bridge request was not connected.");
    bool connected = false;
    bool executable = false;
};

QString safeProviderRuntimeBridgeSummary(const ProviderRuntimeBridgeSummary& summary);
QString safeProviderRuntimeBridgeResponseSummary(const ProviderRuntimeBridgeResponse& response);

class IProviderRuntimeBridge {
public:
    virtual ~IProviderRuntimeBridge() = default;

    virtual ProviderRuntimeBridgeSummary summary() const = 0;
    virtual ProviderRuntimeBridgeResponse
    evaluate(const ProviderRuntimeBridgeRequest& request) const = 0;
};

class StaticProviderRuntimeBridge final : public IProviderRuntimeBridge {
public:
    ProviderRuntimeBridgeSummary summary() const override;
    ProviderRuntimeBridgeResponse
    evaluate(const ProviderRuntimeBridgeRequest& request) const override;
};

enum class RuntimeIntegrationReadiness : std::uint8_t {
    NotReady,
    Blocked,
    ReadyPlaceholder,
};

inline QString runtimeIntegrationReadinessName(RuntimeIntegrationReadiness readiness) {
    switch (readiness) {
    case RuntimeIntegrationReadiness::NotReady:
        return QStringLiteral("Not Ready");
    case RuntimeIntegrationReadiness::Blocked:
        return QStringLiteral("Blocked");
    case RuntimeIntegrationReadiness::ReadyPlaceholder:
        return QStringLiteral("Ready Placeholder");
    }

    return QStringLiteral("Not Ready");
}

struct RuntimeIntegrationCheck {
    QString id;
    QString name;
    bool passed = false;
    QString summary;
};

struct RuntimeIntegrationReport {
    RuntimeIntegrationReadiness readiness = RuntimeIntegrationReadiness::NotReady;
    QString summary;
    QList<RuntimeIntegrationCheck> checks;
    bool executable = false;
};

QString runtimeIntegrationCheckSummary(const RuntimeIntegrationCheck& check);
QStringList runtimeIntegrationCheckSummaries(const QList<RuntimeIntegrationCheck>& checks);
QString safeRuntimeIntegrationReportSummary(const RuntimeIntegrationReport& report);

class StaticRuntimeIntegrationReadiness final {
public:
    RuntimeIntegrationReport evaluate(const LocalRuntimeAdapterDescriptor& adapter,
                                      const ProviderRuntimeBridgeSummary& bridge) const;
};

} // namespace sentinel::core
