// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/plugin/PluginHotReloader.h"
#include "sentinel/core/plugin/ISentinelPlugin.h"
#include "sentinel/core/plugin/IPluginContext.h"
#include "sentinel/core/plugin/PluginManifest.h"
#include "sentinel/core/plugin/PluginSandbox.h"
#include "sentinel/core/plugin/PluginState.h"
#include "sentinel/core/plugin/PluginHostSession.h"
#include <QJsonArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPluginLoader>
#include <QString>
#include <QSet>
#include <QHash>
#include <memory>

namespace sentinel::core {
class IToolRegistry;
} // namespace sentinel::core

namespace sentinel::core::plugin {

struct PluginDescriptor {
    PluginManifest manifest;
    QString pluginFilePath;
    PluginState state{PluginState::Unloaded};
    // Legacy inspection fields remain empty for isolated native plugins.
    ISentinelPlugin* instance{nullptr};
    std::shared_ptr<QPluginLoader> loader;
    std::shared_ptr<IPluginContext> context;
    std::shared_ptr<PluginHostSession> host;
    QJsonArray remoteTools;
    QString failureCategory;
    QString errorString;
};

struct PluginCredentialState {
    QString pluginId;
    PluginCredentialDeclaration declaration;
    bool configured = false;
    bool storeAvailable = false;
};

class PluginManager : public QObject {
    Q_OBJECT
public:
    explicit PluginManager(QString coreVersion = QStringLiteral("1.0.0"),
                           QString pluginStorageDir = QString(), QObject* parent = nullptr);
    ~PluginManager() override;

    void setPluginStorageDir(const QString& dir);
    QString pluginStorageDir() const;

    PluginSandbox& sandbox();
    const PluginSandbox& sandbox() const;

    void setToolRegistry(IToolRegistry* registry);

    // Discovery & Lifecycle Operations
    int discoverPlugins(const QString& searchDir);
    bool loadPlugin(const QString& pluginId);
    bool initializePlugin(const QString& pluginId);
    bool startPlugin(const QString& pluginId);
    bool stopPlugin(const QString& pluginId);
    bool unloadPlugin(const QString& pluginId);
    bool setEnabled(const QString& pluginId, bool enabled);

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
    // Deliberately returns null: native plugins never enter the Sentinel process.
    ISentinelPlugin* pluginInstance(const QString& pluginId) const;
    QList<PluginCredentialState> credentialStates(const QString& pluginId) const;
    bool setCredential(const QString& pluginId, const QString& credentialId,
                       const QString& value);
    bool clearCredential(const QString& pluginId, const QString& credentialId);
    bool isModuleResident(const QString& pluginId) const;

signals:
    void pluginDiscovered(const QString& pluginId);
    void pluginLoaded(const QString& pluginId);
    void pluginUnloaded(const QString& pluginId);
    void pluginRemoved(const QString& pluginId);
    void pluginStateChanged(const QString& pluginId, PluginState newState);
    void pluginError(const QString& pluginId, const QString& error);
    void pluginReloaded(const QString& pluginId);
    void pluginReloadFailed(const QString& pluginId, const QString& error);

private slots:
    void onHotReloadRequested(const QString& pluginId);

private:
    bool registerRemoteTools(const QString& pluginId);
    void finishReload(const QString& pluginId, PluginState previousState, int attempts);
    void updateState(PluginDescriptor& desc, PluginState newState);

    QString m_coreVersion;
    QString m_pluginStorageDir;
    std::shared_ptr<PluginSandbox> m_sandbox{std::make_shared<PluginSandbox>()};
    QMap<QString, PluginDescriptor> m_plugins;
    QList<QString> m_orderedIds;
    QHash<QString, QSet<QString>> m_reloadCredentialIds;

    // Core service pointers (non-owning)
    IToolRegistry* m_toolRegistry{nullptr};

    // Hot-reload support
    std::unique_ptr<PluginHotReloader> m_hotReloader;
    bool m_hotReloadEnabled{false};
};

} // namespace sentinel::core::plugin
