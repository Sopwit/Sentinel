#include "sentinel/core/AppSettings.h"
#include "sentinel/core/InMemorySettingsStore.h"
#include "sentinel/core/JsonSettingsStore.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using sentinel::core::AppSettings;
using sentinel::core::InMemorySettingsStore;

class AppSettingsTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesDefaults();
    void updatesThemeName();
    void ignoresBlankThemeName();
    void updatesConfigurationProfile();
    void exposesLanguageDefaults();
    void persistsLanguageSelection();
    void languageSettingDoesNotChangeRuntimeFlags();
    void exposesRoutingModeDefault();
    void updatesRoutingMode();
    void fallsBackForInvalidRoutingMode();
    void persistsRoutingModeThroughJsonStore();
    void exposesOllamaEndpointDefault();
    void normalizesOllamaEndpoint();
    void persistsSelectedRuntimeProvider();
    void doesNotPersistApiKeyValuesForCloudPlaceholders();
    void persistsSelectedLocalModel();
    void persistsSelectedModelPerProvider();
    void persistsSelectedModelRoles();
    void persistsLocalChatInferenceOptIn();
    void persistsLocalInferenceStreamingOptIn();
    void persistsPromptContextInjectionOptIn();
    void persistsSemanticPromptInclusionOptIn();
    void persistsContextExplainabilityVisibility();
    void persistsCompanionVisibilityPreference();
    void persistsDeveloperModeVisibilityOptIn();
    void persistsSelectedWorkspaceId();
    void persistsWorkspaceDefaults();
    void persistsSelectedSkillProfile();
    void persistsDefaultPermissionPolicyState();
    void persistsNativeExperiencePreferences();
    void persistsVoiceConfigurationPaths();
    void persistsPiperFileOutputExecutionOptIn();
    void persistsLocalAiRuntimeSettingsThroughJsonStore();
};

static std::unique_ptr<AppSettings> makeSettings() {
    return std::make_unique<AppSettings>(std::make_unique<InMemorySettingsStore>());
}

void AppSettingsTest::exposesDefaults() {
    const auto settings = makeSettings();

    QCOMPARE(settings->themeName(), QStringLiteral("Sentinel Dark"));
    QCOMPARE(settings->configurationProfile(), QStringLiteral("Desktop Alpha"));
    QCOMPARE(settings->appLanguage(), QStringLiteral("system"));
    QCOMPARE(settings->availableLanguages(),
             QStringList({QStringLiteral("system"), QStringLiteral("en"), QStringLiteral("tr")}));
    QVERIFY(settings->selectedLocalModel().isEmpty());
    QCOMPARE(settings->selectedRuntimeProvider(), QStringLiteral("ollama"));
    QVERIFY(!settings->localChatInferenceEnabled());
    QVERIFY(!settings->localInferenceStreamingEnabled());
    QVERIFY(!settings->promptContextInjectionEnabled());
    QVERIFY(!settings->semanticPromptInclusionEnabled());
    QVERIFY(settings->contextExplainabilityVisible());
    QVERIFY(!settings->companionEnabled());
    QVERIFY(!settings->developerModeEnabled());
    QVERIFY(settings->piperBinaryPath().isEmpty());
    QVERIFY(settings->piperModelPath().isEmpty());
    QVERIFY(settings->whisperBinaryPath().isEmpty());
    QVERIFY(settings->whisperModelPath().isEmpty());
    QVERIFY(!settings->piperFileOutputExecutionEnabled());
    QCOMPARE(settings->selectedWorkspaceId(), QStringLiteral("personal"));
    QCOMPARE(settings->selectedSkillProfile(), QStringLiteral("developer"));
    QCOMPARE(settings->defaultWorkspaceId(), QStringLiteral("personal"));
    QVERIFY(!settings->localKnowledgeBaseEnabled());
    QVERIFY(settings->retrievalExplainabilityEnabled());
    QCOMPARE(settings->attachmentBehavior(), QStringLiteral("Manual Attachments Only"));
    QCOMPARE(settings->exportDefaultFormat(), QStringLiteral("Markdown"));
    QCOMPARE(settings->defaultPermissionPolicyState(), QStringLiteral("Disabled"));
    QCOMPARE(settings->updateCheckPolicy(), QStringLiteral("Ask Before Checking"));
    QCOMPARE(settings->notificationPolicy(), QStringLiteral("Important Only"));
    QVERIFY(!settings->onboardingComplete());
    QCOMPARE(settings->onboardingUseCase(), QStringLiteral("General Assistant"));
    QVERIFY(settings->recoveryDraftText().isEmpty());
}

