// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/security/PermissionService.h"

#include "sentinel/core/security/PathGuard.h"

namespace sentinel::core {

PermissionEffect PermissionService::evaluateAuthorization(const AuthorizationRequest& request,
                                                           const QString& sessionId) const {
    bool allowed = false;
    bool denied = false;
    for (const auto& grant : grants_) {
        if (grant.request.domain != request.domain || grant.request.access != request.access ||
            (!grant.persistent && grant.sessionId != sessionId))
            continue;

        bool matchesResource = grant.request.resource == request.resource;
        if (!matchesResource && request.domain == SecurityDomain::FileSystem)
            matchesResource = PathGuard::contains(grant.request.resource, request.resource);
        if (!matchesResource && (request.domain == SecurityDomain::Network ||
                                 request.domain == SecurityDomain::Browser)) {
            const QString host = request.resource.toLower();
            const QString scope = grant.request.resource.toLower();
            matchesResource = host == scope || host.endsWith(QLatin1Char('.') + scope);
        }
        if (!matchesResource)
            continue;
        allowed |= grant.effect == PermissionEffect::Allow;
        denied |= grant.effect == PermissionEffect::Deny;
    }
    if (denied)
        return PermissionEffect::Deny;
    return allowed ? PermissionEffect::Allow : PermissionEffect::Ask;
}

void PermissionService::setAuthorization(const AuthorizationRequest& request,
                                         PermissionEffect effect, const QString& sessionId,
                                         bool persistent) {
    if (request.resource.isEmpty() &&
        (request.domain == SecurityDomain::FileSystem || request.domain == SecurityDomain::Network ||
         request.domain == SecurityDomain::Browser ||
         request.domain == SecurityDomain::ExternalService))
        return;

    grants_.removeIf([&](const Grant& grant) {
        return grant.request.domain == request.domain && grant.request.access == request.access &&
               grant.request.resource == request.resource &&
               (persistent || grant.sessionId == sessionId);
    });
    if (effect != PermissionEffect::Ask)
        grants_.append({request, persistent ? QString{} : sessionId, persistent, effect});
}

void PermissionService::grantAuthorization(const AuthorizationRequest& request,
                                           const QString& sessionId, bool persistent) {
    setAuthorization(request, PermissionEffect::Allow, sessionId, persistent);
}

void PermissionService::revokeAuthorization(const AuthorizationRequest& request,
                                            const QString& sessionId) {
    grants_.removeIf([&](const Grant& grant) {
        return !grant.persistent && grant.sessionId == sessionId &&
               grant.request.domain == request.domain && grant.request.access == request.access &&
               grant.request.resource == request.resource;
    });
}

void PermissionService::clearSessionGrants(const QString& sessionId) {
    grants_.removeIf([&](const Grant& grant) {
        return !grant.persistent && grant.sessionId == sessionId;
    });
}

} // namespace sentinel::core
