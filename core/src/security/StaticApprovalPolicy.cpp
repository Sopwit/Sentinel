// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/security/StaticApprovalPolicy.h"

namespace sentinel::core {

ApprovalDecision StaticApprovalPolicy::evaluate(const ToolInvocationPlan& plan) const {
    if (plan.status != ToolInvocationPlanStatus::Planned || plan.invocations.isEmpty()) {
        return {
            ApprovalStatus::NotRequested,
            QStringLiteral("No planned tool invocation to approve."),
            {},
        };
    }

    QList<ToolApprovalRequest> requests;
    for (const auto& invocation : plan.invocations) {
        if (invocation.riskLevel == ToolRiskLevel::Medium ||
            invocation.riskLevel == ToolRiskLevel::High) {
            requests.append(ToolApprovalRequest{
                invocation.toolId,
                invocation.summary,
                invocation.riskLevel,
                {},
            });
        }
    }

    if (!requests.isEmpty()) {
        return {
            ApprovalStatus::RequiresApproval,
            QStringLiteral("One or more planned tool invocations require approval."),
            requests,
        };
    }

    return {
        ApprovalStatus::NotRequired,
        QStringLiteral("Planned tool invocations do not require approval."),
        {},
    };
}

} // namespace sentinel::core
