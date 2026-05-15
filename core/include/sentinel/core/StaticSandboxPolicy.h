#pragma once

#include "sentinel/core/ISandboxPolicy.h"

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
