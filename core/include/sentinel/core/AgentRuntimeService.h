#pragma once

#include "sentinel/core/PermissionPolicyService.h"
#include "sentinel/core/SkillProfileService.h"
#include "sentinel/core/ToolExecutionGateway.h"
#include "sentinel/core/WorkspaceService.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {

enum class AgentReadinessState {
    DryRunReady,
    MetadataOnly,
    Refused,
};

enum class AgentPlanRisk {
    Low,
    Medium,
    High,
    Critical,
};

struct AgentRecord {
    QString agentId;
    QString displayName;
    QString description;
    QString capabilitySummary;
    QStringList supportedToolCategories;
    QString requiredPermissionPosture;
    QString readinessState;
    QString refusalReason;
};

struct AgentPlanRecord {
    QString planId;
    QString goalSummary;
    QStringList orderedPlanSteps;
    QStringList requiredTools;
    QStringList requiredPermissions;
    QString estimatedRisk;
    QString approvalState;
    QString refusalReason;
    QStringList diagnostics;
};

struct AgentRuntimeSummary {
    QString status;
    QString summary;
    QString approvalPosture;
    int agentCount = 0;
    int readyAgentCount = 0;
    int refusedAgentCount = 0;
    QStringList agentSummaries;
    QStringList readinessSummaries;
    QStringList developerDiagnostics;
};

class AgentPlanRegistry final {
public:
    AgentPlanRecord previewPlan(
        const QString& goal, const AgentRecord& agent, const QString& defaultPermissionState,
        const PermissionPolicyService& permissionPolicy, const ToolExecutionGateway& toolGateway,
        const SkillProfileService& skillProfileService, const QString& selectedSkillProfileId,
        const WorkspaceService& workspaceService, const QString& selectedWorkspaceId) const;
};

class AgentRuntimeService final {
public:
    QList<AgentRecord>
    agents(const QString& defaultPermissionState, const PermissionPolicyService& permissionPolicy,
           const ToolExecutionGateway& toolGateway, const SkillProfileService& skillProfileService,
           const QString& selectedSkillProfileId, const WorkspaceService& workspaceService,
           const QString& selectedWorkspaceId) const;
    AgentRecord selectedAgent(const QString& agentId, const QString& defaultPermissionState,
                              const PermissionPolicyService& permissionPolicy,
                              const ToolExecutionGateway& toolGateway,
                              const SkillProfileService& skillProfileService,
                              const QString& selectedSkillProfileId,
                              const WorkspaceService& workspaceService,
                              const QString& selectedWorkspaceId) const;
    AgentRuntimeSummary runtimeSummary(const QString& defaultPermissionState,
                                       const PermissionPolicyService& permissionPolicy,
                                       const ToolExecutionGateway& toolGateway,
                                       const SkillProfileService& skillProfileService,
                                       const QString& selectedSkillProfileId,
                                       const WorkspaceService& workspaceService,
                                       const QString& selectedWorkspaceId) const;
    AgentPlanRecord previewPlan(
        const QString& goal, const QString& agentId, const QString& defaultPermissionState,
        const PermissionPolicyService& permissionPolicy, const ToolExecutionGateway& toolGateway,
        const SkillProfileService& skillProfileService, const QString& selectedSkillProfileId,
        const WorkspaceService& workspaceService, const QString& selectedWorkspaceId) const;
};

QString agentReadinessStateName(AgentReadinessState state);
QString agentPlanRiskName(AgentPlanRisk risk);
QString agentRecordSummary(const AgentRecord& agent);

} // namespace sentinel::core
