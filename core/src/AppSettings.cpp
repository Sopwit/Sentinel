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

} // namespace sentinel::core
