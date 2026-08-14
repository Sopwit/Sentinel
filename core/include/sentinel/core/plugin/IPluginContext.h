// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QVariantMap>
#include <functional>
#include <memory>

namespace sentinel::core {
class IToolRegistry;
class IMemoryStore;
class IProviderCatalog;
}

namespace sentinel::core::plugin {

class IPluginContext {
public:
    virtual ~IPluginContext() = default;

    virtual QString coreVersion() const = 0;
    virtual QString pluginDataDir() const = 0;
    virtual bool hasPermission(const QString& permission) const = 0;
    virtual void logMessage(const QString& level, const QString& message) = 0;
    virtual QJsonObject pluginConfig() const = 0;

    // Core service accessors (expanded API)
    virtual IToolRegistry* toolRegistry() const = 0;
    virtual IMemoryStore* memoryStore() const = 0;
    virtual IProviderCatalog* providerCatalog() const = 0;

    // Service registration for plugin-to-plugin discovery
    virtual void registerService(const QString& serviceName, void* servicePtr) = 0;
    virtual void* lookupService(const QString& serviceName) const = 0;
};

} // namespace sentinel::core::plugin
