// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/security/ISandboxPolicy.h"

#include <QSet>

namespace sentinel::core {

class StaticSandboxPolicy final : public ISandboxPolicy {
public:
    StaticSandboxPolicy();
    explicit StaticSandboxPolicy(QSet<QString> allowedCapabilityIds);

    SandboxEvaluationResult evaluate(const ToolInvocationPlan& plan,
                                     const ApprovalDecision& approval) const override;

private:
    QSet<QString> allowedCapabilityIds_;
};

} // namespace sentinel::core
