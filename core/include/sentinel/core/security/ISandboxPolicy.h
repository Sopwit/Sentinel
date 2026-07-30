// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/ToolApproval.h"
#include "sentinel/core/runtime/ToolInvocationPlan.h"
#include "sentinel/core/runtime/ToolSandbox.h"

namespace sentinel::core {

class ISandboxPolicy {
public:
    virtual ~ISandboxPolicy() = default;

    virtual SandboxEvaluationResult evaluate(const ToolInvocationPlan& plan,
                                             const ApprovalDecision& approval) const = 0;
};

} // namespace sentinel::core
