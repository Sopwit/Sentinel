// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/plugin/IPluginContext.h"
#include "sentinel/core/plugin/PluginPermissions.h"
#include <QString>
#include <QJsonObject>
#include <functional>

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

private:
    QString m_pluginId;
    QString m_coreVersion;
    QString m_dataDir;
    PluginPermissions m_permissions;
    QJsonObject m_config;
    LoggerCallback m_logger;
};

} // namespace sentinel::core::plugin
