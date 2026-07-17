#include "sentinel/core/RuntimeProvider.h"

namespace sentinel::core {

namespace {

bool modelNamesContain(const QString& model, const QList<OllamaModelSummary>& models) {
    for (const auto& candidate : models) {
        if (candidate.name == model) {
            return true;
        }
    }
    return false;
}

QString capabilityFlag(bool enabled, const QString& label) {
    return QStringLiteral("%1: %2").arg(label,
                                        enabled ? QStringLiteral("yes") : QStringLiteral("no"));
}

RuntimeProviderDescriptor disabledCloudProviderDescriptor(const QString& providerId,
                                                          const QString& displayName) {
    return RuntimeProviderDescriptor{
        providerId,
        displayName,
        QStringLiteral("disabled-placeholder-ready"),
        RuntimeReadinessState::Disabled,
        QStringLiteral("Disabled placeholder only; no API key storage or cloud execution exists."),
        QStringLiteral("No endpoint configured"),
        QStringLiteral("No model metadata"),
        QStringLiteral("%1 is placeholder-ready for future configuration metadata only.")
            .arg(displayName),
        RuntimeCapabilitySet{
            false,
            true,
            false,
            true,
            true,
            true,
            true,
            false,
            true,
            false,
            true,
            false,
        },
        {},
        false,
        false,
        false,
    };
}

} // namespace

QString runtimeReadinessStateName(RuntimeReadinessState state) {
    switch (state) {
    case RuntimeReadinessState::Ready:
        return QStringLiteral("ready");
    case RuntimeReadinessState::Unavailable:
        return QStringLiteral("unavailable");
    case RuntimeReadinessState::InvalidEndpoint:
        return QStringLiteral("invalidEndpoint");
    case RuntimeReadinessState::MissingModel:
        return QStringLiteral("missingModel");
    case RuntimeReadinessState::Busy:
        return QStringLiteral("busy");
    case RuntimeReadinessState::Disabled:
        return QStringLiteral("disabled");
    case RuntimeReadinessState::Incompatible:
        return QStringLiteral("incompatible");
    case RuntimeReadinessState::Unauthorized:
        return QStringLiteral("unauthorized");
    case RuntimeReadinessState::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString runtimeCapabilitySummary(const RuntimeCapabilitySet& capabilities) {
    return QStringList{
        capabilityFlag(capabilities.localOnly, QStringLiteral("localOnly")),
        capabilityFlag(capabilities.requiresApiKey, QStringLiteral("requiresApiKey")),
        capabilityFlag(capabilities.supportsOffline, QStringLiteral("supportsOffline")),
        capabilityFlag(capabilities.supportsStructuredOutput,
                       QStringLiteral("supportsStructuredOutput")),
        capabilityFlag(capabilities.supportsReasoning, QStringLiteral("supportsReasoning")),
        capabilityFlag(capabilities.supportsFunctionCalling,
                       QStringLiteral("supportsFunctionCalling")),
        capabilityFlag(capabilities.supportsImages, QStringLiteral("supportsImages")),
        capabilityFlag(capabilities.supportsAudio, QStringLiteral("supportsAudio")),
        capabilityFlag(capabilities.supportsStreaming, QStringLiteral("supportsStreaming")),
        capabilityFlag(capabilities.supportsTools, QStringLiteral("supportsTools")),
        capabilityFlag(capabilities.supportsVision, QStringLiteral("supportsVision")),
        capabilityFlag(capabilities.supportsEmbeddings, QStringLiteral("supportsEmbeddings")),
    }
        .join(QStringLiteral(" / "));
}

QString runtimeProviderSummary(const RuntimeProviderDescriptor& provider) {
    return QStringLiteral("%1 (%2): %3 / %4 / %5")
        .arg(provider.displayName, provider.providerId, provider.runtimeState,
             runtimeReadinessStateName(provider.readiness),
             provider.readinessReason.isEmpty() ? provider.validationState
                                                : provider.readinessReason);
}

QString runtimeProviderCardSummary(const RuntimeProviderDescriptor& provider) {
    return QStringLiteral("%1 - %2 - %3 - %4")
        .arg(provider.displayName, runtimeReadinessStateName(provider.readiness),
             provider.capabilities.localOnly ? QStringLiteral("Local Only")
                                             : QStringLiteral("External/API placeholder"),
             runtimeCapabilitySummary(provider.capabilities));
}

QStringList runtimeProviderSummaries(const QList<RuntimeProviderDescriptor>& providers) {
    QStringList summaries;
    for (const auto& provider : providers) {
        summaries.append(runtimeProviderSummary(provider));
    }
    return summaries;
}

QStringList runtimeProviderCapabilitySummaries(const QList<RuntimeProviderDescriptor>& providers) {
    QStringList summaries;
    for (const auto& provider : providers) {
        summaries.append(QStringLiteral("%1: %2").arg(
            provider.displayName, runtimeCapabilitySummary(provider.capabilities)));
    }
    return summaries;
}

OllamaRuntimeProvider::OllamaRuntimeProvider(QString endpoint, OllamaHealthCheckResult health,
                                             QList<OllamaModelSummary> models,
                                             QString selectedModel, bool localChatEnabled,
                                             bool busy)
    : endpoint_(std::move(endpoint)), health_(std::move(health)), models_(std::move(models)),
      selectedModel_(std::move(selectedModel)), localChatEnabled_(localChatEnabled), busy_(busy) {}

RuntimeProviderDescriptor OllamaRuntimeProvider::descriptor() const {
    RuntimeReadinessState readiness = RuntimeReadinessState::Unknown;
    QString reason = QStringLiteral("Ollama readiness has not been evaluated.");
    const auto selected = selectedModel_.trimmed();

    if (busy_) {
        readiness = RuntimeReadinessState::Busy;
        reason = QStringLiteral("A local Ollama request is already running.");
    } else if (health_.healthStatus == OllamaHealthStatus::InvalidEndpoint ||
               health_.connectionStatus == OllamaConnectionStatus::Blocked) {
        readiness = RuntimeReadinessState::InvalidEndpoint;
        reason = QStringLiteral("Ollama endpoint must be local loopback HTTP.");
    } else if (health_.healthStatus != OllamaHealthStatus::Healthy) {
        readiness = RuntimeReadinessState::Unavailable;
        reason = safeOllamaHealthSummary(health_);
    } else if (selected.isEmpty()) {
        readiness = RuntimeReadinessState::MissingModel;
        reason = QStringLiteral("No local Ollama model is selected.");
    } else if (models_.isEmpty()) {
        readiness = RuntimeReadinessState::MissingModel;
        reason = QStringLiteral("Ollama model metadata is unavailable.");
    } else if (!modelNamesContain(selected, models_)) {
        readiness = RuntimeReadinessState::MissingModel;
        reason = QStringLiteral("Selected model is not present in Ollama metadata.");
    } else {
        readiness = RuntimeReadinessState::Ready;
        reason = localChatEnabled_ ? QStringLiteral("Local Ollama is ready for explicit sends.")
                                   : QStringLiteral("Ollama is reachable; local chat inference is "
                                                    "disabled until enabled by the user.");
    }

    QStringList names;
    for (const auto& model : models_) {
        names.append(model.name);
    }

    return RuntimeProviderDescriptor{
        QStringLiteral("ollama"),
        QStringLiteral("Local Ollama"),
        localChatEnabled_ ? QStringLiteral("configured") : QStringLiteral("available-disabled"),
        readiness,
        health_.summary,
        endpoint_.isEmpty() ? health_.endpoint : endpoint_,
        selected.isEmpty() ? QStringLiteral("No model selected")
                           : QStringLiteral("Selected model: %1").arg(selected),
        reason,
        RuntimeCapabilitySet{
            true,
            false,
            true,
            false,
            false,
            false,
            false,
            false,
            true,
            false,
            false,
            false,
        },
        names,
        true,
        true,
        true,
    };
}

RuntimeProviderDescriptor OpenAICompatibleRuntimeProvider::descriptor() const {
    return disabledCloudProviderDescriptor(QStringLiteral("openai-compatible"),
                                           QStringLiteral("OpenAI-Compatible API"));
}

OpenAICompatibleLocalRuntimeProvider::OpenAICompatibleLocalRuntimeProvider(QString providerId,
                                                                           QString displayName,
                                                                           QString endpointSummary,
                                                                           QString selectedModel)
    : providerId_(std::move(providerId)), displayName_(std::move(displayName)),
      endpointSummary_(std::move(endpointSummary)), selectedModel_(std::move(selectedModel)) {}

RuntimeProviderDescriptor OpenAICompatibleLocalRuntimeProvider::descriptor() const {
    return RuntimeProviderDescriptor{
        providerId_,
        displayName_,
        QStringLiteral("configuration-required"),
        RuntimeReadinessState::Disabled,
        QStringLiteral("Disabled until the user configures an explicit loopback "
                       "OpenAI-compatible endpoint and model."),
        endpointSummary_.trimmed().isEmpty() ? QStringLiteral("Not configured")
                                             : endpointSummary_.trimmed(),
        selectedModel_.trimmed().isEmpty()
            ? QStringLiteral("No model selected")
            : QStringLiteral("Selected model: %1").arg(selectedModel_.trimmed()),
        QStringLiteral("%1 support is local-only and OpenAI-compatible, but no endpoint probing, "
                       "cloud fallback, API key, or automatic model discovery is enabled.")
            .arg(displayName_),
        RuntimeCapabilitySet{
            true,
            false,
            true,
            true,
            false,
            false,
            false,
            false,
            true,
            false,
            false,
            false,
        },
        selectedModel_.trimmed().isEmpty() ? QStringList{} : QStringList{selectedModel_.trimmed()},
        true,
        false,
        true,
    };
}

RuntimeProviderDescriptor ClaudeRuntimeProvider::descriptor() const {
    return disabledCloudProviderDescriptor(QStringLiteral("claude"), QStringLiteral("Claude API"));
}

RuntimeProviderDescriptor GeminiRuntimeProvider::descriptor() const {
    return disabledCloudProviderDescriptor(QStringLiteral("gemini"), QStringLiteral("Gemini API"));
}

RuntimeProviderRegistry::RuntimeProviderRegistry(QList<RuntimeProviderDescriptor> providers,
                                                 QString selectedProviderId)
    : providers_(std::move(providers)), selectedProviderId_(std::move(selectedProviderId)) {
    if (selectedProviderId_.trimmed().isEmpty()) {
        selectedProviderId_ = QStringLiteral("ollama");
    }
}

QList<RuntimeProviderDescriptor> RuntimeProviderRegistry::providers() const {
    return providers_;
}

RuntimeProviderDescriptor RuntimeProviderRegistry::fallbackProvider() const {
    for (const auto& provider : providers_) {
        if (provider.providerId == QStringLiteral("ollama")) {
            return provider;
        }
    }
    return providers_.isEmpty() ? RuntimeProviderDescriptor{} : providers_.first();
}

RuntimeProviderDescriptor RuntimeProviderRegistry::activeProvider() const {
    for (const auto& provider : providers_) {
        if (provider.providerId == selectedProviderId_) {
            return provider.enabled ? provider : fallbackProvider();
        }
    }
    return fallbackProvider();
}

QString RuntimeProviderRegistry::selectedProviderId() const {
    return selectedProviderId_;
}

QString RuntimeProviderRegistry::activeProviderId() const {
    return activeProvider().providerId;
}

QString RuntimeProviderRegistry::activeProviderDisplayName() const {
    const auto provider = activeProvider();
    return provider.displayName.isEmpty() ? QStringLiteral("No Runtime Provider")
                                          : provider.displayName;
}

QString RuntimeProviderRegistry::activeModelLabel() const {
    const auto provider = activeProvider();
    return provider.modelSummary.isEmpty() ? QStringLiteral("No model selected")
                                           : provider.modelSummary;
}

QString RuntimeProviderRegistry::activeReadinessState() const {
    return runtimeReadinessStateName(activeProvider().readiness);
}

QString RuntimeProviderRegistry::activeReadinessSummary() const {
    const auto provider = activeProvider();
    return provider.readinessReason.isEmpty() ? runtimeProviderSummary(provider)
                                              : provider.readinessReason;
}

QString RuntimeProviderRegistry::activeLocalOnlySummary() const {
    const auto provider = activeProvider();
    return provider.capabilities.localOnly ? QStringLiteral("Local Only")
                                           : QStringLiteral("External/API Disabled");
}

QStringList RuntimeProviderRegistry::installedProviderSummaries() const {
    QStringList summaries;
    for (const auto& provider : providers_) {
        if (provider.installed) {
            summaries.append(runtimeProviderSummary(provider));
        }
    }
    return summaries;
}

QStringList RuntimeProviderRegistry::configuredProviderSummaries() const {
    QStringList summaries;
    for (const auto& provider : providers_) {
        if (provider.configured) {
            summaries.append(runtimeProviderSummary(provider));
        }
    }
    return summaries;
}

QStringList RuntimeProviderRegistry::availableLocalRuntimeSummaries() const {
    QStringList summaries;
    for (const auto& provider : providers_) {
        if (provider.enabled && provider.capabilities.localOnly) {
            summaries.append(runtimeProviderSummary(provider));
        }
    }
    return summaries;
}

QStringList RuntimeProviderRegistry::validationTraceSummaries() const {
    QStringList summaries;
    for (const auto& provider : providers_) {
        summaries.append(
            QStringLiteral("%1: readiness=%2; endpoint=%3; model=%4; reason=%5")
                .arg(provider.providerId, runtimeReadinessStateName(provider.readiness),
                     provider.endpointSummary, provider.modelSummary, provider.readinessReason));
    }
    return summaries;
}

} // namespace sentinel::core
