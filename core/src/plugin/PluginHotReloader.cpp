// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginHotReloader.h"
#include "sentinel/core/plugin/PluginManager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace sentinel::core::plugin {

PluginHotReloader::PluginHotReloader(PluginManager* manager, QObject* parent)
    : QObject(parent)
    , m_manager(manager)
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &PluginHotReloader::onDirectoryChanged);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &PluginHotReloader::onFileChanged);

    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(m_config.debounceMs);
    connect(&m_debounceTimer, &QTimer::timeout, this, &PluginHotReloader::processPendingChanges);
}

PluginHotReloader::~PluginHotReloader() {
    stopWatching();
}

void PluginHotReloader::setConfig(const HotReloadConfig& config) {
    bool wasWatching = m_isWatching;
    if (wasWatching) {
        stopWatching();
    }
    m_config = config;
    if (wasWatching) {
        startWatching();
    }
}

HotReloadConfig PluginHotReloader::config() const {
    return m_config;
}

void PluginHotReloader::startWatching() {
    if (m_isWatching) {
        return;
    }

    for (const QString& dir : m_config.watchedDirs) {
        watchDirectory(dir);
    }

    m_isWatching = true;
    qDebug() << "PluginHotReloader: Started watching" << m_config.watchedDirs.size() << "directories";
}

void PluginHotReloader::stopWatching() {
    if (!m_isWatching) {
        return;
    }

    const QStringList dirs = m_watcher.directories();
    if (!dirs.isEmpty()) {
        m_watcher.removePaths(dirs);
    }

    const QStringList files = m_watcher.files();
    if (!files.isEmpty()) {
        m_watcher.removePaths(files);
    }

    m_debounceTimer.stop();
    m_pendingChanges.clear();
    m_isWatching = false;

    qDebug() << "PluginHotReloader: Stopped watching";
}

bool PluginHotReloader::isWatching() const {
    return m_isWatching;
}

void PluginHotReloader::addWatchDir(const QString& dir) {
    if (!m_config.watchedDirs.contains(dir)) {
        m_config.watchedDirs.append(dir);
        if (m_isWatching) {
            watchDirectory(dir);
        }
    }
}

void PluginHotReloader::removeWatchDir(const QString& dir) {
    m_config.watchedDirs.removeAll(dir);
    if (m_isWatching) {
        unwatchDirectory(dir);
    }
}

void PluginHotReloader::onDirectoryChanged(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) {
        return;
    }

    // Check for new or removed plugin.json files
    QStringList pluginFiles = dir.entryList(QStringList() << "plugin.json", QDir::Files);
    for (const QString& file : pluginFiles) {
        QString fullPath = dir.filePath(file);
        QString pluginId = extractPluginIdFromPath(fullPath);
        if (!pluginId.isEmpty()) {
            m_pendingChanges[pluginId] = "modified";
        }
    }

    // Check for new subdirectories that might be plugins
    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& subdir : subdirs) {
        QString subPath = dir.filePath(subdir);
        QString pluginJsonPath = subPath + "/plugin.json";
        if (QFile::exists(pluginJsonPath)) {
            m_pendingChanges[subdir] = "added";
        }
    }

    if (!m_pendingChanges.isEmpty()) {
        m_debounceTimer.start();
    }
}

void PluginHotReloader::onFileChanged(const QString& path) {
    QString pluginId = extractPluginIdFromPath(path);
    if (!pluginId.isEmpty()) {
        m_pendingChanges[pluginId] = "modified";
        m_debounceTimer.start();
    }

    // Re-add the file to watcher if it still exists (QFileSystemWatcher removes it after change)
    if (QFile::exists(path)) {
        m_watcher.addPath(path);
    }
}

void PluginHotReloader::processPendingChanges() {
    if (m_pendingChanges.isEmpty()) {
        return;
    }

    QMap<QString, QString> changes = m_pendingChanges;
    m_pendingChanges.clear();

    for (auto it = changes.begin(); it != changes.end(); ++it) {
        const QString& pluginId = it.key();
        const QString& changeType = it.value();

        qDebug() << QStringLiteral("PluginHotReloader: Processing %1 change for plugin '%2'").arg(changeType, pluginId);

        if (changeType == "added") {
            // New plugin discovered - need full discovery + load
            emit pluginChanged(pluginId, "added");
            emit reloadRequested(pluginId);
        } else if (changeType == "modified") {
            // Existing plugin modified - hot reload
            if (m_manager->isLoaded(pluginId)) {
                emit pluginChanged(pluginId, "modified");
                emit reloadRequested(pluginId);
            }
        } else if (changeType == "removed") {
            // Plugin removed
            emit pluginChanged(pluginId, "removed");
        }
    }
}

QString PluginHotReloader::extractPluginIdFromPath(const QString& path) const {
    // Extract plugin ID from path like ".../plugins/my-plugin/plugin.json"
    QFileInfo info(path);
    QDir parentDir = info.dir();

    // If the file is plugin.json, the parent directory name is likely the plugin ID
    if (info.fileName() == "plugin.json") {
        return parentDir.dirName();
    }

    // If it's a .so/.dylib/.dll file, try to find the plugin.json in the same directory
    if (QLibrary::isLibrary(path)) {
        QString pluginJsonPath = parentDir.filePath("plugin.json");
        if (QFile::exists(pluginJsonPath)) {
            return parentDir.dirName();
        }
    }

    return {};
}

void PluginHotReloader::watchDirectory(const QString& dir) {
    if (QDir(dir).exists()) {
        m_watcher.addPath(dir);
        qDebug() << QStringLiteral("PluginHotReloader: Watching directory '%1'").arg(dir);

        // Also watch existing plugin.json files in this directory
        QDir d(dir);
        QStringList pluginFiles = d.entryList(QStringList() << "plugin.json", QDir::Files);
        for (const QString& file : pluginFiles) {
            QString fullPath = d.filePath(file);
            m_watcher.addPath(fullPath);
        }

        // Watch subdirectories
        QStringList subdirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& subdir : subdirs) {
            QString subPath = d.filePath(subdir);
            m_watcher.addPath(subPath);

            // Watch plugin.json in subdirectory
            QString pluginJsonPath = subPath + "/plugin.json";
            if (QFile::exists(pluginJsonPath)) {
                m_watcher.addPath(pluginJsonPath);
            }
        }
    }
}

void PluginHotReloader::unwatchDirectory(const QString& dir) {
    m_watcher.removePath(dir);
}

} // namespace sentinel::core::plugin
