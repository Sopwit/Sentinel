#include "sentinel/core/RuntimeIntegration.h"

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
    report.readiness = RuntimeIntegrationReadiness::Blocked;
    report.executable = false;
    report.summary = QStringLiteral("Runtime integration readiness is blocked: adapter is "
                                    "metadata-only, bridge is not connected, and execution "
                                    "remains disabled.");
    report.checks = {
        RuntimeIntegrationCheck{
            QStringLiteral("runtime-integration.adapter-contract"),
            QStringLiteral("Adapter Contract"),
            adapter.status == LocalRuntimeAdapterStatus::Placeholder,
            QStringLiteral("Local runtime adapter contract exists as deterministic metadata."),
        },
        RuntimeIntegrationCheck{
            QStringLiteral("runtime-integration.endpoint"),
            QStringLiteral("Endpoint Configuration"),
            false,
            QStringLiteral("No Ollama endpoint, process, or connection configuration exists."),
        },
        RuntimeIntegrationCheck{
            QStringLiteral("runtime-integration.model-discovery"),
            QStringLiteral("Model Discovery"),
            false,
            QStringLiteral("Model discovery is not implemented and no filesystem or runtime scan "
                           "is performed."),
        },
        RuntimeIntegrationCheck{
            QStringLiteral("runtime-integration.provider-bridge"),
            QStringLiteral("Provider Runtime Bridge"),
            bridge.status == ProviderRuntimeBridgeStatus::NotConnected && !bridge.connected,
            QStringLiteral("Provider bridge boundary exists but is not connected to a provider or "
                           "runtime."),
        },
        RuntimeIntegrationCheck{
            QStringLiteral("runtime-integration.execution"),
            QStringLiteral("Execution Permission"),
            false,
            QStringLiteral("Execution lifecycle, runtime permission, safety, and pipeline "
                           "boundaries still block execution."),
        },
    };
    return report;
}

} // namespace sentinel::core
