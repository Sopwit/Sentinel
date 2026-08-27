// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/plugin/IPluginContext.h"
#include "sentinel/core/plugin/PluginState.h"
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <memory>

namespace sentinel::core::plugin {

class ISentinelPlugin {
public:
    virtual ~ISentinelPlugin() = default;

    // Metadata
    virtual QString pluginId() const = 0;
    virtual QString displayName() const = 0;
    virtual QString vendor() const = 0;
    virtual QString version() const = 0;
    virtual QString requiredCoreVersion() const = 0;

    // Lifecycle Methods
    virtual bool initialize(std::shared_ptr<IPluginContext> context) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void shutdown() = 0;

    // State & Config
    virtual PluginState state() const = 0;
    virtual QJsonObject defaultConfig() const = 0;
    virtual void configure(const QJsonObject& config) = 0;
};

} // namespace sentinel::core::plugin

#define ISentinelPlugin_iid "dev.sentinel.ISentinelPlugin/1.0"
Q_DECLARE_INTERFACE(sentinel::core::plugin::ISentinelPlugin, ISentinelPlugin_iid)
