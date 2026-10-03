// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QDir>
#include <QFileInfo>
#include <QString>

// Explicit diagnostic input only. Never infer an authorization root from cwd or HOME.
inline QString validatedProviderCertificationRoot(const QString& configured) {
    if (configured.trimmed().isEmpty() || !QDir::isAbsolutePath(configured))
        return {};
    const QFileInfo info(configured);
    if (!info.exists() || !info.isDir() || !info.isReadable() || info.isSymLink())
        return {};
    const QString canonical = info.canonicalFilePath();
    if (canonical.isEmpty() || QDir(canonical).isRoot() ||
        canonical == QFileInfo(QDir::homePath()).canonicalFilePath() ||
        canonical != QDir::cleanPath(info.absoluteFilePath()))
        return {};
    return canonical;
}
