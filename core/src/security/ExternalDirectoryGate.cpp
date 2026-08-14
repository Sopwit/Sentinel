// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/security/ExternalDirectoryGate.h"
#include <QDir>
#include <QRegularExpression>

namespace sentinel::core {

ExternalDirectoryGate::ExternalDirectoryGate(const ExternalDirectoryPolicy& policy)
    : m_policy(policy), m_workingDir(QDir::currentPath()) {}

bool ExternalDirectoryGate::isAccessAllowed(const QString& path, const QString& workingDir) const {
    if (!m_policy.enabled) return true;

    QString resolved = resolvePath(path);
    if (isWithinDirectory(resolved, workingDir)) return true;
    if (m_grantedPermissions.contains(resolved)) return true;

    return false;
}

bool ExternalDirectoryGate::checkAndRequestPermission(const QString& path, const QString& workingDir) const {
    if (isAccessAllowed(path, workingDir)) return true;
    if (!m_policy.allowExplicitGrant) return false;
    return false;
}

void ExternalDirectoryGate::grantPermission(const QString& path) {
    m_grantedPermissions.insert(resolvePath(path));
}

void ExternalDirectoryGate::revokePermission(const QString& path) {
    m_grantedPermissions.remove(resolvePath(path));
}

bool ExternalDirectoryGate::hasPermission(const QString& path) const {
    return m_grantedPermissions.contains(resolvePath(path));
}

void ExternalDirectoryGate::setWorkingDirectory(const QString& dir) {
    m_workingDir = dir;
}

bool ExternalDirectoryGate::isWithinDirectory(const QString& path, const QString& directory) const {
    QString resolved = QDir(directory).absoluteFilePath(path);
    return resolved.startsWith(directory);
}

QString ExternalDirectoryGate::resolvePath(const QString& path) const {
    if (QDir::isAbsolutePath(path)) return path;
    return QDir(m_workingDir).filePath(path);
}

} // namespace sentinel::core
