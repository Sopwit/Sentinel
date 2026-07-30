// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginSandbox.h"

namespace sentinel::core::plugin {

void PluginSandbox::registerPluginPermissions(const QString& pluginId, const PluginPermissions& permissions) {
    m_pluginPermissions[pluginId] = permissions;
}

void PluginSandbox::grantPermission(const QString& pluginId, const QString& permission) {
    m_pluginPermissions[pluginId].grant(permission);
}

void PluginSandbox::revokePermission(const QString& pluginId, const QString& permission) {
    if (m_pluginPermissions.contains(pluginId)) {
        m_pluginPermissions[pluginId].revoke(permission);
    }
}

bool PluginSandbox::checkPermission(const QString& pluginId, const QString& permission) const {
    if (!m_pluginPermissions.contains(pluginId)) {
        return false;
    }
    return m_pluginPermissions.value(pluginId).has(permission);
}

PluginPermissions PluginSandbox::getPermissions(const QString& pluginId) const {
    return m_pluginPermissions.value(pluginId);
}

void PluginSandbox::clearPlugin(const QString& pluginId) {
    m_pluginPermissions.remove(pluginId);
}

void PluginSandbox::clearAll() {
    m_pluginPermissions.clear();
}

} // namespace sentinel::core::plugin
