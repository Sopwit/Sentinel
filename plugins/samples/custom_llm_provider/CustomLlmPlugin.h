// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include "SentinelPluginSdk.h"

namespace sentinel::samples {

class CustomLlmPlugin : public QObject, public sentinel::core::plugin::ISentinelPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ISentinelPlugin_iid FILE "plugin.json")
    Q_INTERFACES(sentinel::core::plugin::ISentinelPlugin)

public:
    explicit CustomLlmPlugin(QObject* parent = nullptr);
    ~CustomLlmPlugin() override = default;

    QString pluginId() const override;
    QString displayName() const override;
    QString vendor() const override;
    QString version() const override;
    QString requiredCoreVersion() const override;

    bool initialize(std::shared_ptr<sentinel::core::plugin::IPluginContext> context) override;
    bool start() override;
    void stop() override;
    void shutdown() override;

    sentinel::core::plugin::PluginState state() const override;
    QJsonObject defaultConfig() const override;
    void configure(const QJsonObject& config) override;

private:
    std::shared_ptr<sentinel::core::plugin::IPluginContext> m_context;
    sentinel::core::plugin::PluginState m_state{sentinel::core::plugin::PluginState::Unloaded};
    QJsonObject m_config;
};

} // namespace sentinel::samples
