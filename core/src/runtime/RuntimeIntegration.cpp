// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/RuntimeIntegration.h"

#include <utility>

namespace sentinel::core {

QString
localRuntimeAdapterCapabilitySummary(const LocalRuntimeAdapterCapabilitySummary& capability) {
    const auto status =
        capability.available ? QStringLiteral("Available") : QStringLiteral("Unavailable");
    const auto execution =
        capability.executable ? QStringLiteral("Executable") : QStringLiteral("Not Executable");
    return QStringLiteral("%1 (%2, %3): %4")
        .arg(capability.name, status, execution, capability.summary);
}

QStringList localRuntimeAdapterCapabilitySummaries(
    const QList<LocalRuntimeAdapterCapabilitySummary>& capabilities) {
    QStringList summaries;
    for (const auto& capability : capabilities) {
        summaries.append(localRuntimeAdapterCapabilitySummary(capability));
    }
    return summaries;
}

QString safeLocalRuntimeAdapterSummary(const LocalRuntimeAdapterDescriptor& descriptor) {
    if (!descriptor.summary.isEmpty()) {
        return descriptor.summary;
    }

    return QStringLiteral("%1 is %2 / %3.")
        .arg(descriptor.name, localRuntimeAdapterStatusName(descriptor.status),
             localRuntimeAdapterHealthName(descriptor.health));
}

LocalRuntimeAdapterDescriptor StaticLocalRuntimeAdapter::descriptor() const {
    return LocalRuntimeAdapterDescriptor{
        QStringLiteral("ollama-local-runtime-adapter-placeholder"),
        QStringLiteral("Ollama Local Runtime Adapter Placeholder"),
        LocalRuntimeAdapterStatus::Placeholder,
        LocalRuntimeAdapterHealth::MetadataOnly,
        QStringLiteral("Ollama local runtime adapter contract is metadata-only; no runtime "
                       "connection is configured."),
        QList<LocalRuntimeAdapterCapabilitySummary>{
            LocalRuntimeAdapterCapabilitySummary{
                QStringLiteral("adapter.endpoint-configuration"),
                QStringLiteral("Endpoint Configuration"),
                QStringLiteral("Endpoint metadata is not configured in Phase 8.3-8.5."),
                false,
                false,
            },
            LocalRuntimeAdapterCapabilitySummary{
                QStringLiteral("adapter.model-discovery"),
                QStringLiteral("Model Discovery"),
                QStringLiteral("Model discovery is intentionally disabled."),
                false,
                false,
            },
            LocalRuntimeAdapterCapabilitySummary{
                QStringLiteral("adapter.inference-execution"),
                QStringLiteral("Inference Execution"),
                QStringLiteral("Inference execution is not available through the adapter."),
                false,
                false,
            },
        },
    };
}

OllamaLocalRuntimeAdapter::OllamaLocalRuntimeAdapter(OllamaConfig config)
    : config_(std::move(config)) {}

LocalRuntimeAdapterDescriptor OllamaLocalRuntimeAdapter::descriptor() const {
    const OllamaHttpRuntimeClient client(config_, config_.healthCheckTimeoutMs);
    const auto health = client.healthCheck();
    const auto models = client.installedModels();
    const bool connected = health.healthStatus == OllamaHealthStatus::Healthy;
    const bool executable = connected && !models.isEmpty();
    return {
        QStringLiteral("ollama-local-runtime-adapter"),
        QStringLiteral("Ollama Local Runtime Adapter"),
        connected ? LocalRuntimeAdapterStatus::Ready : LocalRuntimeAdapterStatus::Unavailable,
        executable ? LocalRuntimeAdapterHealth::Ready
                   : (connected ? LocalRuntimeAdapterHealth::NotExecutable
                                : LocalRuntimeAdapterHealth::NotConnected),
        executable ? QStringLiteral("Ollama is connected and %1 local model(s) are available.")
                         .arg(models.size())
                   : safeOllamaHealthSummary(health),
        {
            {QStringLiteral("adapter.endpoint-configuration"),
             QStringLiteral("Endpoint Configuration"), health.endpoint, connected, connected},
            {QStringLiteral("adapter.model-discovery"), QStringLiteral("Model Discovery"),
             QStringLiteral("%1 installed model(s) discovered.").arg(models.size()), connected,
             executable},
            {QStringLiteral("adapter.inference-execution"), QStringLiteral("Inference Execution"),
             executable ? QStringLiteral("Local inference is available.")
                        : QStringLiteral("An installed Ollama model is required."),
             executable, executable},
        },
    };
}

QString safeProviderRuntimeBridgeSummary(const ProviderRuntimeBridgeSummary& summary) {
    if (!summary.summary.isEmpty()) {
        return summary.summary;
    }

    return QStringLiteral("Provider runtime bridge is %1.")
        .arg(providerRuntimeBridgeStatusName(summary.status));
}

QString safeProviderRuntimeBridgeResponseSummary(const ProviderRuntimeBridgeResponse& response) {
    if (!response.summary.isEmpty()) {
        return response.summary;
    }

    return QStringLiteral("Provider runtime bridge response is %1.")
        .arg(providerRuntimeBridgeStatusName(response.status));
}

ProviderRuntimeBridgeSummary StaticProviderRuntimeBridge::summary() const {
    return ProviderRuntimeBridgeSummary{
        QStringLiteral("provider-runtime-bridge-placeholder"),
        ProviderRuntimeBridgeStatus::NotConnected,
        QStringLiteral("Provider runtime bridge is not connected and cannot execute provider "
                       "requests."),
        false,
        false,
    };
}

ProviderRuntimeBridgeResponse
StaticProviderRuntimeBridge::evaluate(const ProviderRuntimeBridgeRequest& request) const {
    return ProviderRuntimeBridgeResponse{
        request,
        ProviderRuntimeBridgeStatus::NotConnected,
        QStringLiteral("Provider runtime bridge is metadata-only; no provider or local runtime "
                       "request was executed."),
        false,
        false,
    };
}

OllamaProviderRuntimeBridge::OllamaProviderRuntimeBridge(OllamaConfig config)
    : config_(std::move(config)) {}

ProviderRuntimeBridgeSummary OllamaProviderRuntimeBridge::summary() const {
    const OllamaHttpRuntimeClient client(config_, config_.healthCheckTimeoutMs);
    const auto health = client.healthCheck();
    const bool connected = health.healthStatus == OllamaHealthStatus::Healthy;
    return {
        QStringLiteral("ollama-provider-runtime-bridge"),
        connected ? ProviderRuntimeBridgeStatus::Connected
                  : ProviderRuntimeBridgeStatus::Unavailable,
        connected ? QStringLiteral("Ollama provider bridge is connected and executable.")
                  : safeOllamaHealthSummary(health),
        connected,
        connected,
    };
}

ProviderRuntimeBridgeResponse
OllamaProviderRuntimeBridge::evaluate(const ProviderRuntimeBridgeRequest& request) const {
    const auto bridge = summary();
    return {request, bridge.status, bridge.summary, bridge.connected, bridge.executable};
}

QString runtimeIntegrationCheckSummary(const RuntimeIntegrationCheck& check) {
    const auto status = check.passed ? QStringLiteral("Pass") : QStringLiteral("Blocked");
    return QStringLiteral("%1: %2 - %3").arg(status, check.name, check.summary);
}

QStringList runtimeIntegrationCheckSummaries(const QList<RuntimeIntegrationCheck>& checks) {
    QStringList summaries;
    for (const auto& check : checks) {
        summaries.append(runtimeIntegrationCheckSummary(check));
    }
    return summaries;
}

QString safeRuntimeIntegrationReportSummary(const RuntimeIntegrationReport& report) {
    if (!report.summary.isEmpty()) {
        return report.summary;
    }

    return QStringLiteral("%1 runtime integration readiness: %2 checks.")
        .arg(runtimeIntegrationReadinessName(report.readiness))
        .arg(report.checks.size());
}

RuntimeIntegrationReport
StaticRuntimeIntegrationReadiness::evaluate(const LocalRuntimeAdapterDescriptor& adapter,
                                            const ProviderRuntimeBridgeSummary& bridge) const {
    RuntimeIntegrationReport report;
    const bool adapterReady = adapter.status == LocalRuntimeAdapterStatus::Ready &&
                              adapter.health == LocalRuntimeAdapterHealth::Ready;
    const bool bridgeReady = bridge.status == ProviderRuntimeBridgeStatus::Connected &&
                             bridge.connected && bridge.executable;
    report.readiness = adapterReady && bridgeReady ? RuntimeIntegrationReadiness::Ready
                                                   : RuntimeIntegrationReadiness::Blocked;
    report.executable = adapterReady && bridgeReady;
    report.summary =
        report.executable
            ? QStringLiteral("Ollama runtime integration is connected and executable.")
            : QStringLiteral("Runtime integration is not ready: Ollama health and model "
                             "availability must pass.");
    report.checks = {
        RuntimeIntegrationCheck{
            QStringLiteral("runtime-integration.adapter-contract"),
            QStringLiteral("Adapter Contract"),
            adapterReady,
            safeLocalRuntimeAdapterSummary(adapter),
        },
        RuntimeIntegrationCheck{
            QStringLiteral("runtime-integration.endpoint"),
            QStringLiteral("Endpoint Configuration"),
            adapter.status != LocalRuntimeAdapterStatus::NotConfigured,
            QStringLiteral("Ollama endpoint is configured and health-checked."),
        },
        RuntimeIntegrationCheck{
            QStringLiteral("runtime-integration.model-discovery"),
            QStringLiteral("Model Discovery"),
            adapter.capabilities.size() > 1 && adapter.capabilities.at(1).available,
            QStringLiteral("Installed models are discovered through Ollama /api/tags."),
        },
        RuntimeIntegrationCheck{
            QStringLiteral("runtime-integration.provider-bridge"),
            QStringLiteral("Provider Runtime Bridge"),
            bridgeReady,
            safeProviderRuntimeBridgeSummary(bridge),
        },
        RuntimeIntegrationCheck{
            QStringLiteral("runtime-integration.execution"),
            QStringLiteral("Execution Permission"),
            report.executable,
            report.executable
                ? QStringLiteral("Execution is available behind policy gates.")
                : QStringLiteral("Execution is blocked until runtime readiness passes."),
        },
    };
    return report;
}

} // namespace sentinel::core
