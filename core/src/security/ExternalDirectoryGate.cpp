// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/security/ExternalDirectoryGate.h"
#include "sentinel/core/security/PathGuard.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace sentinel::core {

ExternalDirectoryGate::ExternalDirectoryGate(const ExternalDirectoryPolicy& policy)
    : m_policy(policy), m_workingDir(QDir::currentPath()) {}

QString ExternalDirectoryGate::resolvePath(const QString& path, const QString& workingDir) const {
    QString expanded = path.trimmed();
    if (expanded == QLatin1String("~"))
        expanded = QDir::homePath();
    else if (expanded.startsWith(QStringLiteral("~/")))
        expanded = QDir::home().absoluteFilePath(expanded.mid(2));
    else if (QFileInfo(expanded).isRelative())
        expanded = QDir(workingDir).absoluteFilePath(expanded);
    return PathGuard::canonicalPath(expanded);
}

bool ExternalDirectoryGate::isWithinDirectory(const QString& path, const QString& directory) const {
    return PathGuard::contains(directory, path);
}

bool ExternalDirectoryGate::isSensitive(const QString& path) const {
    const QString home = PathGuard::canonicalPath(QDir::homePath());
    for (const auto& name :
         {".ssh", ".gnupg", ".aws", ".kube", ".password-store", ".config", ".local", ".codex"}) {
        if (isWithinDirectory(path, QDir(home).filePath(QString::fromLatin1(name))))
            return true;
    }
    const QString file = QFileInfo(path).fileName();
    return file == QLatin1String(".env") || file.startsWith(QStringLiteral(".env."));
}

bool ExternalDirectoryGate::canRequestPermission(const QString& path,
                                                 const QString& workingDir) const {
    if (!m_policy.enabled || !m_policy.allowExplicitGrant)
        return false;
    const QString canonical = resolvePath(path, workingDir);
    const QString home = PathGuard::canonicalPath(QDir::homePath());
    if (canonical.isEmpty() || canonical == home || !isWithinDirectory(canonical, home) ||
        isSensitive(canonical))
        return false;
    const QList<QStandardPaths::StandardLocation> locations{
        QStandardPaths::DesktopLocation, QStandardPaths::DocumentsLocation,
        QStandardPaths::DownloadLocation, QStandardPaths::PicturesLocation};
    for (const auto location : locations) {
        const QString directory = QStandardPaths::writableLocation(location);
        if (!directory.isEmpty() && isWithinDirectory(canonical, directory))
            return true;
    }
    return false;
}

bool ExternalDirectoryGate::isAccessAllowed(const QString& path, const QString& workingDir,
                                            bool write) const {
    const QString canonical = resolvePath(path, workingDir);
    if (!isPathSafe(canonical, workingDir))
        return false;
    if (isWithinDirectory(canonical, workingDir))
        return true;
    if (!canRequestPermission(canonical, workingDir))
        return false;
    return m_authorizationCheck && m_authorizationCheck(canonical, write, m_sessionId);
}

bool ExternalDirectoryGate::isPathSafe(const QString& path, const QString& workingDir) const {
    const QString canonical = resolvePath(path, workingDir);
    return !canonical.isEmpty() && !isSensitive(canonical) &&
           (isWithinDirectory(canonical, workingDir) ||
            canRequestPermission(canonical, workingDir));
}

void ExternalDirectoryGate::setWorkingDirectory(const QString& dir) {
    m_workingDir = dir;
}

} // namespace sentinel::core
