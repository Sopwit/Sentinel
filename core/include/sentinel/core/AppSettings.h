#pragma once

#include "sentinel/core/ISettingsStore.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

namespace sentinel::core {

class AppSettings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY themeNameChanged)
    Q_PROPERTY(QString configurationProfile READ configurationProfile WRITE setConfigurationProfile
                   NOTIFY configurationProfileChanged)
    Q_PROPERTY(QString appLanguage READ appLanguage WRITE setAppLanguage NOTIFY appLanguageChanged)
    Q_PROPERTY(QStringList availableLanguages READ availableLanguages CONSTANT)
    Q_PROPERTY(QString routingModeName READ routingModeName WRITE setRoutingModeName NOTIFY
                   routingModeNameChanged)
    Q_PROPERTY(QString ollamaEndpoint READ ollamaEndpoint WRITE setOllamaEndpoint NOTIFY
                   ollamaEndpointChanged)
    Q_PROPERTY(QString selectedRuntimeProvider READ selectedRuntimeProvider WRITE
                   setSelectedRuntimeProvider NOTIFY selectedRuntimeProviderChanged)
    Q_PROPERTY(QString selectedLocalModel READ selectedLocalModel WRITE setSelectedLocalModel NOTIFY
                   selectedLocalModelChanged)
    Q_PROPERTY(bool localChatInferenceEnabled READ localChatInferenceEnabled WRITE
                   setLocalChatInferenceEnabled NOTIFY localChatInferenceEnabledChanged)
    Q_PROPERTY(bool localInferenceStreamingEnabled READ localInferenceStreamingEnabled WRITE
                   setLocalInferenceStreamingEnabled NOTIFY localInferenceStreamingEnabledChanged)
    Q_PROPERTY(int localInferenceTimeoutMs READ localInferenceTimeoutMs WRITE
                   setLocalInferenceTimeoutMs NOTIFY localInferenceTimeoutMsChanged)
    Q_PROPERTY(double localInferenceTemperature READ localInferenceTemperature WRITE
                   setLocalInferenceTemperature NOTIFY localInferenceTemperatureChanged)
    Q_PROPERTY(double localInferenceTopP READ localInferenceTopP WRITE
                   setLocalInferenceTopP NOTIFY localInferenceTopPChanged)
    Q_PROPERTY(int localInferenceMaxTokens READ localInferenceMaxTokens WRITE
                   setLocalInferenceMaxTokens NOTIFY localInferenceMaxTokensChanged)
    Q_PROPERTY(bool promptContextInjectionEnabled READ promptContextInjectionEnabled WRITE
                   setPromptContextInjectionEnabled NOTIFY promptContextInjectionEnabledChanged)
    Q_PROPERTY(bool semanticPromptInclusionEnabled READ semanticPromptInclusionEnabled WRITE
                   setSemanticPromptInclusionEnabled NOTIFY semanticPromptInclusionEnabledChanged)
    Q_PROPERTY(bool contextExplainabilityVisible READ contextExplainabilityVisible WRITE
                   setContextExplainabilityVisible NOTIFY contextExplainabilityVisibleChanged)
    Q_PROPERTY(bool companionEnabled READ companionEnabled WRITE setCompanionEnabled NOTIFY
                   companionEnabledChanged)
    Q_PROPERTY(bool developerModeEnabled READ developerModeEnabled WRITE setDeveloperModeEnabled
                   NOTIFY developerModeEnabledChanged)
    Q_PROPERTY(bool agentAutonomousMode READ agentAutonomousMode WRITE setAgentAutonomousMode NOTIFY
                   agentAutonomousModeChanged)
    Q_PROPERTY(QString piperBinaryPath READ piperBinaryPath WRITE setPiperBinaryPath NOTIFY
                   piperBinaryPathChanged)
    Q_PROPERTY(QString piperModelPath READ piperModelPath WRITE setPiperModelPath NOTIFY
                   piperModelPathChanged)
    Q_PROPERTY(QString whisperBinaryPath READ whisperBinaryPath WRITE setWhisperBinaryPath NOTIFY
                   whisperBinaryPathChanged)
    Q_PROPERTY(QString whisperModelPath READ whisperModelPath WRITE setWhisperModelPath NOTIFY
                   whisperModelPathChanged)
    Q_PROPERTY(bool piperFileOutputExecutionEnabled READ piperFileOutputExecutionEnabled WRITE
                   setPiperFileOutputExecutionEnabled NOTIFY piperFileOutputExecutionEnabledChanged)
    Q_PROPERTY(QString selectedTtsEngine READ selectedTtsEngine WRITE setSelectedTtsEngine NOTIFY
                   selectedTtsEngineChanged)
    Q_PROPERTY(QString kokoroModelPath READ kokoroModelPath WRITE setKokoroModelPath NOTIFY
                   kokoroModelPathChanged)
    Q_PROPERTY(QString kokoroVoice READ kokoroVoice WRITE setKokoroVoice NOTIFY kokoroVoiceChanged)
    Q_PROPERTY(QString activeConversationId READ activeConversationId WRITE setActiveConversationId
                   NOTIFY activeConversationIdChanged)
    Q_PROPERTY(QString selectedWorkspaceId READ selectedWorkspaceId WRITE setSelectedWorkspaceId
                   NOTIFY selectedWorkspaceIdChanged)
    Q_PROPERTY(QString defaultWorkspaceId READ defaultWorkspaceId WRITE setDefaultWorkspaceId NOTIFY
                   workspaceSettingsChanged)
    Q_PROPERTY(QString workspaceCatalogJson READ workspaceCatalogJson WRITE setWorkspaceCatalogJson
                   NOTIFY workspaceSettingsChanged)
    Q_PROPERTY(bool localKnowledgeBaseEnabled READ localKnowledgeBaseEnabled WRITE
                   setLocalKnowledgeBaseEnabled NOTIFY workspaceSettingsChanged)
    Q_PROPERTY(bool retrievalExplainabilityEnabled READ retrievalExplainabilityEnabled WRITE
                   setRetrievalExplainabilityEnabled NOTIFY workspaceSettingsChanged)
    Q_PROPERTY(QString attachmentBehavior READ attachmentBehavior WRITE setAttachmentBehavior NOTIFY
                   workspaceSettingsChanged)
    Q_PROPERTY(QString exportDefaultFormat READ exportDefaultFormat WRITE setExportDefaultFormat
                   NOTIFY workspaceSettingsChanged)
    Q_PROPERTY(bool exportIncludeTimestamps READ exportIncludeTimestamps WRITE
                   setExportIncludeTimestamps NOTIFY workspaceSettingsChanged)
    Q_PROPERTY(bool exportIncludeCitations READ exportIncludeCitations WRITE
                   setExportIncludeCitations NOTIFY workspaceSettingsChanged)
    Q_PROPERTY(bool exportAnonymizeNames READ exportAnonymizeNames WRITE setExportAnonymizeNames
                   NOTIFY workspaceSettingsChanged)
    Q_PROPERTY(bool exportIncludeModelMetadata READ exportIncludeModelMetadata WRITE
                   setExportIncludeModelMetadata NOTIFY workspaceSettingsChanged)
    Q_PROPERTY(QString selectedSkillProfile READ selectedSkillProfile WRITE setSelectedSkillProfile
                   NOTIFY selectedSkillProfileChanged)
    Q_PROPERTY(QString defaultPermissionPolicyState READ defaultPermissionPolicyState WRITE
                   setDefaultPermissionPolicyState NOTIFY defaultPermissionPolicyStateChanged)
    Q_PROPERTY(QString updateCheckPolicy READ updateCheckPolicy WRITE setUpdateCheckPolicy NOTIFY
                   updateCheckPolicyChanged)
    Q_PROPERTY(QString notificationPolicy READ notificationPolicy WRITE setNotificationPolicy NOTIFY
                   notificationPolicyChanged)
    Q_PROPERTY(bool onboardingComplete READ onboardingComplete WRITE setOnboardingComplete NOTIFY
                   onboardingCompleteChanged)
    Q_PROPERTY(QString onboardingUseCase READ onboardingUseCase WRITE setOnboardingUseCase NOTIFY
                   onboardingUseCaseChanged)
    Q_PROPERTY(QString selectedSystemMode READ selectedSystemMode WRITE setSelectedSystemMode NOTIFY
                   selectedSystemModeChanged)
    Q_PROPERTY(QString onboardingAiProvider READ onboardingAiProvider WRITE setOnboardingAiProvider
                   NOTIFY onboardingUseCaseChanged)
    Q_PROPERTY(QString recoveryDraftText READ recoveryDraftText WRITE setRecoveryDraftText NOTIFY
                   recoveryDraftTextChanged)
    Q_PROPERTY(bool reducedMotionEnabled READ reducedMotionEnabled WRITE setReducedMotionEnabled
                   NOTIFY productExperienceChanged)
    Q_PROPERTY(bool highContrastEnabled READ highContrastEnabled WRITE setHighContrastEnabled NOTIFY
                   productExperienceChanged)
    Q_PROPERTY(QString uiDensity READ uiDensity WRITE setUiDensity NOTIFY productExperienceChanged)
    Q_PROPERTY(QString notificationCenterJson READ notificationCenterJson WRITE
                   setNotificationCenterJson NOTIFY productExperienceChanged)
    Q_PROPERTY(QString updateWorkflowState READ updateWorkflowState WRITE setUpdateWorkflowState
                   NOTIFY productExperienceChanged)
    Q_PROPERTY(QString controlledAgentTasksJson READ controlledAgentTasksJson WRITE
                   setControlledAgentTasksJson NOTIFY controlledAgentTasksChanged)
    Q_PROPERTY(QString controlledAgentPermissionsJson READ controlledAgentPermissionsJson WRITE
                   setControlledAgentPermissionsJson NOTIFY controlledAgentTasksChanged)
    Q_PROPERTY(bool notifyModelDownloads READ notifyModelDownloads WRITE setNotifyModelDownloads
                   NOTIFY productExperienceChanged)
    Q_PROPERTY(bool notifyModelRemovals READ notifyModelRemovals WRITE setNotifyModelRemovals NOTIFY
                   productExperienceChanged)
    Q_PROPERTY(bool notifyAgentResponses READ notifyAgentResponses WRITE setNotifyAgentResponses
                   NOTIFY productExperienceChanged)
    Q_PROPERTY(bool notifySystemUpdates READ notifySystemUpdates WRITE setNotifySystemUpdates NOTIFY
                   productExperienceChanged)