void AppSettingsTest::updatesThemeName() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::themeNameChanged);

    settings->setThemeName(QStringLiteral(" Sentinel Light "));

    QCOMPARE(settings->themeName(), QStringLiteral("Sentinel Light"));
    QCOMPARE(spy.count(), 1);

    settings->setThemeName(QStringLiteral("Sentinel Light"));
    QCOMPARE(spy.count(), 1);
}

void AppSettingsTest::ignoresBlankThemeName() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::themeNameChanged);

    settings->setThemeName(QStringLiteral("   "));

    QCOMPARE(settings->themeName(), QStringLiteral("Sentinel Dark"));
    QCOMPARE(spy.count(), 0);
}

void AppSettingsTest::updatesConfigurationProfile() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::configurationProfileChanged);

    settings->setConfigurationProfile(QStringLiteral("Phase 2 Shell"));

    QCOMPARE(settings->configurationProfile(), QStringLiteral("Phase 2 Shell"));
    QCOMPARE(spy.count(), 1);
}

void AppSettingsTest::exposesLanguageDefaults() {
    const auto settings = makeSettings();

    QCOMPARE(settings->appLanguage(), QStringLiteral("system"));
    QCOMPARE(settings->languageDisplayName(QStringLiteral("system")),
             QStringLiteral("System Default"));
    QCOMPARE(settings->languageDisplayName(QStringLiteral("en")), QStringLiteral("English"));
    QCOMPARE(settings->languageDisplayName(QStringLiteral("tr")), QStringLiteral("Türkçe"));
}

void AppSettingsTest::persistsLanguageSelection() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("settings.json"));

    {
        AppSettings settings{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};
        QSignalSpy spy(&settings, &AppSettings::appLanguageChanged);

        settings.setAppLanguage(QStringLiteral(" tr "));

        QCOMPARE(settings.appLanguage(), QStringLiteral("tr"));
        QCOMPARE(spy.count(), 1);

        settings.setAppLanguage(QStringLiteral("unsupported"));
        QCOMPARE(settings.appLanguage(), QStringLiteral("system"));
        QCOMPARE(spy.count(), 2);

        settings.setAppLanguage(QStringLiteral("en"));
    }

    AppSettings reloaded{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};
    QCOMPARE(reloaded.appLanguage(), QStringLiteral("en"));
}

void AppSettingsTest::languageSettingDoesNotChangeRuntimeFlags() {
    const auto settings = makeSettings();

    settings->setLocalChatInferenceEnabled(true);
    settings->setLocalInferenceStreamingEnabled(true);
    settings->setPromptContextInjectionEnabled(true);
    settings->setSemanticPromptInclusionEnabled(true);
    settings->setContextExplainabilityVisible(false);
    settings->setCompanionEnabled(true);
    settings->setDeveloperModeEnabled(true);

    settings->setAppLanguage(QStringLiteral("tr"));

    QVERIFY(settings->localChatInferenceEnabled());
    QVERIFY(settings->localInferenceStreamingEnabled());
    QVERIFY(settings->promptContextInjectionEnabled());
    QVERIFY(settings->semanticPromptInclusionEnabled());
    QVERIFY(!settings->contextExplainabilityVisible());
    QVERIFY(settings->companionEnabled());
    QVERIFY(settings->developerModeEnabled());
}

void AppSettingsTest::exposesRoutingModeDefault() {
    const auto settings = makeSettings();

    QCOMPARE(settings->routingModeName(), QStringLiteral("Local Only"));
    QCOMPARE(
        settings->availableRoutingModes(),
        QStringList({QStringLiteral("Auto"), QStringLiteral("Fast"), QStringLiteral("Balanced"),
                     QStringLiteral("Quality"), QStringLiteral("Local Only"),
                     QStringLiteral("Cloud Allowed"), QStringLiteral("Battery Saver")}));
}

