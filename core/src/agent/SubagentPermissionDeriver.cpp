// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/SubagentPermissionDeriver.h"

namespace sentinel::core {

DerivedPermission SubagentPermissionDeriver::derive(const QString& parentAgentId, const QString& childAgentId) const {
    DerivedPermission perm;
    perm.agentId = childAgentId;
    perm.parentAgentId = parentAgentId;
    perm.allowed = !m_denyByDefault;
    perm.inheritedFrom.append(parentAgentId);
    return perm;
}

bool SubagentPermissionDeriver::isAllowed(const QString& agentId, const QString& action) const {
    Q_UNUSED(agentId)
    Q_UNUSED(action)
    return !m_denyByDefault;
}

QStringList SubagentPermissionDeriver::inheritedPermissions(const QString& agentId) const {
    Q_UNUSED(agentId)
    return {};
}

void SubagentPermissionDeriver::setDenyByDefault(bool deny) { m_denyByDefault = deny; }
bool SubagentPermissionDeriver::denyByDefault() const { return m_denyByDefault; }

} // namespace sentinel::core
