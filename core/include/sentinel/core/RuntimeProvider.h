#pragma once

#include "sentinel/core/OllamaRuntime.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <utility>

namespace sentinel::core {

enum class RuntimeReadinessState : std::uint8_t {
    Ready,
    Unavailable,
    InvalidEndpoint,
    MissingModel,
    Busy,
    Disabled,
    Incompatible,
    Unauthorized,
    Unknown,
};

QString runtimeReadinessStateName(RuntimeReadinessState state);

struct RuntimeCapabilitySet {
    bool localOnly = true;
    bool requiresApiKey = false;
    bool supportsOffline = true;
    bool supportsStructuredOutput = false;
    bool supportsReasoning = false;
    bool supportsFunctionCalling = false;
    bool supportsImages = false;
    bool supportsAudio = false;
    bool supportsStreaming = false;
    bool supportsTools = false;
    bool supportsVision = false;
    bool supportsEmbeddings = false;
};

struct RuntimeProviderDescriptor {
    QString providerId;
    QString displayName;
    QString runtimeState;
    RuntimeReadinessState readiness = RuntimeReadinessState::Unknown;
    QString validationState;
    QString endpointSummary;
    QString modelSummary;
    QString readinessReason;
    RuntimeCapabilitySet capabilities;
    QStringList modelNames;
    bool installed = false;
    bool configured = false;
    bool enabled = false;
};

QString runtimeCapabilitySummary(const RuntimeCapabilitySet& capabilities);
QString runtimeProviderSummary(const RuntimeProviderDescriptor& provider);
QString runtimeProviderCardSummary(const RuntimeProviderDescriptor& provider);
QStringList runtimeProviderSummaries(const QList<RuntimeProviderDescriptor>& providers);
QStringList runtimeProviderCapabilitySummaries(const QList<RuntimeProviderDescriptor>& providers);

class LocalRuntimeProvider {
public:
    virtual ~LocalRuntimeProvider() = default;

    virtual RuntimeProviderDescriptor descriptor() const = 0;
};

class OllamaRuntimeProvider final : public LocalRuntimeProvider {
public:
    OllamaRuntimeProvider(QString endpoint, OllamaHealthCheckResult health,
                          QList<OllamaModelSummary> models, QString selectedModel,
                          bool localChatEnabled, bool busy);

    RuntimeProviderDescriptor descriptor() const override;

private:
    QString endpoint_;
    OllamaHealthCheckResult health_;
    QList<OllamaModelSummary> models_;
    QString selectedModel_;
    bool localChatEnabled_ = false;
    bool busy_ = false;
};

class OpenAICompatibleRuntimeProvider final : public LocalRuntimeProvider {
public:
    RuntimeProviderDescriptor descriptor() const override;
};

class RuntimeProviderRegistry final {
public:
    RuntimeProviderRegistry(QList<RuntimeProviderDescriptor> providers,
                            QString selectedProviderId = QStringLiteral("ollama"));

    QList<RuntimeProviderDescriptor> providers() const;
    RuntimeProviderDescriptor activeProvider() const;
    QString selectedProviderId() const;
    QString activeProviderId() const;
    QString activeProviderDisplayName() const;
    QString activeModelLabel() const;
    QString activeReadinessState() const;
    QString activeReadinessSummary() const;
    QString activeLocalOnlySummary() const;
    QStringList installedProviderSummaries() const;
    QStringList configuredProviderSummaries() const;
    QStringList availableLocalRuntimeSummaries() const;
    QStringList validationTraceSummaries() const;

private:
    RuntimeProviderDescriptor fallbackProvider() const;

    QList<RuntimeProviderDescriptor> providers_;
    QString selectedProviderId_;
};

} // namespace sentinel::core
