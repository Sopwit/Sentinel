// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace sentinel::core {

struct ExternalDirectoryPolicy {
    bool enabled{true};
    bool allowExplicitGrant{true};
    int maxDepth{3};
};

class ExternalDirectoryGate {
public:
    explicit ExternalDirectoryGate(const ExternalDirectoryPolicy& policy = {});

    bool isAccessAllowed(const QString& path, const QString& workingDir) const;
    bool checkAndRequestPermission(const QString& path, const QString& workingDir) const;
    void grantPermission(const QString& path);
    void revokePermission(const QString& path);
    bool hasPermission(const QString& path) const;
    void setWorkingDirectory(const QString& dir);

private:
    bool isWithinDirectory(const QString& path, const QString& directory) const;
    QString resolvePath(const QString& path) const;

    ExternalDirectoryPolicy m_policy;
    QString m_workingDir;
    QSet<QString> m_grantedPermissions;
};

} // namespace sentinel::core
