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

QString StandardPathProvider::conversationDatabasePath() const {
    return writableDirectoryOrFallback(QStandardPaths::AppDataLocation) +
           QStringLiteral("/conversations.sqlite3");
}

QString StandardPathProvider::conversationExportDirectoryPath() const {
    return writableDirectoryOrFallback(QStandardPaths::AppDataLocation) +
           QStringLiteral("/exports");
}

QString StandardPathProvider::localRagDatabasePath() const {
    return writableDirectoryOrFallback(QStandardPaths::AppDataLocation) +
           QStringLiteral("/local_rag.sqlite3");
}

} // namespace sentinel::core
