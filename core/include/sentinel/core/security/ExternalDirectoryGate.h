// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QSet>
#include <QString>
#include <QStringList>
#include <functional>

namespace sentinel::core {

struct ExternalDirectoryPolicy {
    bool enabled{true};
    bool allowExplicitGrant{true};
    int maxDepth{3};
};

// Validates concrete external filesystem resources and delegates grant checks.
class ExternalDirectoryGate {
public:
    explicit ExternalDirectoryGate(const ExternalDirectoryPolicy& policy = {});
    QString resolvePath(const QString& path, const QString& workingDir) const;
    bool isAccessAllowed(const QString& path, const QString& workingDir, bool write = false) const;
    bool isPathSafe(const QString& path, const QString& workingDir) const;
    bool canRequestPermission(const QString& path, const QString& workingDir) const;
    void setWorkingDirectory(const QString& dir);
    void setAuthorizationCheck(std::function<bool(const QString&, bool, const QString&)> check) {
        m_authorizationCheck = std::move(check);
    }
    void setSecuritySessionId(QString sessionId) { m_sessionId = std::move(sessionId); }

private:
    bool isWithinDirectory(const QString& path, const QString& directory) const;
    bool isSensitive(const QString& canonicalPath) const;
    ExternalDirectoryPolicy m_policy;
    QString m_workingDir;
    std::function<bool(const QString&, bool, const QString&)> m_authorizationCheck;
    QString m_sessionId;
};

} // namespace sentinel::core
