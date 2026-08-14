// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QPluginLoader>
#include <memory>
#include "sentinel/core/plugin/ISentinelPlugin.h"
#include "sentinel/core/plugin/PluginManifest.h"
#include "sentinel/core/plugin/PluginSandbox.h"
#include "sentinel/core/plugin/PluginState.h"
#include "sentinel/core/plugin/PluginContext.h"
#include "sentinel/core/plugin/PluginHotReloader.h"

namespace sentinel::core {
class IToolRegistry;
class IMemoryStore;
class IProviderCatalog;
}

namespace sentinel::core::plugin {

struct PluginDescriptor {
    PluginManifest manifest;
    QString pluginFilePath;
    PluginState state{PluginState::Unloaded};
    ISentinelPlugin* instance{nullptr};
    std::shared_ptr<QPluginLoader> loader;
    std::shared_ptr<IPluginContext> context;
    QString errorString;
};

class PluginManager : public QObject {
    Q_OBJECT
public:
    explicit PluginManager(QString coreVersion = QStringLiteral("1.0.0"),
                           QString pluginStorageDir = QString(),
                           QObject* parent = nullptr);
    ~PluginManager() override;

    void setPluginStorageDir(const QString& dir);
    QString pluginStorageDir() const;

    PluginSandbox& sandbox();
    const PluginSandbox& sandbox() const;

    // Core service setters (for plugin context injection)
    void setToolRegistry(IToolRegistry* registry);
    void setMemoryStore(IMemoryStore* store);
    void setProviderCatalog(IProviderCatalog* catalog);

    // Discovery & Lifecycle Operations
    int discoverPlugins(const QString& searchDir);
    bool loadPlugin(const QString& pluginId);
    bool initializePlugin(const QString& pluginId);
    bool startPlugin(const QString& pluginId);
    bool stopPlugin(const QString& pluginId);
    bool unloadPlugin(const QString& pluginId);

    // Hot-reload operations
    bool reloadPlugin(const QString& pluginId);
    void enableHotReload(bool enabled);
    bool isHotReloadEnabled() const;
    void setHotReloadConfig(const HotReloadConfig& config);
    HotReloadConfig hotReloadConfig() const;

    // Batch operations with dependency sorting
    bool initializeAll();
    bool startAll();
    bool stopAll();
    void unloadAll();

    // Query methods
    QList<QString> registeredPluginIds() const;
    bool isLoaded(const QString& pluginId) const;
    PluginState pluginState(const QString& pluginId) const;
    const PluginDescriptor* descriptor(const QString& pluginId) const;
    ISentinelPlugin* pluginInstance(const QString& pluginId) const;

signals:
    void pluginLoaded(const QString& pluginId);
    void pluginUnloaded(const QString& pluginId);
    void pluginStateChanged(const QString& pluginId, PluginState newState);
    void pluginError(const QString& pluginId, const QString& error);
    void pluginReloaded(const QString& pluginId);
    void pluginReloadFailed(const QString& pluginId, const QString& error);

private slots:
    void onHotReloadRequested(const QString& pluginId);

private:
    void updateState(PluginDescriptor& desc, PluginState newState);
    void injectCoreServices(PluginContext* context);

    QString m_coreVersion;
    QString m_pluginStorageDir;
    PluginSandbox m_sandbox;
    QMap<QString, PluginDescriptor> m_plugins;
    QList<QString> m_orderedIds;

    // Core service pointers (non-owning)
    IToolRegistry* m_toolRegistry{nullptr};
    IMemoryStore* m_memoryStore{nullptr};
    IProviderCatalog* m_providerCatalog{nullptr};

    // Hot-reload support
    std::unique_ptr<PluginHotReloader> m_hotReloader;
    bool m_hotReloadEnabled{false};
};

} // namespace sentinel::core::plugin
