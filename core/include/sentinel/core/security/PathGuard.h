#pragma once

#include <QDir>
#include <QString>

namespace sentinel::core {

class PathGuard final {
public:
    static bool contains(const QString& root, const QString& candidate) {
        const QString rootPath = QDir(root).absolutePath();
        const QString candidatePath = QDir(candidate).absolutePath();
        return candidatePath == rootPath || candidatePath.startsWith(rootPath + QDir::separator());
    }

    static QString safePath(const QString& root, const QString& candidate) {
        return contains(root, candidate) ? QDir(candidate).absolutePath() : QString();
    }
};

} // namespace sentinel::core
