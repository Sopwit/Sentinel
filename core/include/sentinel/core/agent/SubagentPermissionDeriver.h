// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

struct DerivedPermission {
    QString agentId;
    QString parentAgentId;
    bool allowed{true};
    QStringList inheritedFrom;
};

class SubagentPermissionDeriver {
public:
    DerivedPermission derive(const QString& parentAgentId, const QString& childAgentId) const;
    bool isAllowed(const QString& agentId, const QString& action) const;
    QStringList inheritedPermissions(const QString& agentId) const;
    void setDenyByDefault(bool deny);
    bool denyByDefault() const;

private:
    bool m_denyByDefault{true};
};

} // namespace sentinel::core
