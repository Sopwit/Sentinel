#pragma once

#include "sentinel/core/ToolApproval.h"
#include "sentinel/core/ToolInvocationPlan.h"
#include "sentinel/core/ToolSandbox.h"

namespace sentinel::core {

class ISandboxPolicy {
public:
    virtual ~ISandboxPolicy() = default;

    virtual SandboxEvaluationResult evaluate(const ToolInvocationPlan& plan,
                                             const ApprovalDecision& approval) const = 0;
};

} // namespace sentinel::core
