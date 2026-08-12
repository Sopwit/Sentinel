// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/ToolApproval.h"
#include "sentinel/core/runtime/ToolInvocationPlan.h"

namespace sentinel::core {

class IApprovalPolicy {
public:
    virtual ~IApprovalPolicy() = default;

    virtual ApprovalDecision evaluate(const ToolInvocationPlan& plan) const = 0;
};

} // namespace sentinel::core
