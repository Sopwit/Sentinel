// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CustomToolPlugin.h"
#include <QJsonArray>
#include <QThread>

namespace {
class EchoTool final : public sentinel::plugin_sdk::IPluginTool {
public:
    explicit EchoTool(bool delayed = false) : delayed_(delayed) {}
    sentinel::plugin_sdk::PluginResult
    execute(const sentinel::plugin_sdk::PluginInvocation& invocation) override {
        if (delayed_) QThread::msleep(200);
        return {true, QStringLiteral("PLUGIN ECHO: %1").arg(
            invocation.arguments.value(QStringLiteral("text")).toString())};
    }

private:
    bool delayed_ = false;
};
} // namespace

namespace sentinel::samples {

CustomToolPlugin::CustomToolPlugin(QObject* parent) : QObject(parent) {}

QString CustomToolPlugin::pluginId() const {
#ifdef SENTINEL_SAMPLE_SECOND_PLUGIN
    return QStringLiteral("dev.sentinel.plugin.second-tool");
#else
    return QStringLiteral("dev.sentinel.plugin.custom-tool");
#endif
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

bool CustomToolPlugin::initialize(sentinel::plugin_sdk::IPluginContext* context) {
    m_context = context;
    m_state = sentinel::plugin_sdk::PluginState::Initialized;
    if (m_context) {
        m_context->logMessage(QStringLiteral("INFO"),
                              QStringLiteral("CustomToolPlugin initialized successfully."));
        sentinel::plugin_sdk::PluginToolDescriptor descriptor;
        descriptor.id = QStringLiteral("echo");
        descriptor.name = QStringLiteral("Echo");
        descriptor.description = QStringLiteral("Echo text through a plugin tool.");
        descriptor.category = QStringLiteral("Plugin sample");
        descriptor.risk = QStringLiteral("medium");
        descriptor.inputSchema = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"),
             QJsonObject{{QStringLiteral("text"),
                          QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}}},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("text")}},
            {QStringLiteral("additionalProperties"), false}};
        m_echoTool = std::make_unique<EchoTool>();
        if (!m_context->registerTool(descriptor, m_echoTool.get()))
            return false;
        sentinel::plugin_sdk::PluginToolDescriptor delayed;
        delayed.id = QStringLiteral("delayed_echo");
        delayed.name = QStringLiteral("Delayed Echo");
        delayed.description = QStringLiteral("Echo text after a short delay.");
        delayed.category = QStringLiteral("Plugin sample");
        delayed.risk = QStringLiteral("medium");
        delayed.inputSchema = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"),
             QJsonObject{{QStringLiteral("text"),
                          QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}}},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("text")}},
            {QStringLiteral("additionalProperties"), false}};
        m_delayedTool = std::make_unique<EchoTool>(true);
        if (!m_context->registerTool(delayed, m_delayedTool.get()))
            return false;
    }
    return true;
}

bool CustomToolPlugin::start() {
    if (m_state != sentinel::plugin_sdk::PluginState::Initialized) {
        return false;
    }
    m_state = sentinel::plugin_sdk::PluginState::Active;
    if (m_context) {
        m_context->logMessage(QStringLiteral("INFO"), QStringLiteral("CustomToolPlugin started."));
    }
    return true;
}

void CustomToolPlugin::stop() {
    if (m_state == sentinel::plugin_sdk::PluginState::Active) {
        m_state = sentinel::plugin_sdk::PluginState::Initialized;
        if (m_context) {
            m_context->logMessage(QStringLiteral("INFO"),
                                  QStringLiteral("CustomToolPlugin stopped."));
        }
    }
}

void CustomToolPlugin::shutdown() {
    stop();
    m_context = nullptr;
    m_echoTool.reset();
    m_delayedTool.reset();
    m_state = sentinel::plugin_sdk::PluginState::Unloaded;
}

sentinel::plugin_sdk::PluginState CustomToolPlugin::state() const {
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
