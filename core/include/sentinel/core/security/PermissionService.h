// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/ToolDescriptor.h"
#include "sentinel/core/security/IPermissionService.h"

#include <QList>
#include <QString>

namespace sentinel::core {

// Owns runtime user grants. Defaults are evaluated by PermissionPolicyService;
// this class stores only explicit decisions.
class PermissionService final {
public:
    PermissionEffect evaluateAuthorization(const AuthorizationRequest& request,
                                           const QString& sessionId) const;
    void setAuthorization(const AuthorizationRequest& request, PermissionEffect effect,
                          const QString& sessionId, bool persistent);
    void grantAuthorization(const AuthorizationRequest& request, const QString& sessionId,
                            bool persistent);
    void revokeAuthorization(const AuthorizationRequest& request, const QString& sessionId);
    void clearSessionGrants(const QString& sessionId);

private:
    struct Grant {
        AuthorizationRequest request;
        QString sessionId;
        bool persistent = false;
        PermissionEffect effect = PermissionEffect::Allow;
    };
    QList<Grant> grants_;
};

} // namespace sentinel::core
