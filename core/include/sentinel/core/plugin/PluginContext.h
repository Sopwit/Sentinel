// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/plugin/IPluginContext.h"
#include "sentinel/core/plugin/PluginPermissions.h"
#include "sentinel/core/plugin/PluginManifest.h"
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <functional>

namespace sentinel::core {
class IToolRegistry;
class IMemoryStore;
class IProviderCatalog;
} // namespace sentinel::core

namespace sentinel::core::plugin {

class PluginContext : public IPluginContext {
public:
    using LoggerCallback = std::function<void(const QString& level, const QString& message)>;

    PluginContext(QString pluginId, QString coreVersion, QString dataDir,
                  PluginPermissions permissions, QJsonObject config = {},
                  LoggerCallback logger = nullptr);

    QString coreVersion() const override;
    QString pluginDataDir() const override;
    bool hasPermission(const QString& permission) const override;
    void logMessage(const QString& level, const QString& message) override;
    QJsonObject pluginConfig() const override;
    std::optional<QString> credential(const QString& credentialId) const override;
    void setCredentialDeclarations(QList<PluginCredentialDeclaration> declarations);

    // Core service accessors
    bool registerTool(ToolDescriptor descriptor, std::shared_ptr<IToolHandler> handler) override;


    // Setters for core services (called by PluginManager during initialization)
    void setToolRegistry(IToolRegistry* registry);
    using ToolRegistrar = std::function<bool(ToolDescriptor, std::shared_ptr<IToolHandler>)>;
    void setToolRegistrar(ToolRegistrar registrar);
    void setMemoryStore(IMemoryStore* store);
    void setProviderCatalog(IProviderCatalog* catalog);
    void setPermissionCheck(std::function<bool(const QString&)> check);

private:
    QString m_pluginId;
    QString m_coreVersion;
    QString m_dataDir;
    PluginPermissions m_permissions;
    QJsonObject m_config;
    QList<PluginCredentialDeclaration> m_credentials;
    LoggerCallback m_logger;

    // Core service pointers (non-owning)
    IToolRegistry* m_toolRegistry{nullptr};
    ToolRegistrar m_toolRegistrar;
    IMemoryStore* m_memoryStore{nullptr};
    IProviderCatalog* m_providerCatalog{nullptr};
    std::function<bool(const QString&)> m_permissionCheck;

};

} // namespace sentinel::core::plugin
