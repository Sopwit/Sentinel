// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/LocalRuntime.h"

#include <utility>

namespace sentinel::core {

QString localRuntimeCapabilitySummary(const LocalRuntimeCapability& capability) {
    const auto state = capability.enabled ? QStringLiteral("Enabled") : QStringLiteral("Disabled");
    if (!capability.summary.isEmpty()) {
        return QStringLiteral("%1 (%2): %3").arg(capability.name, state, capability.summary);
    }

    return QStringLiteral("%1 (%2)").arg(capability.name, state);
}

QString safeLocalRuntimeSummary(const LocalRuntimeDescriptor& descriptor) {
    if (!descriptor.summary.isEmpty()) {
        return descriptor.summary;
    }

    return QStringLiteral("%1: %2 / %3")
        .arg(descriptor.name, localRuntimeStatusName(descriptor.status),
             localRuntimeHealthName(descriptor.health));
}

QStringList localRuntimeCapabilitySummaries(const QList<LocalRuntimeCapability>& capabilities) {
    QStringList summaries;
    for (const auto& capability : capabilities) {
        summaries.append(localRuntimeCapabilitySummary(capability));
    }
    return summaries;
}

QString safeLocalRuntimeResponseSummary(const LocalRuntimeResponse& response) {
    if (!response.summary.isEmpty()) {
        return response.summary;
    }

    return response.status.isEmpty() ? QStringLiteral("Local runtime request refused.")
                                     : response.status;
}

LocalRuntimeDescriptor NullLocalRuntime::descriptor() const {
    return LocalRuntimeDescriptor{
        QStringLiteral("null-local-runtime"),
        QStringLiteral("Null Local Runtime"),
        QStringLiteral("Null Local Runtime is metadata-only; local inference execution is "
                       "disabled."),
        LocalRuntimeStatus::MetadataOnly,
        LocalRuntimeHealth::NotExecutable,
        {
            LocalRuntimeCapability{
                QStringLiteral("local-runtime.metadata"),
                QStringLiteral("Metadata Reporting"),
                QStringLiteral("Reports deterministic local runtime status and health."),
                true,
            },
            LocalRuntimeCapability{
                QStringLiteral("local-runtime.inference"),
                QStringLiteral("Local Inference"),
                QStringLiteral("Inference execution is intentionally disabled."),
                false,
            },
            LocalRuntimeCapability{
                QStringLiteral("local-runtime.streaming"),
                QStringLiteral("Streaming"),
                QStringLiteral("Streaming is intentionally disabled."),
                false,
            },
        },
    };
}

LocalRuntimeResponse NullLocalRuntime::evaluate(const LocalRuntimeRequest& request) const {
    Q_UNUSED(request);
    return LocalRuntimeResponse{
        false,
        QStringLiteral("Refused"),
        QStringLiteral("Local runtime boundary is metadata-only; execution is disabled."),
    };
}

OllamaLocalRuntime::OllamaLocalRuntime(OllamaConfig config) : config_(std::move(config)) {}

LocalRuntimeDescriptor OllamaLocalRuntime::descriptor() const {
    const OllamaHttpRuntimeClient client(config_, config_.healthCheckTimeoutMs);
    const auto health = client.healthCheck();
    const auto models = client.installedModels();
    const bool ready = health.healthStatus == OllamaHealthStatus::Healthy && !models.isEmpty();
    return {
        QStringLiteral("ollama-local-runtime"),
        QStringLiteral("Ollama Local Runtime"),
        ready ? QStringLiteral("Ollama is ready with %1 installed model(s).")
                    .arg(models.size())
              : safeOllamaHealthSummary(health),
        ready ? LocalRuntimeStatus::Active : LocalRuntimeStatus::Unavailable,
        ready ? LocalRuntimeHealth::Ready : LocalRuntimeHealth::Unavailable,
        {
            {QStringLiteral("local-runtime.metadata"), QStringLiteral("Runtime Metadata"),
             QStringLiteral("Live Ollama health and model metadata."), true},
            {QStringLiteral("local-runtime.inference"), QStringLiteral("Local Inference"),
             ready ? QStringLiteral("Inference is available through Ollama.")
                   : QStringLiteral("Ollama health and at least one installed model are required."),
             ready},
            {QStringLiteral("local-runtime.streaming"), QStringLiteral("Streaming"),
             ready ? QStringLiteral("Streaming is available through the Ollama stream endpoint.")
                   : QStringLiteral("Streaming is unavailable until Ollama is ready."),
             ready},
        },
    };
}

LocalRuntimeResponse OllamaLocalRuntime::evaluate(const LocalRuntimeRequest& request) const {
    if (request.prompt.trimmed().isEmpty()) {
        return {false, QStringLiteral("Invalid Request"), QStringLiteral("Prompt is blank.")};
    }
    const auto state = descriptor();
    return {state.health == LocalRuntimeHealth::Ready,
            state.health == LocalRuntimeHealth::Ready ? QStringLiteral("Ready")
                                                       : QStringLiteral("Unavailable"),
            state.summary};
}

} // namespace sentinel::core
