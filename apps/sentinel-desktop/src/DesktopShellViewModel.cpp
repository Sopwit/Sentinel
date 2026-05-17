#include "sentinel/desktop/DesktopShellViewModel.h"

#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/ModeManager.h"

namespace sentinel::desktop {

DesktopShellViewModel::DesktopShellViewModel(core::ApplicationController& controller,
                                             core::ModeManager& modeManager,
                                             core::AppSettings& settings, QObject* parent)
    : QObject(parent), controller_(controller), modeManager_(modeManager), settings_(settings),
      chatMessages_(this) {
    controller_.setRoutingModeByName(settings_.routingModeName());
    chatMessages_.setMessages(controller_.chatHistory());
    connect(&controller_, &core::ApplicationController::chatMessagesChanged, this, [this]() {
        chatMessages_.setMessages(controller_.chatHistory());
        emit chatMessagesChanged();
    });
    connect(&controller_, &core::ApplicationController::memoryEntriesChanged, this,
            &DesktopShellViewModel::memoryEntriesChanged);
    connect(&controller_, &core::ApplicationController::maintenanceStatusChanged, this,
            &DesktopShellViewModel::maintenanceStatusChanged);
    connect(&controller_, &core::ApplicationController::agentStatusChanged, this,
            &DesktopShellViewModel::agentStatusChanged);
    connect(&controller_, &core::ApplicationController::agentResponseChanged, this,
            &DesktopShellViewModel::agentResponseChanged);
    connect(&controller_, &core::ApplicationController::toolPlanChanged, this,
            &DesktopShellViewModel::toolPlanChanged);
    connect(&controller_, &core::ApplicationController::approvalChanged, this,
            &DesktopShellViewModel::approvalChanged);
    connect(&controller_, &core::ApplicationController::sandboxChanged, this,
            &DesktopShellViewModel::sandboxChanged);
    connect(&controller_, &core::ApplicationController::toolExecutionChanged, this,
            &DesktopShellViewModel::toolExecutionChanged);
    connect(&controller_, &core::ApplicationController::agentPipelineChanged, this,
            &DesktopShellViewModel::agentPipelineChanged);
    connect(&controller_, &core::ApplicationController::runtimeContextChanged, this,
            &DesktopShellViewModel::runtimeContextChanged);
    connect(&controller_, &core::ApplicationController::conversationSessionChanged, this,
            &DesktopShellViewModel::conversationSessionChanged);
    connect(&controller_, &core::ApplicationController::conversationStateChanged, this,
            &DesktopShellViewModel::conversationStateChanged);
    connect(&controller_, &core::ApplicationController::agentActivityChanged, this,
            &DesktopShellViewModel::agentActivityChanged);
    connect(&controller_, &core::ApplicationController::modelRoutingChanged, this,
            &DesktopShellViewModel::modelRoutingChanged);
    connect(&controller_, &core::ApplicationController::taskPlanChanged, this,
            &DesktopShellViewModel::taskPlanChanged);
    connect(&controller_, &core::ApplicationController::orchestrationSnapshotChanged, this,
            &DesktopShellViewModel::orchestrationSnapshotChanged);
    connect(&controller_, &core::ApplicationController::localModelSelectionChanged, this,
            &DesktopShellViewModel::localModelSelectionChanged);
    connect(&controller_, &core::ApplicationController::localChatInferenceRoutingChanged, this,
            &DesktopShellViewModel::localChatInferenceRoutingChanged);
    connect(&controller_, &core::ApplicationController::localInferenceChanged, this,
            &DesktopShellViewModel::localInferenceChanged);
    connect(&controller_, &core::ApplicationController::voiceConfigurationChanged, this,
            &DesktopShellViewModel::voiceConfigurationChanged);
    connect(&modeManager_, &core::ModeManager::currentModeChanged, this,
            &DesktopShellViewModel::currentModeChanged);
    connect(&settings_, &core::AppSettings::themeNameChanged, this,
            &DesktopShellViewModel::themeNameChanged);
    connect(&settings_, &core::AppSettings::configurationProfileChanged, this,
            &DesktopShellViewModel::configurationProfileChanged);
    connect(&settings_, &core::AppSettings::routingModeNameChanged, this,
            &DesktopShellViewModel::modelRoutingChanged);
    connect(&settings_, &core::AppSettings::selectedLocalModelChanged, this,
            [this]() { controller_.setSelectedLocalModel(settings_.selectedLocalModel()); });
    connect(&settings_, &core::AppSettings::localChatInferenceEnabledChanged, this, [this]() {
        controller_.setLocalChatInferenceEnabled(settings_.localChatInferenceEnabled());
    });
    connect(&settings_, &core::AppSettings::localInferenceStreamingEnabledChanged, this, [this]() {
        controller_.setLocalInferenceStreamingEnabled(settings_.localInferenceStreamingEnabled());
    });
    connect(&settings_, &core::AppSettings::piperBinaryPathChanged, this,
            [this]() { controller_.setPiperBinaryPath(settings_.piperBinaryPath()); });
    connect(&settings_, &core::AppSettings::piperModelPathChanged, this,
            [this]() { controller_.setPiperModelPath(settings_.piperModelPath()); });
    connect(&settings_, &core::AppSettings::whisperBinaryPathChanged, this,
            [this]() { controller_.setWhisperBinaryPath(settings_.whisperBinaryPath()); });
    connect(&settings_, &core::AppSettings::whisperModelPathChanged, this,
            [this]() { controller_.setWhisperModelPath(settings_.whisperModelPath()); });
    controller_.setSelectedLocalModel(settings_.selectedLocalModel());
    controller_.setLocalChatInferenceEnabled(settings_.localChatInferenceEnabled());
    controller_.setLocalInferenceStreamingEnabled(settings_.localInferenceStreamingEnabled());
    controller_.setPiperBinaryPath(settings_.piperBinaryPath());
    controller_.setPiperModelPath(settings_.piperModelPath());
    controller_.setWhisperBinaryPath(settings_.whisperBinaryPath());
    controller_.setWhisperModelPath(settings_.whisperModelPath());
}

QString DesktopShellViewModel::providerName() const {
    return controller_.providerName();
}

QString DesktopShellViewModel::providerStatus() const {
    return controller_.providerStatus();
}

QString DesktopShellViewModel::agentStatus() const {
    return controller_.agentStatus();
}

QString DesktopShellViewModel::lastAgentResponse() const {
    return controller_.lastAgentResponse();
}

QString DesktopShellViewModel::latestToolPlanStatus() const {
    return controller_.latestToolPlanStatus();
}

QString DesktopShellViewModel::latestToolPlanSummary() const {
    return controller_.latestToolPlanSummary();
}

QString DesktopShellViewModel::latestApprovalStatus() const {
    return controller_.latestApprovalStatus();
}

QString DesktopShellViewModel::latestApprovalSummary() const {
    return controller_.latestApprovalSummary();
}

QString DesktopShellViewModel::latestSandboxStatus() const {
    return controller_.latestSandboxStatus();
}

QString DesktopShellViewModel::latestSandboxSummary() const {
    return controller_.latestSandboxSummary();
}

QString DesktopShellViewModel::latestToolExecutionStatus() const {
    return controller_.latestToolExecutionStatus();
}

QString DesktopShellViewModel::latestToolExecutionSummary() const {
    return controller_.latestToolExecutionSummary();
}

QString DesktopShellViewModel::latestAgentPipelineStatus() const {
    return controller_.latestAgentPipelineStatus();
}

QString DesktopShellViewModel::latestAgentPipelineSummary() const {
    return controller_.latestAgentPipelineSummary();
}

QString DesktopShellViewModel::runtimeSessionId() const {
    return controller_.runtimeSessionId();
}

QString DesktopShellViewModel::runtimeContextStatus() const {
    return controller_.runtimeContextStatus();
}

QString DesktopShellViewModel::runtimeContextSummary() const {
    return controller_.runtimeContextSummary();
}

QStringList DesktopShellViewModel::runtimeContextActiveToolIds() const {
    return controller_.runtimeContextActiveToolIds();
}

QString DesktopShellViewModel::conversationSessionId() const {
    return controller_.conversationSessionId();
}

QString DesktopShellViewModel::conversationSessionStatus() const {
    return controller_.conversationSessionStatus();
}

QString DesktopShellViewModel::interactionMode() const {
    return controller_.interactionMode();
}

QString DesktopShellViewModel::attentionState() const {
    return controller_.attentionState();
}

QString DesktopShellViewModel::contextWindowSummary() const {
    return controller_.contextWindowSummary();
}

QString DesktopShellViewModel::conversationState() const {
    return controller_.conversationState();
}

QString DesktopShellViewModel::conversationTransitionStatus() const {
    return controller_.conversationTransitionStatus();
}

QString DesktopShellViewModel::conversationTransitionSummary() const {
    return controller_.conversationTransitionSummary();
}

int DesktopShellViewModel::agentActivityCount() const {
    return controller_.agentActivityCount();
}

QString DesktopShellViewModel::latestAgentActivitySummary() const {
    return controller_.latestAgentActivitySummary();
}

QString DesktopShellViewModel::currentRoutingMode() const {
    return controller_.currentRoutingMode();
}

void DesktopShellViewModel::setRoutingModeByName(const QString& routingModeName) {
    const auto before = controller_.currentRoutingMode();
    settings_.setRoutingModeName(routingModeName);
    controller_.setRoutingModeByName(settings_.routingModeName());
    if (before == controller_.currentRoutingMode()) {
        emit modelRoutingChanged();
        emit conversationSessionChanged();
        emit orchestrationSnapshotChanged();
    }
}

QStringList DesktopShellViewModel::availableRoutingModes() const {
    return settings_.availableRoutingModes();
}

QString DesktopShellViewModel::modelRoutingStatus() const {
    return controller_.modelRoutingStatus();
}

QString DesktopShellViewModel::selectedModelProviderSummary() const {
    return controller_.selectedModelProviderSummary();
}

QString DesktopShellViewModel::latestTaskPlanStatus() const {
    return controller_.latestTaskPlanStatus();
}

QString DesktopShellViewModel::latestTaskPlanSummary() const {
    return controller_.latestTaskPlanSummary();
}

int DesktopShellViewModel::plannedTaskStepCount() const {
    return controller_.plannedTaskStepCount();
}

int DesktopShellViewModel::registeredAgentCount() const {
    return controller_.registeredAgentCount();
}

QStringList DesktopShellViewModel::activeAgentSummaries() const {
    return controller_.activeAgentSummaries();
}

QString DesktopShellViewModel::currentAgentSummary() const {
    return controller_.currentAgentSummary();
}

QString DesktopShellViewModel::currentMemoryAffinitySummary() const {
    return controller_.currentMemoryAffinitySummary();
}

int DesktopShellViewModel::providerCatalogCount() const {
    return controller_.providerCatalogCount();
}

QStringList DesktopShellViewModel::providerCatalogSummaries() const {
    return controller_.providerCatalogSummaries();
}

int DesktopShellViewModel::memoryCatalogCount() const {
    return controller_.memoryCatalogCount();
}

QStringList DesktopShellViewModel::memoryCatalogSummaries() const {
    return controller_.memoryCatalogSummaries();
}

QString DesktopShellViewModel::orchestrationSnapshotStatus() const {
    return controller_.orchestrationSnapshotStatus();
}

QString DesktopShellViewModel::orchestrationSnapshotSummary() const {
    return controller_.orchestrationSnapshotSummary();
}

QStringList DesktopShellViewModel::orchestrationSignals() const {
    return controller_.orchestrationSignals();
}

QString DesktopShellViewModel::orchestrationReadinessStatus() const {
    return controller_.orchestrationReadinessStatus();
}

QString DesktopShellViewModel::orchestrationReadinessSummary() const {
    return controller_.orchestrationReadinessSummary();
}

QStringList DesktopShellViewModel::orchestrationDiagnostics() const {
    return controller_.orchestrationDiagnostics();
}

QString DesktopShellViewModel::localRuntimeStatus() const {
    return controller_.localRuntimeStatus();
}

QString DesktopShellViewModel::localRuntimeHealth() const {
    return controller_.localRuntimeHealth();
}

QString DesktopShellViewModel::localRuntimeSummary() const {
    return controller_.localRuntimeSummary();
}

QStringList DesktopShellViewModel::localRuntimeCapabilities() const {
    return controller_.localRuntimeCapabilities();
}

QString DesktopShellViewModel::localRuntimeResponseStatus() const {
    return controller_.localRuntimeResponseStatus();
}

QString DesktopShellViewModel::localRuntimeResponseSummary() const {
    return controller_.localRuntimeResponseSummary();
}

int DesktopShellViewModel::localRuntimeSessionCount() const {
    return controller_.localRuntimeSessionCount();
}

QString DesktopShellViewModel::localRuntimeSessionStatus() const {
    return controller_.localRuntimeSessionStatus();
}

QString DesktopShellViewModel::localRuntimeSessionHealth() const {
    return controller_.localRuntimeSessionHealth();
}

QString DesktopShellViewModel::localRuntimeSessionSummary() const {
    return controller_.localRuntimeSessionSummary();
}

QString DesktopShellViewModel::localRuntimeAllocationSummary() const {
    return controller_.localRuntimeAllocationSummary();
}

QString DesktopShellViewModel::localRuntimeReservationSummary() const {
    return controller_.localRuntimeReservationSummary();
}

QStringList DesktopShellViewModel::localRuntimeSessionSummaries() const {
    return controller_.localRuntimeSessionSummaries();
}

int DesktopShellViewModel::runtimeCapabilityCount() const {
    return controller_.runtimeCapabilityCount();
}

QStringList DesktopShellViewModel::enabledRuntimeCapabilitySummaries() const {
    return controller_.enabledRuntimeCapabilitySummaries();
}

QStringList DesktopShellViewModel::disabledRuntimeCapabilitySummaries() const {
    return controller_.disabledRuntimeCapabilitySummaries();
}

QString DesktopShellViewModel::runtimeNegotiationProfileSummary() const {
    return controller_.runtimeNegotiationProfileSummary();
}

QString DesktopShellViewModel::runtimeNegotiationSummary() const {
    return controller_.runtimeNegotiationSummary();
}

QString DesktopShellViewModel::localOnlyRuntimeEnforcementSummary() const {
    return controller_.localOnlyRuntimeEnforcementSummary();
}

QString DesktopShellViewModel::runtimePermissionDecision() const {
    return controller_.runtimePermissionDecision();
}

QString DesktopShellViewModel::runtimePermissionSummary() const {
    return controller_.runtimePermissionSummary();
}

QString DesktopShellViewModel::runtimeSafetyDecision() const {
    return controller_.runtimeSafetyDecision();
}

QString DesktopShellViewModel::runtimeSafetySummary() const {
    return controller_.runtimeSafetySummary();
}

QString DesktopShellViewModel::runtimePipelineStatus() const {
    return controller_.runtimePipelineStatus();
}

QString DesktopShellViewModel::runtimePipelineSummary() const {
    return controller_.runtimePipelineSummary();
}

QStringList DesktopShellViewModel::runtimePipelineTraceSummaries() const {
    return controller_.runtimePipelineTraceSummaries();
}

QString DesktopShellViewModel::executionLifecycleState() const {
    return controller_.executionLifecycleState();
}

QString DesktopShellViewModel::executionLifecycleStatus() const {
    return controller_.executionLifecycleStatus();
}

QString DesktopShellViewModel::executionLifecycleSummary() const {
    return controller_.executionLifecycleSummary();
}

QStringList DesktopShellViewModel::executionLifecycleTraceSummaries() const {
    return controller_.executionLifecycleTraceSummaries();
}

QString DesktopShellViewModel::executionSessionId() const {
    return controller_.executionSessionId();
}

QString DesktopShellViewModel::executionSessionStatus() const {
    return controller_.executionSessionStatus();
}

QString DesktopShellViewModel::executionSessionOwnership() const {
    return controller_.executionSessionOwnership();
}

QString DesktopShellViewModel::executionCoordinationMode() const {
    return controller_.executionCoordinationMode();
}

QString DesktopShellViewModel::executionSessionSummary() const {
    return controller_.executionSessionSummary();
}

QString DesktopShellViewModel::executionCoordinationSnapshotSummary() const {
    return controller_.executionCoordinationSnapshotSummary();
}

QString DesktopShellViewModel::localRuntimeAdapterStatus() const {
    return controller_.localRuntimeAdapterStatus();
}

QString DesktopShellViewModel::localRuntimeAdapterHealth() const {
    return controller_.localRuntimeAdapterHealth();
}

QString DesktopShellViewModel::localRuntimeAdapterSummary() const {
    return controller_.localRuntimeAdapterSummary();
}

QStringList DesktopShellViewModel::localRuntimeAdapterCapabilitySummaries() const {
    return controller_.localRuntimeAdapterCapabilitySummaries();
}

QString DesktopShellViewModel::providerRuntimeBridgeStatus() const {
    return controller_.providerRuntimeBridgeStatus();
}

QString DesktopShellViewModel::providerRuntimeBridgeSummary() const {
    return controller_.providerRuntimeBridgeSummary();
}

QString DesktopShellViewModel::providerRuntimeBridgeResponseSummary() const {
    return controller_.providerRuntimeBridgeResponseSummary();
}

QString DesktopShellViewModel::runtimeIntegrationReadinessStatus() const {
    return controller_.runtimeIntegrationReadinessStatus();
}

QString DesktopShellViewModel::runtimeIntegrationReadinessSummary() const {
    return controller_.runtimeIntegrationReadinessSummary();
}

QStringList DesktopShellViewModel::runtimeIntegrationReadinessChecks() const {
    return controller_.runtimeIntegrationReadinessChecks();
}

QString DesktopShellViewModel::ollamaEndpoint() const {
    return controller_.ollamaEndpoint();
}

QString DesktopShellViewModel::ollamaConnectionStatus() const {
    return controller_.ollamaConnectionStatus();
}

QString DesktopShellViewModel::ollamaHealthStatus() const {
    return controller_.ollamaHealthStatus();
}

QString DesktopShellViewModel::ollamaHealthSummary() const {
    return controller_.ollamaHealthSummary();
}

int DesktopShellViewModel::ollamaModelCount() const {
    return controller_.ollamaModelCount();
}

QStringList DesktopShellViewModel::ollamaModelNames() const {
    return controller_.ollamaModelNames();
}

QStringList DesktopShellViewModel::ollamaModelSummaries() const {
    return controller_.ollamaModelSummaries();
}

QString DesktopShellViewModel::selectedLocalModel() const {
    return controller_.selectedLocalModel();
}

void DesktopShellViewModel::setSelectedLocalModel(const QString& model) {
    settings_.setSelectedLocalModel(model);
    if (controller_.selectedLocalModel() != settings_.selectedLocalModel()) {
        controller_.setSelectedLocalModel(settings_.selectedLocalModel());
    }
}

QString DesktopShellViewModel::selectedLocalModelStatus() const {
    return controller_.selectedLocalModelStatus();
}

QString DesktopShellViewModel::selectedLocalModelSummary() const {
    return controller_.selectedLocalModelSummary();
}

QString DesktopShellViewModel::selectedLocalModelMetadataSummary() const {
    return controller_.selectedLocalModelMetadataSummary();
}

QString DesktopShellViewModel::activeLocalRuntimeBadge() const {
    return controller_.activeLocalRuntimeBadge();
}

QString DesktopShellViewModel::modelManagementStatus() const {
    return controller_.modelManagementStatus();
}

QString DesktopShellViewModel::modelManagementSummary() const {
    return controller_.modelManagementSummary();
}

QString DesktopShellViewModel::modelManagementActionAvailability() const {
    return controller_.modelManagementActionAvailability();
}

QStringList DesktopShellViewModel::modelRecommendationSummaries() const {
    return controller_.modelRecommendationSummaries();
}

QStringList DesktopShellViewModel::modelRequirementSummaries() const {
    return controller_.modelRequirementSummaries();
}

QString DesktopShellViewModel::voiceRuntimeMode() const {
    return controller_.voiceRuntimeMode();
}

bool DesktopShellViewModel::voiceEnabled() const {
    return controller_.voiceEnabled();
}

QString DesktopShellViewModel::voiceReadinessStatus() const {
    return controller_.voiceReadinessStatus();
}

QString DesktopShellViewModel::voiceReadinessSummary() const {
    return controller_.voiceReadinessSummary();
}

QStringList DesktopShellViewModel::voiceReadinessChecks() const {
    return controller_.voiceReadinessChecks();
}

QStringList DesktopShellViewModel::voiceCapabilitySummaries() const {
    return controller_.voiceCapabilitySummaries();
}

QString DesktopShellViewModel::textToSpeechStatus() const {
    return controller_.textToSpeechStatus();
}

QString DesktopShellViewModel::textToSpeechSummary() const {
    return controller_.textToSpeechSummary();
}

QString DesktopShellViewModel::speechToTextStatus() const {
    return controller_.speechToTextStatus();
}

QString DesktopShellViewModel::speechToTextSummary() const {
    return controller_.speechToTextSummary();
}

QString DesktopShellViewModel::voiceSessionId() const {
    return controller_.voiceSessionId();
}

QString DesktopShellViewModel::voiceSessionStatus() const {
    return controller_.voiceSessionStatus();
}

QString DesktopShellViewModel::voiceSessionSummary() const {
    return controller_.voiceSessionSummary();
}

QString DesktopShellViewModel::voicePipelineStatus() const {
    return controller_.voicePipelineStatus();
}

QString DesktopShellViewModel::voicePipelineSummary() const {
    return controller_.voicePipelineSummary();
}

QStringList DesktopShellViewModel::voicePipelineTraceSummaries() const {
    return controller_.voicePipelineTraceSummaries();
}

QString DesktopShellViewModel::voiceRuntimeStatus() const {
    return controller_.voiceRuntimeStatus();
}

QString DesktopShellViewModel::voiceRuntimeSummary() const {
    return controller_.voiceRuntimeSummary();
}

QStringList DesktopShellViewModel::voiceRuntimeCheckSummaries() const {
    return controller_.voiceRuntimeCheckSummaries();
}

bool DesktopShellViewModel::voiceRuntimeAvailable() const {
    return controller_.voiceRuntimeAvailable();
}

bool DesktopShellViewModel::voiceTextToSpeechAvailable() const {
    return controller_.voiceTextToSpeechAvailable();
}

bool DesktopShellViewModel::voiceSpeechToTextAvailable() const {
    return controller_.voiceSpeechToTextAvailable();
}

bool DesktopShellViewModel::voiceMicrophoneEnabled() const {
    return controller_.voiceMicrophoneEnabled();
}

bool DesktopShellViewModel::voicePlaybackEnabled() const {
    return controller_.voicePlaybackEnabled();
}

bool DesktopShellViewModel::voiceLocalOnlyPolicy() const {
    return controller_.voiceLocalOnlyPolicy();
}

bool DesktopShellViewModel::voiceProcessExecutionEnabled() const {
    return controller_.voiceProcessExecutionEnabled();
}

QString DesktopShellViewModel::voiceRuntimeEnvironmentStatus() const {
    return controller_.voiceRuntimeEnvironmentStatus();
}

QString DesktopShellViewModel::voiceRuntimeEnvironmentSummary() const {
    return controller_.voiceRuntimeEnvironmentSummary();
}

QStringList DesktopShellViewModel::voiceBinarySummaries() const {
    return controller_.voiceBinarySummaries();
}

QStringList DesktopShellViewModel::voiceModelSummaries() const {
    return controller_.voiceModelSummaries();
}

QStringList DesktopShellViewModel::voiceRuntimePermissionSummaries() const {
    return controller_.voiceRuntimePermissionSummaries();
}

QString DesktopShellViewModel::voiceRuntimeSafetyStatus() const {
    return controller_.voiceRuntimeSafetyStatus();
}

QString DesktopShellViewModel::voiceRuntimeSafetySummary() const {
    return controller_.voiceRuntimeSafetySummary();
}

QStringList DesktopShellViewModel::voiceRuntimeSafetyChecks() const {
    return controller_.voiceRuntimeSafetyChecks();
}

bool DesktopShellViewModel::voiceRuntimeExecutionAllowed() const {
    return controller_.voiceRuntimeExecutionAllowed();
}

QString DesktopShellViewModel::piperTtsStatus() const {
    return controller_.piperTtsStatus();
}

QString DesktopShellViewModel::piperTtsSummary() const {
    return controller_.piperTtsSummary();
}

QStringList DesktopShellViewModel::piperTtsReadinessChecks() const {
    return controller_.piperTtsReadinessChecks();
}

bool DesktopShellViewModel::piperTtsReady() const {
    return controller_.piperTtsReady();
}

QString DesktopShellViewModel::piperTtsFileOutputStatus() const {
    return controller_.piperTtsFileOutputStatus();
}

QString DesktopShellViewModel::piperTtsFileOutputSummary() const {
    return controller_.piperTtsFileOutputSummary();
}

QString DesktopShellViewModel::piperBinaryPath() const {
    return controller_.piperBinaryPath();
}

void DesktopShellViewModel::setPiperBinaryPath(const QString& path) {
    settings_.setPiperBinaryPath(path);
    if (controller_.piperBinaryPath() != settings_.piperBinaryPath()) {
        controller_.setPiperBinaryPath(settings_.piperBinaryPath());
    }
}

QString DesktopShellViewModel::piperModelPath() const {
    return controller_.piperModelPath();
}

void DesktopShellViewModel::setPiperModelPath(const QString& path) {
    settings_.setPiperModelPath(path);
    if (controller_.piperModelPath() != settings_.piperModelPath()) {
        controller_.setPiperModelPath(settings_.piperModelPath());
    }
}

QString DesktopShellViewModel::whisperBinaryPath() const {
    return controller_.whisperBinaryPath();
}

void DesktopShellViewModel::setWhisperBinaryPath(const QString& path) {
    settings_.setWhisperBinaryPath(path);
    if (controller_.whisperBinaryPath() != settings_.whisperBinaryPath()) {
        controller_.setWhisperBinaryPath(settings_.whisperBinaryPath());
    }
}

QString DesktopShellViewModel::whisperModelPath() const {
    return controller_.whisperModelPath();
}

void DesktopShellViewModel::setWhisperModelPath(const QString& path) {
    settings_.setWhisperModelPath(path);
    if (controller_.whisperModelPath() != settings_.whisperModelPath()) {
        controller_.setWhisperModelPath(settings_.whisperModelPath());
    }
}

QStringList DesktopShellViewModel::voiceConfigurationSummaries() const {
    return controller_.voiceConfigurationSummaries();
}

QString DesktopShellViewModel::voiceConfigurationReadinessSummary() const {
    return controller_.voiceConfigurationReadinessSummary();
}

QStringList DesktopShellViewModel::voiceConfigurationStatusBadges() const {
    return controller_.voiceConfigurationStatusBadges();
}

QStringList DesktopShellViewModel::voiceConfigurationHintSummaries() const {
    return controller_.voiceConfigurationHintSummaries();
}

bool DesktopShellViewModel::localChatInferenceEnabled() const {
    return controller_.localChatInferenceEnabled();
}

void DesktopShellViewModel::setLocalChatInferenceEnabled(bool enabled) {
    settings_.setLocalChatInferenceEnabled(enabled);
    if (controller_.localChatInferenceEnabled() != settings_.localChatInferenceEnabled()) {
        controller_.setLocalChatInferenceEnabled(settings_.localChatInferenceEnabled());
    }
}

QString DesktopShellViewModel::localChatInferenceStatus() const {
    return controller_.localChatInferenceStatus();
}

QString DesktopShellViewModel::localChatInferenceSummary() const {
    return controller_.localChatInferenceSummary();
}

bool DesktopShellViewModel::localInferenceStreamingEnabled() const {
    return controller_.localInferenceStreamingEnabled();
}

void DesktopShellViewModel::setLocalInferenceStreamingEnabled(bool enabled) {
    settings_.setLocalInferenceStreamingEnabled(enabled);
    if (controller_.localInferenceStreamingEnabled() !=
        settings_.localInferenceStreamingEnabled()) {
        controller_.setLocalInferenceStreamingEnabled(settings_.localInferenceStreamingEnabled());
    }
}

bool DesktopShellViewModel::localInferenceBusy() const {
    return controller_.localInferenceBusy();
}

QString DesktopShellViewModel::localInferenceRuntimeState() const {
    return controller_.localInferenceRuntimeState();
}

QString DesktopShellViewModel::localInferenceStatus() const {
    return controller_.localInferenceStatus();
}

QString DesktopShellViewModel::localInferenceSummary() const {
    return controller_.localInferenceSummary();
}

QString DesktopShellViewModel::localInferenceLastResponseSummary() const {
    return controller_.localInferenceLastResponseSummary();
}

QString DesktopShellViewModel::localInferenceLatencySummary() const {
    return controller_.localInferenceLatencySummary();
}

QStringList DesktopShellViewModel::localInferenceTraceSummaries() const {
    return controller_.localInferenceTraceSummaries();
}

bool DesktopShellViewModel::localInferenceStreamingAvailable() const {
    return controller_.localInferenceStreamingAvailable();
}

QString DesktopShellViewModel::localInferenceStreamStatus() const {
    return controller_.localInferenceStreamStatus();
}

QString DesktopShellViewModel::localInferenceStreamSummary() const {
    return controller_.localInferenceStreamSummary();
}

QString DesktopShellViewModel::localInferenceStreamingText() const {
    return controller_.localInferenceStreamingText();
}

int DesktopShellViewModel::availableToolCount() const {
    return controller_.availableToolCount();
}

QStringList DesktopShellViewModel::availableToolIds() const {
    return controller_.availableToolIds();
}

QString DesktopShellViewModel::memoryStatus() const {
    return controller_.memoryStatus();
}

QString DesktopShellViewModel::chatHistoryStatus() const {
    return controller_.chatHistoryStatus();
}

QString DesktopShellViewModel::memoryMaintenanceStatus() const {
    return controller_.memoryMaintenanceStatus();
}

QString DesktopShellViewModel::chatMaintenanceStatus() const {
    return controller_.chatMaintenanceStatus();
}

QString DesktopShellViewModel::currentModeName() const {
    return modeManager_.currentModeName();
}

QStringList DesktopShellViewModel::availableModes() const {
    return modeManager_.availableModes();
}

QString DesktopShellViewModel::currentPage() const {
    return currentPage_;
}

void DesktopShellViewModel::setCurrentPage(const QString& page) {
    const auto normalized = normalizedPageOrDefault(page);
    if (normalized == currentPage_) {
        return;
    }

    currentPage_ = normalized;
    emit currentPageChanged();
}

QStringList DesktopShellViewModel::availablePages() const {
    return {
        QStringLiteral("Memory"),
        QStringLiteral("Dashboard"),
        QStringLiteral("Agents"),
        QStringLiteral("Settings"),
    };
}

ChatMessageListModel* DesktopShellViewModel::chatMessages() {
    return &chatMessages_;
}

QStringList DesktopShellViewModel::memoryEntries() const {
    return controller_.memoryEntries();
}

QString DesktopShellViewModel::themeName() const {
    return settings_.themeName();
}

void DesktopShellViewModel::setThemeName(const QString& themeName) {
    settings_.setThemeName(themeName);
}

QString DesktopShellViewModel::configurationProfile() const {
    return settings_.configurationProfile();
}

void DesktopShellViewModel::setConfigurationProfile(const QString& configurationProfile) {
    settings_.setConfigurationProfile(configurationProfile);
}

bool DesktopShellViewModel::sendMessage(const QString& message) {
    return controller_.sendMessage(message);
}

bool DesktopShellViewModel::runLocalInference(const QString& prompt, const QString& model) {
    return controller_.runLocalInference(prompt, model);
}

bool DesktopShellViewModel::runAgentRequest(const QString& request) {
    return controller_.runAgentRequest(request);
}

bool DesktopShellViewModel::clearMemory() {
    return controller_.clearMemory();
}

bool DesktopShellViewModel::clearChat() {
    return controller_.clearChat();
}

void DesktopShellViewModel::setModeByName(const QString& modeName) {
    modeManager_.setModeByName(modeName);
}

void DesktopShellViewModel::remember(const QString& key, const QString& value) {
    controller_.remember(key, value);
}

QString DesktopShellViewModel::normalizedPageOrDefault(const QString& page) {
    const auto trimmed = page.trimmed();
    const QStringList pages{
        QStringLiteral("Memory"),
        QStringLiteral("Dashboard"),
        QStringLiteral("Agents"),
        QStringLiteral("Settings"),
    };
    return pages.contains(trimmed) ? trimmed : QStringLiteral("Dashboard");
}

} // namespace sentinel::desktop
