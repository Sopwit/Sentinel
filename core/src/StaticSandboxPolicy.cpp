#include "sentinel/core/StaticSandboxPolicy.h"

#include <utility>

namespace sentinel::core {

namespace {

CapabilityDescriptor metadataCapability() {
    return {
        QStringLiteral("tool.metadata.read"),
        QStringLiteral("Read metadata-only planned tool invocation details."),
    };
}

CapabilityDescriptor riskCapability(ToolRiskLevel riskLevel) {
    if (riskLevel == ToolRiskLevel::Medium) {
        return {
            QStringLiteral("tool.risk.medium"),
            QStringLiteral("Future runtime support for medium-risk tool capability."),
        };
    }

    return {
        QStringLiteral("tool.risk.high"),
        QStringLiteral("Future runtime support for high-risk tool capability."),
    };
}

QList<CapabilityDescriptor> capabilitiesForInvocation(const PlannedToolInvocation& invocation) {
    auto capabilities = invocation.requiredCapabilities;
    if (capabilities.isEmpty()) {
        capabilities.append(metadataCapability());
    }

    if (invocation.riskLevel == ToolRiskLevel::Medium ||
        invocation.riskLevel == ToolRiskLevel::High) {
        capabilities.append(riskCapability(invocation.riskLevel));
    }

    return capabilities;
}

} // namespace

StaticSandboxPolicy::StaticSandboxPolicy()
    : allowedCapabilityIds_({
          QStringLiteral("tool.metadata.read"),
          QStringLiteral("tool.risk.medium"),
          QStringLiteral("tool.risk.high")
      }) {}

StaticSandboxPolicy::StaticSandboxPolicy(QSet<QString> allowedCapabilityIds)
    : allowedCapabilityIds_(std::move(allowedCapabilityIds)) {}

SandboxEvaluationResult StaticSandboxPolicy::evaluate(const ToolInvocationPlan& plan,
                                                      const ApprovalDecision& approval) const {
    if (plan.status != ToolInvocationPlanStatus::Planned || plan.invocations.isEmpty()) {
        return {
            SandboxStatus::NotEvaluated,
            QStringLiteral("No planned tool invocation to evaluate for sandbox capabilities."),
            {},
        };
    }

    if (approval.status == ApprovalStatus::Denied ||
        approval.status == ApprovalStatus::RequiresApproval) {
        return {
            SandboxStatus::BlockedByApproval,
            QStringLiteral("Sandbox capability evaluation is blocked by approval metadata."),
            {},
        };
    }

    QList<SandboxCapabilityDecision> capabilityDecisions;
    auto allAllowed = true;

    for (const auto& invocation : plan.invocations) {
        const auto capabilities = capabilitiesForInvocation(invocation);
        for (const auto& capability : capabilities) {
            const auto allowed = allowedCapabilityIds_.contains(capability.id);
            allAllowed = allAllowed && allowed;
            capabilityDecisions.append(SandboxCapabilityDecision{
                invocation.toolId,
                capability,
                allowed,
                allowed ? QStringLiteral("Capability is allowed by sandbox metadata policy.")
                        : QStringLiteral("Capability is not allowed by sandbox metadata policy."),
            });
        }
    }

    if (!allAllowed) {
        return {
            SandboxStatus::Denied,
            QStringLiteral("One or more planned capabilities are outside sandbox metadata policy."),
            capabilityDecisions,
        };
    }

    return {
        SandboxStatus::Allowed,
        QStringLiteral("Planned tool capabilities are allowed by sandbox metadata policy."),
        capabilityDecisions,
    };
}

} // namespace sentinel::core
