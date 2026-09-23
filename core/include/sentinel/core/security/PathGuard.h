#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace sentinel::core {

class PathGuard final {
public:
    static bool contains(const QString& root, const QString& candidate) {
        const QString rootPath = canonicalPath(root);
        const QString candidatePath = canonicalPath(candidate);
        if (rootPath.isEmpty() || candidatePath.isEmpty()) {
            return false;
        }
        if (candidatePath == rootPath) {
            return true;
        }
        const QString prefix = rootPath.endsWith(QDir::separator())
                                   ? rootPath
                                   : rootPath + QDir::separator();
        return candidatePath.startsWith(prefix);
    }

    static QString safePath(const QString& root, const QString& candidate) {
        return contains(root, candidate) ? canonicalPath(candidate) : QString();
    }

    static QString canonicalPath(const QString& path) {
        if (path.trimmed().isEmpty()) {
            return {};
        }

        const QFileInfo direct(path);
        if (direct.exists()) {
            const QString canonical = direct.canonicalFilePath();
            if (!canonical.isEmpty()) {
                return QDir::cleanPath(canonical);
            }
        }

        const QString absolute = QDir::cleanPath(direct.absoluteFilePath());
        if (absolute.isEmpty()) {
            return {};
        }

        QStringList missingParts;
        QString existing = absolute;
        while (!existing.isEmpty() && !QFileInfo::exists(existing)) {
            const QFileInfo current(existing);
            const QString parent = current.absolutePath();
            if (parent == existing) {
                break;
            }
            missingParts.prepend(current.fileName());
            existing = parent;
        }

        const QFileInfo baseInfo(existing);
        const QString base =
            baseInfo.exists() && !baseInfo.canonicalFilePath().isEmpty()
                ? baseInfo.canonicalFilePath()
                : existing;
        if (base.isEmpty()) {
            return {};
        }
        if (missingParts.isEmpty()) {
            return QDir::cleanPath(base);
        }
        return QDir::cleanPath(base + QLatin1Char('/') + missingParts.join(QLatin1Char('/')));
    }
};

} // namespace sentinel::core
