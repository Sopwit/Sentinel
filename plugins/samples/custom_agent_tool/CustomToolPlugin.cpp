// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CustomToolPlugin.h"
#include <QJsonArray>
#include <QTimer>

namespace {
class EchoTool final : public sentinel::core::IToolHandler {
public:
    explicit EchoTool(bool delayed = false) : delayed_(delayed) {}
    sentinel::core::IToolExecutor::Cancel
    execute(const sentinel::core::ToolExecutionRequest& request, const QString&, const QString&,
            sentinel::core::IToolExecutor::Output,
            sentinel::core::IToolExecutor::Completion completion) override {
        QString text;
        for (const auto& argument : request.plan.invocations.first().arguments)
            if (argument.id == QStringLiteral("text"))
                text = argument.value;
        auto finish = [completion = std::move(completion), text] {
            completion({sentinel::core::ToolExecutionStatus::Succeeded,
                        QStringLiteral("PLUGIN ECHO: %1").arg(text)});
        };
        if (delayed_)
            QTimer::singleShot(200, std::move(finish));
        else
            finish();
        return {};
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

bool CustomToolPlugin::initialize(std::shared_ptr<sentinel::core::plugin::IPluginContext> context) {
    m_context = std::move(context);
    m_state = sentinel::core::plugin::PluginState::Initialized;
    if (m_context) {
        m_context->logMessage(QStringLiteral("INFO"),
                              QStringLiteral("CustomToolPlugin initialized successfully."));
        sentinel::core::ToolDescriptor descriptor;
        descriptor.id = QStringLiteral("echo");
        descriptor.name = QStringLiteral("Echo");
        descriptor.description = QStringLiteral("Echo text through a plugin tool.");
        descriptor.category = QStringLiteral("Plugin sample");
        descriptor.riskLevel = sentinel::core::ToolRiskLevel::Medium;
        descriptor.inputSchema = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"),
             QJsonObject{{QStringLiteral("text"),
                          QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}}},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("text")}}};
        if (!m_context->registerTool(std::move(descriptor), std::make_shared<EchoTool>()))
            return false;
        sentinel::core::ToolDescriptor delayed;
        delayed.id = QStringLiteral("delayed_echo");
        delayed.name = QStringLiteral("Delayed Echo");
        delayed.description = QStringLiteral("Echo text after an asynchronous timer.");
        delayed.category = QStringLiteral("Plugin sample");
        delayed.riskLevel = sentinel::core::ToolRiskLevel::Medium;
        delayed.inputSchema = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"),
             QJsonObject{{QStringLiteral("text"),
                          QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}}},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("text")}}};
        if (!m_context->registerTool(std::move(delayed), std::make_shared<EchoTool>(true)))
            return false;
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
            m_context->logMessage(QStringLiteral("INFO"),
                                  QStringLiteral("CustomToolPlugin stopped."));
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
