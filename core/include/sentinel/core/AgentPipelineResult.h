#pragma once

#include "sentinel/core/ToolExecution.h"

#include <QString>

namespace sentinel::core {

inline QString safeToolPlanSummary(const ToolInvocationPlan& plan) {
    return plan.summary.isEmpty() ? QStringLiteral("No tool plan yet.") : plan.summary;
}

inline QString safeApprovalSummary(const ApprovalDecision& approval) {
    return approval.summary.isEmpty() ? QStringLiteral("No approval decision yet.")
                                      : approval.summary;
}

inline QString safeSandboxSummary(const SandboxEvaluationResult& sandbox) {
    return sandbox.summary.isEmpty() ? QStringLiteral("No sandbox evaluation yet.")
                                     : sandbox.summary;
}

inline QString safeToolExecutionSummary(const ToolExecutionResult& execution) {
    return execution.summary.isEmpty() ? QStringLiteral("No tool execution boundary result yet.")
                                       : execution.summary;
}

struct AgentPipelineResult {
    ToolInvocationPlan plan;
    ApprovalDecision approval;
    SandboxEvaluationResult sandbox;
    ToolExecutionResult execution;
    QString summary = QStringLiteral("No agent pipeline result yet.");

    ToolInvocationPlanStatus planningStatus() const {
        return plan.status;
    }

    ApprovalStatus approvalStatus() const {
        return approval.status;
    }

    SandboxStatus sandboxStatus() const {
        return sandbox.status;
    }

    ToolExecutionStatus executionStatus() const {
        return execution.status;
    }
};

inline QString agentPipelineStatusName(const AgentPipelineResult& result) {
    return toolExecutionStatusName(result.executionStatus());
}

inline QString safeAgentPipelineSummary(const AgentPipelineResult& result) {
    return result.summary.isEmpty() ? QStringLiteral("No agent pipeline result yet.")
                                    : result.summary;
}

} // namespace sentinel::core
