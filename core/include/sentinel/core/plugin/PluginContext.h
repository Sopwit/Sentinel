// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/plugin/IPluginContext.h"
#include "sentinel/core/plugin/PluginPermissions.h"
#include <QString>
#include <QJsonObject>
#include <QMap>
#include <functional>

namespace sentinel::core {
class IToolRegistry;
class IMemoryStore;
class IProviderCatalog;
}

namespace sentinel::core::plugin {

class PluginContext : public IPluginContext {
public:
    using LoggerCallback = std::function<void(const QString& level, const QString& message)>;

    PluginContext(QString pluginId,
                  QString coreVersion,
                  QString dataDir,
                  PluginPermissions permissions,
                  QJsonObject config = {},
                  LoggerCallback logger = nullptr);

    QString coreVersion() const override;
    QString pluginDataDir() const override;
    bool hasPermission(const QString& permission) const override;
    void logMessage(const QString& level, const QString& message) override;
    QJsonObject pluginConfig() const override;

    // Core service accessors
    IToolRegistry* toolRegistry() const override;
    IMemoryStore* memoryStore() const override;
    IProviderCatalog* providerCatalog() const override;

    // Service registration for plugin-to-plugin discovery
    void registerService(const QString& serviceName, void* servicePtr) override;
    void* lookupService(const QString& serviceName) const override;

    // Setters for core services (called by PluginManager during initialization)
    void setToolRegistry(IToolRegistry* registry);
    void setMemoryStore(IMemoryStore* store);
    void setProviderCatalog(IProviderCatalog* catalog);

private:
    QString m_pluginId;
    QString m_coreVersion;
    QString m_dataDir;
    PluginPermissions m_permissions;
    QJsonObject m_config;
    LoggerCallback m_logger;

    // Core service pointers (non-owning)
    IToolRegistry* m_toolRegistry{nullptr};
    IMemoryStore* m_memoryStore{nullptr};
    IProviderCatalog* m_providerCatalog{nullptr};

    // Plugin-to-plugin service registry (shared across all contexts)
    static QMap<QString, void*> s_serviceRegistry;
};

} // namespace sentinel::core::plugin
