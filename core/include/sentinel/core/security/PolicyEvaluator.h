#pragma once

#include "sentinel/core/security/IPermissionService.h"

namespace sentinel::core {

class PolicyEvaluator final {
public:
    static PermissionEffect evaluate(const QList<PermissionRule>& rules, const QString& action,
                                     const QString& resource);
};

} // namespace sentinel::core
