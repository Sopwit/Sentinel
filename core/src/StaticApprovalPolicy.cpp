#include "sentinel/core/StaticApprovalPolicy.h"

#include <utility>

namespace sentinel::core {

StaticApprovalPolicy::StaticApprovalPolicy(QMap<QString, ApprovalStatus> toolDecisions)
    : toolDecisions_(std::move(toolDecisions)) {}

ApprovalDecision StaticApprovalPolicy::evaluate(const ToolInvocationPlan& plan) const {
    if (plan.status != ToolInvocationPlanStatus::Planned || plan.invocations.isEmpty()) {
        return {
            ApprovalStatus::NotRequested,
            QStringLiteral("No planned tool invocation to approve."),
            {},
        };
    }

    QList<ToolApprovalRequest> requests;
    auto hasDenied = false;
    auto hasApprovedOverride = false;

    for (const auto& invocation : plan.invocations) {
        const auto overrideDecision = toolDecisions_.find(invocation.toolId);
        if (overrideDecision != toolDecisions_.end()) {
            if (overrideDecision.value() == ApprovalStatus::Denied) {
                hasDenied = true;
            } else if (overrideDecision.value() == ApprovalStatus::Approved) {
                hasApprovedOverride = true;
            }
        }

        if (invocation.riskLevel == ToolRiskLevel::Medium ||
            invocation.riskLevel == ToolRiskLevel::High) {
            requests.append(ToolApprovalRequest{
                invocation.toolId,
                invocation.summary,
                invocation.riskLevel,
                {
                    PermissionDescriptor{
                        QStringLiteral("tool.metadata.approval"),
                        QStringLiteral("Approval metadata for planned tool invocation."),
                    },
                },
            });
        }
    }

    if (hasDenied) {
        return {
            ApprovalStatus::Denied,
            QStringLiteral("At least one planned tool invocation is denied by policy."),
            requests,
        };
    }

    if (!requests.isEmpty()) {
        return {
            ApprovalStatus::Approved,
            QStringLiteral("All planned tool invocations are auto-approved by runtime policy."),
            requests,
        };
    }

    if (hasApprovedOverride) {
        return {
            ApprovalStatus::Approved,
            QStringLiteral("Planned tool invocations are approved by policy metadata."),
            {},
        };
    }

    return {
        ApprovalStatus::NotRequired,
        QStringLiteral("Planned tool invocations do not require approval."),
        {},
    };
}

} // namespace sentinel::core
