// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/AgentRuntimeService.h"

namespace sentinel::core {

namespace {

constexpr auto kExecutionDisabledReason =
    "Agent execution is available through the approval and sandbox-gated execution pipeline.";

QString normalizedGoal(const QString& goal) {
    const auto trimmed = goal.simplified();
    if (trimmed.isEmpty()) {
        return QStringLiteral("Inspect the current request and prepare a safe execution plan.");
    }
    return trimmed.left(180);
}

QString permissionPostureForDomain(const QString& domainId, const QString& defaultPermissionState,
                                   const PermissionPolicyService& permissionPolicy) {
    for (const auto& summary : permissionPolicy.permissionSummaries(defaultPermissionState)) {
        if (summary.domainId == domainId) {
            return summary.state;
        }
    }
    return permissionPolicy.normalizedState(defaultPermissionState);
}

QList<AgentRecord> baseAgents() {
    return {
        {QStringLiteral("general-assistant"),
         QStringLiteral("General Assistant"),
         QStringLiteral("Plans broad assistant responses and user-facing next steps."),
         QStringLiteral("Goal clarification, conversation planning, and safe response shaping."),
         {QStringLiteral("Conversation")},
         QStringLiteral("agent-execution"),
         QStringLiteral(""),
         QStringLiteral("")},
        {QStringLiteral("coding-assistant"),
         QStringLiteral("Coding Assistant"),
         QStringLiteral("Plans code-oriented work without reading or writing workspace files."),
         QStringLiteral("Code task decomposition, review planning, and verification planning."),
         {QStringLiteral("Conversation"), QStringLiteral("Filesystem"), QStringLiteral("System")},
         QStringLiteral("agent-execution"),
         QStringLiteral(""),
         QStringLiteral("")},
        {QStringLiteral("research-assistant"),
         QStringLiteral("Research Assistant"),
         QStringLiteral("Plans research flows without web, provider, or cloud access."),
         QStringLiteral("Question breakdown, source needs, and citation checklist planning."),
         {QStringLiteral("Conversation"), QStringLiteral("Network"), QStringLiteral("Provider")},
         QStringLiteral("agent-execution"),
         QStringLiteral(""),
         QStringLiteral("")},
        {QStringLiteral("workspace-assistant"),
         QStringLiteral("Workspace Assistant"),
         QStringLiteral("Plans workspace-scoped actions while workspace access remains disabled."),
         QStringLiteral("Workspace readiness review, scope planning, and permission checks."),
         {QStringLiteral("Workspace"), QStringLiteral("Filesystem"), QStringLiteral("System")},
         QStringLiteral("agent-execution"),
         QStringLiteral(""),
         QStringLiteral("")},
        {QStringLiteral("voice-assistant"),
         QStringLiteral("Voice Assistant"),
         QStringLiteral(
             "Plans voice workflows without microphone, playback, STT, or TTS activation."),
         QStringLiteral(
             "Voice intent planning, capture/playback permission review, and fallback copy."),
         {QStringLiteral("Voice"), QStringLiteral("Conversation")},
         QStringLiteral("agent-execution"),
         QStringLiteral(""),
         QStringLiteral("")},
    };
}

QStringList candidateToolsForGoal(const QString& goal, const AgentRecord& agent) {
    const auto lower = goal.toLower();
    QStringList tools{QStringLiteral("summarize-current-conversation")};

    if (agent.agentId == QStringLiteral("workspace-assistant") ||
        lower.contains(QStringLiteral("workspace"))) {
        tools.append(QStringLiteral("open-workspace"));
    }
    if (agent.agentId == QStringLiteral("coding-assistant") ||
        lower.contains(QStringLiteral("code")) || lower.contains(QStringLiteral("file")) ||
        lower.contains(QStringLiteral("test"))) {
        tools.append({QStringLiteral("read-file"), QStringLiteral("write-file"),
                      QStringLiteral("run-command")});
    }
    if (agent.agentId == QStringLiteral("research-assistant") ||
        lower.contains(QStringLiteral("research")) || lower.contains(QStringLiteral("web"))) {
        tools.append(QStringLiteral("web-search"));
    }
    if (agent.agentId == QStringLiteral("voice-assistant") ||
        lower.contains(QStringLiteral("voice")) || lower.contains(QStringLiteral("speak")) ||
        lower.contains(QStringLiteral("transcribe"))) {
        tools.append({QStringLiteral("voice-transcribe"), QStringLiteral("voice-speak")});
    }

    tools.removeDuplicates();
    return tools;
}

ToolGatewayMetadata metadataForTool(const QString& toolId, const ToolExecutionGateway& gateway) {
    for (const auto& tool : gateway.toolMetadata()) {
        if (tool.toolId == toolId) {
            return tool;
        }
    }
    return {toolId,
            toolId,
            QStringLiteral("Unknown"),
            QStringLiteral("Unknown tool descriptor."),
            QStringLiteral("tool-execution"),
            ToolGatewayRiskLevel::High,
            ToolGatewayScope::Local,
            ToolExecutionAvailability::Refused,
            QStringLiteral("Unknown tools are refused by the execution planner.")};
}

AgentPlanRisk maxRiskForTools(const QStringList& toolIds, const ToolExecutionGateway& gateway) {
    auto risk = AgentPlanRisk::Low;
    for (const auto& toolId : toolIds) {
        const auto tool = metadataForTool(toolId, gateway);
        if (tool.riskLevel == ToolGatewayRiskLevel::Critical) {
            return AgentPlanRisk::Critical;
        }
        if (tool.riskLevel == ToolGatewayRiskLevel::High) {
            risk = AgentPlanRisk::High;
        } else if (tool.riskLevel == ToolGatewayRiskLevel::Medium && risk == AgentPlanRisk::Low) {
            risk = AgentPlanRisk::Medium;
        }
    }
    return risk;
}

QStringList permissionDomainsForTools(const QStringList& toolIds,
                                      const ToolExecutionGateway& gateway) {
    QStringList domains{QStringLiteral("agent-execution")};
    for (const auto& toolId : toolIds) {
        domains.append(metadataForTool(toolId, gateway).requiredPermissionDomain);
    }
    domains.removeDuplicates();
    return domains;
}

} // namespace

AgentPlanRecord AgentPlanRegistry::previewPlan(
    const QString& goal, const AgentRecord& agent, const QString& defaultPermissionState,
    const PermissionPolicyService& permissionPolicy, const ToolExecutionGateway& toolGateway,
    const SkillProfileService& skillProfileService, const QString& selectedSkillProfileId,
    const WorkspaceService& workspaceService, const QString& selectedWorkspaceId) const {
    const auto safeGoal = normalizedGoal(goal);
    const auto toolIds = candidateToolsForGoal(safeGoal, agent);
    const auto permissionDomains = permissionDomainsForTools(toolIds, toolGateway);
    const auto profile = skillProfileService.selectedProfile(selectedSkillProfileId);
    const auto workspace = workspaceService.selectedWorkspace(selectedWorkspaceId);
    const auto workspaceReadiness = workspaceService.readiness(selectedWorkspaceId);

    QStringList requiredTools;
    for (const auto& toolId : toolIds) {
        const auto tool = metadataForTool(toolId, toolGateway);
        requiredTools.append(QStringLiteral("%1 / %2 / %3")
                                 .arg(tool.displayName,
                                      toolExecutionAvailabilityName(tool.availability),
                                      tool.refusalReason));
    }

    QStringList requiredPermissions;
    for (const auto& domain : permissionDomains) {
        requiredPermissions.append(QStringLiteral("%1 / %2").arg(
            domain, permissionPostureForDomain(domain, defaultPermissionState, permissionPolicy)));
    }

    QStringList steps{
        QStringLiteral(
            "Classify goal for %1 and identify the tools required for execution.")
            .arg(agent.displayName),
        QStringLiteral("Check selected profile metadata: %1 / %2.")
            .arg(profile.name, profile.readiness),
        QStringLiteral("Check workspace metadata: %1 / %2.")
            .arg(workspace.name, workspaceReadiness.status),
        QStringLiteral(
            "Map candidate tool descriptors and permission domains without opening handles."),
        QStringLiteral(
            "Return an inspectable plan; execution remains gated by approval and sandbox policy."),
    };

    return {
        QStringLiteral("agent-plan-%1").arg(agent.agentId),
        safeGoal,
        steps,
        requiredTools,
        requiredPermissions,
        agentPlanRiskName(maxRiskForTools(toolIds, toolGateway)),
        QStringLiteral("Approval required before execution"),
        QString::fromLatin1(kExecutionDisabledReason),
        {
            QStringLiteral("Agent: %1").arg(agent.displayName),
            QStringLiteral("Profile: %1").arg(profile.name),
            QStringLiteral("Workspace: %1 / %2").arg(workspace.name, workspace.accessState),
            QStringLiteral("Permission posture: %1")
                .arg(permissionPolicy.normalizedState(defaultPermissionState)),
            QStringLiteral("Candidate tool ids: %1").arg(toolIds.join(QStringLiteral(", "))),
            QStringLiteral("Execution grant: approval and sandbox policy required"),
        },
    };
}

QList<AgentRecord> AgentRuntimeService::agents(const QString& defaultPermissionState,
                                               const PermissionPolicyService& permissionPolicy,
                                               const ToolExecutionGateway& toolGateway,
                                               const SkillProfileService& skillProfileService,
                                               const QString& selectedSkillProfileId,
                                               const WorkspaceService& workspaceService,
                                               const QString& selectedWorkspaceId) const {
    auto records = baseAgents();
    const auto agentPosture = permissionPostureForDomain(QStringLiteral("agent-execution"),
                                                         defaultPermissionState, permissionPolicy);
    const auto profile = skillProfileService.selectedProfile(selectedSkillProfileId);
    const auto workspace = workspaceService.selectedWorkspace(selectedWorkspaceId);
    const auto workspaceReadiness = workspaceService.readiness(selectedWorkspaceId);

    for (auto& agent : records) {
        agent.requiredPermissionPosture = agentPosture;
        agent.readinessState = agentReadinessStateName(AgentReadinessState::DryRunReady);
        agent.refusalReason = QString::fromLatin1(kExecutionDisabledReason);

        const auto toolIds = candidateToolsForGoal(agent.description, agent);
        QStringList toolAvailabilities;
        for (const auto& toolId : toolIds) {
            const auto tool = metadataForTool(toolId, toolGateway);
            toolAvailabilities.append(QStringLiteral("%1 / %2")
                                          .arg(tool.displayName,
                                               toolExecutionAvailabilityName(tool.availability)));
        }

        agent.capabilitySummary = QStringLiteral(
            "%1 Tools: %2. Profile: %3 / %4. Workspace: %5 / %6.")
                                      .arg(agent.capabilitySummary,
                                           toolAvailabilities.isEmpty()
                                               ? QStringLiteral("none")
                                               : toolAvailabilities.join(QStringLiteral(", ")))
                                      .arg(profile.name, profile.readiness)
                                      .arg(workspace.name, workspaceReadiness.status);
    }
    return records;
}

AgentRecord AgentRuntimeService::selectedAgent(
    const QString& agentId, const QString& defaultPermissionState,
    const PermissionPolicyService& permissionPolicy, const ToolExecutionGateway& toolGateway,
    const SkillProfileService& skillProfileService, const QString& selectedSkillProfileId,
    const WorkspaceService& workspaceService, const QString& selectedWorkspaceId) const {
    const auto records =
        agents(defaultPermissionState, permissionPolicy, toolGateway, skillProfileService,
               selectedSkillProfileId, workspaceService, selectedWorkspaceId);
    for (const auto& agent : records) {
        if (agent.agentId == agentId) {
            return agent;
        }
    }
    return records.isEmpty() ? AgentRecord{} : records.first();
}

AgentRuntimeSummary AgentRuntimeService::runtimeSummary(
    const QString& defaultPermissionState, const PermissionPolicyService& permissionPolicy,
    const ToolExecutionGateway& toolGateway, const SkillProfileService& skillProfileService,
    const QString& selectedSkillProfileId, const WorkspaceService& workspaceService,
    const QString& selectedWorkspaceId) const {
    const auto records =
        agents(defaultPermissionState, permissionPolicy, toolGateway, skillProfileService,
               selectedSkillProfileId, workspaceService, selectedWorkspaceId);
    const auto profile = skillProfileService.selectedProfile(selectedSkillProfileId);
    const auto workspace = workspaceService.selectedWorkspace(selectedWorkspaceId);
    const auto toolSummary = toolGateway.registrySummary(defaultPermissionState, permissionPolicy);

    AgentRuntimeSummary summary{
        QStringLiteral("Ready for approved execution"),
        QStringLiteral("Agent Runtime assembles plans and executes tools only after approval and "
                       "sandbox policy allow the request."),
        QStringLiteral("Approval and sandbox gates enforced"),
        static_cast<int>(records.size()),
        static_cast<int>(records.size()),
        0,
        {},
        {},
        {},
    };

    for (const auto& agent : records) {
        summary.agentSummaries.append(agentRecordSummary(agent));
        summary.readinessSummaries.append(
            QStringLiteral("%1 / %2 / %3")
                .arg(agent.displayName, agent.readinessState, agent.requiredPermissionPosture));
    }
    summary.developerDiagnostics = {
        QStringLiteral("Selected profile: %1 / %2").arg(profile.name, profile.readiness),
        QStringLiteral("Selected workspace: %1 / %2").arg(workspace.name, workspace.accessState),
        QStringLiteral("Tool gateway: %1 tools / %2 refused")
            .arg(toolSummary.toolCount)
            .arg(toolSummary.refusedCount),
        QStringLiteral("Permission default: %1")
            .arg(permissionPolicy.normalizedState(defaultPermissionState)),
        QStringLiteral("Runtime execution grant: approval and sandbox policy required"),
    };
    return summary;
}

AgentPlanRecord AgentRuntimeService::previewPlan(
    const QString& goal, const QString& agentId, const QString& defaultPermissionState,
    const PermissionPolicyService& permissionPolicy, const ToolExecutionGateway& toolGateway,
    const SkillProfileService& skillProfileService, const QString& selectedSkillProfileId,
    const WorkspaceService& workspaceService, const QString& selectedWorkspaceId) const {
    const auto agent = selectedAgent(agentId, defaultPermissionState, permissionPolicy, toolGateway,
                                     skillProfileService, selectedSkillProfileId, workspaceService,
                                     selectedWorkspaceId);
    return AgentPlanRegistry{}.previewPlan(goal, agent, defaultPermissionState, permissionPolicy,
                                           toolGateway, skillProfileService, selectedSkillProfileId,
                                           workspaceService, selectedWorkspaceId);
}

QString agentReadinessStateName(AgentReadinessState state) {
    switch (state) {
    case AgentReadinessState::DryRunReady:
        return QStringLiteral("Ready for approved execution");
    case AgentReadinessState::MetadataOnly:
        return QStringLiteral("Metadata only");
    case AgentReadinessState::Refused:
        return QStringLiteral("Refused");
    }
    return QStringLiteral("Metadata only");
}

QString agentPlanRiskName(AgentPlanRisk risk) {
    switch (risk) {
    case AgentPlanRisk::Low:
        return QStringLiteral("Low");
    case AgentPlanRisk::Medium:
        return QStringLiteral("Medium");
    case AgentPlanRisk::High:
        return QStringLiteral("High");
    case AgentPlanRisk::Critical:
        return QStringLiteral("Critical");
    }
    return QStringLiteral("Low");
}

QString agentRecordSummary(const AgentRecord& agent) {
    return QStringLiteral("%1 / %2 / tools %3 / posture %4")
        .arg(agent.displayName, agent.readinessState,
             agent.supportedToolCategories.join(QStringLiteral(", ")),
             agent.requiredPermissionPosture);
}

} // namespace sentinel::core
