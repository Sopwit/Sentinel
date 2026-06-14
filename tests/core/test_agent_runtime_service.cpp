#include "sentinel/core/AgentRuntimeService.h"

#include <QtTest>

using sentinel::core::AgentRuntimeService;
using sentinel::core::PermissionPolicyService;
using sentinel::core::SkillProfileService;
using sentinel::core::ToolExecutionGateway;
using sentinel::core::WorkspaceService;

class AgentRuntimeServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesBuiltInAgentCatalog();
    void producesDryRunPlanWithoutExecutionGrant();
    void consultsPermissionToolProfileAndWorkspaceMetadata();
};

void AgentRuntimeServiceTest::exposesBuiltInAgentCatalog() {
    const AgentRuntimeService runtime;
    const PermissionPolicyService permissions;
    const ToolExecutionGateway tools;
    const SkillProfileService profiles;
    const WorkspaceService workspaces;

    const auto summary = runtime.runtimeSummary(QStringLiteral("Disabled"), permissions, tools,
                                                profiles, QStringLiteral("developer"), workspaces,
                                                QStringLiteral("personal"));

    QCOMPARE(summary.status, QStringLiteral("Dry-run planning only"));
    QCOMPARE(summary.agentCount, 5);
    QCOMPARE(summary.readyAgentCount, 5);
    QCOMPARE(summary.refusedAgentCount, 0);
    QCOMPARE(summary.approvalPosture, QStringLiteral("Approval cannot enable execution"));
    QVERIFY(summary.agentSummaries.join(QStringLiteral("\n"))
                .contains(QStringLiteral("Coding Assistant")));
    QVERIFY(summary.summary.contains(QStringLiteral("cannot execute tools")));
}

void AgentRuntimeServiceTest::producesDryRunPlanWithoutExecutionGrant() {
    const AgentRuntimeService runtime;
    const PermissionPolicyService permissions;
    const ToolExecutionGateway tools;
    const SkillProfileService profiles;
    const WorkspaceService workspaces;

    const auto plan = runtime.previewPlan(QStringLiteral("Update code and run tests"),
                                          QStringLiteral("coding-assistant"),
                                          QStringLiteral("Enabled"), permissions, tools, profiles,
                                          QStringLiteral("developer"), workspaces,
                                          QStringLiteral("personal"));

    QCOMPARE(plan.planId, QStringLiteral("dry-run-coding-assistant"));
    QCOMPARE(plan.estimatedRisk, QStringLiteral("Critical"));
    QCOMPARE(plan.approvalState, QStringLiteral("Approval disabled / dry-run only"));
    QVERIFY(plan.refusalReason.contains(QStringLiteral("execution is disabled")));
    QVERIFY(plan.requiredTools.join(QStringLiteral("\n")).contains(QStringLiteral("Run Command")));
    QVERIFY(plan.requiredPermissions.join(QStringLiteral("\n"))
                .contains(QStringLiteral("subprocess-execution / Enabled")));
    QVERIFY(plan.diagnostics.contains(QStringLiteral("Execution grant: none")));
}

void AgentRuntimeServiceTest::consultsPermissionToolProfileAndWorkspaceMetadata() {
    const AgentRuntimeService runtime;
    const PermissionPolicyService permissions;
    const ToolExecutionGateway tools;
    const SkillProfileService profiles;
    const WorkspaceService workspaces;

    const auto plan = runtime.previewPlan(QStringLiteral("Research workspace voice options"),
                                          QStringLiteral("research-assistant"),
                                          QStringLiteral("Ask Every Time"), permissions, tools,
                                          profiles, QStringLiteral("researcher"), workspaces,
                                          QStringLiteral("personal"));
    const auto diagnostics = plan.diagnostics.join(QStringLiteral("\n"));

    QVERIFY(plan.requiredTools.join(QStringLiteral("\n")).contains(QStringLiteral("Web Search")));
    QVERIFY(plan.requiredTools.join(QStringLiteral("\n")).contains(QStringLiteral("Open Workspace")));
    QVERIFY(plan.requiredTools.join(QStringLiteral("\n")).contains(QStringLiteral("Voice Speak")));
    QVERIFY(plan.requiredPermissions.join(QStringLiteral("\n"))
                .contains(QStringLiteral("cloud-provider-access / Ask Every Time")));
    QVERIFY(diagnostics.contains(QStringLiteral("Profile: Researcher")));
    QVERIFY(diagnostics.contains(QStringLiteral("Workspace: Personal")));
    QVERIFY(diagnostics.contains(QStringLiteral("Permission posture: Ask Every Time")));
}

QTEST_MAIN(AgentRuntimeServiceTest)

#include "test_agent_runtime_service.moc"
