#include "sentinel/core/ToolExecutionGateway.h"

namespace sentinel::core {

namespace {

QString permissionPostureForDomain(const QString& domainId, const QString& defaultPermissionState,
                                   const PermissionPolicyService& permissionPolicy) {
    for (const auto& summary : permissionPolicy.permissionSummaries(defaultPermissionState)) {
        if (summary.domainId == domainId) {
            return summary.state;
        }
    }
    return permissionPolicy.normalizedState(defaultPermissionState);
}

QString toolSummaryLine(const ToolGatewaySummary& summary) {
    return QStringLiteral("%1 / %2 / %3 / %4")
        .arg(summary.displayName, summary.availability, summary.permissionPosture,
             summary.requiredPermissionDomain);
}

} // namespace

QList<ToolGatewayMetadata> ToolExecutionGateway::toolMetadata() const {
    return {
        {QStringLiteral("open-workspace"), QStringLiteral("Open Workspace"),
         QStringLiteral("Workspace"),
         QStringLiteral("Future explicit workspace picker and scope selection."),
         QStringLiteral("workspace-access"), ToolGatewayRiskLevel::Medium,
         ToolGatewayScope::Local, ToolExecutionAvailability::Unavailable,
         QStringLiteral("Workspace opening is unavailable until a later workspace activation "
                        "phase adds user-visible path grants.")},
        {QStringLiteral("read-file"), QStringLiteral("Read File"), QStringLiteral("Filesystem"),
         QStringLiteral("Future scoped file read inside an approved workspace."),
         QStringLiteral("workspace-access"), ToolGatewayRiskLevel::High, ToolGatewayScope::Local,
         ToolExecutionAvailability::Refused,
         QStringLiteral("File reading is refused; the gateway has no filesystem authority.")},
        {QStringLiteral("write-file"), QStringLiteral("Write File"), QStringLiteral("Filesystem"),
         QStringLiteral("Future scoped file write after explicit approval and audit."),
         QStringLiteral("filesystem-write"), ToolGatewayRiskLevel::Critical,
         ToolGatewayScope::Local, ToolExecutionAvailability::Refused,
         QStringLiteral("File writing is refused; mutation authority is out of scope.")},
        {QStringLiteral("run-command"), QStringLiteral("Run Command"), QStringLiteral("System"),
         QStringLiteral("Future subprocess execution inside an audited sandbox."),
         QStringLiteral("subprocess-execution"), ToolGatewayRiskLevel::Critical,
         ToolGatewayScope::Local, ToolExecutionAvailability::Refused,
         QStringLiteral("Subprocess execution is refused; no process launch path exists.")},
        {QStringLiteral("summarize-current-conversation"),
         QStringLiteral("Summarize Current Conversation"), QStringLiteral("Conversation"),
         QStringLiteral("Metadata-safe placeholder for future explicit conversation summary "
                        "requests."),
         QStringLiteral("context-injection"), ToolGatewayRiskLevel::Low, ToolGatewayScope::Local,
         ToolExecutionAvailability::MetadataSafe,
         QStringLiteral("Metadata only; this does not generate, mutate, or inject a summary.")},
        {QStringLiteral("export-conversation"), QStringLiteral("Export Conversation"),
         QStringLiteral("Conversation"),
         QStringLiteral("Future explicit transcript export through a user-visible flow."),
         QStringLiteral("filesystem-write"), ToolGatewayRiskLevel::Medium,
         ToolGatewayScope::Local, ToolExecutionAvailability::Unavailable,
         QStringLiteral("Conversation export through the gateway is unavailable; no gateway file "
                        "write is enabled.")},
        {QStringLiteral("voice-transcribe"), QStringLiteral("Voice Transcribe"),
         QStringLiteral("Voice"), QStringLiteral("Future microphone capture and STT pipeline."),
         QStringLiteral("voice-capture"), ToolGatewayRiskLevel::High, ToolGatewayScope::Local,
         ToolExecutionAvailability::Refused,
         QStringLiteral("Voice transcription is refused; microphone and STT activation are "
                        "disabled.")},
        {QStringLiteral("voice-speak"), QStringLiteral("Voice Speak"), QStringLiteral("Voice"),
         QStringLiteral("Future TTS synthesis and playback pipeline."),
         QStringLiteral("voice-playback"), ToolGatewayRiskLevel::High, ToolGatewayScope::Local,
         ToolExecutionAvailability::Refused,
         QStringLiteral("Voice playback is refused; audio output and TTS activation are disabled.")},
        {QStringLiteral("web-search"), QStringLiteral("Web Search"), QStringLiteral("Network"),
         QStringLiteral("Future explicit web lookup through a permission-gated provider."),
         QStringLiteral("cloud-provider-access"), ToolGatewayRiskLevel::High,
         ToolGatewayScope::Cloud, ToolExecutionAvailability::Refused,
         QStringLiteral("Web search is refused; no web or cloud access is enabled.")},
        {QStringLiteral("provider-test-call"), QStringLiteral("Provider Test Call"),
         QStringLiteral("Provider"),
         QStringLiteral("Future explicit cloud/local provider connectivity test."),
         QStringLiteral("cloud-provider-access"), ToolGatewayRiskLevel::High,
         ToolGatewayScope::LocalAndCloud, ToolExecutionAvailability::Refused,
         QStringLiteral("Provider test calls are refused; no provider/API call is enabled.")},
    };
}

QList<ToolGatewaySummary>
ToolExecutionGateway::toolSummaries(const QString& defaultPermissionState,
                                    const PermissionPolicyService& permissionPolicy) const {
    QList<ToolGatewaySummary> summaries;
    for (const auto& tool : toolMetadata()) {
        const auto permissionPosture = permissionPostureForDomain(
            tool.requiredPermissionDomain, defaultPermissionState, permissionPolicy);
        const auto risk = toolGatewayRiskLevelName(tool.riskLevel);
        const auto scope = toolGatewayScopeName(tool.scope);
        const auto availability = toolExecutionAvailabilityName(tool.availability);
        const auto summary =
            QStringLiteral("%1 requires %2 permission, risk %3, scope %4, availability %5.")
                .arg(tool.displayName, permissionPosture, risk, scope, availability);

        summaries.append({
            tool.toolId,
            tool.displayName,
            tool.category,
            tool.requiredPermissionDomain,
            permissionPosture,
            risk,
            scope,
            availability,
            tool.refusalReason,
            summary,
            {
                QStringLiteral("Tool id: %1").arg(tool.toolId),
                QStringLiteral("Category: %1").arg(tool.category),
                QStringLiteral("Permission domain: %1").arg(tool.requiredPermissionDomain),
                QStringLiteral("Permission posture: %1").arg(permissionPosture),
                QStringLiteral("Risk: %1").arg(risk),
                QStringLiteral("Scope: %1").arg(scope),
                QStringLiteral("Execution availability: %1").arg(availability),
                tool.refusalReason,
                QStringLiteral("Gateway execution grant: none in this phase"),
            },
        });
    }
    return summaries;
}

ToolGatewayRegistrySummary ToolExecutionGateway::registrySummary(
    const QString& defaultPermissionState,
    const PermissionPolicyService& permissionPolicy) const {
    const auto permissionPosture = permissionPolicy.normalizedState(defaultPermissionState);
    ToolGatewayRegistrySummary registry{
        QStringLiteral("Metadata only"),
        QStringLiteral("Tool Execution Gateway classifies tools and permission posture only; it "
                       "does not run tools or grant authority."),
        permissionPosture,
        0,
        0,
        0,
        0,
        {},
        {},
    };

    const auto summaries = toolSummaries(defaultPermissionState, permissionPolicy);
    registry.toolCount = summaries.size();
    for (const auto& summary : summaries) {
        if (summary.availability ==
            toolExecutionAvailabilityName(ToolExecutionAvailability::MetadataSafe)) {
            ++registry.metadataSafeCount;
        } else if (summary.availability ==
                   toolExecutionAvailabilityName(ToolExecutionAvailability::Unavailable)) {
            ++registry.unavailableCount;
        } else {
            ++registry.refusedCount;
        }
        registry.toolSummaries.append(toolSummaryLine(summary));
        registry.developerDiagnostics.append(summary.diagnostics.join(QStringLiteral(" / ")));
    }

    return registry;
}

QString toolGatewayRiskLevelName(ToolGatewayRiskLevel riskLevel) {
    switch (riskLevel) {
    case ToolGatewayRiskLevel::Low:
        return QStringLiteral("Low");
    case ToolGatewayRiskLevel::Medium:
        return QStringLiteral("Medium");
    case ToolGatewayRiskLevel::High:
        return QStringLiteral("High");
    case ToolGatewayRiskLevel::Critical:
        return QStringLiteral("Critical");
    }
    return QStringLiteral("Low");
}

QString toolGatewayScopeName(ToolGatewayScope scope) {
    switch (scope) {
    case ToolGatewayScope::Local:
        return QStringLiteral("Local");
    case ToolGatewayScope::Cloud:
        return QStringLiteral("Cloud");
    case ToolGatewayScope::LocalAndCloud:
        return QStringLiteral("Local + Cloud");
    }
    return QStringLiteral("Local");
}

QString toolExecutionAvailabilityName(ToolExecutionAvailability availability) {
    switch (availability) {
    case ToolExecutionAvailability::MetadataSafe:
        return QStringLiteral("Metadata safe");
    case ToolExecutionAvailability::Unavailable:
        return QStringLiteral("Unavailable");
    case ToolExecutionAvailability::Refused:
        return QStringLiteral("Refused");
    }
    return QStringLiteral("Refused");
}

} // namespace sentinel::core