void AppSettingsTest::updatesRoutingMode() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::routingModeNameChanged);

    settings->setRoutingModeName(QStringLiteral(" Balanced "));

    QCOMPARE(settings->routingModeName(), QStringLiteral("Balanced"));
    QCOMPARE(spy.count(), 1);

    settings->setRoutingModeName(QStringLiteral("Balanced"));
    QCOMPARE(spy.count(), 1);
}

void AppSettingsTest::fallsBackForInvalidRoutingMode() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::routingModeNameChanged);

    settings->setRoutingModeName(QStringLiteral("Balanced"));
    settings->setRoutingModeName(QStringLiteral("invalid-mode"));

    QCOMPARE(settings->routingModeName(), QStringLiteral("Local Only"));
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsRoutingModeThroughJsonStore() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("settings.json"));

    {
        AppSettings settings{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};
        settings.setRoutingModeName(QStringLiteral("Cloud Allowed"));
    }

    AppSettings reloaded{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};

    QCOMPARE(reloaded.routingModeName(), QStringLiteral("Cloud Allowed"));
}

void AppSettingsTest::exposesOllamaEndpointDefault() {
    const auto settings = makeSettings();

    QCOMPARE(settings->ollamaEndpoint(), QStringLiteral("http://127.0.0.1:11434"));
}

void AppSettingsTest::normalizesOllamaEndpoint() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::ollamaEndpointChanged);

    settings->setOllamaEndpoint(QStringLiteral(" http://localhost:11434/ "));

    QCOMPARE(settings->ollamaEndpoint(), QStringLiteral("http://localhost:11434"));
    QCOMPARE(spy.count(), 1);

    settings->setOllamaEndpoint(QStringLiteral("https://example.com"));

    QCOMPARE(settings->ollamaEndpoint(), QStringLiteral("http://127.0.0.1:11434"));
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsSelectedRuntimeProvider() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::selectedRuntimeProviderChanged);

    QCOMPARE(settings->selectedRuntimeProvider(), QStringLiteral("ollama"));

    settings->setSelectedRuntimeProvider(QStringLiteral(" openai-compatible "));

    QCOMPARE(settings->selectedRuntimeProvider(), QStringLiteral("openai-compatible"));
    QCOMPARE(spy.count(), 1);

    settings->setSelectedRuntimeProvider(QStringLiteral("claude"));
    QCOMPARE(settings->selectedRuntimeProvider(), QStringLiteral("claude"));
    QCOMPARE(spy.count(), 2);

    settings->setSelectedRuntimeProvider(QStringLiteral("gemini"));
    QCOMPARE(settings->selectedRuntimeProvider(), QStringLiteral("gemini"));
    QCOMPARE(spy.count(), 3);

    settings->setSelectedRuntimeProvider(QStringLiteral("unknown-provider"));

    QCOMPARE(settings->selectedRuntimeProvider(), QStringLiteral("ollama"));
    QCOMPARE(spy.count(), 4);
}

void AppSettingsTest::doesNotPersistApiKeyValuesForCloudPlaceholders() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("settings.json"));

    {
        AppSettings settings{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};
        settings.setSelectedRuntimeProvider(QStringLiteral("openai-compatible"));
        settings.setSelectedRuntimeProvider(QStringLiteral("claude"));
        settings.setSelectedRuntimeProvider(QStringLiteral("gemini"));
    }

    QFile file{filePath};
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto stored = QString::fromUtf8(file.readAll());
    QVERIFY(stored.contains(QStringLiteral("selectedRuntimeProvider")));
    QVERIFY(!stored.contains(QStringLiteral("apiKey"), Qt::CaseInsensitive));
    QVERIFY(!stored.contains(QStringLiteral("secret"), Qt::CaseInsensitive));
    QVERIFY(!stored.contains(QStringLiteral("token"), Qt::CaseInsensitive));
    QVERIFY(!stored.contains(QStringLiteral("sk-test-secret")));
}

