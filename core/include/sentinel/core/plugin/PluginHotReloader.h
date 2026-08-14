// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QFileSystemWatcher>
#include <QMap>
#include <QTimer>
#include <memory>

namespace sentinel::core::plugin {

class PluginManager;

struct HotReloadConfig {
    bool enabled{true};
    int debounceMs{1000};
    QStringList watchedDirs;
};

class PluginHotReloader : public QObject {
    Q_OBJECT
public:
    explicit PluginHotReloader(PluginManager* manager, QObject* parent = nullptr);
    ~PluginHotReloader() override;

    void setConfig(const HotReloadConfig& config);
    HotReloadConfig config() const;

    void startWatching();
    void stopWatching();
    bool isWatching() const;

    void addWatchDir(const QString& dir);
    void removeWatchDir(const QString& dir);

signals:
    void pluginChanged(const QString& pluginId, const QString& changeType);
    void reloadRequested(const QString& pluginId);

private slots:
    void onDirectoryChanged(const QString& path);
    void onFileChanged(const QString& path);
    void processPendingChanges();

private:
    void watchDirectory(const QString& dir);
    void unwatchDirectory(const QString& dir);
    QString extractPluginIdFromPath(const QString& path) const;

    PluginManager* m_manager;
    HotReloadConfig m_config;
    QFileSystemWatcher m_watcher;
    QTimer m_debounceTimer;
    QMap<QString, QString> m_pendingChanges;
    bool m_isWatching{false};
};

} // namespace sentinel::core::plugin
