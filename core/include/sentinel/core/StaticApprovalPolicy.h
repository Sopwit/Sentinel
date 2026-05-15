#pragma once

#include "sentinel/core/IApprovalPolicy.h"

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
