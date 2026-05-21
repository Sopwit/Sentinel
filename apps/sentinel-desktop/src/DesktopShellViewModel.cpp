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
    connect(&controller_, &core::ApplicationController::conversationRuntimeChanged, this,
            &DesktopShellViewModel::conversationRuntimeChanged);
    connect(&controller_, &core::ApplicationController::conversationSearchChanged, this,
            &DesktopShellViewModel::conversationSearchChanged);
    connect(&controller_, &core::ApplicationController::conversationExportChanged, this,
            &DesktopShellViewModel::conversationExportChanged);
    connect(&controller_, &core::ApplicationController::conversationDeleteChanged, this,
            &DesktopShellViewModel::conversationDeleteChanged);
    connect(&controller_, &core::ApplicationController::memoryCandidatesChanged, this,
            &DesktopShellViewModel::memoryCandidatesChanged);
    connect(&controller_, &core::ApplicationController::memoryRecallChanged, this,
            &DesktopShellViewModel::memoryRecallChanged);
    connect(&controller_, &core::ApplicationController::contextAssemblyChanged, this,
            &DesktopShellViewModel::contextAssemblyChanged);
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
    connect(&controller_, &core::ApplicationController::promptContextInjectionChanged, this,
            &DesktopShellViewModel::promptContextInjectionChanged);
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
    connect(&settings_, &core::AppSettings::promptContextInjectionEnabledChanged, this, [this]() {
        controller_.setPromptContextInjectionEnabled(settings_.promptContextInjectionEnabled());
    });
    connect(&settings_, &core::AppSettings::semanticPromptInclusionEnabledChanged, this, [this]() {
        controller_.setSemanticPromptInclusionEnabled(settings_.semanticPromptInclusionEnabled());
    });
    connect(&settings_, &core::AppSettings::piperBinaryPathChanged, this,
            [this]() { controller_.setPiperBinaryPath(settings_.piperBinaryPath()); });
    connect(&settings_, &core::AppSettings::piperModelPathChanged, this,
            [this]() { controller_.setPiperModelPath(settings_.piperModelPath()); });
    connect(&settings_, &core::AppSettings::whisperBinaryPathChanged, this,
            [this]() { controller_.setWhisperBinaryPath(settings_.whisperBinaryPath()); });
    connect(&settings_, &core::AppSettings::whisperModelPathChanged, this,
            [this]() { controller_.setWhisperModelPath(settings_.whisperModelPath()); });
    connect(&settings_, &core::AppSettings::piperFileOutputExecutionEnabledChanged, this, [this]() {
        controller_.setPiperFileOutputExecutionEnabled(settings_.piperFileOutputExecutionEnabled());
    });
    controller_.setSelectedLocalModel(settings_.selectedLocalModel());
    controller_.setLocalChatInferenceEnabled(settings_.localChatInferenceEnabled());
    controller_.setLocalInferenceStreamingEnabled(settings_.localInferenceStreamingEnabled());
    controller_.setPromptContextInjectionEnabled(settings_.promptContextInjectionEnabled());
    controller_.setSemanticPromptInclusionEnabled(settings_.semanticPromptInclusionEnabled());
    controller_.setPiperBinaryPath(settings_.piperBinaryPath());
    controller_.setPiperModelPath(settings_.piperModelPath());
    controller_.setWhisperBinaryPath(settings_.whisperBinaryPath());
    controller_.setWhisperModelPath(settings_.whisperModelPath());
    controller_.setPiperFileOutputExecutionEnabled(settings_.piperFileOutputExecutionEnabled());
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

QString DesktopShellViewModel::conversationRuntimeSummary() const {
    return controller_.conversationRuntimeSummary();
}

QStringList DesktopShellViewModel::conversationRuntimeSummaryLines() const {
    return controller_.conversationRuntimeSummaryLines();
}

QString DesktopShellViewModel::conversationRuntimeRequestId() const {
    return controller_.conversationRuntimeRequestId();
}

QString DesktopShellViewModel::conversationRuntimeActiveModel() const {
    return controller_.conversationRuntimeActiveModel();
}

QString DesktopShellViewModel::conversationRuntimeActiveRoute() const {
    return controller_.conversationRuntimeActiveRoute();
}

bool DesktopShellViewModel::conversationRuntimeStreaming() const {
    return controller_.conversationRuntimeStreaming();
}

QString DesktopShellViewModel::conversationRuntimeLastSuccessSummary() const {
    return controller_.conversationRuntimeLastSuccessSummary();
}

QString DesktopShellViewModel::conversationRuntimeLastErrorSummary() const {
    return controller_.conversationRuntimeLastErrorSummary();
}

