#include "sentinel/core/NullToolExecutor.h"

namespace sentinel::core {

ToolExecutionResult NullToolExecutor::execute(const ToolExecutionRequest& request) const {
    if (request.plan.status != ToolInvocationPlanStatus::Planned ||
        request.plan.invocations.isEmpty()) {
        return {
            ToolExecutionStatus::EmptyPlan,
            QStringLiteral("No planned tool invocation reached the execution boundary."),
        };
    }

    for (const auto& invocation : request.plan.invocations) {
        if (!request.knownToolIds.contains(invocation.toolId)) {
            return {
                ToolExecutionStatus::UnknownTool,
                QStringLiteral("Execution boundary rejected unknown tool metadata: %1")
                    .arg(invocation.toolId),
            };
        }
    }

    if (request.approval.status == ApprovalStatus::Denied ||
        request.approval.status == ApprovalStatus::RequiresApproval) {
        return {
            ToolExecutionStatus::Blocked,
            QStringLiteral("Execution boundary blocked by approval metadata."),
        };
    }

    if (request.sandbox.status != SandboxStatus::Allowed) {
        return {
            ToolExecutionStatus::Blocked,
            QStringLiteral("Execution boundary blocked by sandbox capability metadata."),
        };
    }

    return {
        ToolExecutionStatus::PlaceholderSucceeded,
        QStringLiteral("Placeholder tool execution completed without performing actions."),
    };
}

} // namespace sentinel::core
