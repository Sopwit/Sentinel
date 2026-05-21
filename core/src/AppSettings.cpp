#include "sentinel/core/AppSettings.h"

#include "sentinel/core/ModelRouting.h"
#include "sentinel/core/OllamaRuntime.h"

namespace sentinel::core {

AppSettings::AppSettings(std::unique_ptr<ISettingsStore> store, QObject* parent)
    : QObject(parent), store_(std::move(store)) {}

QString AppSettings::themeName() const {
    const auto fallback = QString::fromLatin1(defaultThemeName);
    return store_ ? store_->value(QString::fromLatin1(themeNameKey), fallback) : fallback;
}

void AppSettings::setThemeName(const QString& themeName) {
    const auto normalized = themeName.trimmed();
    if (normalized.isEmpty() || normalized == this->themeName() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(themeNameKey), normalized);
    emit themeNameChanged();
}

QString AppSettings::configurationProfile() const {
    const auto fallback = QString::fromLatin1(defaultConfigurationProfile);
    return store_ ? store_->value(QString::fromLatin1(configurationProfileKey), fallback)
                  : fallback;
}

void AppSettings::setConfigurationProfile(const QString& configurationProfile) {
    const auto normalized = configurationProfile.trimmed();
    if (normalized.isEmpty() || normalized == this->configurationProfile() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(configurationProfileKey), normalized);
    emit configurationProfileChanged();
}

QString AppSettings::routingModeName() const {
    const auto fallback = sentinel::core::routingModeName(RoutingMode::LocalOnly);
    return store_ ? normalizedRoutingModeName(
                        store_->value(QString::fromLatin1(routingModeKey), fallback))
                  : fallback;
}

void AppSettings::setRoutingModeName(const QString& routingModeName) {
    const auto normalized = normalizedRoutingModeName(routingModeName);
    if (normalized == this->routingModeName() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(routingModeKey), normalized);
    emit routingModeNameChanged();
}

QStringList AppSettings::availableRoutingModes() const {
    return routingModeNames();
}

QString AppSettings::ollamaEndpoint() const {
    const auto fallback = OllamaEndpoint::defaultEndpoint().toString();
    return store_ ? OllamaEndpoint::fromUserInput(
                        store_->value(QString::fromLatin1(ollamaEndpointKey), fallback))
                        .toString()
                  : fallback;
}

void AppSettings::setOllamaEndpoint(const QString& endpoint) {
    const auto normalized = OllamaEndpoint::fromUserInput(endpoint).toString();
    if (normalized == ollamaEndpoint() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(ollamaEndpointKey), normalized);
    emit ollamaEndpointChanged();
}

QString AppSettings::selectedLocalModel() const {
    return store_ ? store_->value(QString::fromLatin1(selectedLocalModelKey), {}).trimmed()
                  : QString();
}

void AppSettings::setSelectedLocalModel(const QString& model) {
    const auto normalized = model.trimmed();
    if (normalized == selectedLocalModel() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(selectedLocalModelKey), normalized);
    emit selectedLocalModelChanged();
}

bool AppSettings::localChatInferenceEnabled() const {
    return store_ ? store_->value(QString::fromLatin1(localChatInferenceEnabledKey),
                                  QStringLiteral("false")) == QStringLiteral("true")
                  : false;
}

void AppSettings::setLocalChatInferenceEnabled(bool enabled) {
    if (enabled == localChatInferenceEnabled() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(localChatInferenceEnabledKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit localChatInferenceEnabledChanged();
}

bool AppSettings::localInferenceStreamingEnabled() const {
    return store_ ? store_->value(QString::fromLatin1(localInferenceStreamingEnabledKey),
                                  QStringLiteral("false")) == QStringLiteral("true")
                  : false;
}

void AppSettings::setLocalInferenceStreamingEnabled(bool enabled) {
    if (enabled == localInferenceStreamingEnabled() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(localInferenceStreamingEnabledKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit localInferenceStreamingEnabledChanged();
}

bool AppSettings::promptContextInjectionEnabled() const {
    return store_ ? store_->value(QString::fromLatin1(promptContextInjectionEnabledKey),
                                  QStringLiteral("false")) == QStringLiteral("true")
                  : false;
}

void AppSettings::setPromptContextInjectionEnabled(bool enabled) {
    if (enabled == promptContextInjectionEnabled() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(promptContextInjectionEnabledKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit promptContextInjectionEnabledChanged();
}

bool AppSettings::semanticPromptInclusionEnabled() const {
    return store_ ? store_->value(QString::fromLatin1(semanticPromptInclusionEnabledKey),
                                  QStringLiteral("false")) == QStringLiteral("true")
                  : false;
}

void AppSettings::setSemanticPromptInclusionEnabled(bool enabled) {
    if (enabled == semanticPromptInclusionEnabled() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(semanticPromptInclusionEnabledKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit semanticPromptInclusionEnabledChanged();
}

QString AppSettings::piperBinaryPath() const {
    return store_ ? store_->value(QString::fromLatin1(piperBinaryPathKey), {}).trimmed()
                  : QString();
}

void AppSettings::setPiperBinaryPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == piperBinaryPath() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(piperBinaryPathKey), normalized);
    emit piperBinaryPathChanged();
}

QString AppSettings::piperModelPath() const {
    return store_ ? store_->value(QString::fromLatin1(piperModelPathKey), {}).trimmed() : QString();
}

void AppSettings::setPiperModelPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == piperModelPath() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(piperModelPathKey), normalized);
    emit piperModelPathChanged();
}

QString AppSettings::whisperBinaryPath() const {
    return store_ ? store_->value(QString::fromLatin1(whisperBinaryPathKey), {}).trimmed()
                  : QString();
}

void AppSettings::setWhisperBinaryPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == whisperBinaryPath() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(whisperBinaryPathKey), normalized);
    emit whisperBinaryPathChanged();
}

QString AppSettings::whisperModelPath() const {
    return store_ ? store_->value(QString::fromLatin1(whisperModelPathKey), {}).trimmed()
                  : QString();
}

void AppSettings::setWhisperModelPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == whisperModelPath() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(whisperModelPathKey), normalized);
    emit whisperModelPathChanged();
}

bool AppSettings::piperFileOutputExecutionEnabled() const {
    return store_ ? store_->value(QString::fromLatin1(piperFileOutputExecutionEnabledKey),
                                  QStringLiteral("false")) == QStringLiteral("true")
                  : false;
}

void AppSettings::setPiperFileOutputExecutionEnabled(bool enabled) {
    if (enabled == piperFileOutputExecutionEnabled() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(piperFileOutputExecutionEnabledKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit piperFileOutputExecutionEnabledChanged();
}

} // namespace sentinel::core
