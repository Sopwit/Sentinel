#include "sentinel/core/StandardPathProvider.h"

#include <QDir>
#include <QStandardPaths>

namespace sentinel::core {

namespace {

QString writableDirectoryOrFallback(QStandardPaths::StandardLocation location) {
    const auto path = QStandardPaths::writableLocation(location);
    return path.isEmpty() ? QDir::currentPath() : path;
}

} // namespace

QString StandardPathProvider::settingsFilePath() const {
    return writableDirectoryOrFallback(QStandardPaths::AppConfigLocation) +
           QStringLiteral("/settings.json");
}

QString StandardPathProvider::memoryDatabasePath() const {
    return writableDirectoryOrFallback(QStandardPaths::AppDataLocation) +
           QStringLiteral("/memory.sqlite3");
}

QString StandardPathProvider::chatHistoryDatabasePath() const {
    return writableDirectoryOrFallback(QStandardPaths::AppDataLocation) +
           QStringLiteral("/chat_history.sqlite3");
}

} // namespace sentinel::core