public:
    explicit AppSettings(std::unique_ptr<ISettingsStore> store, QObject* parent = nullptr);

    QString themeName() const;
    void setThemeName(const QString& themeName);

    QString configurationProfile() const;
    void setConfigurationProfile(const QString& configurationProfile);

    QString appLanguage() const;
    void setAppLanguage(const QString& language);
    QStringList availableLanguages() const;
    QString languageDisplayName(const QString& language) const;

    QString routingModeName() const;
    void setRoutingModeName(const QString& routingModeName);
    QStringList availableRoutingModes() const;

    QString ollamaEndpoint() const;
    void setOllamaEndpoint(const QString& endpoint);
    QString selectedRuntimeProvider() const;
    void setSelectedRuntimeProvider(const QString& providerId);

    QString selectedLocalModel() const;
    void setSelectedLocalModel(const QString& model);
    QString selectedModelForProvider(const QString& providerId) const;
    void setSelectedModelForProvider(const QString& providerId, const QString& model);
    QString selectedModelForRole(const QString& roleId) const;
    void setSelectedModelForRole(const QString& roleId, const QString& modelId);

    bool localChatInferenceEnabled() const;
    void setLocalChatInferenceEnabled(bool enabled);
    bool localInferenceStreamingEnabled() const;
    void setLocalInferenceStreamingEnabled(bool enabled);
    int localInferenceTimeoutMs() const;
    void setLocalInferenceTimeoutMs(int timeoutMs);
    double localInferenceTemperature() const;
    void setLocalInferenceTemperature(double temperature);
    double localInferenceTopP() const;
    void setLocalInferenceTopP(double topP);
    int localInferenceMaxTokens() const;
    void setLocalInferenceMaxTokens(int maxTokens);
    bool promptContextInjectionEnabled() const;
    void setPromptContextInjectionEnabled(bool enabled);
    bool semanticPromptInclusionEnabled() const;
    void setSemanticPromptInclusionEnabled(bool enabled);
    bool contextExplainabilityVisible() const;
    void setContextExplainabilityVisible(bool visible);
    bool companionEnabled() const;
    void setCompanionEnabled(bool enabled);
    bool developerModeEnabled() const;
    void setDeveloperModeEnabled(bool enabled);
    bool agentAutonomousMode() const;
    void setAgentAutonomousMode(bool enabled);
    QString piperBinaryPath() const;
    void setPiperBinaryPath(const QString& path);
    QString piperModelPath() const;
    void setPiperModelPath(const QString& path);
    QString whisperBinaryPath() const;
    void setWhisperBinaryPath(const QString& path);
    QString whisperModelPath() const;
    void setWhisperModelPath(const QString& path);
    bool piperFileOutputExecutionEnabled() const;
    void setPiperFileOutputExecutionEnabled(bool enabled);
    QString selectedTtsEngine() const;
    void setSelectedTtsEngine(const QString& engine);
    QString kokoroModelPath() const;
    void setKokoroModelPath(const QString& path);
    QString kokoroVoice() const;
    void setKokoroVoice(const QString& voice);
    QString activeConversationId() const;
    void setActiveConversationId(const QString& conversationId);
    QString selectedWorkspaceId() const;
    void setSelectedWorkspaceId(const QString& workspaceId);
    QString defaultWorkspaceId() const;
    void setDefaultWorkspaceId(const QString& workspaceId);
    QString workspaceCatalogJson() const;
    void setWorkspaceCatalogJson(const QString& catalogJson);
    bool localKnowledgeBaseEnabled() const;
    void setLocalKnowledgeBaseEnabled(bool enabled);
    bool retrievalExplainabilityEnabled() const;
    void setRetrievalExplainabilityEnabled(bool enabled);
    QString attachmentBehavior() const;
    void setAttachmentBehavior(const QString& behavior);
    QString exportDefaultFormat() const;
    void setExportDefaultFormat(const QString& format);
    bool exportIncludeTimestamps() const;
    void setExportIncludeTimestamps(bool enabled);
    bool exportIncludeCitations() const;
    void setExportIncludeCitations(bool enabled);
    bool exportAnonymizeNames() const;
    void setExportAnonymizeNames(bool enabled);
    bool exportIncludeModelMetadata() const;
    void setExportIncludeModelMetadata(bool enabled);
    QString selectedSkillProfile() const;
    void setSelectedSkillProfile(const QString& profileId);
    QString defaultPermissionPolicyState() const;
    void setDefaultPermissionPolicyState(const QString& state);
    QString updateCheckPolicy() const;
    void setUpdateCheckPolicy(const QString& policy);
    QString notificationPolicy() const;
    void setNotificationPolicy(const QString& policy);
    bool onboardingComplete() const;
    void setOnboardingComplete(bool complete);
    QString onboardingUseCase() const;
    void setOnboardingUseCase(const QString& useCase);
    QString onboardingAiProvider() const;
    void setOnboardingAiProvider(const QString& provider);
    QString selectedSystemMode() const;
    void setSelectedSystemMode(const QString& mode);
    QString recoveryDraftText() const;
    void setRecoveryDraftText(const QString& text);
    bool reducedMotionEnabled() const;
    void setReducedMotionEnabled(bool enabled);
    bool highContrastEnabled() const;
    void setHighContrastEnabled(bool enabled);
    QString uiDensity() const;
    void setUiDensity(const QString& density);
    QString notificationCenterJson() const;
    void setNotificationCenterJson(const QString& json);
    QString updateWorkflowState() const;
    void setUpdateWorkflowState(const QString& state);
    QString controlledAgentTasksJson() const;
    void setControlledAgentTasksJson(const QString& json);
    QString controlledAgentPermissionsJson() const;
    void setControlledAgentPermissionsJson(const QString& json);

    bool notifyModelDownloads() const;
    void setNotifyModelDownloads(bool enabled);
    bool notifyModelRemovals() const;
    void setNotifyModelRemovals(bool enabled);
    bool notifyAgentResponses() const;
    void setNotifyAgentResponses(bool enabled);
    bool notifySystemUpdates() const;
    void setNotifySystemUpdates(bool enabled);