void AppSettingsTest::persistsSelectedLocalModel() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::selectedLocalModelChanged);

    settings->setSelectedLocalModel(QStringLiteral(" llama3.2 "));

    QCOMPARE(settings->selectedLocalModel(), QStringLiteral("llama3.2"));
    QCOMPARE(spy.count(), 1);

    settings->setSelectedLocalModel(QStringLiteral("llama3.2"));
    QCOMPARE(spy.count(), 1);
}

void AppSettingsTest::persistsSelectedModelPerProvider() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::selectedLocalModelChanged);

    settings->setSelectedModelForProvider(QStringLiteral("ollama"),
                                          QStringLiteral(" llama3.2 "));
    settings->setSelectedModelForProvider(QStringLiteral("openai-compatible"),
                                          QStringLiteral(" future-model "));

    QCOMPARE(settings->selectedLocalModel(), QStringLiteral("llama3.2"));
    QCOMPARE(settings->selectedModelForProvider(QStringLiteral("ollama")),
             QStringLiteral("llama3.2"));
    QCOMPARE(settings->selectedModelForProvider(QStringLiteral("openai-compatible")),
             QStringLiteral("future-model"));
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsSelectedModelRoles() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::selectedModelRoleChanged);

    QVERIFY(settings->selectedModelForRole(QStringLiteral("coding")).isEmpty());

    settings->setSelectedModelForRole(QStringLiteral(" coding "),
                                      QStringLiteral(" qwen2.5-coder:7b "));

    QCOMPARE(settings->selectedModelForRole(QStringLiteral("coding")),
             QStringLiteral("qwen2.5-coder:7b"));
    QCOMPARE(spy.count(), 1);

    settings->setSelectedModelForRole(QStringLiteral("unknown-role"), QStringLiteral("ignored"));
    QVERIFY(settings->selectedModelForRole(QStringLiteral("unknown-role")).isEmpty());
    QCOMPARE(spy.count(), 1);
}

