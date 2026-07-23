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
         QStringLiteral("Workspace"), QStringLiteral("Workspace picker and scope selection."),
         QStringLiteral("workspace-access"), ToolGatewayRiskLevel::Medium, ToolGatewayScope::Local,
         ToolExecutionAvailability::Available,
         QStringLiteral(
             "Workspace gateway is operational and allows defining active context path grants.")},
        {QStringLiteral("read-file"), QStringLiteral("Read File"), QStringLiteral("Filesystem"),
         QStringLiteral("Scoped file read inside an approved workspace path."),
         QStringLiteral("workspace-access"), ToolGatewayRiskLevel::High, ToolGatewayScope::Local,
         ToolExecutionAvailability::Available,
         QStringLiteral(
             "File reading is operational; files can be read inside active workspaces.")},
        {QStringLiteral("write-file"), QStringLiteral("Write File"), QStringLiteral("Filesystem"),
         QStringLiteral("Scoped file write after approval and validation checks."),
         QStringLiteral("filesystem-write"), ToolGatewayRiskLevel::Critical,
         ToolGatewayScope::Local, ToolExecutionAvailability::Available,
         QStringLiteral(
             "File writing is operational; files can be edited/created inside active workspaces.")},
        {QStringLiteral("run-command"), QStringLiteral("Run Command"), QStringLiteral("System"),
         QStringLiteral("Subprocess execution inside an audited sandbox environment."),
         QStringLiteral("subprocess-execution"), ToolGatewayRiskLevel::Critical,
         ToolGatewayScope::Local, ToolExecutionAvailability::Available,
         QStringLiteral("Subprocess execution is operational; commands can be run synchronously "
                        "via QProcess.")},
        {QStringLiteral("summarize-current-conversation"),
         QStringLiteral("Summarize Current Conversation"), QStringLiteral("Conversation"),
         QStringLiteral("Conversation summary generation and prompt injection."),
         QStringLiteral("context-injection"), ToolGatewayRiskLevel::Low, ToolGatewayScope::Local,
         ToolExecutionAvailability::Available,
         QStringLiteral("Conversation summarization is operational.")},
        {QStringLiteral("export-conversation"), QStringLiteral("Export Conversation"),
         QStringLiteral("Conversation"),
         QStringLiteral("Explicit transcript export to local disk file."),
         QStringLiteral("filesystem-write"), ToolGatewayRiskLevel::Medium, ToolGatewayScope::Local,
         ToolExecutionAvailability::Available,
         QStringLiteral("Conversation export is operational.")},
        {QStringLiteral("voice-transcribe"), QStringLiteral("Voice Transcribe"),
         QStringLiteral("Voice"),
         QStringLiteral("Microphone audio capture and STT speech-to-text pipeline."),
         QStringLiteral("voice-capture"), ToolGatewayRiskLevel::High, ToolGatewayScope::Local,
         ToolExecutionAvailability::Available,
         QStringLiteral("Voice transcription is operational via local Whisper CLI client.")},
        {QStringLiteral("voice-speak"), QStringLiteral("Voice Speak"), QStringLiteral("Voice"),
         QStringLiteral("TTS text-to-speech audio synthesis and playback."),
         QStringLiteral("voice-playback"), ToolGatewayRiskLevel::High, ToolGatewayScope::Local,
         ToolExecutionAvailability::Available,
         QStringLiteral("Voice playback is operational via local Piper TTS client.")},
        {QStringLiteral("web-search"), QStringLiteral("Web Search"), QStringLiteral("Network"),
         QStringLiteral("Web lookup querying the network via local client."),
         QStringLiteral("cloud-provider-access"), ToolGatewayRiskLevel::High,
         ToolGatewayScope::Cloud, ToolExecutionAvailability::Available,
         QStringLiteral("Web search is operational.")},
        {QStringLiteral("provider-test-call"), QStringLiteral("Provider Test Call"),
         QStringLiteral("Provider"),
         QStringLiteral("Cloud and local provider API connectivity test."),
         QStringLiteral("cloud-provider-access"), ToolGatewayRiskLevel::High,
         ToolGatewayScope::LocalAndCloud, ToolExecutionAvailability::Available,
         QStringLiteral("Provider test calls are operational.")},
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
                QStringLiteral("Gateway execution grant: allowed"),
            },
        });
    }
    return summaries;
}

ToolGatewayRegistrySummary
ToolExecutionGateway::registrySummary(const QString& defaultPermissionState,
                                      const PermissionPolicyService& permissionPolicy) const {
    const auto permissionPosture = permissionPolicy.normalizedState(defaultPermissionState);
    ToolGatewayRegistrySummary registry{
        QStringLiteral("Operational"),
        QStringLiteral("Tool Execution Gateway is fully operational and processes local system and "
                       "cloud tools safely."),
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
                toolExecutionAvailabilityName(ToolExecutionAvailability::MetadataSafe) ||
            summary.availability ==
                toolExecutionAvailabilityName(ToolExecutionAvailability::Available)) {
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
    case ToolExecutionAvailability::Available:
        return QStringLiteral("Available");
    case ToolExecutionAvailability::Unavailable:
        return QStringLiteral("Unavailable");
    case ToolExecutionAvailability::Refused:
        return QStringLiteral("Refused");
    }
    return QStringLiteral("Refused");
}

} // namespace sentinel::core
