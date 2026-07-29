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
    return QDir(writableDirectoryOrFallback(QStandardPaths::DocumentsLocation))
        .filePath(QStringLiteral("Sentinel"));
}

} // namespace

QString StandardPathProvider::settingsFilePath() const {
    return QDir(writableDirectoryOrFallback(QStandardPaths::AppConfigLocation))
        .filePath(QStringLiteral("settings.json"));
}

QString StandardPathProvider::memoryDatabasePath() const {
    return QDir(sentinelDocumentsPath()).filePath(QStringLiteral("memory.sqlite3"));
}

QString StandardPathProvider::chatHistoryDatabasePath() const {
    return QDir(sentinelDocumentsPath()).filePath(QStringLiteral("chat_history.sqlite3"));
}

QString StandardPathProvider::conversationDatabasePath() const {
    return QDir(sentinelDocumentsPath()).filePath(QStringLiteral("conversations.sqlite3"));
}

QString StandardPathProvider::conversationExportDirectoryPath() const {
    return QDir(sentinelDocumentsPath()).filePath(QStringLiteral("exports"));
}

QString StandardPathProvider::localRagDatabasePath() const {
    return QDir(sentinelDocumentsPath()).filePath(QStringLiteral("local_rag.sqlite3"));
}

QString StandardPathProvider::logDirectoryPath() const {
    return QDir(writableDirectoryOrFallback(QStandardPaths::AppLocalDataLocation))
        .filePath(QStringLiteral("Logs"));
}

QString StandardPathProvider::crashDumpDirectoryPath() const {
    return QDir(writableDirectoryOrFallback(QStandardPaths::AppLocalDataLocation))
        .filePath(QStringLiteral("Crashes"));
}

} // namespace sentinel::core
