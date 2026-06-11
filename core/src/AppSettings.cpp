#include "sentinel/core/AppSettings.h"

#include "sentinel/core/ModelRouting.h"
#include "sentinel/core/OllamaRuntime.h"

namespace sentinel::core {

namespace {

bool isKnownRuntimeProviderId(const QString& providerId) {
    return providerId == QStringLiteral("ollama") ||
           providerId == QStringLiteral("openai-compatible") ||
           providerId == QStringLiteral("claude") || providerId == QStringLiteral("gemini");
}

} // namespace

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

QString AppSettings::appLanguage() const {
    const auto fallback = QString::fromLatin1(defaultAppLanguage);
    const auto stored = store_ ? store_->value(QString::fromLatin1(appLanguageKey), fallback)
                               : fallback;
    const auto normalized = stored.trimmed().toLower();
    return availableLanguages().contains(normalized) ? normalized : fallback;
}

void AppSettings::setAppLanguage(const QString& language) {
    const auto normalized = language.trimmed().toLower();
    const auto selected = availableLanguages().contains(normalized)
                              ? normalized
                              : QString::fromLatin1(defaultAppLanguage);
    if (selected == appLanguage() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(appLanguageKey), selected);
    emit appLanguageChanged();
}

QStringList AppSettings::availableLanguages() const {
    return {QStringLiteral("system"), QStringLiteral("en"), QStringLiteral("tr")};
}

QString AppSettings::languageDisplayName(const QString& language) const {
    const auto normalized = language.trimmed().toLower();
    if (normalized == QStringLiteral("tr")) {
        return tr("Türkçe");
    }
    if (normalized == QStringLiteral("en")) {
        return tr("English");
    }
    return tr("System Default");
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

QString AppSettings::selectedRuntimeProvider() const {
    const auto fallback = QString::fromLatin1(defaultSelectedRuntimeProvider);
    const auto stored = store_ ? store_->value(QString::fromLatin1(selectedRuntimeProviderKey),
                                               fallback)
                               : fallback;
    const auto normalized = stored.trimmed().toLower();
    if (isKnownRuntimeProviderId(normalized)) {
        return normalized;
    }
    return fallback;
}

void AppSettings::setSelectedRuntimeProvider(const QString& providerId) {
    const auto normalized = providerId.trimmed().toLower();
    const auto selected = isKnownRuntimeProviderId(normalized)
                              ? normalized
                              : QString::fromLatin1(defaultSelectedRuntimeProvider);
    if (selected == selectedRuntimeProvider() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(selectedRuntimeProviderKey), selected);
    emit selectedRuntimeProviderChanged();
}

QString AppSettings::selectedLocalModel() const {
    return selectedModelForProvider(QStringLiteral("ollama"));
}

void AppSettings::setSelectedLocalModel(const QString& model) {
    setSelectedModelForProvider(QStringLiteral("ollama"), model);
}

QString AppSettings::selectedModelForProvider(const QString& providerId) const {
    if (!store_) {
        return {};
    }

    const auto normalizedProvider = providerId.trimmed().toLower();
    if (normalizedProvider.isEmpty()) {
        return {};
    }

    const auto providerKey =
        QString::fromLatin1(selectedProviderModelKeyPrefix) + normalizedProvider;
    const auto fallback = normalizedProvider == QStringLiteral("ollama")
                              ? store_->value(QString::fromLatin1(selectedLocalModelKey), {})
                              : QString();
    return store_->value(providerKey, fallback).trimmed();
}

void AppSettings::setSelectedModelForProvider(const QString& providerId, const QString& model) {
    if (!store_) {
        return;
    }

    const auto normalizedProvider = providerId.trimmed().toLower();
    if (normalizedProvider.isEmpty()) {
        return;
    }

    const auto normalized = model.trimmed();
    if (normalized == selectedModelForProvider(normalizedProvider)) {
        return;
    }

    store_->setValue(QString::fromLatin1(selectedProviderModelKeyPrefix) + normalizedProvider,
                     normalized);
    if (normalizedProvider == QStringLiteral("ollama")) {
        store_->setValue(QString::fromLatin1(selectedLocalModelKey), normalized);
    }
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

bool AppSettings::contextExplainabilityVisible() const {
    return store_ ? store_->value(QString::fromLatin1(contextExplainabilityVisibleKey),
                                  QStringLiteral("true")) == QStringLiteral("true")
                  : true;
}

void AppSettings::setContextExplainabilityVisible(bool visible) {
    if (visible == contextExplainabilityVisible() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(contextExplainabilityVisibleKey),
                     visible ? QStringLiteral("true") : QStringLiteral("false"));
    emit contextExplainabilityVisibleChanged();
}

bool AppSettings::developerModeEnabled() const {
    return store_ ? store_->value(QString::fromLatin1(developerModeEnabledKey),
                                  QStringLiteral("false")) == QStringLiteral("true")
                  : false;
}

void AppSettings::setDeveloperModeEnabled(bool enabled) {
    if (enabled == developerModeEnabled() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(developerModeEnabledKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit developerModeEnabledChanged();
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

QString AppSettings::activeConversationId() const {
    return store_ ? store_->value(QString::fromLatin1(activeConversationIdKey), {}).trimmed()
                  : QString();
}

void AppSettings::setActiveConversationId(const QString& conversationId) {
    const auto normalized = conversationId.trimmed();
    if (normalized == activeConversationId() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(activeConversationIdKey), normalized);
    emit activeConversationIdChanged();
}

} // namespace sentinel::core
