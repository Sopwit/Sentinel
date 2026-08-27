// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginContext.h"
#include <QDebug>

namespace sentinel::core::plugin {

QMap<QString, void*> PluginContext::s_serviceRegistry;

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
    return m_permissions.has(permission);
}

void PluginContext::logMessage(const QString& level, const QString& message) {
    if (m_logger) {
        m_logger(level, message);
    } else {
        qDebug() << QStringLiteral("[%1][%2] %3").arg(m_pluginId, level, message);
    }
}

QJsonObject PluginContext::pluginConfig() const {
    return m_config;
}

IToolRegistry* PluginContext::toolRegistry() const {
    return m_toolRegistry;
}

IMemoryStore* PluginContext::memoryStore() const {
    return m_memoryStore;
}

IProviderCatalog* PluginContext::providerCatalog() const {
    return m_providerCatalog;
}

void PluginContext::registerService(const QString& serviceName, void* servicePtr) {
    if (s_serviceRegistry.contains(serviceName)) {
        qWarning()
            << QStringLiteral("Plugin '%1' overwriting service '%2'").arg(m_pluginId, serviceName);
    }
    s_serviceRegistry[serviceName] = servicePtr;
    qDebug() << QStringLiteral("Plugin '%1' registered service '%2'").arg(m_pluginId, serviceName);
}

void* PluginContext::lookupService(const QString& serviceName) const {
    return s_serviceRegistry.value(serviceName, nullptr);
}

void PluginContext::setToolRegistry(IToolRegistry* registry) {
    m_toolRegistry = registry;
}

void PluginContext::setMemoryStore(IMemoryStore* store) {
    m_memoryStore = store;
}

void PluginContext::setProviderCatalog(IProviderCatalog* catalog) {
    m_providerCatalog = catalog;
}

} // namespace sentinel::core::plugin
