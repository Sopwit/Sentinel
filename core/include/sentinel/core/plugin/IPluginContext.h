// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core::plugin {

class IPluginContext {
public:
    virtual ~IPluginContext() = default;

    virtual QString coreVersion() const = 0;
    virtual QString pluginDataDir() const = 0;
    virtual bool hasPermission(const QString& permission) const = 0;
    virtual void logMessage(const QString& level, const QString& message) = 0;
    virtual QJsonObject pluginConfig() const = 0;
};

} // namespace sentinel::core::plugin
