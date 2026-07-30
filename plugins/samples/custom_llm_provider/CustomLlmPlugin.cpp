#include "CustomLlmPlugin.h"

namespace sentinel::samples {

CustomLlmPlugin::CustomLlmPlugin(QObject* parent)
    : QObject(parent)
{
}

QString CustomLlmPlugin::pluginId() const {
    return QStringLiteral("dev.sentinel.plugin.ollama-extended");
}

QString CustomLlmPlugin::displayName() const {
    return QStringLiteral("Ollama Extended Provider");
}

QString CustomLlmPlugin::vendor() const {
    return QStringLiteral("Sopwit Community");
}

QString CustomLlmPlugin::version() const {
    return QStringLiteral("1.0.0");
}

QString CustomLlmPlugin::requiredCoreVersion() const {
    return QStringLiteral(">=1.0.0");
}

bool CustomLlmPlugin::initialize(std::shared_ptr<sentinel::core::plugin::IPluginContext> context) {
    m_context = std::move(context);
    m_state = sentinel::core::plugin::PluginState::Initialized;
    if (m_context) {
        m_context->logMessage(QStringLiteral("INFO"), QStringLiteral("CustomLlmPlugin initialized successfully."));
    }
    return true;
}

bool CustomLlmPlugin::start() {
    if (m_state != sentinel::core::plugin::PluginState::Initialized) {
        return false;
    }
    m_state = sentinel::core::plugin::PluginState::Active;
    if (m_context) {
        m_context->logMessage(QStringLiteral("INFO"), QStringLiteral("CustomLlmPlugin started."));
    }
    return true;
}

void CustomLlmPlugin::stop() {
    if (m_state == sentinel::core::plugin::PluginState::Active) {
        m_state = sentinel::core::plugin::PluginState::Initialized;
        if (m_context) {
            m_context->logMessage(QStringLiteral("INFO"), QStringLiteral("CustomLlmPlugin stopped."));
        }
    }
}

void CustomLlmPlugin::shutdown() {
    stop();
    m_context.reset();
    m_state = sentinel::core::plugin::PluginState::Unloaded;
}

sentinel::core::plugin::PluginState CustomLlmPlugin::state() const {
    return m_state;
}

QJsonObject CustomLlmPlugin::defaultConfig() const {
    QJsonObject config;
    config[QStringLiteral("temperature")] = 0.7;
    config[QStringLiteral("max_tokens")] = 2048;
    return config;
}

void CustomLlmPlugin::configure(const QJsonObject& config) {
    m_config = config;
}

} // namespace sentinel::samples