void AppSettingsTest::persistsLocalChatInferenceOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::localChatInferenceEnabledChanged);

    QVERIFY(!settings->localChatInferenceEnabled());

    settings->setLocalChatInferenceEnabled(true);

    QVERIFY(settings->localChatInferenceEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setLocalChatInferenceEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setLocalChatInferenceEnabled(false);
    QVERIFY(!settings->localChatInferenceEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsLocalInferenceStreamingOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::localInferenceStreamingEnabledChanged);

    QVERIFY(!settings->localInferenceStreamingEnabled());

    settings->setLocalInferenceStreamingEnabled(true);

    QVERIFY(settings->localInferenceStreamingEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setLocalInferenceStreamingEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setLocalInferenceStreamingEnabled(false);
    QVERIFY(!settings->localInferenceStreamingEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsPromptContextInjectionOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::promptContextInjectionEnabledChanged);

    QVERIFY(!settings->promptContextInjectionEnabled());

    settings->setPromptContextInjectionEnabled(true);

    QVERIFY(settings->promptContextInjectionEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setPromptContextInjectionEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setPromptContextInjectionEnabled(false);
    QVERIFY(!settings->promptContextInjectionEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsSemanticPromptInclusionOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::semanticPromptInclusionEnabledChanged);

    QVERIFY(!settings->semanticPromptInclusionEnabled());

    settings->setSemanticPromptInclusionEnabled(true);

    QVERIFY(settings->semanticPromptInclusionEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setSemanticPromptInclusionEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setSemanticPromptInclusionEnabled(false);
    QVERIFY(!settings->semanticPromptInclusionEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsContextExplainabilityVisibility() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::contextExplainabilityVisibleChanged);

    QVERIFY(settings->contextExplainabilityVisible());

    settings->setContextExplainabilityVisible(false);

    QVERIFY(!settings->contextExplainabilityVisible());
    QCOMPARE(spy.count(), 1);

    settings->setContextExplainabilityVisible(false);
    QCOMPARE(spy.count(), 1);

    settings->setContextExplainabilityVisible(true);
    QVERIFY(settings->contextExplainabilityVisible());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsCompanionVisibilityPreference() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::companionEnabledChanged);

    QVERIFY(!settings->companionEnabled());

    settings->setCompanionEnabled(true);

    QVERIFY(settings->companionEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setCompanionEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setCompanionEnabled(false);
    QVERIFY(!settings->companionEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsDeveloperModeVisibilityOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::developerModeEnabledChanged);

    QVERIFY(!settings->developerModeEnabled());

    settings->setDeveloperModeEnabled(true);

    QVERIFY(settings->developerModeEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setDeveloperModeEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setDeveloperModeEnabled(false);
    QVERIFY(!settings->developerModeEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsSelectedWorkspaceId() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::selectedWorkspaceIdChanged);

    settings->setSelectedWorkspaceId(QStringLiteral(" personal "));

    QCOMPARE(settings->selectedWorkspaceId(), QStringLiteral("personal"));
    QCOMPARE(spy.count(), 0);

    settings->setSelectedWorkspaceId(QStringLiteral("future-project"));
    QCOMPARE(settings->selectedWorkspaceId(), QStringLiteral("future-project"));
    QCOMPARE(spy.count(), 1);

    settings->setSelectedWorkspaceId(QStringLiteral("   "));
    QCOMPARE(settings->selectedWorkspaceId(), QStringLiteral("personal"));
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsWorkspaceDefaults() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::workspaceSettingsChanged);

    settings->setDefaultWorkspaceId(QStringLiteral("research"));
    settings->setWorkspaceCatalogJson(QStringLiteral("{\"version\":1,\"customWorkspaces\":[]}"));
    settings->setLocalKnowledgeBaseEnabled(true);
    settings->setRetrievalExplainabilityEnabled(false);
    settings->setAttachmentBehavior(QStringLiteral("paste"));
    settings->setExportDefaultFormat(QStringLiteral("docx"));
    settings->setExportIncludeTimestamps(false);
    settings->setExportIncludeCitations(false);
    settings->setExportAnonymizeNames(true);
    settings->setExportIncludeModelMetadata(false);

    QCOMPARE(settings->defaultWorkspaceId(), QStringLiteral("research"));
    QVERIFY(settings->workspaceCatalogJson().contains(QStringLiteral("customWorkspaces")));
    QVERIFY(settings->localKnowledgeBaseEnabled());
    QVERIFY(!settings->retrievalExplainabilityEnabled());
    QCOMPARE(settings->attachmentBehavior(), QStringLiteral("Paste Attachment Enabled"));
    QCOMPARE(settings->exportDefaultFormat(), QStringLiteral("DOCX"));
    QVERIFY(!settings->exportIncludeTimestamps());
    QVERIFY(!settings->exportIncludeCitations());
    QVERIFY(settings->exportAnonymizeNames());
    QVERIFY(!settings->exportIncludeModelMetadata());
    QCOMPARE(spy.count(), 10);
}

void AppSettingsTest::persistsSelectedSkillProfile() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::selectedSkillProfileChanged);

    settings->setSelectedSkillProfile(QStringLiteral(" student "));
    QCOMPARE(settings->selectedSkillProfile(), QStringLiteral("student"));
    QCOMPARE(spy.count(), 1);

    settings->setSelectedSkillProfile(QStringLiteral("unknown"));
    QCOMPARE(settings->selectedSkillProfile(), QStringLiteral("developer"));
    QCOMPARE(spy.count(), 2);

    settings->setSelectedSkillProfile(QStringLiteral("developer"));
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsDefaultPermissionPolicyState() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::defaultPermissionPolicyStateChanged);

    settings->setDefaultPermissionPolicyState(QStringLiteral(" ask-every-time "));
    QCOMPARE(settings->defaultPermissionPolicyState(), QStringLiteral("Ask Every Time"));
    QCOMPARE(spy.count(), 1);

    settings->setDefaultPermissionPolicyState(QStringLiteral("Enabled"));
    QCOMPARE(settings->defaultPermissionPolicyState(), QStringLiteral("Enabled"));
    QCOMPARE(spy.count(), 2);

    settings->setDefaultPermissionPolicyState(QStringLiteral("unknown"));
    QCOMPARE(settings->defaultPermissionPolicyState(), QStringLiteral("Disabled"));
    QCOMPARE(spy.count(), 3);

    settings->setDefaultPermissionPolicyState(QStringLiteral("Disabled"));
    QCOMPARE(spy.count(), 3);
}

