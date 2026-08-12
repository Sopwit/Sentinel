// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/security/IApprovalPolicy.h"

#include <QMap>

namespace sentinel::core {

class StaticApprovalPolicy final : public IApprovalPolicy {
public:
    StaticApprovalPolicy() = default;
    explicit StaticApprovalPolicy(QMap<QString, ApprovalStatus> toolDecisions);

    ApprovalDecision evaluate(const ToolInvocationPlan& plan) const override;

private:
    QMap<QString, ApprovalStatus> toolDecisions_;
};

} // namespace sentinel::core
