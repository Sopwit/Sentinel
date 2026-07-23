#include "sentinel/core/StandardPathProvider.h"

#include <QDir>
#include <QStandardPaths>

namespace sentinel::core {

namespace {

QString writableDirectoryOrFallback(QStandardPaths::StandardLocation location) {
    const auto path = QStandardPaths::writableLocation(location);
    return path.isEmpty() ? QDir::currentPath() : path;
}

QString sentinelDocumentsPath() {
    return writableDirectoryOrFallback(QStandardPaths::DocumentsLocation) +
           QStringLiteral("/Sentinel");
}

} // namespace

QString StandardPathProvider::settingsFilePath() const {
    return writableDirectoryOrFallback(QStandardPaths::AppConfigLocation) +
           QStringLiteral("/settings.json");
}

QString StandardPathProvider::memoryDatabasePath() const {
    return sentinelDocumentsPath() +
           QStringLiteral("/memory.sqlite3");
}

QString StandardPathProvider::chatHistoryDatabasePath() const {
    return sentinelDocumentsPath() +
           QStringLiteral("/chat_history.sqlite3");
}

QString StandardPathProvider::conversationDatabasePath() const {
    return sentinelDocumentsPath() +
           QStringLiteral("/conversations.sqlite3");
}

QString StandardPathProvider::conversationExportDirectoryPath() const {
    return sentinelDocumentsPath() +
           QStringLiteral("/exports");
}

QString StandardPathProvider::localRagDatabasePath() const {
    return sentinelDocumentsPath() +
           QStringLiteral("/local_rag.sqlite3");
}

} // namespace sentinel::core
