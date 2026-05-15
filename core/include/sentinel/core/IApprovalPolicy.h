#pragma once

#include "sentinel/core/ToolApproval.h"
#include "sentinel/core/ToolInvocationPlan.h"

namespace sentinel::core {

class IApprovalPolicy {
public:
    virtual ~IApprovalPolicy() = default;

    virtual ApprovalDecision evaluate(const ToolInvocationPlan& plan) const = 0;
};

} // namespace sentinel::core
