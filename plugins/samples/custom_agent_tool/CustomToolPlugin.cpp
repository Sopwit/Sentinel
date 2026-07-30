// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CustomToolPlugin.h"

namespace sentinel::samples {

CustomToolPlugin::CustomToolPlugin(QObject* parent)
    : QObject(parent)
{
}

QString CustomToolPlugin::pluginId() const {
    return QStringLiteral("dev.sentinel.plugin.custom-tool");
}

QString CustomToolPlugin::displayName() const {
    return QStringLiteral("Custom Agent Tool");
}

QString CustomToolPlugin::vendor() const {
    return QStringLiteral("Sopwit Community");
}

QString CustomToolPlugin::version() const {
    return QStringLiteral("1.0.0");
}

QString CustomToolPlugin::requiredCoreVersion() const {
    return QStringLiteral(">=1.0.0");
}

bool CustomToolPlugin::initialize(std::shared_ptr<sentinel::core::plugin::IPluginContext> context) {
    m_context = std::move(context);
    m_state = sentinel::core::plugin::PluginState::Initialized;
    if (m_context) {
        m_context->logMessage(QStringLiteral("INFO"), QStringLiteral("CustomToolPlugin initialized successfully."));
    }
    return true;
}

bool CustomToolPlugin::start() {
    if (m_state != sentinel::core::plugin::PluginState::Initialized) {
        return false;
    }
    m_state = sentinel::core::plugin::PluginState::Active;
    if (m_context) {
        m_context->logMessage(QStringLiteral("INFO"), QStringLiteral("CustomToolPlugin started."));
    }
    return true;
}

void CustomToolPlugin::stop() {
    if (m_state == sentinel::core::plugin::PluginState::Active) {
        m_state = sentinel::core::plugin::PluginState::Initialized;
        if (m_context) {
            m_context->logMessage(QStringLiteral("INFO"), QStringLiteral("CustomToolPlugin stopped."));
        }
    }
}

void CustomToolPlugin::shutdown() {
    stop();
    m_context.reset();
    m_state = sentinel::core::plugin::PluginState::Unloaded;
}

sentinel::core::plugin::PluginState CustomToolPlugin::state() const {
    return m_state;
}

QJsonObject CustomToolPlugin::defaultConfig() const {
    QJsonObject config;
    config[QStringLiteral("enabled")] = true;
    config[QStringLiteral("timeout_ms")] = 5000;
    return config;
}

void CustomToolPlugin::configure(const QJsonObject& config) {
    m_config = config;
}

} // namespace sentinel::samples
