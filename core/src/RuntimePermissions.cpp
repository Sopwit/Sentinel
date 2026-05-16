#include "sentinel/core/RuntimePermissions.h"

namespace sentinel::core {

QString safeRuntimePermissionRequestSummary(const RuntimePermissionRequest& request) {
    if (!request.summary.isEmpty()) {
        return request.summary;
    }

    return QStringLiteral("%1 / %2").arg(runtimePermissionName(request.permission),
                                         runtimePermissionLevelName(request.level));
}

QString safeRuntimePermissionDecisionSummary(const RuntimePermissionDecision& decision) {
    if (!decision.summary.isEmpty()) {
        return decision.summary;
    }

    return QStringLiteral("%1 permission decision for %2.")
        .arg(runtimePermissionDecisionStatusName(decision.status),
             runtimePermissionName(decision.request.permission));
}

RuntimePermissionDecision
StaticRuntimePermissionPolicy::evaluate(const RuntimePermissionRequest& request) const {
    if (request.level == RuntimePermissionLevel::None) {
        return RuntimePermissionDecision{
            RuntimePermissionDecisionStatus::NotRequested,
            request,
            QStringLiteral("No runtime permission is requested."),
        };
    }

    return RuntimePermissionDecision{
        RuntimePermissionDecisionStatus::Denied,
        request,
        QStringLiteral("Runtime permission policy is metadata-only and denies execution by "
                       "default."),
    };
}

} // namespace sentinel::core
