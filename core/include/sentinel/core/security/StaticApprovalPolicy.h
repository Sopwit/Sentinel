// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/security/IApprovalPolicy.h"

namespace sentinel::core {

class StaticApprovalPolicy final : public IApprovalPolicy {
public:
    StaticApprovalPolicy() = default;
    ApprovalDecision evaluate(const ToolInvocationPlan& plan) const override;
};

} // namespace sentinel::core
