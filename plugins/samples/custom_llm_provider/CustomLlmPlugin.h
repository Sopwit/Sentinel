// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include "SentinelPluginSdk.h"

namespace sentinel::samples {

class CustomLlmPlugin : public QObject, public sentinel::plugin_sdk::ISentinelPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ISentinelPlugin_iid FILE "plugin.json")
    Q_INTERFACES(sentinel::plugin_sdk::ISentinelPlugin)

public:
    explicit CustomLlmPlugin(QObject* parent = nullptr);
    ~CustomLlmPlugin() override = default;

    QString pluginId() const override;
    QString displayName() const override;
    QString vendor() const override;
    QString version() const override;
    QString requiredCoreVersion() const override;

    bool initialize(sentinel::plugin_sdk::IPluginContext* context) override;
    bool start() override;
    void stop() override;
    void shutdown() override;

    sentinel::plugin_sdk::PluginState state() const override;
    QJsonObject defaultConfig() const override;
    void configure(const QJsonObject& config) override;

private:
    sentinel::plugin_sdk::IPluginContext* m_context{nullptr};
    sentinel::plugin_sdk::PluginState m_state{sentinel::plugin_sdk::PluginState::Unloaded};
    QJsonObject m_config;
};

} // namespace sentinel::samples
