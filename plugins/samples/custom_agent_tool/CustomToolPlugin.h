// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "SentinelPluginSdk.h"
#include <QObject>
#include <memory>

#ifndef SENTINEL_SAMPLE_PLUGIN_MANIFEST
#define SENTINEL_SAMPLE_PLUGIN_MANIFEST "plugin.json"
#endif

namespace sentinel::samples {

class CustomToolPlugin : public QObject, public sentinel::plugin_sdk::ISentinelPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ISentinelPlugin_iid FILE SENTINEL_SAMPLE_PLUGIN_MANIFEST)
    Q_INTERFACES(sentinel::plugin_sdk::ISentinelPlugin)

public:
    explicit CustomToolPlugin(QObject* parent = nullptr);
    ~CustomToolPlugin() override = default;

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
    std::unique_ptr<sentinel::plugin_sdk::IPluginTool> m_echoTool;
    std::unique_ptr<sentinel::plugin_sdk::IPluginTool> m_delayedTool;
    QJsonObject m_config;
};

} // namespace sentinel::samples
