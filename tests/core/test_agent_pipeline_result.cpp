#include "sentinel/core/AgentPipelineResult.h"

#include <QtTest>

class AgentPipelineResultTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesDefaultSafeStatusAndSummary();
    void usesFallbacksForEmptySummaries();
    void exposesDeterministicStageSummaries();
};

void AgentPipelineResultTest::exposesDefaultSafeStatusAndSummary() {
    const sentinel::core::AgentPipelineResult result;

    QCOMPARE(result.planningStatus(), sentinel::core::ToolInvocationPlanStatus::NotRequested);
    QCOMPARE(result.approvalStatus(), sentinel::core::ApprovalStatus::NotRequested);
    QCOMPARE(result.sandboxStatus(), sentinel::core::SandboxStatus::NotEvaluated);
    QCOMPARE(result.executionStatus(), sentinel::core::ToolExecutionStatus::NotRequested);
    QCOMPARE(sentinel::core::agentPipelineStatusName(result), QStringLiteral("Not Requested"));
    QCOMPARE(sentinel::core::safeAgentPipelineSummary(result),
             QStringLiteral("No agent pipeline result yet."));
    QCOMPARE(sentinel::core::safeToolPlanSummary(result.plan), QStringLiteral("No tool plan yet."));
    QCOMPARE(sentinel::core::safeApprovalSummary(result.approval),
             QStringLiteral("No approval decision yet."));
    QCOMPARE(sentinel::core::safeSandboxSummary(result.sandbox),
             QStringLiteral("No sandbox evaluation yet."));
    QCOMPARE(sentinel::core::safeToolExecutionSummary(result.execution),
             QStringLiteral("No tool execution boundary result yet."));
}

void AgentPipelineResultTest::usesFallbacksForEmptySummaries() {
    sentinel::core::AgentPipelineResult result;
    result.summary.clear();
    result.plan.summary.clear();
    result.approval.summary.clear();
    result.sandbox.summary.clear();
    result.execution.summary.clear();

    QCOMPARE(sentinel::core::safeAgentPipelineSummary(result),
             QStringLiteral("No agent pipeline result yet."));
    QCOMPARE(sentinel::core::safeToolPlanSummary(result.plan), QStringLiteral("No tool plan yet."));
    QCOMPARE(sentinel::core::safeApprovalSummary(result.approval),
             QStringLiteral("No approval decision yet."));
    QCOMPARE(sentinel::core::safeSandboxSummary(result.sandbox),
             QStringLiteral("No sandbox evaluation yet."));
    QCOMPARE(sentinel::core::safeToolExecutionSummary(result.execution),
             QStringLiteral("No tool execution boundary result yet."));
}

void AgentPipelineResultTest::exposesDeterministicStageSummaries() {
    sentinel::core::AgentPipelineResult result;
    result.plan = {
        sentinel::core::ToolInvocationPlanStatus::Planned,
        QStringLiteral("Plan summary."),
        {},
    };
    result.approval = {
        sentinel::core::ApprovalStatus::NotRequired,
        QStringLiteral("Approval summary."),
        {},
    };
    result.sandbox = {
        sentinel::core::SandboxStatus::Allowed,
        QStringLiteral("Sandbox summary."),
        {},
    };
    result.execution = {
        sentinel::core::ToolExecutionStatus::PlaceholderSucceeded,
        QStringLiteral("Execution summary."),
    };
    result.summary = sentinel::core::safeToolExecutionSummary(result.execution);

    QCOMPARE(result.planningStatus(), sentinel::core::ToolInvocationPlanStatus::Planned);
    QCOMPARE(result.approvalStatus(), sentinel::core::ApprovalStatus::NotRequired);
    QCOMPARE(result.sandboxStatus(), sentinel::core::SandboxStatus::Allowed);
    QCOMPARE(result.executionStatus(), sentinel::core::ToolExecutionStatus::PlaceholderSucceeded);
    QCOMPARE(sentinel::core::agentPipelineStatusName(result),
             QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(sentinel::core::safeAgentPipelineSummary(result),
             QStringLiteral("Execution summary."));
    QCOMPARE(sentinel::core::safeToolPlanSummary(result.plan), QStringLiteral("Plan summary."));
    QCOMPARE(sentinel::core::safeApprovalSummary(result.approval),
             QStringLiteral("Approval summary."));
    QCOMPARE(sentinel::core::safeSandboxSummary(result.sandbox),
             QStringLiteral("Sandbox summary."));
    QCOMPARE(sentinel::core::safeToolExecutionSummary(result.execution),
             QStringLiteral("Execution summary."));
}

QTEST_MAIN(AgentPipelineResultTest)
#include "test_agent_pipeline_result.moc"
