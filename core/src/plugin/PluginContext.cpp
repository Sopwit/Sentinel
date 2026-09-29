// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginContext.h"
#include "sentinel/core/plugin/PluginPermissions.h"
#include <QDebug>

namespace sentinel::core::plugin {


PluginContext::PluginContext(QString pluginId, QString coreVersion, QString dataDir,
                             PluginPermissions permissions, QJsonObject config,
                             LoggerCallback logger)
    : m_pluginId(std::move(pluginId)), m_coreVersion(std::move(coreVersion)),
      m_dataDir(std::move(dataDir)), m_permissions(std::move(permissions)),
      m_config(std::move(config)), m_logger(std::move(logger)) {}

QString PluginContext::coreVersion() const {
    return m_coreVersion;
}

QString PluginContext::pluginDataDir() const {
    return m_dataDir;
}

bool PluginContext::hasPermission(const QString& permission) const {
    return m_permissions.has(permission) &&
           (!m_permissionCheck || m_permissionCheck(permission));
}

void PluginContext::logMessage(const QString& level, const QString& message) {
    QString safe = message.left(4096);
    for (const auto& declaration : m_credentials) {
        const auto secret = credential(declaration.id);
        if (secret && !secret->isEmpty()) safe.replace(*secret, QStringLiteral("[credential redacted]"));
    }
    if (m_logger) {
        m_logger(level, safe);
    } else {
        qDebug() << QStringLiteral("[%1][%2] %3").arg(m_pluginId, level, safe);
    }
}

QJsonObject PluginContext::pluginConfig() const {
    return m_config;
}

void PluginContext::setCredentialDeclarations(QList<PluginCredentialDeclaration> declarations) {
    m_credentials = std::move(declarations);
}

std::optional<QString> PluginContext::credential(const QString& credentialId) const {
    Q_UNUSED(credentialId)
    return std::nullopt;
}

bool PluginContext::registerTool(ToolDescriptor descriptor, std::shared_ptr<IToolHandler> handler) {
    return m_toolRegistrar && m_toolRegistrar(std::move(descriptor), std::move(handler));
}

void PluginContext::setToolRegistry(IToolRegistry* registry) {
    m_toolRegistry = registry;
}

void PluginContext::setToolRegistrar(ToolRegistrar registrar) {
    m_toolRegistrar = std::move(registrar);
}

void PluginContext::setMemoryStore(IMemoryStore* store) {
    m_memoryStore = store;
}

void PluginContext::setProviderCatalog(IProviderCatalog* catalog) {
    m_providerCatalog = catalog;
}

void PluginContext::setPermissionCheck(std::function<bool(const QString&)> check) {
    m_permissionCheck = std::move(check);
}

} // namespace sentinel::core::plugin