void AppSettingsTest::persistsNativeExperiencePreferences() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("settings.json"));

    {
        AppSettings settings{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};
        QSignalSpy updateSpy(&settings, &AppSettings::updateCheckPolicyChanged);
        QSignalSpy notificationSpy(&settings, &AppSettings::notificationPolicyChanged);
        QSignalSpy onboardingSpy(&settings, &AppSettings::onboardingCompleteChanged);

        settings.setUpdateCheckPolicy(QStringLiteral(" weekly "));
        settings.setNotificationPolicy(QStringLiteral("all"));
        settings.setOnboardingUseCase(QStringLiteral("coding"));
        settings.setOnboardingComplete(true);
        settings.setRecoveryDraftText(QStringLiteral("draft"));

        QCOMPARE(settings.updateCheckPolicy(), QStringLiteral("Weekly"));
        QCOMPARE(settings.notificationPolicy(), QStringLiteral("All"));
        QCOMPARE(settings.onboardingUseCase(), QStringLiteral("Coding"));
        QVERIFY(settings.onboardingComplete());
        QCOMPARE(settings.recoveryDraftText(), QStringLiteral("draft"));
        QCOMPARE(updateSpy.count(), 1);
        QCOMPARE(notificationSpy.count(), 1);
        QCOMPARE(onboardingSpy.count(), 1);
    }

    AppSettings reloaded{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};
    QCOMPARE(reloaded.updateCheckPolicy(), QStringLiteral("Weekly"));
    QCOMPARE(reloaded.notificationPolicy(), QStringLiteral("All"));
    QCOMPARE(reloaded.onboardingUseCase(), QStringLiteral("Coding"));
    QVERIFY(reloaded.onboardingComplete());
    QCOMPARE(reloaded.recoveryDraftText(), QStringLiteral("draft"));

    reloaded.setUpdateCheckPolicy(QStringLiteral("unknown"));
    reloaded.setNotificationPolicy(QStringLiteral("unknown"));
    reloaded.setOnboardingUseCase(QStringLiteral("unknown"));
    QCOMPARE(reloaded.updateCheckPolicy(), QStringLiteral("Ask Before Checking"));
    QCOMPARE(reloaded.notificationPolicy(), QStringLiteral("Important Only"));
    QCOMPARE(reloaded.onboardingUseCase(), QStringLiteral("General Assistant"));
}

void AppSettingsTest::persistsVoiceConfigurationPaths() {
    const auto settings = makeSettings();
    QSignalSpy piperBinarySpy(settings.get(), &AppSettings::piperBinaryPathChanged);
    QSignalSpy piperModelSpy(settings.get(), &AppSettings::piperModelPathChanged);
    QSignalSpy whisperBinarySpy(settings.get(), &AppSettings::whisperBinaryPathChanged);
    QSignalSpy whisperModelSpy(settings.get(), &AppSettings::whisperModelPathChanged);

    settings->setPiperBinaryPath(QStringLiteral(" /tmp/piper "));
    settings->setPiperModelPath(QStringLiteral(" /tmp/piper.onnx "));
    settings->setWhisperBinaryPath(QStringLiteral(" /tmp/whisper "));
    settings->setWhisperModelPath(QStringLiteral(" /tmp/whisper-models "));

    QCOMPARE(settings->piperBinaryPath(), QStringLiteral("/tmp/piper"));
    QCOMPARE(settings->piperModelPath(), QStringLiteral("/tmp/piper.onnx"));
    QCOMPARE(settings->whisperBinaryPath(), QStringLiteral("/tmp/whisper"));
    QCOMPARE(settings->whisperModelPath(), QStringLiteral("/tmp/whisper-models"));
    QCOMPARE(piperBinarySpy.count(), 1);
    QCOMPARE(piperModelSpy.count(), 1);
    QCOMPARE(whisperBinarySpy.count(), 1);
    QCOMPARE(whisperModelSpy.count(), 1);
}