signals:
    void themeNameChanged();
    void configurationProfileChanged();
    void appLanguageChanged();
    void routingModeNameChanged();
    void ollamaEndpointChanged();
    void selectedRuntimeProviderChanged();
    void selectedLocalModelChanged();
    void selectedModelRoleChanged();
    void localChatInferenceEnabledChanged();
    void localInferenceStreamingEnabledChanged();
    void localInferenceTimeoutMsChanged();
    void localInferenceTemperatureChanged();
    void localInferenceTopPChanged();
    void localInferenceMaxTokensChanged();
    void promptContextInjectionEnabledChanged();
    void semanticPromptInclusionEnabledChanged();
    void contextExplainabilityVisibleChanged();
    void companionEnabledChanged();
    void developerModeEnabledChanged();
    void agentAutonomousModeChanged();
    void piperBinaryPathChanged();
    void piperModelPathChanged();
    void whisperBinaryPathChanged();
    void whisperModelPathChanged();
    void piperFileOutputExecutionEnabledChanged();
    void selectedTtsEngineChanged();
    void kokoroModelPathChanged();
    void kokoroVoiceChanged();
    void activeConversationIdChanged();
    void selectedWorkspaceIdChanged();
    void workspaceSettingsChanged();
    void selectedSkillProfileChanged();
    void defaultPermissionPolicyStateChanged();
    void updateCheckPolicyChanged();
    void notificationPolicyChanged();
    void onboardingCompleteChanged();
    void onboardingUseCaseChanged();
    void selectedSystemModeChanged();
    void recoveryDraftTextChanged();
    void productExperienceChanged();
    void controlledAgentTasksChanged();

