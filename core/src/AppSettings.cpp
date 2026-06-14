#include "sentinel/core/AppSettings.h"

#include "sentinel/core/ModelRegistry.h"
#include "sentinel/core/ModelRouting.h"
#include "sentinel/core/OllamaRuntime.h"

#include <algorithm>

namespace sentinel::core {

namespace {

bool isKnownRuntimeProviderId(const QString& providerId) {
    return providerId == QStringLiteral("ollama") ||
           providerId == QStringLiteral("openai-compatible-local") ||
           providerId == QStringLiteral("lm-studio") ||
           providerId == QStringLiteral("llama-cpp-server") ||
           providerId == QStringLiteral("openai-compatible") ||
           providerId == QStringLiteral("claude") || providerId == QStringLiteral("gemini");
}

bool isKnownSkillProfileId(const QString& profileId) {
    return profileId == QStringLiteral("developer") || profileId == QStringLiteral("student") ||
           profileId == QStringLiteral("researcher") ||
           profileId == QStringLiteral("personal-assistant") ||
           profileId == QStringLiteral("custom");
}

bool isKnownModelRoleId(const QString& roleId) {
    return modelRoleIds().contains(roleId.trimmed().toLower());
}

QString normalizedPermissionPolicyState(const QString& state) {
    const auto normalized = state.trimmed().toLower();
    if (normalized == QStringLiteral("ask every time") ||
        normalized == QStringLiteral("ask-every-time")) {
        return QStringLiteral("Ask Every Time");
    }
    if (normalized == QStringLiteral("trusted")) {
        return QStringLiteral("Trusted");
    }
    if (normalized == QStringLiteral("enabled")) {
        return QStringLiteral("Enabled");
    }
    return QStringLiteral("Disabled");
}

QString normalizedUpdatePolicy(const QString& policy) {
    const auto normalized = policy.trimmed().toLower();
    if (normalized == QStringLiteral("never")) {
        return QStringLiteral("Never");
    }
    if (normalized == QStringLiteral("weekly")) {
        return QStringLiteral("Weekly");
    }
    if (normalized == QStringLiteral("on startup") || normalized == QStringLiteral("startup")) {
        return QStringLiteral("On Startup");
    }
    return QStringLiteral("Ask Before Checking");
}

QString normalizedNotificationPolicy(const QString& policy) {
    const auto normalized = policy.trimmed().toLower();
    if (normalized == QStringLiteral("disabled")) {
        return QStringLiteral("Disabled");
    }
    if (normalized == QStringLiteral("all")) {
        return QStringLiteral("All");
    }
    if (normalized == QStringLiteral("custom")) {
        return QStringLiteral("Custom");
    }
    return QStringLiteral("Important Only");
}

QString normalizedOnboardingUseCase(const QString& useCase) {
    const auto normalized = useCase.trimmed().toLower();
    if (normalized == QStringLiteral("coding")) {
        return QStringLiteral("Coding");
    }
    if (normalized == QStringLiteral("study")) {
        return QStringLiteral("Study");
    }
    if (normalized == QStringLiteral("writing")) {
        return QStringLiteral("Writing");
    }
    return QStringLiteral("General Assistant");
}

QString normalizedWorkspaceIdSetting(const QString& workspaceId) {
    const auto normalized = workspaceId.trimmed();
    return normalized.isEmpty() ? QStringLiteral("personal") : normalized;
}

QString normalizedAttachmentBehavior(const QString& behavior) {
    const auto normalized = behavior.trimmed().toLower();
    if (normalized == QStringLiteral("replace") ||
        normalized == QStringLiteral("replace existing attachment")) {
        return QStringLiteral("Replace Existing Attachment");
    }
    if (normalized == QStringLiteral("paste enabled") || normalized == QStringLiteral("paste") ||
        normalized == QStringLiteral("paste attachment enabled")) {
        return QStringLiteral("Paste Attachment Enabled");
    }
    return QStringLiteral("Manual Attachments Only");
}

QString normalizedExportFormat(const QString& format) {
    const auto normalized = format.trimmed().toLower();
    if (normalized == QStringLiteral("pdf")) {
        return QStringLiteral("PDF");
    }
    if (normalized == QStringLiteral("txt") || normalized == QStringLiteral("text")) {
        return QStringLiteral("TXT");
    }
    if (normalized == QStringLiteral("docx")) {
        return QStringLiteral("DOCX");
    }
    if (normalized == QStringLiteral("json")) {
        return QStringLiteral("JSON");
    }
    return QStringLiteral("Markdown");
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

QString AppSettings::selectedModelForRole(const QString& roleId) const {
    if (!store_) {
        return {};
    }

    const auto normalizedRole = roleId.trimmed().toLower();
    if (!isKnownModelRoleId(normalizedRole)) {
        return {};
    }

    return store_
        ->value(QString::fromLatin1(selectedRoleModelKeyPrefix) + normalizedRole, {})
        .trimmed();
}

void AppSettings::setSelectedModelForRole(const QString& roleId, const QString& modelId) {
    if (!store_) {
        return;
    }

    const auto normalizedRole = roleId.trimmed().toLower();
    if (!isKnownModelRoleId(normalizedRole)) {
        return;
    }

    const auto normalizedModel = modelId.trimmed();
    if (normalizedModel == selectedModelForRole(normalizedRole)) {
        return;
    }

    store_->setValue(QString::fromLatin1(selectedRoleModelKeyPrefix) + normalizedRole,
                     normalizedModel);
    emit selectedModelRoleChanged();
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

int AppSettings::localInferenceTimeoutMs() const {
    const auto fallback = QString::number(defaultLocalInferenceTimeoutMs);
    bool ok = false;
    const auto value =
        store_ ? store_->value(QString::fromLatin1(localInferenceTimeoutMsKey), fallback).toInt(&ok)
               : defaultLocalInferenceTimeoutMs;
    if (!ok) {
        return defaultLocalInferenceTimeoutMs;
    }
    return std::clamp(value, 1000, 300000);
}

void AppSettings::setLocalInferenceTimeoutMs(int timeoutMs) {
    const auto normalized = std::clamp(timeoutMs, 1000, 300000);
    if (normalized == localInferenceTimeoutMs() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(localInferenceTimeoutMsKey), QString::number(normalized));
    emit localInferenceTimeoutMsChanged();
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

bool AppSettings::companionEnabled() const {
    return store_ ? store_->value(QString::fromLatin1(companionEnabledKey),
                                  QStringLiteral("false")) == QStringLiteral("true")
                  : false;
}

void AppSettings::setCompanionEnabled(bool enabled) {
    if (enabled == companionEnabled() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(companionEnabledKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit companionEnabledChanged();
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

QString AppSettings::selectedWorkspaceId() const {
    const auto fallback = QString::fromLatin1(defaultSelectedWorkspaceId);
    const auto stored = store_ ? store_->value(QString::fromLatin1(selectedWorkspaceIdKey), fallback)
                               : fallback;
    return normalizedWorkspaceIdSetting(stored);
}

void AppSettings::setSelectedWorkspaceId(const QString& workspaceId) {
    const auto selected = normalizedWorkspaceIdSetting(workspaceId);
    if (selected == selectedWorkspaceId() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(selectedWorkspaceIdKey), selected);
    emit selectedWorkspaceIdChanged();
}

QString AppSettings::defaultWorkspaceId() const {
    const auto fallback = QString::fromLatin1(defaultSelectedWorkspaceId);
    const auto stored = store_ ? store_->value(QString::fromLatin1(defaultWorkspaceIdKey), fallback)
                               : fallback;
    return normalizedWorkspaceIdSetting(stored);
}

void AppSettings::setDefaultWorkspaceId(const QString& workspaceId) {
    const auto selected = normalizedWorkspaceIdSetting(workspaceId);
    if (selected == defaultWorkspaceId() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(defaultWorkspaceIdKey), selected);
    emit workspaceSettingsChanged();
}

QString AppSettings::workspaceCatalogJson() const {
    return store_ ? store_->value(QString::fromLatin1(workspaceCatalogJsonKey), {}) : QString();
}

void AppSettings::setWorkspaceCatalogJson(const QString& catalogJson) {
    if (catalogJson == workspaceCatalogJson() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(workspaceCatalogJsonKey), catalogJson);
    emit workspaceSettingsChanged();
}

bool AppSettings::localKnowledgeBaseEnabled() const {
    return store_ ? store_->value(QString::fromLatin1(localKnowledgeBaseEnabledKey),
                                  QStringLiteral("false")) == QStringLiteral("true")
                  : false;
}

void AppSettings::setLocalKnowledgeBaseEnabled(bool enabled) {
    if (enabled == localKnowledgeBaseEnabled() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(localKnowledgeBaseEnabledKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit workspaceSettingsChanged();
}

bool AppSettings::retrievalExplainabilityEnabled() const {
    return store_ ? store_->value(QString::fromLatin1(retrievalExplainabilityEnabledKey),
                                  QStringLiteral("true")) == QStringLiteral("true")
                  : true;
}

void AppSettings::setRetrievalExplainabilityEnabled(bool enabled) {
    if (enabled == retrievalExplainabilityEnabled() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(retrievalExplainabilityEnabledKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit workspaceSettingsChanged();
}

QString AppSettings::attachmentBehavior() const {
    const auto fallback = QString::fromLatin1(defaultAttachmentBehavior);
    const auto stored = store_ ? store_->value(QString::fromLatin1(attachmentBehaviorKey), fallback)
                               : fallback;
    return normalizedAttachmentBehavior(stored);
}

void AppSettings::setAttachmentBehavior(const QString& behavior) {
    const auto selected = normalizedAttachmentBehavior(behavior);
    if (selected == attachmentBehavior() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(attachmentBehaviorKey), selected);
    emit workspaceSettingsChanged();
}

QString AppSettings::exportDefaultFormat() const {
    const auto fallback = QString::fromLatin1(defaultExportFormat);
    const auto stored = store_ ? store_->value(QString::fromLatin1(exportDefaultFormatKey), fallback)
                               : fallback;
    return normalizedExportFormat(stored);
}

void AppSettings::setExportDefaultFormat(const QString& format) {
    const auto selected = normalizedExportFormat(format);
    if (selected == exportDefaultFormat() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(exportDefaultFormatKey), selected);
    emit workspaceSettingsChanged();
}

bool AppSettings::exportIncludeTimestamps() const {
    return store_ ? store_->value(QString::fromLatin1(exportIncludeTimestampsKey),
                                  QStringLiteral("true")) == QStringLiteral("true")
                  : true;
}

void AppSettings::setExportIncludeTimestamps(bool enabled) {
    if (enabled == exportIncludeTimestamps() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(exportIncludeTimestampsKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit workspaceSettingsChanged();
}

bool AppSettings::exportIncludeCitations() const {
    return store_ ? store_->value(QString::fromLatin1(exportIncludeCitationsKey),
                                  QStringLiteral("true")) == QStringLiteral("true")
                  : true;
}

void AppSettings::setExportIncludeCitations(bool enabled) {
    if (enabled == exportIncludeCitations() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(exportIncludeCitationsKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit workspaceSettingsChanged();
}

bool AppSettings::exportAnonymizeNames() const {
    return store_ ? store_->value(QString::fromLatin1(exportAnonymizeNamesKey),
                                  QStringLiteral("false")) == QStringLiteral("true")
                  : false;
}

void AppSettings::setExportAnonymizeNames(bool enabled) {
    if (enabled == exportAnonymizeNames() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(exportAnonymizeNamesKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit workspaceSettingsChanged();
}

bool AppSettings::exportIncludeModelMetadata() const {
    return store_ ? store_->value(QString::fromLatin1(exportIncludeModelMetadataKey),
                                  QStringLiteral("true")) == QStringLiteral("true")
                  : true;
}

void AppSettings::setExportIncludeModelMetadata(bool enabled) {
    if (enabled == exportIncludeModelMetadata() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(exportIncludeModelMetadataKey),
                     enabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit workspaceSettingsChanged();
}

QString AppSettings::selectedSkillProfile() const {
    const auto fallback = QString::fromLatin1(defaultSelectedSkillProfile);
    const auto stored = store_ ? store_->value(QString::fromLatin1(selectedSkillProfileKey),
                                               fallback)
                               : fallback;
    const auto normalized = stored.trimmed().toLower();
    return isKnownSkillProfileId(normalized) ? normalized : fallback;
}

void AppSettings::setSelectedSkillProfile(const QString& profileId) {
    const auto normalized = profileId.trimmed().toLower();
    const auto selected = isKnownSkillProfileId(normalized)
                              ? normalized
                              : QString::fromLatin1(defaultSelectedSkillProfile);
    if (selected == selectedSkillProfile() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(selectedSkillProfileKey), selected);
    emit selectedSkillProfileChanged();
}

QString AppSettings::defaultPermissionPolicyState() const {
    const auto fallback = QString::fromLatin1(defaultPermissionPolicyStateValue);
    const auto stored =
        store_ ? store_->value(QString::fromLatin1(defaultPermissionPolicyStateKey), fallback)
               : fallback;
    return normalizedPermissionPolicyState(stored);
}

void AppSettings::setDefaultPermissionPolicyState(const QString& state) {
    const auto selected = normalizedPermissionPolicyState(state);
    if (selected == defaultPermissionPolicyState() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(defaultPermissionPolicyStateKey), selected);
    emit defaultPermissionPolicyStateChanged();
}

QString AppSettings::updateCheckPolicy() const {
    const auto fallback = QString::fromLatin1(defaultUpdateCheckPolicy);
    const auto stored = store_ ? store_->value(QString::fromLatin1(updateCheckPolicyKey), fallback)
                               : fallback;
    return normalizedUpdatePolicy(stored);
}

void AppSettings::setUpdateCheckPolicy(const QString& policy) {
    const auto selected = normalizedUpdatePolicy(policy);
    if (selected == updateCheckPolicy() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(updateCheckPolicyKey), selected);
    emit updateCheckPolicyChanged();
}

QString AppSettings::notificationPolicy() const {
    const auto fallback = QString::fromLatin1(defaultNotificationPolicy);
    const auto stored = store_ ? store_->value(QString::fromLatin1(notificationPolicyKey), fallback)
                               : fallback;
    return normalizedNotificationPolicy(stored);
}

void AppSettings::setNotificationPolicy(const QString& policy) {
    const auto selected = normalizedNotificationPolicy(policy);
    if (selected == notificationPolicy() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(notificationPolicyKey), selected);
    emit notificationPolicyChanged();
}

bool AppSettings::onboardingComplete() const {
    return store_ ? store_->value(QString::fromLatin1(onboardingCompleteKey),
                                  QStringLiteral("false")) == QStringLiteral("true")
                  : false;
}

void AppSettings::setOnboardingComplete(bool complete) {
    if (complete == onboardingComplete() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(onboardingCompleteKey),
                     complete ? QStringLiteral("true") : QStringLiteral("false"));
    emit onboardingCompleteChanged();
}

QString AppSettings::onboardingUseCase() const {
    const auto fallback = QString::fromLatin1(defaultOnboardingUseCase);
    const auto stored = store_ ? store_->value(QString::fromLatin1(onboardingUseCaseKey), fallback)
                               : fallback;
    return normalizedOnboardingUseCase(stored);
}

void AppSettings::setOnboardingUseCase(const QString& useCase) {
    const auto selected = normalizedOnboardingUseCase(useCase);
    if (selected == onboardingUseCase() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(onboardingUseCaseKey), selected);
    emit onboardingUseCaseChanged();
}

QString AppSettings::recoveryDraftText() const {
    return store_ ? store_->value(QString::fromLatin1(recoveryDraftTextKey), {}) : QString();
}

void AppSettings::setRecoveryDraftText(const QString& text) {
    if (text == recoveryDraftText() || !store_) {
        return;
    }

    store_->setValue(QString::fromLatin1(recoveryDraftTextKey), text);
    emit recoveryDraftTextChanged();
}

} // namespace sentinel::core