void AppSettingsTest::persistsPiperFileOutputExecutionOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::piperFileOutputExecutionEnabledChanged);

    QVERIFY(!settings->piperFileOutputExecutionEnabled());

    settings->setPiperFileOutputExecutionEnabled(true);

    QVERIFY(settings->piperFileOutputExecutionEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setPiperFileOutputExecutionEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setPiperFileOutputExecutionEnabled(false);
    QVERIFY(!settings->piperFileOutputExecutionEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsLocalAiRuntimeSettingsThroughJsonStore() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("settings.json"));

    {
        AppSettings settings{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};
        settings.setSelectedRuntimeProvider(QStringLiteral("openai-compatible"));
        settings.setSelectedLocalModel(QStringLiteral(" llama3.2 "));
        settings.setLocalChatInferenceEnabled(true);
        settings.setLocalInferenceStreamingEnabled(true);
        settings.setPromptContextInjectionEnabled(true);
        settings.setSemanticPromptInclusionEnabled(true);
        settings.setContextExplainabilityVisible(false);
        settings.setCompanionEnabled(true);
        settings.setDeveloperModeEnabled(true);
        settings.setSelectedWorkspaceId(QStringLiteral("future-project"));
        settings.setDefaultWorkspaceId(QStringLiteral("future-project"));
        settings.setLocalKnowledgeBaseEnabled(true);
        settings.setRetrievalExplainabilityEnabled(false);
        settings.setExportDefaultFormat(QStringLiteral("json"));
        settings.setSelectedSkillProfile(QStringLiteral("researcher"));
        settings.setDefaultPermissionPolicyState(QStringLiteral("Trusted"));
        settings.setPiperBinaryPath(QStringLiteral("/opt/piper/piper"));
        settings.setPiperModelPath(QStringLiteral("/opt/piper/model.onnx"));
        settings.setWhisperBinaryPath(QStringLiteral("/opt/whisper/whisper"));
        settings.setWhisperModelPath(QStringLiteral("/opt/whisper/models"));
        settings.setPiperFileOutputExecutionEnabled(true);
    }

    AppSettings reloaded{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};

    QCOMPARE(reloaded.selectedRuntimeProvider(), QStringLiteral("openai-compatible"));
    QCOMPARE(reloaded.selectedLocalModel(), QStringLiteral("llama3.2"));
    QVERIFY(reloaded.localChatInferenceEnabled());
    QVERIFY(reloaded.localInferenceStreamingEnabled());
    QVERIFY(reloaded.promptContextInjectionEnabled());
    QVERIFY(reloaded.semanticPromptInclusionEnabled());
    QVERIFY(!reloaded.contextExplainabilityVisible());
    QVERIFY(reloaded.companionEnabled());
    QVERIFY(reloaded.developerModeEnabled());
    QCOMPARE(reloaded.selectedWorkspaceId(), QStringLiteral("future-project"));
    QCOMPARE(reloaded.defaultWorkspaceId(), QStringLiteral("future-project"));
    QVERIFY(reloaded.localKnowledgeBaseEnabled());
    QVERIFY(!reloaded.retrievalExplainabilityEnabled());
    QCOMPARE(reloaded.exportDefaultFormat(), QStringLiteral("JSON"));
    QCOMPARE(reloaded.selectedSkillProfile(), QStringLiteral("researcher"));
    QCOMPARE(reloaded.defaultPermissionPolicyState(), QStringLiteral("Trusted"));
    QCOMPARE(reloaded.piperBinaryPath(), QStringLiteral("/opt/piper/piper"));
    QCOMPARE(reloaded.piperModelPath(), QStringLiteral("/opt/piper/model.onnx"));
    QCOMPARE(reloaded.whisperBinaryPath(), QStringLiteral("/opt/whisper/whisper"));
    QCOMPARE(reloaded.whisperModelPath(), QStringLiteral("/opt/whisper/models"));
    QVERIFY(reloaded.piperFileOutputExecutionEnabled());
}

QTEST_MAIN(AppSettingsTest)

#include "test_app_settings.moc"
