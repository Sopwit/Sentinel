// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/AgentRuntimeService.h"

#include <QtTest>

using sentinel::core::AgentRecord;
using sentinel::core::AgentRuntimeService;
using sentinel::core::PermissionPolicyService;
using sentinel::core::SkillProfileService;
using sentinel::core::ToolExecutionGateway;
using sentinel::core::WorkspaceService;

class AgentRuntimeServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesBuiltInAgentCatalog();
    void producesPlanWithApprovalGate();
    void consultsPermissionToolProfileAndWorkspaceMetadata();
};

void AgentRuntimeServiceTest::exposesBuiltInAgentCatalog() {
    const AgentRuntimeService runtime;
    const PermissionPolicyService permissions;
    const ToolExecutionGateway tools;
    const SkillProfileService profiles;
    const WorkspaceService workspaces;

    const auto summary =
        runtime.runtimeSummary(QStringLiteral("Disabled"), permissions, tools, profiles,
                               QStringLiteral("developer"), workspaces, QStringLiteral("personal"));

    QCOMPARE(summary.status, QStringLiteral("Ready for approved execution"));
    QCOMPARE(summary.agentCount, 5);
    QCOMPARE(summary.readyAgentCount, 5);
    QCOMPARE(summary.refusedAgentCount, 0);
    QCOMPARE(summary.approvalPosture, QStringLiteral("Approval and sandbox gates enforced"));
    QVERIFY(summary.agentSummaries.join(QStringLiteral("\n"))
                .contains(QStringLiteral("Coding Assistant")));
    QVERIFY(summary.summary.contains(QStringLiteral("executes tools only after approval")));

    const auto records =
        runtime.agents(QStringLiteral("Disabled"), permissions, tools, profiles,
                       QStringLiteral("developer"), workspaces, QStringLiteral("personal"));
    for (const auto& record : records) {
        QVERIFY(record.capabilitySummary.contains(QStringLiteral("Profile: Developer")));
        QVERIFY(record.capabilitySummary.contains(QStringLiteral("Workspace: Personal")));
        QVERIFY(record.capabilitySummary.contains(QStringLiteral("Tools:")));
    }
    const auto codingAssistant = [&records]() {
        for (const auto& record : records) {
            if (record.agentId == QStringLiteral("coding-assistant")) {
                return record;
            }
        }
        return AgentRecord{};
    }();
    QVERIFY(codingAssistant.capabilitySummary.contains(QStringLiteral("Run Command")));
    QVERIFY(codingAssistant.capabilitySummary.contains(QStringLiteral("Read File")));
}

void AgentRuntimeServiceTest::producesPlanWithApprovalGate() {
    const AgentRuntimeService runtime;
    const PermissionPolicyService permissions;
    const ToolExecutionGateway tools;
    const SkillProfileService profiles;
    const WorkspaceService workspaces;

    const auto plan = runtime.previewPlan(
        QStringLiteral("Update code and run tests"), QStringLiteral("coding-assistant"),
        QStringLiteral("Enabled"), permissions, tools, profiles, QStringLiteral("developer"),
        workspaces, QStringLiteral("personal"));

    QCOMPARE(plan.planId, QStringLiteral("agent-plan-coding-assistant"));
    QCOMPARE(plan.estimatedRisk, QStringLiteral("Critical"));
    QCOMPARE(plan.approvalState, QStringLiteral("Approval required before execution"));
    QVERIFY(plan.refusalReason.contains(QStringLiteral("available through the approval")));
    QVERIFY(plan.requiredTools.join(QStringLiteral("\n")).contains(QStringLiteral("Run Command")));
    QVERIFY(plan.requiredPermissions.join(QStringLiteral("\n"))
                .contains(QStringLiteral("subprocess-execution / Enabled")));
    QVERIFY(plan.diagnostics.contains(
        QStringLiteral("Execution grant: approval and sandbox policy required")));
}

void AgentRuntimeServiceTest::consultsPermissionToolProfileAndWorkspaceMetadata() {
    const AgentRuntimeService runtime;
    const PermissionPolicyService permissions;
    const ToolExecutionGateway tools;
    const SkillProfileService profiles;
    const WorkspaceService workspaces;

    const auto plan = runtime.previewPlan(
        QStringLiteral("Research workspace voice options"), QStringLiteral("research-assistant"),
        QStringLiteral("Ask Every Time"), permissions, tools, profiles,
        QStringLiteral("researcher"), workspaces, QStringLiteral("personal"));
    const auto diagnostics = plan.diagnostics.join(QStringLiteral("\n"));

    QVERIFY(plan.requiredTools.join(QStringLiteral("\n")).contains(QStringLiteral("Web Search")));
    QVERIFY(
        plan.requiredTools.join(QStringLiteral("\n")).contains(QStringLiteral("Open Workspace")));
    QVERIFY(plan.requiredTools.join(QStringLiteral("\n")).contains(QStringLiteral("Voice Speak")));
    QVERIFY(plan.requiredPermissions.join(QStringLiteral("\n"))
                .contains(QStringLiteral("cloud-provider-access / Ask Every Time")));
    QVERIFY(diagnostics.contains(QStringLiteral("Profile: Researcher")));
    QVERIFY(diagnostics.contains(QStringLiteral("Workspace: Personal")));
    QVERIFY(diagnostics.contains(QStringLiteral("Permission posture: Ask Every Time")));
}

QTEST_MAIN(AgentRuntimeServiceTest)

#include "test_agent_runtime_service.moc"