private:
    static constexpr auto themeNameKey = "themeName";
    static constexpr auto configurationProfileKey = "configurationProfile";
    static constexpr auto appLanguageKey = "appLanguage";
    static constexpr auto routingModeKey = "routingMode";
    static constexpr auto ollamaEndpointKey = "ollamaEndpoint";
    static constexpr auto selectedRuntimeProviderKey = "selectedRuntimeProvider";
    static constexpr auto selectedLocalModelKey = "selectedLocalModel";
    static constexpr auto selectedProviderModelKeyPrefix = "selectedModel.";
    static constexpr auto selectedRoleModelKeyPrefix = "selectedModelRole.";
    static constexpr auto localChatInferenceEnabledKey = "localChatInferenceEnabled";
    static constexpr auto localInferenceStreamingEnabledKey = "localInferenceStreamingEnabled";
    static constexpr auto localInferenceTimeoutMsKey = "localInferenceTimeoutMs";
    static constexpr auto localInferenceTemperatureKey = "localInferenceTemperature";
    static constexpr auto localInferenceTopPKey = "localInferenceTopP";
    static constexpr auto localInferenceMaxTokensKey = "localInferenceMaxTokens";
    static constexpr auto promptContextInjectionEnabledKey = "promptContextInjectionEnabled";
    static constexpr auto semanticPromptInclusionEnabledKey = "semanticPromptInclusionEnabled";
    static constexpr auto contextExplainabilityVisibleKey = "contextExplainabilityVisible";
    static constexpr auto companionEnabledKey = "companionEnabled";
    static constexpr auto developerModeEnabledKey = "developerModeEnabled";
    static constexpr auto agentAutonomousModeKey = "agentAutonomousMode";
    static constexpr auto piperBinaryPathKey = "piperBinaryPath";
    static constexpr auto piperModelPathKey = "piperModelPath";
    static constexpr auto whisperBinaryPathKey = "whisperBinaryPath";
    static constexpr auto whisperModelPathKey = "whisperModelPath";
    static constexpr auto piperFileOutputExecutionEnabledKey = "piperFileOutputExecutionEnabled";
    static constexpr auto selectedTtsEngineKey = "selectedTtsEngine";
    static constexpr auto kokoroModelPathKey = "kokoroModelPath";
    static constexpr auto kokoroVoiceKey = "kokoroVoice";
    static constexpr auto activeConversationIdKey = "activeConversationId";
    static constexpr auto selectedWorkspaceIdKey = "selectedWorkspaceId";
    static constexpr auto defaultWorkspaceIdKey = "defaultWorkspaceId";
    static constexpr auto workspaceCatalogJsonKey = "workspaceCatalogJson";
    static constexpr auto localKnowledgeBaseEnabledKey = "localKnowledgeBaseEnabled";
    static constexpr auto retrievalExplainabilityEnabledKey = "retrievalExplainabilityEnabled";
    static constexpr auto attachmentBehaviorKey = "attachmentBehavior";
    static constexpr auto exportDefaultFormatKey = "exportDefaultFormat";
    static constexpr auto exportIncludeTimestampsKey = "exportIncludeTimestamps";
    static constexpr auto exportIncludeCitationsKey = "exportIncludeCitations";
    static constexpr auto exportAnonymizeNamesKey = "exportAnonymizeNames";
    static constexpr auto exportIncludeModelMetadataKey = "exportIncludeModelMetadata";
    static constexpr auto selectedSkillProfileKey = "selectedSkillProfile";
    static constexpr auto defaultPermissionPolicyStateKey = "defaultPermissionPolicyState";
    static constexpr auto updateCheckPolicyKey = "updateCheckPolicy";
    static constexpr auto notificationPolicyKey = "notificationPolicy";
    static constexpr auto onboardingCompleteKey = "onboardingComplete";
    static constexpr auto onboardingUseCaseKey = "onboardingUseCase";
    static constexpr auto onboardingAiProviderKey = "onboardingAiProvider";
    static constexpr auto selectedSystemModeKey = "selectedSystemMode";
    static constexpr auto recoveryDraftTextKey = "recoveryDraftText";
    static constexpr auto reducedMotionEnabledKey = "reducedMotionEnabled";
    static constexpr auto highContrastEnabledKey = "highContrastEnabled";
    static constexpr auto uiDensityKey = "uiDensity";
    static constexpr auto notificationCenterJsonKey = "notificationCenterJson";
    static constexpr auto updateWorkflowStateKey = "updateWorkflowState";
    static constexpr auto controlledAgentTasksJsonKey = "controlledAgentTasksJson";
    static constexpr auto controlledAgentPermissionsJsonKey = "controlledAgentPermissionsJson";
    static constexpr auto notifyModelDownloadsKey = "notifyModelDownloads";
    static constexpr auto notifyModelRemovalsKey = "notifyModelRemovals";
    static constexpr auto notifyAgentResponsesKey = "notifyAgentResponses";
    static constexpr auto notifySystemUpdatesKey = "notifySystemUpdates";
    static constexpr auto defaultThemeName = "Liquid Glass Light";
    static constexpr auto defaultConfigurationProfile = "Desktop Alpha";
    static constexpr auto defaultAppLanguage = "en";
    static constexpr auto defaultSelectedRuntimeProvider = "ollama";
    static constexpr auto defaultSelectedWorkspaceId = "personal";
    static constexpr auto defaultAttachmentBehavior = "Manual Attachments Only";
    static constexpr auto defaultExportFormat = "Markdown";
    static constexpr auto defaultSelectedSkillProfile = "personal-assistant";
    static constexpr auto defaultPermissionPolicyStateValue = "Disabled";
    static constexpr auto defaultUpdateCheckPolicy = "Ask Before Checking";
    static constexpr auto defaultNotificationPolicy = "Important Only";
    static constexpr auto defaultOnboardingUseCase = "General Assistant";
    static constexpr auto defaultOnboardingAiProvider = "Ollama";
    static constexpr auto defaultUiDensity = "Comfortable";
    static constexpr auto defaultUpdateWorkflowState = "Not Checked";
    static constexpr auto defaultSelectedTtsEngine = "Piper";
    static constexpr auto defaultKokoroVoice = "af_bella";
    static constexpr int defaultLocalInferenceTimeoutMs = 30000;
    static constexpr double defaultLocalInferenceTemperature = 0.7;
    static constexpr double defaultLocalInferenceTopP = 0.9;
    static constexpr int defaultLocalInferenceMaxTokens = 2048;

    std::unique_ptr<ISettingsStore> store_;
};

} // namespace sentinel::core
