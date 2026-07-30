#include "sentinel/core/plugin/PluginContext.h"
#include <QDebug>

namespace sentinel::core::plugin {

PluginContext::PluginContext(QString pluginId,
                             QString coreVersion,
                             QString dataDir,
                             PluginPermissions permissions,
                             QJsonObject config,
                             LoggerCallback logger)
    : m_pluginId(std::move(pluginId))
    , m_coreVersion(std::move(coreVersion))
    , m_dataDir(std::move(dataDir))
    , m_permissions(std::move(permissions))
    , m_config(std::move(config))
    , m_logger(std::move(logger))
{
}

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

} // namespace sentinel::core::plugin
