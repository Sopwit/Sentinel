// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonArray>
#include <QSet>
#include <QString>
#include <QStringList>

namespace sentinel::core::plugin {

namespace Permissions {
inline const QString NetworkLoopback = QStringLiteral("network.loopback.read_write");
inline const QString NetworkExternal = QStringLiteral("network.external");
inline const QString ModelConfigRead = QStringLiteral("model.config.read");
inline const QString ModelConfigWrite = QStringLiteral("model.config.write");
inline const QString FileSystemRead = QStringLiteral("filesystem.read");
inline const QString FileSystemWrite = QStringLiteral("filesystem.write");
inline const QString ToolExecution = QStringLiteral("tool.execute");
inline const QString DatabaseAccess = QStringLiteral("database.access");
} // namespace Permissions

class PluginPermissions {
public:
    PluginPermissions() = default;
    explicit PluginPermissions(const QStringList& permissions);

    void grant(const QString& permission);
    void revoke(const QString& permission);
    bool has(const QString& permission) const;

    QStringList toList() const;
    QJsonArray toJsonArray() const;

    static PluginPermissions fromJsonArray(const QJsonArray& array);

private:
    QSet<QString> m_permissions;
};

} // namespace sentinel::core::plugin
