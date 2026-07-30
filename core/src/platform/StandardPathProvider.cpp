// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/platform/StandardPathProvider.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
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

QString portableDirectoryPath() {
    const QString appDir = QCoreApplication::applicationDirPath();
    if (appDir.isEmpty()) {
        return QDir::currentPath();
    }
    return appDir;
}

} // namespace

StandardPathProvider::StandardPathProvider(bool portableOverride)
    : m_portableOverride(portableOverride), m_hasPortableOverride(true) {}

bool StandardPathProvider::detectPortableMode() const {
    if (m_hasPortableOverride) {
        return m_portableOverride;
    }

    if (QCoreApplication::instance()) {
        const auto args = QCoreApplication::arguments();
        if (args.contains(QStringLiteral("--portable"))) {
            return true;
        }
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    if (!appDir.isEmpty()) {
        const QString portableFile = QDir(appDir).filePath(QStringLiteral("portable.txt"));
        if (QFile::exists(portableFile)) {
            return true;
        }
    }

    return false;
}

bool StandardPathProvider::isPortable() const {
    return detectPortableMode();
}

void StandardPathProvider::setPortable(bool portable) {
    m_portableOverride = portable;
    m_hasPortableOverride = true;
}

QString StandardPathProvider::settingsFilePath() const {
    if (isPortable()) {
        return QDir(portableDirectoryPath()).filePath(QStringLiteral("settings.json"));
    }
    return QDir(writableDirectoryOrFallback(QStandardPaths::AppConfigLocation))
        .filePath(QStringLiteral("settings.json"));
}

QString StandardPathProvider::memoryDatabasePath() const {
    if (isPortable()) {
        return QDir(portableDirectoryPath()).filePath(QStringLiteral("memory.sqlite3"));
    }
    return QDir(sentinelDocumentsPath()).filePath(QStringLiteral("memory.sqlite3"));
}

QString StandardPathProvider::chatHistoryDatabasePath() const {
    if (isPortable()) {
        return QDir(portableDirectoryPath()).filePath(QStringLiteral("chat_history.sqlite3"));
    }
    return QDir(sentinelDocumentsPath()).filePath(QStringLiteral("chat_history.sqlite3"));
}

QString StandardPathProvider::conversationDatabasePath() const {
    if (isPortable()) {
        return QDir(portableDirectoryPath()).filePath(QStringLiteral("conversations.sqlite3"));
    }
    return QDir(sentinelDocumentsPath()).filePath(QStringLiteral("conversations.sqlite3"));
}

QString StandardPathProvider::conversationExportDirectoryPath() const {
    if (isPortable()) {
        return QDir(portableDirectoryPath()).filePath(QStringLiteral("exports"));
    }
    return QDir(sentinelDocumentsPath()).filePath(QStringLiteral("exports"));
}

QString StandardPathProvider::localRagDatabasePath() const {
    if (isPortable()) {
        return QDir(portableDirectoryPath()).filePath(QStringLiteral("local_rag.sqlite3"));
    }
    return QDir(sentinelDocumentsPath()).filePath(QStringLiteral("local_rag.sqlite3"));
}

QString StandardPathProvider::logDirectoryPath() const {
    if (isPortable()) {
        return QDir(portableDirectoryPath()).filePath(QStringLiteral("Logs"));
    }
    return QDir(writableDirectoryOrFallback(QStandardPaths::AppLocalDataLocation))
        .filePath(QStringLiteral("Logs"));
}

QString StandardPathProvider::crashDumpDirectoryPath() const {
    if (isPortable()) {
        return QDir(portableDirectoryPath()).filePath(QStringLiteral("Crashes"));
    }
    return QDir(writableDirectoryOrFallback(QStandardPaths::AppLocalDataLocation))
        .filePath(QStringLiteral("Crashes"));
}

} // namespace sentinel::core