QString DesktopShellViewModel::conversationRuntimeLastLatencySummary() const {
    return controller_.conversationRuntimeLastLatencySummary();
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

QString DesktopShellViewModel::agentTaskRuntimeStatus() const {
    return controller_.agentTaskRuntimeStatus();
}

QString DesktopShellViewModel::agentTaskRuntimeSummary() const {
    return controller_.agentTaskRuntimeSummary();
}

int DesktopShellViewModel::agentTaskRuntimeTaskCount() const {
    return controller_.agentTaskRuntimeTaskCount();
}

int DesktopShellViewModel::agentTaskQueueCount() const {
    return controller_.agentTaskQueueCount();
}

int DesktopShellViewModel::agentTaskQueueActiveCount() const {
    return controller_.agentTaskQueueActiveCount();
}

int DesktopShellViewModel::agentTaskQueuePlannedCount() const {
    return controller_.agentTaskQueuePlannedCount();
}

int DesktopShellViewModel::agentTaskQueueBlockedCount() const {
    return controller_.agentTaskQueueBlockedCount();
}

int DesktopShellViewModel::agentTaskQueueCompletedCount() const {
    return controller_.agentTaskQueueCompletedCount();
}

int DesktopShellViewModel::agentTaskQueueRefusedCount() const {
    return controller_.agentTaskQueueRefusedCount();
}

QString DesktopShellViewModel::latestAgentTaskSummary() const {
    return controller_.latestAgentTaskSummary();
}

QString DesktopShellViewModel::latestAgentTaskLifecycleSummary() const {
    return controller_.latestAgentTaskLifecycleSummary();
}

QStringList DesktopShellViewModel::agentTaskQueueSummaries() const {
    return controller_.agentTaskQueueSummaries();
}

QStringList DesktopShellViewModel::agentTaskTraceSummaries() const {
    return controller_.agentTaskTraceSummaries();
}

QString DesktopShellViewModel::agentPlanningSessionStatus() const {
    return controller_.agentPlanningSessionStatus();
}

QString DesktopShellViewModel::agentPlanningSessionSummary() const {
    return controller_.agentPlanningSessionSummary();
}

int DesktopShellViewModel::agentPlanningCandidateCount() const {
    return controller_.agentPlanningCandidateCount();
}

int DesktopShellViewModel::agentPlanningRefusedCount() const {
    return controller_.agentPlanningRefusedCount();
}

QStringList DesktopShellViewModel::agentPlanningCandidateSummaries() const {
    return controller_.agentPlanningCandidateSummaries();
}

QStringList DesktopShellViewModel::agentPlanningArbitrationSummaries() const {
    return controller_.agentPlanningArbitrationSummaries();
}

QStringList DesktopShellViewModel::agentPlanningRefusalSummaries() const {
    return controller_.agentPlanningRefusalSummaries();
}

QString DesktopShellViewModel::agentPlanningFallbackSummary() const {
    return controller_.agentPlanningFallbackSummary();
}

QString DesktopShellViewModel::agentCapabilityRegistryStatus() const {
    return controller_.agentCapabilityRegistryStatus();
}

QString DesktopShellViewModel::agentCapabilityRegistrySummary() const {
    return controller_.agentCapabilityRegistrySummary();
}

int DesktopShellViewModel::agentCapabilityCount() const {
    return controller_.agentCapabilityCount();
}

int DesktopShellViewModel::agentCapabilityEnabledCount() const {
    return controller_.agentCapabilityEnabledCount();
}

int DesktopShellViewModel::agentCapabilityDisabledCount() const {
    return controller_.agentCapabilityDisabledCount();
}

int DesktopShellViewModel::agentCapabilityRestrictedCount() const {
    return controller_.agentCapabilityRestrictedCount();
}

QStringList DesktopShellViewModel::agentCapabilitySummaries() const {
    return controller_.agentCapabilitySummaries();
}

QStringList DesktopShellViewModel::agentCapabilityReadinessSummaries() const {
    return controller_.agentCapabilityReadinessSummaries();
}

QStringList DesktopShellViewModel::agentCapabilitySafetySummaries() const {
    return controller_.agentCapabilitySafetySummaries();
}

QString DesktopShellViewModel::toolContractRegistryStatus() const {
    return controller_.toolContractRegistryStatus();
}

QString DesktopShellViewModel::toolContractRegistrySummary() const {
    return controller_.toolContractRegistrySummary();
}

int DesktopShellViewModel::toolContractCount() const {
    return controller_.toolContractCount();
}

int DesktopShellViewModel::toolContractEnabledCount() const {
    return controller_.toolContractEnabledCount();
}

int DesktopShellViewModel::toolContractDisabledCount() const {
    return controller_.toolContractDisabledCount();
}

int DesktopShellViewModel::toolContractRestrictedCount() const {
    return controller_.toolContractRestrictedCount();
}

QStringList DesktopShellViewModel::toolContractSummaries() const {
    return controller_.toolContractSummaries();
}

QStringList DesktopShellViewModel::toolContractPermissionSummaries() const {
    return controller_.toolContractPermissionSummaries();
}

QStringList DesktopShellViewModel::toolContractSandboxSummaries() const {
    return controller_.toolContractSandboxSummaries();
}

QStringList DesktopShellViewModel::toolContractReadinessSummaries() const {
    return controller_.toolContractReadinessSummaries();
}

QStringList DesktopShellViewModel::toolContractSafetySummaries() const {
    return controller_.toolContractSafetySummaries();
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

QStringList DesktopShellViewModel::voiceConfigurationValidationSummaries() const {
    return controller_.voiceConfigurationValidationSummaries();
}

QString DesktopShellViewModel::piperFileOutputReadinessStatus() const {
    return controller_.piperFileOutputReadinessStatus();
}

QString DesktopShellViewModel::piperFileOutputReadinessSummary() const {
    return controller_.piperFileOutputReadinessSummary();
}

bool DesktopShellViewModel::piperFileOutputExecutionEnabled() const {
    return controller_.piperFileOutputExecutionEnabled();
}

void DesktopShellViewModel::setPiperFileOutputExecutionEnabled(bool enabled) {
    settings_.setPiperFileOutputExecutionEnabled(enabled);
    if (controller_.piperFileOutputExecutionEnabled() !=
        settings_.piperFileOutputExecutionEnabled()) {
        controller_.setPiperFileOutputExecutionEnabled(settings_.piperFileOutputExecutionEnabled());
    }
}

QString DesktopShellViewModel::piperFileOutputExecutionStatus() const {
    return controller_.piperFileOutputExecutionStatus();
}

QString DesktopShellViewModel::piperFileOutputExecutionSummary() const {
    return controller_.piperFileOutputExecutionSummary();
}

QString DesktopShellViewModel::piperFileOutputAudioPathSummary() const {
    return controller_.piperFileOutputAudioPathSummary();
}

QString DesktopShellViewModel::whisperPreparationReadinessStatus() const {
    return controller_.whisperPreparationReadinessStatus();
}

QString DesktopShellViewModel::whisperPreparationReadinessSummary() const {
    return controller_.whisperPreparationReadinessSummary();
}

// QML-facing convenience API keeps explicit path arguments to avoid a larger invokable contract
// change. NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void DesktopShellViewModel::applyVoiceConfigurationPaths(const QString& piperBinaryPath,
                                                         const QString& piperModelPath,
                                                         const QString& whisperBinaryPath,
                                                         const QString& whisperModelPath) {
    setPiperBinaryPath(piperBinaryPath);
    setPiperModelPath(piperModelPath);
    setWhisperBinaryPath(whisperBinaryPath);
    setWhisperModelPath(whisperModelPath);
}

bool DesktopShellViewModel::generatePiperTtsFile(const QString& text) {
    return controller_.generatePiperTtsFile(text);
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

bool DesktopShellViewModel::promptContextInjectionEnabled() const {
    return controller_.promptContextInjectionEnabled();
}

void DesktopShellViewModel::setPromptContextInjectionEnabled(bool enabled) {
    settings_.setPromptContextInjectionEnabled(enabled);
    if (controller_.promptContextInjectionEnabled() != settings_.promptContextInjectionEnabled()) {
        controller_.setPromptContextInjectionEnabled(settings_.promptContextInjectionEnabled());
    }
}

QString DesktopShellViewModel::promptContextInjectionStatus() const {
    return controller_.promptContextInjectionStatus();
}

QString DesktopShellViewModel::promptContextInjectionSummary() const {
    return controller_.promptContextInjectionSummary();
}

int DesktopShellViewModel::promptContextInjectedBlockCount() const {
    return controller_.promptContextInjectedBlockCount();
}

QString DesktopShellViewModel::promptContextSourceSummary() const {
    return controller_.promptContextSourceSummary();
}

QString DesktopShellViewModel::promptContextSizeSummary() const {
    return controller_.promptContextSizeSummary();
}

QStringList DesktopShellViewModel::promptContextBlockSummaries() const {
    return controller_.promptContextBlockSummaries();
}

QString DesktopShellViewModel::conversationWindowStatus() const {
    return controller_.conversationWindowStatus();
}

QString DesktopShellViewModel::conversationWindowSummary() const {
    return controller_.conversationWindowSummary();
}

QString DesktopShellViewModel::conversationWindowBudgetSummary() const {
    return controller_.conversationWindowBudgetSummary();
}

int DesktopShellViewModel::conversationWindowBudgetCharacters() const {
    return controller_.conversationWindowBudgetCharacters();
}

int DesktopShellViewModel::conversationWindowIncludedMessageCount() const {
    return controller_.conversationWindowIncludedMessageCount();
}

int DesktopShellViewModel::conversationWindowTruncatedMessageCount() const {
    return controller_.conversationWindowTruncatedMessageCount();
}

int DesktopShellViewModel::conversationWindowOmittedMessageCount() const {
    return controller_.conversationWindowOmittedMessageCount();
}

QString DesktopShellViewModel::conversationSummaryStatus() const {
    return controller_.conversationSummaryStatus();
}

QString DesktopShellViewModel::conversationSummaryText() const {
    return controller_.conversationSummaryText();
}

QString DesktopShellViewModel::conversationSummaryBudgetSummary() const {
    return controller_.conversationSummaryBudgetSummary();
}

int DesktopShellViewModel::conversationSummaryBudgetCharacters() const {
    return controller_.conversationSummaryBudgetCharacters();
}

int DesktopShellViewModel::conversationSummaryBlockCount() const {
    return controller_.conversationSummaryBlockCount();
}

int DesktopShellViewModel::conversationSummaryMessageCount() const {
    return controller_.conversationSummaryMessageCount();
}

int DesktopShellViewModel::conversationSummaryOmittedMessageCount() const {
    return controller_.conversationSummaryOmittedMessageCount();
}

int DesktopShellViewModel::conversationSummaryTruncatedBlockCount() const {
    return controller_.conversationSummaryTruncatedBlockCount();
}

QStringList DesktopShellViewModel::conversationSummaryBlockSummaries() const {
    return controller_.conversationSummaryBlockSummaries();
}

QString DesktopShellViewModel::retrievalPlanningStatus() const {
    return controller_.retrievalPlanningStatus();
}

QString DesktopShellViewModel::retrievalPlanningSummary() const {
    return controller_.retrievalPlanningSummary();
}

QString DesktopShellViewModel::retrievalPlanningReadiness() const {
    return controller_.retrievalPlanningReadiness();
}

QString DesktopShellViewModel::retrievalPlanningBudgetSummary() const {
    return controller_.retrievalPlanningBudgetSummary();
}

QString DesktopShellViewModel::retrievalPlanningSourceSummary() const {
    return controller_.retrievalPlanningSourceSummary();
}

int DesktopShellViewModel::retrievalPlanningSelectedSourceCount() const {
    return controller_.retrievalPlanningSelectedSourceCount();
}

int DesktopShellViewModel::retrievalPlanningExcludedSourceCount() const {
    return controller_.retrievalPlanningExcludedSourceCount();
}

int DesktopShellViewModel::retrievalPlanningSelectedCandidateCount() const {
    return controller_.retrievalPlanningSelectedCandidateCount();
}

int DesktopShellViewModel::retrievalPlanningExcludedCandidateCount() const {
    return controller_.retrievalPlanningExcludedCandidateCount();
}

int DesktopShellViewModel::retrievalPlanningTruncatedCandidateCount() const {
    return controller_.retrievalPlanningTruncatedCandidateCount();
}

QStringList DesktopShellViewModel::retrievalPlanningSourceSummaries() const {
    return controller_.retrievalPlanningSourceSummaries();
}

core::SemanticRetrievalPolicy DesktopShellViewModel::semanticRetrievalPolicy() const {
    return controller_.semanticRetrievalPolicy();
}

bool DesktopShellViewModel::semanticRetrievalEnabled() const {
    return controller_.semanticRetrievalEnabled();
}

QString DesktopShellViewModel::semanticRetrievalStatus() const {
    return controller_.semanticRetrievalStatus();
}

QString DesktopShellViewModel::semanticRetrievalSummary() const {
    return controller_.semanticRetrievalSummary();
}

QString DesktopShellViewModel::semanticReadiness() const {
    return controller_.semanticReadiness();
}

QString DesktopShellViewModel::embeddingProviderReadiness() const {
    return controller_.embeddingProviderReadiness();
}

QString DesktopShellViewModel::embeddingProviderSummary() const {
    return controller_.embeddingProviderSummary();
}

QString DesktopShellViewModel::vectorIndexReadiness() const {
    return controller_.vectorIndexReadiness();
}

QString DesktopShellViewModel::vectorIndexSummary() const {
    return controller_.vectorIndexSummary();
}

int DesktopShellViewModel::vectorIndexedItemCount() const {
    return controller_.vectorIndexedItemCount();
}

QString DesktopShellViewModel::semanticProviderMode() const {
    return controller_.semanticProviderMode();
}

QString DesktopShellViewModel::selectedSemanticProviderName() const {
    return controller_.selectedSemanticProviderName();
}

QString DesktopShellViewModel::semanticProviderReadiness() const {
    return controller_.semanticProviderReadiness();
}

QString DesktopShellViewModel::semanticProviderHealth() const {
    return controller_.semanticProviderHealth();
}

QString DesktopShellViewModel::semanticProviderStatusSummary() const {
    return controller_.semanticProviderStatusSummary();
}

QString DesktopShellViewModel::semanticActivationReadiness() const {
    return controller_.semanticActivationReadiness();
}

QString DesktopShellViewModel::semanticActivationSummary() const {
    return controller_.semanticActivationSummary();
}

QStringList DesktopShellViewModel::semanticProviderCapabilitySummaries() const {
    return controller_.semanticProviderCapabilitySummaries();
}

QStringList DesktopShellViewModel::semanticActivationRequiredSteps() const {
    return controller_.semanticActivationRequiredSteps();
}

QStringList DesktopShellViewModel::semanticRetrievalReadinessChecks() const {
    return controller_.semanticRetrievalReadinessChecks();
}

QString DesktopShellViewModel::semanticCandidateStatus() const {
    return controller_.semanticCandidateStatus();
}

QString DesktopShellViewModel::semanticCandidateSummary() const {
    return controller_.semanticCandidateSummary();
}

QString DesktopShellViewModel::semanticCandidateBudgetSummary() const {
    return controller_.semanticCandidateBudgetSummary();
}

QString DesktopShellViewModel::semanticCandidateArbitrationSummary() const {
    return controller_.semanticCandidateArbitrationSummary();
}

int DesktopShellViewModel::semanticCandidateCount() const {
    return controller_.semanticCandidateCount();
}

int DesktopShellViewModel::semanticCandidateSelectedCount() const {
    return controller_.semanticCandidateSelectedCount();
}

int DesktopShellViewModel::semanticCandidateExcludedCount() const {
    return controller_.semanticCandidateExcludedCount();
}

int DesktopShellViewModel::semanticCandidateTruncatedCount() const {
    return controller_.semanticCandidateTruncatedCount();
}

QStringList DesktopShellViewModel::semanticCandidateParticipationSummaries() const {
    return controller_.semanticCandidateParticipationSummaries();
}

QString DesktopShellViewModel::hybridRetrievalStatus() const {
    return controller_.hybridRetrievalStatus();
}

QString DesktopShellViewModel::hybridRetrievalReadiness() const {
    return controller_.hybridRetrievalReadiness();
}

QString DesktopShellViewModel::hybridRetrievalSummary() const {
    return controller_.hybridRetrievalSummary();
}

QStringList DesktopShellViewModel::hybridRetrievalReadinessChecks() const {
    return controller_.hybridRetrievalReadinessChecks();
}

QString DesktopShellViewModel::semanticArbitrationStatus() const {
    return controller_.semanticArbitrationStatus();
}

QString DesktopShellViewModel::semanticArbitrationReadiness() const {
    return controller_.semanticArbitrationReadiness();
}

QString DesktopShellViewModel::semanticArbitrationSummary() const {
    return controller_.semanticArbitrationSummary();
}

QString DesktopShellViewModel::semanticArbitrationBudgetSummary() const {
    return controller_.semanticArbitrationBudgetSummary();
}

QStringList DesktopShellViewModel::semanticArbitrationSelectionSummaries() const {
    return controller_.semanticArbitrationSelectionSummaries();
}

QStringList DesktopShellViewModel::semanticArbitrationChecks() const {
    return controller_.semanticArbitrationChecks();
}

QString DesktopShellViewModel::embeddingRuntimeReadiness() const {
    return controller_.embeddingRuntimeReadiness();
}

QString DesktopShellViewModel::embeddingRuntimeSummary() const {
    return controller_.embeddingRuntimeSummary();
}

QString DesktopShellViewModel::embeddingRuntimeBudgetSummary() const {
    return controller_.embeddingRuntimeBudgetSummary();
}

QStringList DesktopShellViewModel::embeddingRuntimeRequirementSummaries() const {
    return controller_.embeddingRuntimeRequirementSummaries();
}

QStringList DesktopShellViewModel::embeddingRuntimeConstraintSummaries() const {
    return controller_.embeddingRuntimeConstraintSummaries();
}

QString DesktopShellViewModel::isolatedEmbeddingRuntimeStatus() const {
    return controller_.isolatedEmbeddingRuntimeStatus();
}

QString DesktopShellViewModel::isolatedEmbeddingRuntimeHealth() const {
    return controller_.isolatedEmbeddingRuntimeHealth();
}

QString DesktopShellViewModel::isolatedEmbeddingRuntimeReadiness() const {
    return controller_.isolatedEmbeddingRuntimeReadiness();
}

QString DesktopShellViewModel::isolatedEmbeddingRuntimeSummary() const {
    return controller_.isolatedEmbeddingRuntimeSummary();
}

QString DesktopShellViewModel::isolatedEmbeddingRuntimeBoundedState() const {
    return controller_.isolatedEmbeddingRuntimeBoundedState();
}

QStringList DesktopShellViewModel::isolatedEmbeddingRuntimeChecks() const {
    return controller_.isolatedEmbeddingRuntimeChecks();
}

QString DesktopShellViewModel::vectorPersistenceStatus() const {
    return controller_.vectorPersistenceStatus();
}

QString DesktopShellViewModel::vectorPersistenceHealth() const {
    return controller_.vectorPersistenceHealth();
}

QString DesktopShellViewModel::vectorPersistenceReadiness() const {
    return controller_.vectorPersistenceReadiness();
}

QString DesktopShellViewModel::vectorPersistenceSummary() const {
    return controller_.vectorPersistenceSummary();
}

QString DesktopShellViewModel::vectorPersistenceBoundedState() const {
    return controller_.vectorPersistenceBoundedState();
}

int DesktopShellViewModel::vectorPersistenceIndexedItemCount() const {
    return controller_.vectorPersistenceIndexedItemCount();
}

QStringList DesktopShellViewModel::vectorPersistenceChecks() const {
    return controller_.vectorPersistenceChecks();
}

QString DesktopShellViewModel::semanticSearchStatus() const {
    return controller_.semanticSearchStatus();
}

QString DesktopShellViewModel::semanticSearchReadiness() const {
    return controller_.semanticSearchReadiness();
}

QString DesktopShellViewModel::semanticSearchSummary() const {
    return controller_.semanticSearchSummary();
}

QString DesktopShellViewModel::semanticSearchBudgetSummary() const {
    return controller_.semanticSearchBudgetSummary();
}

QString DesktopShellViewModel::semanticSearchRuntimeState() const {
    return controller_.semanticSearchRuntimeState();
}

int DesktopShellViewModel::semanticSearchCandidateCount() const {
    return controller_.semanticSearchCandidateCount();
}

QString DesktopShellViewModel::semanticSearchArbitrationSummary() const {
    return controller_.semanticSearchArbitrationSummary();
}

QStringList DesktopShellViewModel::semanticSearchCandidateSummaries() const {
    return controller_.semanticSearchCandidateSummaries();
}

QStringList DesktopShellViewModel::semanticSearchChecks() const {
    return controller_.semanticSearchChecks();
}

QString DesktopShellViewModel::hybridBridgeStatus() const {
    return controller_.hybridBridgeStatus();
}

QString DesktopShellViewModel::hybridBridgeReadiness() const {
    return controller_.hybridBridgeReadiness();
}

QString DesktopShellViewModel::hybridBridgeSummary() const {
    return controller_.hybridBridgeSummary();
}

QString DesktopShellViewModel::hybridBridgeBudgetSummary() const {
    return controller_.hybridBridgeBudgetSummary();
}

QString DesktopShellViewModel::hybridBridgeSourceSummary() const {
    return controller_.hybridBridgeSourceSummary();
}

QString DesktopShellViewModel::hybridBridgeArbitrationSummary() const {
    return controller_.hybridBridgeArbitrationSummary();
}

QString DesktopShellViewModel::hybridBridgeFallbackSummary() const {
    return controller_.hybridBridgeFallbackSummary();
}

int DesktopShellViewModel::hybridBridgeCandidateCount() const {
    return controller_.hybridBridgeCandidateCount();
}

int DesktopShellViewModel::hybridBridgeSemanticFillCount() const {
    return controller_.hybridBridgeSemanticFillCount();
}

QStringList DesktopShellViewModel::hybridBridgeCandidateSummaries() const {
    return controller_.hybridBridgeCandidateSummaries();
}

QStringList DesktopShellViewModel::hybridBridgeChecks() const {
    return controller_.hybridBridgeChecks();
}

QString DesktopShellViewModel::semanticAcceptanceStatus() const {
    return controller_.semanticAcceptanceStatus();
}

QString DesktopShellViewModel::semanticAcceptanceReadiness() const {
    return controller_.semanticAcceptanceReadiness();
}

QString DesktopShellViewModel::semanticAcceptanceSummary() const {
    return controller_.semanticAcceptanceSummary();
}

QString DesktopShellViewModel::semanticAcceptanceBudgetSummary() const {
    return controller_.semanticAcceptanceBudgetSummary();
}

QString DesktopShellViewModel::semanticAcceptanceSourceSummary() const {
    return controller_.semanticAcceptanceSourceSummary();
}

QString DesktopShellViewModel::semanticAcceptanceArbitrationSummary() const {
    return controller_.semanticAcceptanceArbitrationSummary();
}

QString DesktopShellViewModel::semanticAcceptanceFallbackSummary() const {
    return controller_.semanticAcceptanceFallbackSummary();
}

int DesktopShellViewModel::semanticAcceptanceAcceptedCount() const {
    return controller_.semanticAcceptanceAcceptedCount();
}

int DesktopShellViewModel::semanticAcceptanceBudgetCharacters() const {
    return controller_.semanticAcceptanceBudgetCharacters();
}

QStringList DesktopShellViewModel::semanticAcceptanceCandidateSummaries() const {
    return controller_.semanticAcceptanceCandidateSummaries();
}

QStringList DesktopShellViewModel::semanticAcceptanceChecks() const {
    return controller_.semanticAcceptanceChecks();
}

QString DesktopShellViewModel::semanticSupplementAssemblyStatus() const {
    return controller_.semanticSupplementAssemblyStatus();
}

QString DesktopShellViewModel::semanticSupplementAssemblyReadiness() const {
    return controller_.semanticSupplementAssemblyReadiness();
}

QString DesktopShellViewModel::semanticSupplementAssemblySummary() const {
    return controller_.semanticSupplementAssemblySummary();
}

QString DesktopShellViewModel::semanticSupplementAssemblyBudgetSummary() const {
    return controller_.semanticSupplementAssemblyBudgetSummary();
}

QString DesktopShellViewModel::semanticSupplementAssemblySafetySummary() const {
    return controller_.semanticSupplementAssemblySafetySummary();
}

int DesktopShellViewModel::semanticSupplementAssemblyBlockCount() const {
    return controller_.semanticSupplementAssemblyBlockCount();
}

int DesktopShellViewModel::semanticSupplementAssemblyBudgetCharacters() const {
    return controller_.semanticSupplementAssemblyBudgetCharacters();
}

QStringList DesktopShellViewModel::semanticSupplementAssemblyChecks() const {
    return controller_.semanticSupplementAssemblyChecks();
}

QString DesktopShellViewModel::semanticPromptAuthorityStatus() const {
    return controller_.semanticPromptAuthorityStatus();
}

QString DesktopShellViewModel::semanticPromptAuthorityDecisionSummary() const {
    return controller_.semanticPromptAuthorityDecisionSummary();
}

QString DesktopShellViewModel::semanticPromptAuthoritySafetySummary() const {
    return controller_.semanticPromptAuthoritySafetySummary();
}

QString DesktopShellViewModel::semanticPromptAuthorityReadinessSummary() const {
    return controller_.semanticPromptAuthorityReadinessSummary();
}

QString DesktopShellViewModel::semanticPromptAuthorityFallbackSummary() const {
    return controller_.semanticPromptAuthorityFallbackSummary();
}

QString DesktopShellViewModel::semanticPromptAuthorityAuditSummary() const {
    return controller_.semanticPromptAuthorityAuditSummary();
}

int DesktopShellViewModel::semanticPromptAuthorityWouldIncludeBlockCount() const {
    return controller_.semanticPromptAuthorityWouldIncludeBlockCount();
}

QStringList DesktopShellViewModel::semanticPromptAuthorityChecks() const {
    return controller_.semanticPromptAuthorityChecks();
}

bool DesktopShellViewModel::semanticPromptInclusionEnabled() const {
    return controller_.semanticPromptInclusionEnabled();
}

void DesktopShellViewModel::setSemanticPromptInclusionEnabled(bool enabled) {
    settings_.setSemanticPromptInclusionEnabled(enabled);
    if (controller_.semanticPromptInclusionEnabled() !=
        settings_.semanticPromptInclusionEnabled()) {
        controller_.setSemanticPromptInclusionEnabled(settings_.semanticPromptInclusionEnabled());
    }
}

QString DesktopShellViewModel::semanticPromptInclusionStatus() const {
    return controller_.semanticPromptInclusionStatus();
}

QString DesktopShellViewModel::semanticPromptInclusionSummary() const {
    return controller_.semanticPromptInclusionSummary();
}

int DesktopShellViewModel::semanticPromptInclusionIncludedCount() const {
    return controller_.semanticPromptInclusionIncludedCount();
}

QString DesktopShellViewModel::semanticPromptInclusionBudgetSummary() const {
    return controller_.semanticPromptInclusionBudgetSummary();
}

QString DesktopShellViewModel::semanticPromptInclusionFallbackSummary() const {
    return controller_.semanticPromptInclusionFallbackSummary();
}

QString DesktopShellViewModel::semanticPromptInclusionAuditSummary() const {
    return controller_.semanticPromptInclusionAuditSummary();
}

bool DesktopShellViewModel::semanticPromptInclusionDeterministicAuthorityPreserved() const {
    return controller_.semanticPromptInclusionDeterministicAuthorityPreserved();
}

QStringList DesktopShellViewModel::semanticPromptInclusionChecks() const {
    return controller_.semanticPromptInclusionChecks();
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

QString DesktopShellViewModel::conversationStoreStatus() const {
    return controller_.conversationStoreStatus();
}

int DesktopShellViewModel::conversationStoreConversationCount() const {
    return controller_.conversationStoreConversationCount();
}

QString DesktopShellViewModel::activeConversationSummary() const {
    return controller_.activeConversationSummary();
}

QStringList DesktopShellViewModel::conversationStoreSummaries() const {
    return controller_.conversationStoreSummaries();
}

QString DesktopShellViewModel::activeConversationId() const {
    return controller_.activeConversationId();
}

bool DesktopShellViewModel::activeConversationArchived() const {
    return controller_.activeConversationArchived();
}

QString DesktopShellViewModel::activeConversationStateSummary() const {
    return controller_.activeConversationStateSummary();
}

QStringList DesktopShellViewModel::conversationIds() const {
    return controller_.conversationIds();
}

QStringList DesktopShellViewModel::conversationTitles() const {
    return controller_.conversationTitles();
}

QStringList DesktopShellViewModel::conversationActiveSummaries() const {
    return controller_.conversationActiveSummaries();
}

QStringList DesktopShellViewModel::conversationLastUpdatedSummaries() const {
    return controller_.conversationLastUpdatedSummaries();
}

QStringList DesktopShellViewModel::conversationMessageCountSummaries() const {
    return controller_.conversationMessageCountSummaries();
}

QStringList DesktopShellViewModel::conversationArchivedSummaries() const {
    return controller_.conversationArchivedSummaries();
}

int DesktopShellViewModel::activeConversationCount() const {
    return controller_.activeConversationCount();
}

int DesktopShellViewModel::archivedConversationCount() const {
    return controller_.archivedConversationCount();
}

int DesktopShellViewModel::userCreatedConversationCount() const {
    return controller_.userCreatedConversationCount();
}

bool DesktopShellViewModel::conversationBrowserEmptyStateVisible() const {
    return controller_.conversationBrowserEmptyStateVisible();
}

QString DesktopShellViewModel::conversationBrowserEmptyStateSummary() const {
    return controller_.conversationBrowserEmptyStateSummary();
}

QString DesktopShellViewModel::conversationHistorySummaryText() const {
    return controller_.conversationHistorySummaryText();
}

QStringList DesktopShellViewModel::conversationHistorySummaryLines() const {
    return controller_.conversationHistorySummaryLines();
}

int DesktopShellViewModel::conversationHistoryMessageCount() const {
    return controller_.conversationHistoryMessageCount();
}

QString DesktopShellViewModel::conversationPersistenceStatus() const {
    return controller_.conversationPersistenceStatus();
}

QString DesktopShellViewModel::conversationLastSavedStatus() const {
    return controller_.conversationLastSavedStatus();
}

QString DesktopShellViewModel::conversationLastRestoredStatus() const {
    return controller_.conversationLastRestoredStatus();
}

QString DesktopShellViewModel::conversationBrowserStatus() const {
    return controller_.conversationBrowserStatus();
}

QString DesktopShellViewModel::conversationBrowserSummaryText() const {
    return controller_.conversationBrowserSummaryText();
}

int DesktopShellViewModel::conversationListEntryCount() const {
    return controller_.conversationListEntryCount();
}

QString DesktopShellViewModel::conversationListCurrentTitle() const {
    return controller_.conversationListCurrentTitle();
}

int DesktopShellViewModel::conversationListCurrentMessageCount() const {
    return controller_.conversationListCurrentMessageCount();
}

QString DesktopShellViewModel::conversationListCurrentPersistenceStatus() const {
    return controller_.conversationListCurrentPersistenceStatus();
}

QString DesktopShellViewModel::conversationListCurrentLastUpdatedSummary() const {
    return controller_.conversationListCurrentLastUpdatedSummary();
}

QString DesktopShellViewModel::conversationListCurrentSearchAvailabilitySummary() const {
    return controller_.conversationListCurrentSearchAvailabilitySummary();
}

QString DesktopShellViewModel::conversationListCurrentExportAvailabilitySummary() const {
    return controller_.conversationListCurrentExportAvailabilitySummary();
}

QString DesktopShellViewModel::conversationListCurrentSummary() const {
    return controller_.conversationListCurrentSummary();
}

QString DesktopShellViewModel::conversationCurrentStorageMode() const {
    return controller_.conversationCurrentStorageMode();
}

QString DesktopShellViewModel::conversationFutureStorageMode() const {
    return controller_.conversationFutureStorageMode();
}

QString DesktopShellViewModel::conversationMigrationReadiness() const {
    return controller_.conversationMigrationReadiness();
}

QString DesktopShellViewModel::conversationMigrationStatusSummary() const {
    return controller_.conversationMigrationStatusSummary();
}

QString DesktopShellViewModel::conversationSchemaStatusSummary() const {
    return controller_.conversationSchemaStatusSummary();
}

QString DesktopShellViewModel::conversationSearchQueryText() const {
    return controller_.conversationSearchQueryText();
}

QString DesktopShellViewModel::conversationSearchStatus() const {
    return controller_.conversationSearchStatus();
}

QString DesktopShellViewModel::conversationSearchSummaryText() const {
    return controller_.conversationSearchSummaryText();
}

int DesktopShellViewModel::conversationSearchResultCount() const {
    return controller_.conversationSearchResultCount();
}

QStringList DesktopShellViewModel::conversationSearchResultSummaries() const {
    return controller_.conversationSearchResultSummaries();
}

bool DesktopShellViewModel::conversationExportAvailable() const {
    return controller_.conversationExportAvailable();
}

QString DesktopShellViewModel::conversationExportReadinessStatus() const {
    return controller_.conversationExportReadinessStatus();
}

QString DesktopShellViewModel::conversationExportReadinessSummary() const {
    return controller_.conversationExportReadinessSummary();
}

QStringList DesktopShellViewModel::conversationExportReadinessChecks() const {
    return controller_.conversationExportReadinessChecks();
}

QString DesktopShellViewModel::conversationExportLastResultSummary() const {
    return controller_.conversationExportLastResultSummary();
}

QString DesktopShellViewModel::conversationExportLastStatus() const {
    return controller_.conversationExportLastStatus();
}

QString DesktopShellViewModel::conversationExportLastFileName() const {
    return controller_.conversationExportLastFileName();
}

int DesktopShellViewModel::conversationExportLastMessageCount() const {
    return controller_.conversationExportLastMessageCount();
}

QString DesktopShellViewModel::conversationExportLastTimestamp() const {
    return controller_.conversationExportLastTimestamp();
}

bool DesktopShellViewModel::conversationDeleteAvailable() const {
    return controller_.conversationDeleteAvailable();
}

QString DesktopShellViewModel::conversationDeletePolicyStatus() const {
    return controller_.conversationDeletePolicyStatus();
}

QString DesktopShellViewModel::conversationDeletePolicySummary() const {
    return controller_.conversationDeletePolicySummary();
}

QStringList DesktopShellViewModel::conversationDeletePolicyRequirements() const {
    return controller_.conversationDeletePolicyRequirements();
}

QString DesktopShellViewModel::conversationDeleteReadinessStatus() const {
    return controller_.conversationDeleteReadinessStatus();
}

QString DesktopShellViewModel::conversationDeleteReadinessSummary() const {
    return controller_.conversationDeleteReadinessSummary();
}

QStringList DesktopShellViewModel::conversationDeleteReadinessChecks() const {
    return controller_.conversationDeleteReadinessChecks();
}

QString DesktopShellViewModel::conversationDeleteLastStatus() const {
    return controller_.conversationDeleteLastStatus();
}

QString DesktopShellViewModel::conversationDeleteLastResultSummary() const {
    return controller_.conversationDeleteLastResultSummary();
}

int DesktopShellViewModel::memoryCandidateCount() const {
    return controller_.memoryCandidateCount();
}

int DesktopShellViewModel::pendingMemoryCandidateCount() const {
    return controller_.pendingMemoryCandidateCount();
}

int DesktopShellViewModel::approvedMemoryCandidateCount() const {
    return controller_.approvedMemoryCandidateCount();
}

int DesktopShellViewModel::rejectedMemoryCandidateCount() const {
    return controller_.rejectedMemoryCandidateCount();
}

int DesktopShellViewModel::archivedMemoryCandidateCount() const {
    return controller_.archivedMemoryCandidateCount();
}

int DesktopShellViewModel::committedMemoryCandidateCount() const {
    return controller_.committedMemoryCandidateCount();
}

QStringList DesktopShellViewModel::memoryCandidateIds() const {
    return controller_.memoryCandidateIds();
}

QStringList DesktopShellViewModel::memoryCandidateReviewStates() const {
    return controller_.memoryCandidateReviewStates();
}

QStringList DesktopShellViewModel::memoryCandidateCommitStatuses() const {
    return controller_.memoryCandidateCommitStatuses();
}

QStringList DesktopShellViewModel::memoryCandidateSummaries() const {
    return controller_.memoryCandidateSummaries();
}

QStringList DesktopShellViewModel::pendingMemoryCandidateSummaries() const {
    return controller_.pendingMemoryCandidateSummaries();
}

QStringList DesktopShellViewModel::approvedMemoryCandidateSummaries() const {
    return controller_.approvedMemoryCandidateSummaries();
}

QStringList DesktopShellViewModel::rejectedMemoryCandidateSummaries() const {
    return controller_.rejectedMemoryCandidateSummaries();
}

QStringList DesktopShellViewModel::archivedMemoryCandidateSummaries() const {
    return controller_.archivedMemoryCandidateSummaries();
}

QString DesktopShellViewModel::lastMemoryCandidateReviewStatus() const {
    return controller_.lastMemoryCandidateReviewStatus();
}

QString DesktopShellViewModel::lastMemoryCandidateReviewSummary() const {
    return controller_.lastMemoryCandidateReviewSummary();
}

QString DesktopShellViewModel::memoryCommitReadinessStatus() const {
    return controller_.memoryCommitReadinessStatus();
}

QString DesktopShellViewModel::memoryCommitReadinessSummary() const {
    return controller_.memoryCommitReadinessSummary();
}

QStringList DesktopShellViewModel::memoryCommitReadinessChecks() const {
    return controller_.memoryCommitReadinessChecks();
}

int DesktopShellViewModel::memoryCommitPlanCount() const {
    return controller_.memoryCommitPlanCount();
}

QString DesktopShellViewModel::memoryCommitTargetSummary() const {
    return controller_.memoryCommitTargetSummary();
}

QStringList DesktopShellViewModel::memoryCommitCandidateSummaries() const {
    return controller_.memoryCommitCandidateSummaries();
}

QString DesktopShellViewModel::lastMemoryCommitStatus() const {
    return controller_.lastMemoryCommitStatus();
}

QString DesktopShellViewModel::lastMemoryCommitResultSummary() const {
    return controller_.lastMemoryCommitResultSummary();
}

QString DesktopShellViewModel::memoryRecallPolicyStatus() const {
    return controller_.memoryRecallPolicyStatus();
}

QString DesktopShellViewModel::memoryRecallPolicySummary() const {
    return controller_.memoryRecallPolicySummary();
}

QString DesktopShellViewModel::memoryRecallQueryText() const {
    return controller_.memoryRecallQueryText();
}

QString DesktopShellViewModel::memoryRecallStatus() const {
    return controller_.memoryRecallStatus();
}

QString DesktopShellViewModel::memoryRecallSummaryText() const {
    return controller_.memoryRecallSummaryText();
}

int DesktopShellViewModel::memoryRecallResultCount() const {
    return controller_.memoryRecallResultCount();
}

QStringList DesktopShellViewModel::memoryRecallResultSummaries() const {
    return controller_.memoryRecallResultSummaries();
}

QString DesktopShellViewModel::contextAssemblyPolicyStatus() const {
    return controller_.contextAssemblyPolicyStatus();
}

QString DesktopShellViewModel::contextAssemblyPolicySummary() const {
    return controller_.contextAssemblyPolicySummary();
}

QString DesktopShellViewModel::contextAssemblyStatus() const {
    return controller_.contextAssemblyStatus();
}

QString DesktopShellViewModel::contextAssemblySummaryText() const {
    return controller_.contextAssemblySummaryText();
}

int DesktopShellViewModel::contextAssemblySourceCount() const {
    return controller_.contextAssemblySourceCount();
}

int DesktopShellViewModel::contextAssemblyAvailableSourceCount() const {
    return controller_.contextAssemblyAvailableSourceCount();
}

int DesktopShellViewModel::contextAssemblyCandidateBlockCount() const {
    return controller_.contextAssemblyCandidateBlockCount();
}

int DesktopShellViewModel::contextAssemblyEstimatedSize() const {
    return controller_.contextAssemblyEstimatedSize();
}

QString DesktopShellViewModel::conversationContextAvailability() const {
    return controller_.conversationContextAvailability();
}

QString DesktopShellViewModel::committedMemoryContextAvailability() const {
    return controller_.committedMemoryContextAvailability();
}

QString DesktopShellViewModel::runtimeMetadataContextAvailability() const {
    return controller_.runtimeMetadataContextAvailability();
}

QString DesktopShellViewModel::orchestrationContextAvailability() const {
    return controller_.orchestrationContextAvailability();
}

QStringList DesktopShellViewModel::contextAssemblySourceSummaries() const {
    return controller_.contextAssemblySourceSummaries();
}

QStringList DesktopShellViewModel::contextAssemblyReadinessChecks() const {
    return controller_.contextAssemblyReadinessChecks();
}

int DesktopShellViewModel::memoryEntryCount() const {
    return controller_.memoryEntryCount();
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

bool DesktopShellViewModel::searchConversation(const QString& query) {
    return controller_.searchConversation(query);
}

void DesktopShellViewModel::clearConversationSearch() {
    controller_.clearConversationSearch();
}

bool DesktopShellViewModel::exportTranscript(const QString& format) {
    return controller_.exportTranscript(format);
}

bool DesktopShellViewModel::requestConversationExport(const QString& format) {
    return exportTranscript(format);
}

QString DesktopShellViewModel::createConversation(const QString& title) {
    return controller_.createConversation(title);
}

bool DesktopShellViewModel::switchConversation(const QString& conversationId) {
    return controller_.switchConversation(conversationId);
}

bool DesktopShellViewModel::renameConversation(const QString& conversationId,
                                               const QString& title) {
    return controller_.renameConversation(conversationId, title);
}

bool DesktopShellViewModel::archiveConversation(const QString& conversationId) {
    return controller_.archiveConversation(conversationId);
}

bool DesktopShellViewModel::unarchiveConversation(const QString& conversationId) {
    return controller_.unarchiveConversation(conversationId);
}

bool DesktopShellViewModel::requestPermanentDeleteConversation(const QString& conversationId) {
    return controller_.requestPermanentDeleteConversation(conversationId);
}

QString DesktopShellViewModel::createMemoryCandidateFromConversationText(const QString& text) {
    return controller_.createMemoryCandidateFromConversationText(text);
}

bool DesktopShellViewModel::approveMemoryCandidate(const QString& candidateId) {
    return controller_.approveMemoryCandidate(candidateId);
}

bool DesktopShellViewModel::rejectMemoryCandidate(const QString& candidateId) {
    return controller_.rejectMemoryCandidate(candidateId);
}

bool DesktopShellViewModel::resetMemoryCandidate(const QString& candidateId) {
    return controller_.resetMemoryCandidate(candidateId);
}

bool DesktopShellViewModel::archiveMemoryCandidate(const QString& candidateId) {
    return controller_.archiveMemoryCandidate(candidateId);
}

bool DesktopShellViewModel::requestMemoryCandidateCommit(const QString& candidateId) {
    return controller_.requestMemoryCandidateCommit(candidateId);
}

bool DesktopShellViewModel::recallLocalMemory(const QString& query) {
    return controller_.recallLocalMemory(query);
}

void DesktopShellViewModel::clearLocalMemoryRecall() {
    controller_.clearLocalMemoryRecall();
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
