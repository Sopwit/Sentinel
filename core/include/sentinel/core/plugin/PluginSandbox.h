// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/plugin/PluginPermissions.h"
#include <QMap>
#include <QReadWriteLock>
#include <QString>

namespace sentinel::core::plugin {

class PluginSandbox {
public:
    PluginSandbox() = default;

    void registerPluginPermissions(const QString& pluginId, const PluginPermissions& permissions);
    void grantPermission(const QString& pluginId, const QString& permission);
    void revokePermission(const QString& pluginId, const QString& permission);

    bool checkPermission(const QString& pluginId, const QString& permission) const;
    PluginPermissions getPermissions(const QString& pluginId) const;
    void setActive(const QString& pluginId, bool active);

    void clearPlugin(const QString& pluginId);
    void clearAll();

private:
    mutable QReadWriteLock m_mutex;
    QMap<QString, PluginPermissions> m_pluginPermissions;
    QMap<QString, PluginPermissions> m_declaredPermissions;
    QMap<QString, bool> m_active;
};

} // namespace sentinel::core::plugin
