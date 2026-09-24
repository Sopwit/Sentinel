// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

namespace sentinel::core {

struct ExternalDirectoryPolicy {
    bool enabled{true};
    bool allowExplicitGrant{true};
    int maxDepth{3};
};

// Session-scoped grants for user-owned paths outside the workspace.
class ExternalDirectoryGate {
public:
    explicit ExternalDirectoryGate(const ExternalDirectoryPolicy& policy = {});
    QString resolvePath(const QString& path, const QString& workingDir) const;
    bool isAccessAllowed(const QString& path, const QString& workingDir, bool write = false) const;
    bool checkAndRequestPermission(const QString& path, const QString& workingDir) const;
    bool canRequestPermission(const QString& path, const QString& workingDir) const;
    void grantPermission(const QString& path, bool write = false);
    void revokePermission(const QString& path);
    bool hasPermission(const QString& path) const;
    void clearPermissions();
    void setWorkingDirectory(const QString& dir);

private:
    bool isWithinDirectory(const QString& path, const QString& directory) const;
    bool isSensitive(const QString& canonicalPath) const;
    ExternalDirectoryPolicy m_policy;
    QString m_workingDir;
    QSet<QString> m_readGrants;
    QSet<QString> m_writeGrants;
};

} // namespace sentinel::core
