// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginSandbox.h"
#include <QReadLocker>
#include <QWriteLocker>

namespace sentinel::core::plugin {

void PluginSandbox::registerPluginPermissions(const QString& pluginId,
                                              const PluginPermissions& permissions) {
    QWriteLocker lock(&m_mutex);
    m_pluginPermissions[pluginId] = permissions;
}

void PluginSandbox::grantPermission(const QString& pluginId, const QString& permission) {
    QWriteLocker lock(&m_mutex);
    m_pluginPermissions[pluginId].grant(permission);
}

void PluginSandbox::revokePermission(const QString& pluginId, const QString& permission) {
    QWriteLocker lock(&m_mutex);
    if (m_pluginPermissions.contains(pluginId)) {
        m_pluginPermissions[pluginId].revoke(permission);
    }
}

bool PluginSandbox::checkPermission(const QString& pluginId, const QString& permission) const {
    QReadLocker lock(&m_mutex);
    if (!m_pluginPermissions.contains(pluginId)) {
        return false;
    }
    return m_pluginPermissions.value(pluginId).has(permission);
}

PluginPermissions PluginSandbox::getPermissions(const QString& pluginId) const {
    QReadLocker lock(&m_mutex);
    return m_pluginPermissions.value(pluginId);
}

void PluginSandbox::clearPlugin(const QString& pluginId) {
    QWriteLocker lock(&m_mutex);
    m_pluginPermissions.remove(pluginId);
}

void PluginSandbox::clearAll() {
    QWriteLocker lock(&m_mutex);
    m_pluginPermissions.clear();
}

} // namespace sentinel::core::plugin
