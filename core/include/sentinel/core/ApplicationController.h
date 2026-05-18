#pragma once

#include "sentinel/core/AgentActivityLog.h"
#include "sentinel/core/AgentPipelineResult.h"
#include "sentinel/core/AgentRuntimeContext.h"
#include "sentinel/core/ChatSession.h"
#include "sentinel/core/ConversationHistoryMetadata.h"
#include "sentinel/core/ConversationSession.h"
#include "sentinel/core/ConversationStateGraph.h"
#include "sentinel/core/ExecutionLifecycle.h"
#include "sentinel/core/IAgentRegistry.h"
#include "sentinel/core/IAgentRuntime.h"
#include "sentinel/core/IApprovalPolicy.h"
#include "sentinel/core/IChatHistoryStore.h"
#include "sentinel/core/IChatProvider.h"
#include "sentinel/core/IConversationStore.h"
#include "sentinel/core/IMemoryCatalog.h"
#include "sentinel/core/IMemoryStore.h"
#include "sentinel/core/IModelRouter.h"
#include "sentinel/core/IProviderCatalog.h"
#include "sentinel/core/ISandboxPolicy.h"
#include "sentinel/core/ITaskPlanner.h"
#include "sentinel/core/IToolExecutor.h"
#include "sentinel/core/LocalInference.h"
#include "sentinel/core/LocalRuntime.h"
#include "sentinel/core/LocalRuntimeSession.h"
#include "sentinel/core/ModelManagement.h"
#include "sentinel/core/OllamaRuntime.h"
#include "sentinel/core/OrchestrationDiagnostics.h"
#include "sentinel/core/OrchestrationSnapshot.h"
#include "sentinel/core/PiperTts.h"
#include "sentinel/core/RuntimeCapabilities.h"
#include "sentinel/core/RuntimeIntegration.h"
#include "sentinel/core/RuntimePermissions.h"
#include "sentinel/core/RuntimePipeline.h"
#include "sentinel/core/RuntimeSafety.h"
#include "sentinel/core/Voice.h"

#include <QObject>
#include <QStringList>
#include <memory>

namespace sentinel::core {

class ApplicationController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString providerName READ providerName CONSTANT)
    Q_PROPERTY(QString providerStatus READ providerStatus CONSTANT)
    Q_PROPERTY(QString agentStatus READ agentStatus NOTIFY agentStatusChanged)
    Q_PROPERTY(QString lastAgentResponse READ lastAgentResponse NOTIFY agentResponseChanged)
    Q_PROPERTY(QString latestToolPlanStatus READ latestToolPlanStatus NOTIFY toolPlanChanged)
    Q_PROPERTY(QString latestToolPlanSummary READ latestToolPlanSummary NOTIFY toolPlanChanged)
    Q_PROPERTY(QString latestApprovalStatus READ latestApprovalStatus NOTIFY approvalChanged)
    Q_PROPERTY(QString latestApprovalSummary READ latestApprovalSummary NOTIFY approvalChanged)
    Q_PROPERTY(QString latestSandboxStatus READ latestSandboxStatus NOTIFY sandboxChanged)
    Q_PROPERTY(QString latestSandboxSummary READ latestSandboxSummary NOTIFY sandboxChanged)
    Q_PROPERTY(QString latestToolExecutionStatus READ latestToolExecutionStatus NOTIFY
                   toolExecutionChanged)
    Q_PROPERTY(QString latestToolExecutionSummary READ latestToolExecutionSummary NOTIFY
                   toolExecutionChanged)
    Q_PROPERTY(QString latestAgentPipelineStatus READ latestAgentPipelineStatus NOTIFY
                   agentPipelineChanged)
    Q_PROPERTY(QString latestAgentPipelineSummary READ latestAgentPipelineSummary NOTIFY
                   agentPipelineChanged)
    Q_PROPERTY(QString runtimeSessionId READ runtimeSessionId NOTIFY runtimeContextChanged)
    Q_PROPERTY(QString runtimeContextStatus READ runtimeContextStatus NOTIFY runtimeContextChanged)
    Q_PROPERTY(
        QString runtimeContextSummary READ runtimeContextSummary NOTIFY runtimeContextChanged)
    Q_PROPERTY(QStringList runtimeContextActiveToolIds READ runtimeContextActiveToolIds NOTIFY
                   runtimeContextChanged)
    Q_PROPERTY(
        QString conversationSessionId READ conversationSessionId NOTIFY conversationSessionChanged)
    Q_PROPERTY(QString conversationSessionStatus READ conversationSessionStatus NOTIFY
                   conversationSessionChanged)
    Q_PROPERTY(QString interactionMode READ interactionMode NOTIFY conversationSessionChanged)
    Q_PROPERTY(QString attentionState READ attentionState NOTIFY conversationSessionChanged)
    Q_PROPERTY(
        QString contextWindowSummary READ contextWindowSummary NOTIFY conversationSessionChanged)
    Q_PROPERTY(QString conversationState READ conversationState NOTIFY conversationStateChanged)
    Q_PROPERTY(QString conversationTransitionStatus READ conversationTransitionStatus NOTIFY
                   conversationStateChanged)
    Q_PROPERTY(QString conversationTransitionSummary READ conversationTransitionSummary NOTIFY
                   conversationStateChanged)
    Q_PROPERTY(QString conversationRuntimeSummary READ conversationRuntimeSummary NOTIFY
                   conversationRuntimeChanged)
    Q_PROPERTY(QStringList conversationRuntimeSummaryLines READ conversationRuntimeSummaryLines
                   NOTIFY conversationRuntimeChanged)
    Q_PROPERTY(QString conversationRuntimeRequestId READ conversationRuntimeRequestId NOTIFY
                   conversationRuntimeChanged)
    Q_PROPERTY(QString conversationRuntimeActiveModel READ conversationRuntimeActiveModel NOTIFY
                   conversationRuntimeChanged)
    Q_PROPERTY(QString conversationRuntimeActiveRoute READ conversationRuntimeActiveRoute NOTIFY
                   conversationRuntimeChanged)
    Q_PROPERTY(bool conversationRuntimeStreaming READ conversationRuntimeStreaming NOTIFY
                   conversationRuntimeChanged)
    Q_PROPERTY(QString conversationRuntimeLastSuccessSummary READ
                   conversationRuntimeLastSuccessSummary NOTIFY conversationRuntimeChanged)
    Q_PROPERTY(QString conversationRuntimeLastErrorSummary READ conversationRuntimeLastErrorSummary
                   NOTIFY conversationRuntimeChanged)
    Q_PROPERTY(QString conversationRuntimeLastLatencySummary READ
                   conversationRuntimeLastLatencySummary NOTIFY conversationRuntimeChanged)
    Q_PROPERTY(int agentActivityCount READ agentActivityCount NOTIFY agentActivityChanged)
    Q_PROPERTY(QString latestAgentActivitySummary READ latestAgentActivitySummary NOTIFY
                   agentActivityChanged)
    Q_PROPERTY(QString currentRoutingMode READ currentRoutingMode WRITE setRoutingModeByName NOTIFY
                   modelRoutingChanged)
    Q_PROPERTY(QString modelRoutingStatus READ modelRoutingStatus NOTIFY modelRoutingChanged)
    Q_PROPERTY(QString selectedModelProviderSummary READ selectedModelProviderSummary NOTIFY
                   modelRoutingChanged)
    Q_PROPERTY(QString latestTaskPlanStatus READ latestTaskPlanStatus NOTIFY taskPlanChanged)
    Q_PROPERTY(QString latestTaskPlanSummary READ latestTaskPlanSummary NOTIFY taskPlanChanged)
    Q_PROPERTY(int plannedTaskStepCount READ plannedTaskStepCount NOTIFY taskPlanChanged)
    Q_PROPERTY(int registeredAgentCount READ registeredAgentCount CONSTANT)
    Q_PROPERTY(QStringList activeAgentSummaries READ activeAgentSummaries CONSTANT)
    Q_PROPERTY(QString currentAgentSummary READ currentAgentSummary NOTIFY taskPlanChanged)
    Q_PROPERTY(QString currentMemoryAffinitySummary READ currentMemoryAffinitySummary NOTIFY
                   taskPlanChanged)
    Q_PROPERTY(int providerCatalogCount READ providerCatalogCount CONSTANT)
    Q_PROPERTY(QStringList providerCatalogSummaries READ providerCatalogSummaries CONSTANT)
    Q_PROPERTY(int memoryCatalogCount READ memoryCatalogCount CONSTANT)
    Q_PROPERTY(QStringList memoryCatalogSummaries READ memoryCatalogSummaries CONSTANT)
    Q_PROPERTY(QString orchestrationSnapshotStatus READ orchestrationSnapshotStatus NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(QString orchestrationSnapshotSummary READ orchestrationSnapshotSummary NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(QStringList orchestrationSignals READ orchestrationSignals NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(QString orchestrationReadinessStatus READ orchestrationReadinessStatus NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(QString orchestrationReadinessSummary READ orchestrationReadinessSummary NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(QStringList orchestrationDiagnostics READ orchestrationDiagnostics NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(QString localRuntimeStatus READ localRuntimeStatus CONSTANT)
    Q_PROPERTY(QString localRuntimeHealth READ localRuntimeHealth CONSTANT)
    Q_PROPERTY(QString localRuntimeSummary READ localRuntimeSummary CONSTANT)
    Q_PROPERTY(QStringList localRuntimeCapabilities READ localRuntimeCapabilities CONSTANT)
    Q_PROPERTY(QString localRuntimeResponseStatus READ localRuntimeResponseStatus CONSTANT)
    Q_PROPERTY(QString localRuntimeResponseSummary READ localRuntimeResponseSummary CONSTANT)
    Q_PROPERTY(int localRuntimeSessionCount READ localRuntimeSessionCount CONSTANT)
    Q_PROPERTY(QString localRuntimeSessionStatus READ localRuntimeSessionStatus CONSTANT)
    Q_PROPERTY(QString localRuntimeSessionHealth READ localRuntimeSessionHealth CONSTANT)
    Q_PROPERTY(QString localRuntimeSessionSummary READ localRuntimeSessionSummary CONSTANT)
    Q_PROPERTY(QString localRuntimeAllocationSummary READ localRuntimeAllocationSummary CONSTANT)
    Q_PROPERTY(QString localRuntimeReservationSummary READ localRuntimeReservationSummary CONSTANT)
    Q_PROPERTY(QStringList localRuntimeSessionSummaries READ localRuntimeSessionSummaries CONSTANT)
    Q_PROPERTY(int runtimeCapabilityCount READ runtimeCapabilityCount CONSTANT)
    Q_PROPERTY(QStringList enabledRuntimeCapabilitySummaries READ enabledRuntimeCapabilitySummaries
                   CONSTANT)
    Q_PROPERTY(QStringList disabledRuntimeCapabilitySummaries READ
                   disabledRuntimeCapabilitySummaries CONSTANT)
    Q_PROPERTY(
        QString runtimeNegotiationProfileSummary READ runtimeNegotiationProfileSummary CONSTANT)
    Q_PROPERTY(QString runtimeNegotiationSummary READ runtimeNegotiationSummary CONSTANT)
    Q_PROPERTY(
        QString localOnlyRuntimeEnforcementSummary READ localOnlyRuntimeEnforcementSummary CONSTANT)
    Q_PROPERTY(QString runtimePermissionDecision READ runtimePermissionDecision CONSTANT)
    Q_PROPERTY(QString runtimePermissionSummary READ runtimePermissionSummary CONSTANT)
    Q_PROPERTY(QString runtimeSafetyDecision READ runtimeSafetyDecision CONSTANT)
    Q_PROPERTY(QString runtimeSafetySummary READ runtimeSafetySummary CONSTANT)
    Q_PROPERTY(QString runtimePipelineStatus READ runtimePipelineStatus CONSTANT)
    Q_PROPERTY(QString runtimePipelineSummary READ runtimePipelineSummary CONSTANT)
    Q_PROPERTY(
        QStringList runtimePipelineTraceSummaries READ runtimePipelineTraceSummaries CONSTANT)
    Q_PROPERTY(QString executionLifecycleState READ executionLifecycleState CONSTANT)
    Q_PROPERTY(QString executionLifecycleStatus READ executionLifecycleStatus CONSTANT)
    Q_PROPERTY(QString executionLifecycleSummary READ executionLifecycleSummary CONSTANT)
    Q_PROPERTY(
        QStringList executionLifecycleTraceSummaries READ executionLifecycleTraceSummaries CONSTANT)
    Q_PROPERTY(QString executionSessionId READ executionSessionId CONSTANT)
    Q_PROPERTY(QString executionSessionStatus READ executionSessionStatus CONSTANT)
    Q_PROPERTY(QString executionSessionOwnership READ executionSessionOwnership CONSTANT)
    Q_PROPERTY(QString executionCoordinationMode READ executionCoordinationMode CONSTANT)
    Q_PROPERTY(QString executionSessionSummary READ executionSessionSummary CONSTANT)
    Q_PROPERTY(QString executionCoordinationSnapshotSummary READ
                   executionCoordinationSnapshotSummary CONSTANT)
    Q_PROPERTY(QString localRuntimeAdapterStatus READ localRuntimeAdapterStatus CONSTANT)
    Q_PROPERTY(QString localRuntimeAdapterHealth READ localRuntimeAdapterHealth CONSTANT)
    Q_PROPERTY(QString localRuntimeAdapterSummary READ localRuntimeAdapterSummary CONSTANT)
    Q_PROPERTY(QStringList localRuntimeAdapterCapabilitySummaries READ
                   localRuntimeAdapterCapabilitySummaries CONSTANT)
    Q_PROPERTY(QString providerRuntimeBridgeStatus READ providerRuntimeBridgeStatus CONSTANT)
    Q_PROPERTY(QString providerRuntimeBridgeSummary READ providerRuntimeBridgeSummary CONSTANT)
    Q_PROPERTY(QString providerRuntimeBridgeResponseSummary READ
                   providerRuntimeBridgeResponseSummary CONSTANT)
    Q_PROPERTY(
        QString runtimeIntegrationReadinessStatus READ runtimeIntegrationReadinessStatus CONSTANT)
    Q_PROPERTY(
        QString runtimeIntegrationReadinessSummary READ runtimeIntegrationReadinessSummary CONSTANT)
    Q_PROPERTY(QStringList runtimeIntegrationReadinessChecks READ runtimeIntegrationReadinessChecks
                   CONSTANT)
    Q_PROPERTY(QString ollamaEndpoint READ ollamaEndpoint CONSTANT)
    Q_PROPERTY(QString ollamaConnectionStatus READ ollamaConnectionStatus CONSTANT)
    Q_PROPERTY(QString ollamaHealthStatus READ ollamaHealthStatus CONSTANT)
    Q_PROPERTY(QString ollamaHealthSummary READ ollamaHealthSummary CONSTANT)
    Q_PROPERTY(int ollamaModelCount READ ollamaModelCount CONSTANT)
    Q_PROPERTY(QStringList ollamaModelNames READ ollamaModelNames CONSTANT)
    Q_PROPERTY(QStringList ollamaModelSummaries READ ollamaModelSummaries CONSTANT)
    Q_PROPERTY(QString selectedLocalModel READ selectedLocalModel WRITE setSelectedLocalModel NOTIFY
                   localModelSelectionChanged)
    Q_PROPERTY(QString selectedLocalModelStatus READ selectedLocalModelStatus NOTIFY
                   localModelSelectionChanged)
    Q_PROPERTY(QString selectedLocalModelSummary READ selectedLocalModelSummary NOTIFY
                   localModelSelectionChanged)
    Q_PROPERTY(QString selectedLocalModelMetadataSummary READ selectedLocalModelMetadataSummary
                   NOTIFY localModelSelectionChanged)
    Q_PROPERTY(QString activeLocalRuntimeBadge READ activeLocalRuntimeBadge NOTIFY
                   localModelSelectionChanged)
    Q_PROPERTY(
        QString modelManagementStatus READ modelManagementStatus NOTIFY localModelSelectionChanged)
    Q_PROPERTY(QString modelManagementSummary READ modelManagementSummary NOTIFY
                   localModelSelectionChanged)
    Q_PROPERTY(
        QString modelManagementActionAvailability READ modelManagementActionAvailability CONSTANT)
    Q_PROPERTY(QStringList modelRecommendationSummaries READ modelRecommendationSummaries CONSTANT)
    Q_PROPERTY(QStringList modelRequirementSummaries READ modelRequirementSummaries CONSTANT)
    Q_PROPERTY(QString voiceRuntimeMode READ voiceRuntimeMode CONSTANT)
    Q_PROPERTY(bool voiceEnabled READ voiceEnabled CONSTANT)
    Q_PROPERTY(QString voiceReadinessStatus READ voiceReadinessStatus CONSTANT)
    Q_PROPERTY(QString voiceReadinessSummary READ voiceReadinessSummary CONSTANT)
    Q_PROPERTY(QStringList voiceReadinessChecks READ voiceReadinessChecks CONSTANT)
    Q_PROPERTY(QStringList voiceCapabilitySummaries READ voiceCapabilitySummaries CONSTANT)
    Q_PROPERTY(QString textToSpeechStatus READ textToSpeechStatus CONSTANT)
    Q_PROPERTY(QString textToSpeechSummary READ textToSpeechSummary CONSTANT)
    Q_PROPERTY(QString speechToTextStatus READ speechToTextStatus CONSTANT)
    Q_PROPERTY(QString speechToTextSummary READ speechToTextSummary CONSTANT)
    Q_PROPERTY(QString voiceSessionId READ voiceSessionId CONSTANT)
    Q_PROPERTY(QString voiceSessionStatus READ voiceSessionStatus CONSTANT)
    Q_PROPERTY(QString voiceSessionSummary READ voiceSessionSummary CONSTANT)
    Q_PROPERTY(QString voicePipelineStatus READ voicePipelineStatus CONSTANT)
    Q_PROPERTY(QString voicePipelineSummary READ voicePipelineSummary CONSTANT)
    Q_PROPERTY(QStringList voicePipelineTraceSummaries READ voicePipelineTraceSummaries CONSTANT)
    Q_PROPERTY(QString voiceRuntimeStatus READ voiceRuntimeStatus CONSTANT)
    Q_PROPERTY(QString voiceRuntimeSummary READ voiceRuntimeSummary CONSTANT)
    Q_PROPERTY(QStringList voiceRuntimeCheckSummaries READ voiceRuntimeCheckSummaries CONSTANT)
    Q_PROPERTY(bool voiceRuntimeAvailable READ voiceRuntimeAvailable CONSTANT)
    Q_PROPERTY(bool voiceTextToSpeechAvailable READ voiceTextToSpeechAvailable CONSTANT)
    Q_PROPERTY(bool voiceSpeechToTextAvailable READ voiceSpeechToTextAvailable CONSTANT)
    Q_PROPERTY(bool voiceMicrophoneEnabled READ voiceMicrophoneEnabled CONSTANT)
    Q_PROPERTY(bool voicePlaybackEnabled READ voicePlaybackEnabled CONSTANT)
    Q_PROPERTY(bool voiceLocalOnlyPolicy READ voiceLocalOnlyPolicy CONSTANT)
    Q_PROPERTY(bool voiceProcessExecutionEnabled READ voiceProcessExecutionEnabled CONSTANT)
    Q_PROPERTY(QString voiceRuntimeEnvironmentStatus READ voiceRuntimeEnvironmentStatus NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QString voiceRuntimeEnvironmentSummary READ voiceRuntimeEnvironmentSummary NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(
        QStringList voiceBinarySummaries READ voiceBinarySummaries NOTIFY voiceConfigurationChanged)
    Q_PROPERTY(
        QStringList voiceModelSummaries READ voiceModelSummaries NOTIFY voiceConfigurationChanged)
    Q_PROPERTY(
        QStringList voiceRuntimePermissionSummaries READ voiceRuntimePermissionSummaries CONSTANT)
    Q_PROPERTY(QString voiceRuntimeSafetyStatus READ voiceRuntimeSafetyStatus CONSTANT)
    Q_PROPERTY(QString voiceRuntimeSafetySummary READ voiceRuntimeSafetySummary CONSTANT)
    Q_PROPERTY(QStringList voiceRuntimeSafetyChecks READ voiceRuntimeSafetyChecks CONSTANT)
    Q_PROPERTY(bool voiceRuntimeExecutionAllowed READ voiceRuntimeExecutionAllowed CONSTANT)
    Q_PROPERTY(QString piperTtsStatus READ piperTtsStatus NOTIFY voiceConfigurationChanged)
    Q_PROPERTY(QString piperTtsSummary READ piperTtsSummary NOTIFY voiceConfigurationChanged)
    Q_PROPERTY(QStringList piperTtsReadinessChecks READ piperTtsReadinessChecks NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(bool piperTtsReady READ piperTtsReady NOTIFY voiceConfigurationChanged)
    Q_PROPERTY(QString piperTtsFileOutputStatus READ piperTtsFileOutputStatus NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QString piperTtsFileOutputSummary READ piperTtsFileOutputSummary NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QString piperBinaryPath READ piperBinaryPath WRITE setPiperBinaryPath NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QString piperModelPath READ piperModelPath WRITE setPiperModelPath NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QString whisperBinaryPath READ whisperBinaryPath WRITE setWhisperBinaryPath NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QString whisperModelPath READ whisperModelPath WRITE setWhisperModelPath NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QStringList voiceConfigurationSummaries READ voiceConfigurationSummaries NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QString voiceConfigurationReadinessSummary READ voiceConfigurationReadinessSummary
                   NOTIFY voiceConfigurationChanged)
    Q_PROPERTY(QStringList voiceConfigurationStatusBadges READ voiceConfigurationStatusBadges NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QStringList voiceConfigurationHintSummaries READ voiceConfigurationHintSummaries
                   NOTIFY voiceConfigurationChanged)
    Q_PROPERTY(QStringList voiceConfigurationValidationSummaries READ
                   voiceConfigurationValidationSummaries NOTIFY voiceConfigurationChanged)
    Q_PROPERTY(QString piperFileOutputReadinessStatus READ piperFileOutputReadinessStatus NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QString piperFileOutputReadinessSummary READ piperFileOutputReadinessSummary NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(bool piperFileOutputExecutionEnabled READ piperFileOutputExecutionEnabled WRITE
                   setPiperFileOutputExecutionEnabled NOTIFY voiceConfigurationChanged)
    Q_PROPERTY(QString piperFileOutputExecutionStatus READ piperFileOutputExecutionStatus NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QString piperFileOutputExecutionSummary READ piperFileOutputExecutionSummary NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QString piperFileOutputAudioPathSummary READ piperFileOutputAudioPathSummary NOTIFY
                   voiceConfigurationChanged)
    Q_PROPERTY(QString whisperPreparationReadinessStatus READ whisperPreparationReadinessStatus
                   NOTIFY voiceConfigurationChanged)
    Q_PROPERTY(QString whisperPreparationReadinessSummary READ whisperPreparationReadinessSummary
                   NOTIFY voiceConfigurationChanged)
    Q_PROPERTY(bool localChatInferenceEnabled READ localChatInferenceEnabled WRITE
                   setLocalChatInferenceEnabled NOTIFY localChatInferenceRoutingChanged)
    Q_PROPERTY(QString localChatInferenceStatus READ localChatInferenceStatus NOTIFY
                   localChatInferenceRoutingChanged)
    Q_PROPERTY(QString localChatInferenceSummary READ localChatInferenceSummary NOTIFY
                   localChatInferenceRoutingChanged)
    Q_PROPERTY(bool localInferenceStreamingEnabled READ localInferenceStreamingEnabled WRITE
                   setLocalInferenceStreamingEnabled NOTIFY localInferenceChanged)
    Q_PROPERTY(bool localInferenceBusy READ localInferenceBusy NOTIFY localInferenceChanged)
    Q_PROPERTY(QString localInferenceRuntimeState READ localInferenceRuntimeState NOTIFY
                   localInferenceChanged)
    Q_PROPERTY(QString localInferenceStatus READ localInferenceStatus NOTIFY localInferenceChanged)
    Q_PROPERTY(
        QString localInferenceSummary READ localInferenceSummary NOTIFY localInferenceChanged)
    Q_PROPERTY(QString localInferenceLastResponseSummary READ localInferenceLastResponseSummary
                   NOTIFY localInferenceChanged)
    Q_PROPERTY(QString localInferenceLatencySummary READ localInferenceLatencySummary NOTIFY
                   localInferenceChanged)
    Q_PROPERTY(QStringList localInferenceTraceSummaries READ localInferenceTraceSummaries NOTIFY
                   localInferenceChanged)
    Q_PROPERTY(bool localInferenceStreamingAvailable READ localInferenceStreamingAvailable NOTIFY
                   localInferenceChanged)
    Q_PROPERTY(QString localInferenceStreamStatus READ localInferenceStreamStatus NOTIFY
                   localInferenceChanged)
    Q_PROPERTY(QString localInferenceStreamSummary READ localInferenceStreamSummary NOTIFY
                   localInferenceChanged)
    Q_PROPERTY(QString localInferenceStreamingText READ localInferenceStreamingText NOTIFY
                   localInferenceChanged)
    Q_PROPERTY(int availableToolCount READ availableToolCount CONSTANT)
    Q_PROPERTY(QStringList availableToolIds READ availableToolIds CONSTANT)
    Q_PROPERTY(QStringList chatMessages READ chatMessages NOTIFY chatMessagesChanged)
    Q_PROPERTY(
        QString conversationStoreStatus READ conversationStoreStatus NOTIFY chatMessagesChanged)
    Q_PROPERTY(int conversationStoreConversationCount READ conversationStoreConversationCount NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(
        QString activeConversationSummary READ activeConversationSummary NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList conversationStoreSummaries READ conversationStoreSummaries NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(QString activeConversationId READ activeConversationId NOTIFY chatMessagesChanged)
    Q_PROPERTY(
        bool activeConversationArchived READ activeConversationArchived NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList conversationIds READ conversationIds NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList conversationTitles READ conversationTitles NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList conversationLastUpdatedSummaries READ conversationLastUpdatedSummaries
                   NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList conversationMessageCountSummaries READ conversationMessageCountSummaries
                   NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList conversationArchivedSummaries READ conversationArchivedSummaries NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(QString conversationHistorySummaryText READ conversationHistorySummaryText NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(QStringList conversationHistorySummaryLines READ conversationHistorySummaryLines
                   NOTIFY chatMessagesChanged)
    Q_PROPERTY(int conversationHistoryMessageCount READ conversationHistoryMessageCount NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(QString conversationPersistenceStatus READ conversationPersistenceStatus NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(QString conversationLastSavedStatus READ conversationLastSavedStatus NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(QString conversationLastRestoredStatus READ conversationLastRestoredStatus NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(
        QString conversationBrowserStatus READ conversationBrowserStatus NOTIFY chatMessagesChanged)
    Q_PROPERTY(QString conversationBrowserSummaryText READ conversationBrowserSummaryText NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(
        int conversationListEntryCount READ conversationListEntryCount NOTIFY chatMessagesChanged)
    Q_PROPERTY(QString conversationListCurrentTitle READ conversationListCurrentTitle NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(int conversationListCurrentMessageCount READ conversationListCurrentMessageCount
                   NOTIFY chatMessagesChanged)
    Q_PROPERTY(QString conversationListCurrentPersistenceStatus READ
                   conversationListCurrentPersistenceStatus NOTIFY chatMessagesChanged)
    Q_PROPERTY(QString conversationListCurrentLastUpdatedSummary READ
                   conversationListCurrentLastUpdatedSummary NOTIFY chatMessagesChanged)
    Q_PROPERTY(
        QString conversationListCurrentSearchAvailabilitySummary READ
            conversationListCurrentSearchAvailabilitySummary NOTIFY conversationSearchChanged)
    Q_PROPERTY(
        QString conversationListCurrentExportAvailabilitySummary READ
            conversationListCurrentExportAvailabilitySummary NOTIFY conversationExportChanged)
    Q_PROPERTY(QString conversationListCurrentSummary READ conversationListCurrentSummary NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(QString conversationCurrentStorageMode READ conversationCurrentStorageMode CONSTANT)
    Q_PROPERTY(QString conversationFutureStorageMode READ conversationFutureStorageMode CONSTANT)
    Q_PROPERTY(QString conversationMigrationReadiness READ conversationMigrationReadiness CONSTANT)
    Q_PROPERTY(
        QString conversationMigrationStatusSummary READ conversationMigrationStatusSummary CONSTANT)
    Q_PROPERTY(
        QString conversationSchemaStatusSummary READ conversationSchemaStatusSummary CONSTANT)
    Q_PROPERTY(QString conversationSearchQueryText READ conversationSearchQueryText NOTIFY
                   conversationSearchChanged)
    Q_PROPERTY(QString conversationSearchStatus READ conversationSearchStatus NOTIFY
                   conversationSearchChanged)
    Q_PROPERTY(QString conversationSearchSummaryText READ conversationSearchSummaryText NOTIFY
                   conversationSearchChanged)
    Q_PROPERTY(int conversationSearchResultCount READ conversationSearchResultCount NOTIFY
                   conversationSearchChanged)
    Q_PROPERTY(QStringList conversationSearchResultSummaries READ conversationSearchResultSummaries
                   NOTIFY conversationSearchChanged)
    Q_PROPERTY(bool conversationExportAvailable READ conversationExportAvailable CONSTANT)
    Q_PROPERTY(
        QString conversationExportReadinessStatus READ conversationExportReadinessStatus CONSTANT)
    Q_PROPERTY(
        QString conversationExportReadinessSummary READ conversationExportReadinessSummary CONSTANT)
    Q_PROPERTY(QStringList conversationExportReadinessChecks READ conversationExportReadinessChecks
                   CONSTANT)
    Q_PROPERTY(QString conversationExportLastResultSummary READ conversationExportLastResultSummary
                   NOTIFY conversationExportChanged)
    Q_PROPERTY(QString conversationExportLastStatus READ conversationExportLastStatus NOTIFY
                   conversationExportChanged)
    Q_PROPERTY(QString conversationExportLastFileName READ conversationExportLastFileName NOTIFY
                   conversationExportChanged)
    Q_PROPERTY(int conversationExportLastMessageCount READ conversationExportLastMessageCount NOTIFY
                   conversationExportChanged)
    Q_PROPERTY(QString conversationExportLastTimestamp READ conversationExportLastTimestamp NOTIFY
                   conversationExportChanged)
    Q_PROPERTY(QStringList memoryEntries READ memoryEntries NOTIFY memoryEntriesChanged)
    Q_PROPERTY(QString memoryMaintenanceStatus READ memoryMaintenanceStatus NOTIFY
                   maintenanceStatusChanged)
    Q_PROPERTY(
        QString chatMaintenanceStatus READ chatMaintenanceStatus NOTIFY maintenanceStatusChanged)

public:
    ApplicationController(
        std::unique_ptr<IChatProvider> provider, std::unique_ptr<IMemoryStore> memoryStore,
        std::unique_ptr<ChatSession> chatSession = nullptr,
        std::unique_ptr<IChatHistoryStore> chatHistoryStore = nullptr,
        std::unique_ptr<IAgentRuntime> agentRuntime = nullptr,
        std::unique_ptr<IApprovalPolicy> approvalPolicy = nullptr,
        std::unique_ptr<ISandboxPolicy> sandboxPolicy = nullptr,
        std::unique_ptr<IToolExecutor> toolExecutor = nullptr,
        std::unique_ptr<IModelRouter> modelRouter = nullptr,
        std::unique_ptr<IProviderCatalog> providerCatalog = nullptr,
        std::unique_ptr<ITaskPlanner> taskPlanner = nullptr,
        std::unique_ptr<IAgentRegistry> agentRegistry = nullptr,
        std::unique_ptr<IMemoryCatalog> memoryCatalog = nullptr,
        std::unique_ptr<ILocalRuntime> localRuntime = nullptr,
        std::unique_ptr<ILocalRuntimeSessionManager> localRuntimeSessions = nullptr,
        std::unique_ptr<IRuntimeCapabilityRegistry> runtimeCapabilities = nullptr,
        std::unique_ptr<IRuntimePermissionPolicy> runtimePermissionPolicy = nullptr,
        std::unique_ptr<IRuntimeSafetyPolicy> runtimeSafetyPolicy = nullptr,
        std::unique_ptr<IRuntimePipeline> runtimePipeline = nullptr,
        std::unique_ptr<IExecutionLifecycle> executionLifecycle = nullptr,
        std::unique_ptr<ExecutionCoordinator> executionCoordinator = nullptr,
        std::unique_ptr<ILocalRuntimeAdapter> localRuntimeAdapter = nullptr,
        std::unique_ptr<IProviderRuntimeBridge> providerRuntimeBridge = nullptr,
        std::unique_ptr<StaticRuntimeIntegrationReadiness> runtimeIntegrationReadiness = nullptr,
        std::unique_ptr<IOllamaRuntimeClient> ollamaRuntimeClient = nullptr,
        std::unique_ptr<ILocalInferenceClient> localInferenceClient = nullptr,
        std::unique_ptr<ILocalInferenceStreamClient> localInferenceStreamClient = nullptr,
        std::unique_ptr<IModelManagementService> modelManagementService = nullptr,
        std::unique_ptr<ITextToSpeechProvider> textToSpeechProvider = nullptr,
        std::unique_ptr<ISpeechToTextProvider> speechToTextProvider = nullptr,
        std::unique_ptr<IVoiceRuntimeCoordinator> voiceRuntimeCoordinator = nullptr,
        std::unique_ptr<IVoiceRuntimeEnvironment> voiceRuntimeEnvironment = nullptr,
        std::unique_ptr<PiperTextToSpeechProvider> piperTextToSpeechProvider = nullptr,
        std::unique_ptr<ILocalInferenceWorker> localInferenceWorker = nullptr,
        std::unique_ptr<IConversationStore> conversationStore = nullptr, QObject* parent = nullptr);

    QString providerName() const;
    QString providerStatus() const;
    QString agentStatus() const;
    QString lastAgentResponse() const;
    QString latestToolPlanStatus() const;
    QString latestToolPlanSummary() const;
    QString latestApprovalStatus() const;
    QString latestApprovalSummary() const;
    QString latestSandboxStatus() const;
    QString latestSandboxSummary() const;
    QString latestToolExecutionStatus() const;
    QString latestToolExecutionSummary() const;
    QString latestAgentPipelineStatus() const;
    QString latestAgentPipelineSummary() const;
    QString runtimeSessionId() const;
    QString runtimeContextStatus() const;
    QString runtimeContextSummary() const;
    QStringList runtimeContextActiveToolIds() const;
    const ConversationSession& currentConversationSession() const;
    QString conversationSessionId() const;
    QString conversationSessionStatus() const;
    QString interactionMode() const;
    QString attentionState() const;
    QString contextWindowSummary() const;
    QString conversationState() const;
    QString conversationTransitionStatus() const;
    QString conversationTransitionSummary() const;
    QString conversationRuntimeSummary() const;
    QStringList conversationRuntimeSummaryLines() const;
    QString conversationRuntimeRequestId() const;
    QString conversationRuntimeActiveModel() const;
    QString conversationRuntimeActiveRoute() const;
    bool conversationRuntimeStreaming() const;
    QString conversationRuntimeLastSuccessSummary() const;
    QString conversationRuntimeLastErrorSummary() const;
    QString conversationRuntimeLastLatencySummary() const;
    int agentActivityCount() const;
    QString latestAgentActivitySummary() const;
    QString currentRoutingMode() const;
    void setRoutingModeByName(const QString& routingModeName);
    QString modelRoutingStatus() const;
    QString selectedModelProviderSummary() const;
    QString latestTaskPlanStatus() const;
    QString latestTaskPlanSummary() const;
    int plannedTaskStepCount() const;
    int registeredAgentCount() const;
    QStringList activeAgentSummaries() const;
    QString currentAgentSummary() const;
    QString currentMemoryAffinitySummary() const;
    int providerCatalogCount() const;
    QStringList providerCatalogSummaries() const;
    int memoryCatalogCount() const;
    QStringList memoryCatalogSummaries() const;
    OrchestrationSnapshot currentOrchestrationSnapshot() const;
    QString orchestrationSnapshotStatus() const;
    QString orchestrationSnapshotSummary() const;
    QStringList orchestrationSignals() const;
    OrchestrationReadinessReport currentOrchestrationReadinessReport() const;
    QString orchestrationReadinessStatus() const;
    QString orchestrationReadinessSummary() const;
    QStringList orchestrationDiagnostics() const;
    QString localRuntimeStatus() const;
    QString localRuntimeHealth() const;
    QString localRuntimeSummary() const;
    QStringList localRuntimeCapabilities() const;
    QString localRuntimeResponseStatus() const;
    QString localRuntimeResponseSummary() const;
    int localRuntimeSessionCount() const;
    QString localRuntimeSessionStatus() const;
    QString localRuntimeSessionHealth() const;
    QString localRuntimeSessionSummary() const;
    QString localRuntimeAllocationSummary() const;
    QString localRuntimeReservationSummary() const;
    QStringList localRuntimeSessionSummaries() const;
    int runtimeCapabilityCount() const;
    QStringList enabledRuntimeCapabilitySummaries() const;
    QStringList disabledRuntimeCapabilitySummaries() const;
    QString runtimeNegotiationProfileSummary() const;
    QString runtimeNegotiationSummary() const;
    QString localOnlyRuntimeEnforcementSummary() const;
    QString runtimePermissionDecision() const;
    QString runtimePermissionSummary() const;
    QString runtimeSafetyDecision() const;
    QString runtimeSafetySummary() const;
    QString runtimePipelineStatus() const;
    QString runtimePipelineSummary() const;
    QStringList runtimePipelineTraceSummaries() const;
    QString executionLifecycleState() const;
    QString executionLifecycleStatus() const;
    QString executionLifecycleSummary() const;
    QStringList executionLifecycleTraceSummaries() const;
    QString executionSessionId() const;
    QString executionSessionStatus() const;
    QString executionSessionOwnership() const;
    QString executionCoordinationMode() const;
    QString executionSessionSummary() const;
    QString executionCoordinationSnapshotSummary() const;
    QString localRuntimeAdapterStatus() const;
    QString localRuntimeAdapterHealth() const;
    QString localRuntimeAdapterSummary() const;
    QStringList localRuntimeAdapterCapabilitySummaries() const;
    QString providerRuntimeBridgeStatus() const;
    QString providerRuntimeBridgeSummary() const;
    QString providerRuntimeBridgeResponseSummary() const;
    QString runtimeIntegrationReadinessStatus() const;
    QString runtimeIntegrationReadinessSummary() const;
    QStringList runtimeIntegrationReadinessChecks() const;
    QString ollamaEndpoint() const;
    QString ollamaConnectionStatus() const;
    QString ollamaHealthStatus() const;
    QString ollamaHealthSummary() const;
    int ollamaModelCount() const;
    QStringList ollamaModelNames() const;
    QStringList ollamaModelSummaries() const;
    QString selectedLocalModel() const;
    void setSelectedLocalModel(const QString& model);
    QString selectedLocalModelStatus() const;
    QString selectedLocalModelSummary() const;
    QString selectedLocalModelMetadataSummary() const;
    QString activeLocalRuntimeBadge() const;
    QString modelManagementStatus() const;
    QString modelManagementSummary() const;
    QString modelManagementActionAvailability() const;
    QStringList modelRecommendationSummaries() const;
    QStringList modelRequirementSummaries() const;
    QString voiceRuntimeMode() const;
    bool voiceEnabled() const;
    VoiceReadinessReport currentVoiceReadinessReport() const;
    QString voiceReadinessStatus() const;
    QString voiceReadinessSummary() const;
    QStringList voiceReadinessChecks() const;
    QStringList voiceCapabilitySummaries() const;
    QString textToSpeechStatus() const;
    QString textToSpeechSummary() const;
    QString speechToTextStatus() const;
    QString speechToTextSummary() const;
    VoiceSession currentVoiceSession() const;
    VoicePipelineResult currentVoicePipelineResult() const;
    VoiceRuntimeSummary currentVoiceRuntimeSummary() const;
    QString voiceSessionId() const;
    QString voiceSessionStatus() const;
    QString voiceSessionSummary() const;
    QString voicePipelineStatus() const;
    QString voicePipelineSummary() const;
    QStringList voicePipelineTraceSummaries() const;
    QString voiceRuntimeStatus() const;
    QString voiceRuntimeSummary() const;
    QStringList voiceRuntimeCheckSummaries() const;
    bool voiceRuntimeAvailable() const;
    bool voiceTextToSpeechAvailable() const;
    bool voiceSpeechToTextAvailable() const;
    bool voiceMicrophoneEnabled() const;
    bool voicePlaybackEnabled() const;
    bool voiceLocalOnlyPolicy() const;
    bool voiceProcessExecutionEnabled() const;
    QString voiceRuntimeEnvironmentStatus() const;
    QString voiceRuntimeEnvironmentSummary() const;
    QStringList voiceBinarySummaries() const;
    QStringList voiceModelSummaries() const;
    QStringList voiceRuntimePermissionSummaries() const;
    QString voiceRuntimeSafetyStatus() const;
    QString voiceRuntimeSafetySummary() const;
    QStringList voiceRuntimeSafetyChecks() const;
    bool voiceRuntimeExecutionAllowed() const;
    QString piperTtsStatus() const;
    QString piperTtsSummary() const;
    QStringList piperTtsReadinessChecks() const;
    bool piperTtsReady() const;
    QString piperTtsFileOutputStatus() const;
    QString piperTtsFileOutputSummary() const;
    QString piperBinaryPath() const;
    void setPiperBinaryPath(const QString& path);
    QString piperModelPath() const;
    void setPiperModelPath(const QString& path);
    QString whisperBinaryPath() const;
    void setWhisperBinaryPath(const QString& path);
    QString whisperModelPath() const;
    void setWhisperModelPath(const QString& path);
    QStringList voiceConfigurationSummaries() const;
    QString voiceConfigurationReadinessSummary() const;
    QStringList voiceConfigurationStatusBadges() const;
    QStringList voiceConfigurationHintSummaries() const;
    QStringList voiceConfigurationValidationSummaries() const;
    QString piperFileOutputReadinessStatus() const;
    QString piperFileOutputReadinessSummary() const;
    bool piperFileOutputExecutionEnabled() const;
    void setPiperFileOutputExecutionEnabled(bool enabled);
    QString piperFileOutputExecutionStatus() const;
    QString piperFileOutputExecutionSummary() const;
    QString piperFileOutputAudioPathSummary() const;
    QString whisperPreparationReadinessStatus() const;
    QString whisperPreparationReadinessSummary() const;
    bool localChatInferenceEnabled() const;
    void setLocalChatInferenceEnabled(bool enabled);
    QString localChatInferenceStatus() const;
    QString localChatInferenceSummary() const;
    bool localInferenceStreamingEnabled() const;
    void setLocalInferenceStreamingEnabled(bool enabled);
    bool localInferenceBusy() const;
    QString localInferenceRuntimeState() const;
    QString localInferenceStatus() const;
    QString localInferenceSummary() const;
    QString localInferenceLastResponseSummary() const;
    QString localInferenceLatencySummary() const;
    QStringList localInferenceTraceSummaries() const;
    bool localInferenceStreamingAvailable() const;
    QString localInferenceStreamStatus() const;
    QString localInferenceStreamSummary() const;
    QString localInferenceStreamingText() const;
    int availableToolCount() const;
    QStringList availableToolIds() const;
    QString memoryStatus() const;
    QString chatHistoryStatus() const;
    QString conversationStoreStatus() const;
    int conversationStoreConversationCount() const;
    QString activeConversationSummary() const;
    QStringList conversationStoreSummaries() const;
    QString activeConversationId() const;
    bool activeConversationArchived() const;
    QStringList conversationIds() const;
    QStringList conversationTitles() const;
    QStringList conversationLastUpdatedSummaries() const;
    QStringList conversationMessageCountSummaries() const;
    QStringList conversationArchivedSummaries() const;
    ConversationHistorySummary conversationHistorySummary() const;
    QString conversationHistorySummaryText() const;
    QStringList conversationHistorySummaryLines() const;
    int conversationHistoryMessageCount() const;
    QString conversationPersistenceStatus() const;
    QString conversationLastSavedStatus() const;
    QString conversationLastRestoredStatus() const;
    ConversationId currentConversationId() const;
    ConversationDescriptor currentConversationDescriptor() const;
    ConversationDisplayTitle currentConversationDisplayTitle() const;
    ConversationListEntry currentConversationListEntry() const;
    ConversationListSummary conversationListSummary() const;
    QString conversationBrowserStatus() const;
    QString conversationBrowserSummaryText() const;
    int conversationListEntryCount() const;
    QString conversationListCurrentTitle() const;
    int conversationListCurrentMessageCount() const;
    QString conversationListCurrentPersistenceStatus() const;
    QString conversationListCurrentLastUpdatedSummary() const;
    QString conversationListCurrentSearchAvailabilitySummary() const;
    QString conversationListCurrentExportAvailabilitySummary() const;
    QString conversationListCurrentSummary() const;
    ConversationSchemaPlan conversationSchemaPlan() const;
    QString conversationCurrentStorageMode() const;
    QString conversationFutureStorageMode() const;
    QString conversationMigrationReadiness() const;
    QString conversationMigrationStatusSummary() const;
    QString conversationSchemaStatusSummary() const;
    ConversationSearchSummary conversationSearchSummary() const;
    QString conversationSearchQueryText() const;
    QString conversationSearchStatus() const;
    QString conversationSearchSummaryText() const;
    int conversationSearchResultCount() const;
    QStringList conversationSearchResultSummaries() const;
    ConversationExportReadiness conversationExportReadiness() const;
    ConversationExportResult latestConversationExportResult() const;
    bool conversationExportAvailable() const;
    QString conversationExportReadinessStatus() const;
    QString conversationExportReadinessSummary() const;
    QStringList conversationExportReadinessChecks() const;
    QString conversationExportLastResultSummary() const;
    QString conversationExportLastStatus() const;
    QString conversationExportLastFileName() const;
    int conversationExportLastMessageCount() const;
    QString conversationExportLastTimestamp() const;
    QString memoryMaintenanceStatus() const;
    QString chatMaintenanceStatus() const;
    const QList<ChatMessage>& chatHistory() const;
    QStringList chatMessages() const;
    QStringList memoryEntries() const;
    void setConversationExportDirectory(const QString& directoryPath);

    Q_INVOKABLE bool sendMessage(const QString& message);
    Q_INVOKABLE bool runLocalInference(const QString& prompt, const QString& model);
    Q_INVOKABLE bool cancelLocalInference();
    Q_INVOKABLE bool generatePiperTtsFile(const QString& text);
    Q_INVOKABLE bool searchConversation(const QString& query);
    Q_INVOKABLE void clearConversationSearch();
    Q_INVOKABLE bool exportTranscript(const QString& format);
    Q_INVOKABLE bool requestConversationExport(const QString& format);
    Q_INVOKABLE QString createConversation(const QString& title);
    Q_INVOKABLE bool switchConversation(const QString& conversationId);
    Q_INVOKABLE bool renameConversation(const QString& conversationId, const QString& title);
    Q_INVOKABLE bool archiveConversation(const QString& conversationId);
    Q_INVOKABLE bool unarchiveConversation(const QString& conversationId);
    Q_INVOKABLE bool runAgentRequest(const QString& request);
    Q_INVOKABLE bool clearMemory();
    Q_INVOKABLE bool clearChat();
    Q_INVOKABLE void remember(const QString& key, const QString& value);

signals:
    void chatMessagesChanged();
    void memoryEntriesChanged();
    void maintenanceStatusChanged();
    void agentStatusChanged();
    void agentResponseChanged();
    void toolPlanChanged();
    void approvalChanged();
    void sandboxChanged();
    void toolExecutionChanged();
    void agentPipelineChanged();
    void runtimeContextChanged();
    void conversationSessionChanged();
    void conversationStateChanged();
    void conversationRuntimeChanged();
    void conversationSearchChanged();
    void conversationExportChanged();
    void agentActivityChanged();
    void modelRoutingChanged();
    void taskPlanChanged();
    void orchestrationSnapshotChanged();
    void localModelSelectionChanged();
    void localChatInferenceRoutingChanged();
    void localInferenceChanged();
    void voiceConfigurationChanged();

private:
    AgentPipelineResult buildAgentPipelineResult(const AgentRequest& request) const;
    void appendPipelineActivity(const AgentPipelineResult& result);
    void resetCompletedConversationState();
    void transitionConversationState(ConversationState nextState, const QString& reason);
    void refreshLatestTaskPlan();
    void refreshConversationSession();
    void setMemoryMaintenanceStatus(const QString& status);
    void setChatMaintenanceStatus(const QString& status);
    RuntimePermissionRequest runtimePermissionRequest() const;
    RuntimePermissionDecision currentRuntimePermissionDecision() const;
    RuntimeSafetyReport currentRuntimeSafetyReport() const;
    RuntimePipelineResult currentRuntimePipelineResult() const;
    ExecutionRequest executionRequest() const;
    ExecutionLifecycleResult currentExecutionLifecycleResult() const;
    ExecutionCoordinationSnapshot currentExecutionCoordinationSnapshot() const;
    ProviderRuntimeBridgeRequest providerRuntimeBridgeRequest() const;
    ProviderRuntimeBridgeResponse currentProviderRuntimeBridgeResponse() const;
    RuntimeIntegrationReport currentRuntimeIntegrationReport() const;
    OllamaHealthCheckResult currentOllamaHealthCheck() const;
    QList<OllamaModelSummary> currentOllamaModels() const;
    QString effectiveLocalModel(const QString& requestedModel) const;
    bool discoveredModelNamesContain(const QString& model,
                                     const QList<OllamaModelSummary>& models) const;
    bool localInferenceEndpointAllowed() const;
    bool runLocalInferenceStream(const QString& prompt, const QString& model);
    void finishLocalInferenceRequest(const QString& requestId,
                                     const LocalInferenceResponse& response);
    void updateLocalInferenceStreamRequest(const QString& requestId,
                                           const LocalInferenceStreamChunk& chunk);
    void finishLocalInferenceStreamRequest(const QString& requestId,
                                           const LocalInferenceStreamResult& result);
    void finalizeLocalChatInference(bool succeeded);
    void initializeActiveConversation();
    bool ensureActiveConversation();
    void loadActiveConversationTranscript();
    bool persistActiveConversationMessage(const ChatMessage& message);
    ConversationMessageRecord
    conversationMessageRecordFromChatMessage(const ChatMessage& message) const;
    ChatMessage
    chatMessageFromConversationMessageRecord(const ConversationMessageRecord& message) const;
    QList<ConversationRecord> conversationRecords() const;
    ConversationRecord activeConversationRecord() const;
    void resetConversationRuntimeState();
    void refreshConversationHistorySummary();
    void resetConversationSearchSummary();
    void setConversationRuntimeRequest(const QString& requestId, const QString& model,
                                       const QString& route, bool streaming);
    void setConversationRuntimeResult(bool succeeded, const QString& summary,
                                      qint64 latencyMs = -1);
    LocalInferenceResponse blockedLocalInferenceResponse(const LocalInferenceRequest& request,
                                                         LocalInferenceError error,
                                                         const QString& summary) const;

    std::unique_ptr<IChatProvider> provider_;
    std::unique_ptr<IAgentRuntime> agentRuntime_;
    std::unique_ptr<IApprovalPolicy> approvalPolicy_;
    std::unique_ptr<ISandboxPolicy> sandboxPolicy_;
    std::unique_ptr<IToolExecutor> toolExecutor_;
    std::unique_ptr<IProviderCatalog> providerCatalog_;
    std::unique_ptr<IModelRouter> modelRouter_;
    std::unique_ptr<ITaskPlanner> taskPlanner_;
    std::unique_ptr<IAgentRegistry> agentRegistry_;
    std::unique_ptr<IMemoryCatalog> memoryCatalog_;
    std::unique_ptr<ILocalRuntime> localRuntime_;
    std::unique_ptr<ILocalRuntimeSessionManager> localRuntimeSessions_;
    std::unique_ptr<IRuntimeCapabilityRegistry> runtimeCapabilities_;
    std::unique_ptr<IRuntimePermissionPolicy> runtimePermissionPolicy_;
    std::unique_ptr<IRuntimeSafetyPolicy> runtimeSafetyPolicy_;
    std::unique_ptr<IRuntimePipeline> runtimePipeline_;
    std::unique_ptr<IExecutionLifecycle> executionLifecycle_;
    std::unique_ptr<ExecutionCoordinator> executionCoordinator_;
    std::unique_ptr<ILocalRuntimeAdapter> localRuntimeAdapter_;
    std::unique_ptr<IProviderRuntimeBridge> providerRuntimeBridge_;
    std::unique_ptr<StaticRuntimeIntegrationReadiness> runtimeIntegrationReadiness_;
    std::unique_ptr<IOllamaRuntimeClient> ollamaRuntimeClient_;
    std::unique_ptr<ILocalInferenceWorker> localInferenceWorker_;
    bool localInferenceClientIsRealOllama_ = false;
    bool localInferenceStreamClientIsRealOllama_ = false;
    std::unique_ptr<IModelManagementService> modelManagementService_;
    std::unique_ptr<ITextToSpeechProvider> textToSpeechProvider_;
    std::unique_ptr<ISpeechToTextProvider> speechToTextProvider_;
    std::unique_ptr<IVoiceRuntimeCoordinator> voiceRuntimeCoordinator_;
    std::unique_ptr<IVoiceRuntimeEnvironment> voiceRuntimeEnvironment_;
    std::unique_ptr<PiperTextToSpeechProvider> piperTextToSpeechProvider_;
    std::unique_ptr<IMemoryStore> memoryStore_;
    std::unique_ptr<ChatSession> chatSession_;
    std::unique_ptr<IChatHistoryStore> chatHistoryStore_;
    std::unique_ptr<IConversationStore> conversationStore_;
    QString memoryMaintenanceStatus_ = QStringLiteral("Ready");
    QString chatMaintenanceStatus_ = QStringLiteral("Ready");
    QString lastAgentResponse_ = QStringLiteral("No agent request yet.");
    AgentPipelineResult latestAgentPipelineResult_;
    TaskPlan latestTaskPlan_;
    RuntimeSession runtimeSession_;
    ConversationSessionStore conversationSession_;
    StaticConversationStateGraph conversationStateGraph_;
    AgentActivityLog agentActivityLog_;
    QString selectedLocalModel_;
    bool localChatInferenceEnabled_ = false;
    bool localInferenceStreamingEnabled_ = false;
    bool localInferenceBusy_ = false;
    bool activeLocalInferenceIsChatRequest_ = false;
    quint64 localInferenceRequestSequence_ = 0;
    QString activeLocalInferenceRequestId_;
    QString conversationRuntimeRequestId_ = QStringLiteral("None");
    QString conversationRuntimeActiveModel_ = QStringLiteral("None");
    QString conversationRuntimeActiveRoute_ = QStringLiteral("Provider");
    bool conversationRuntimeStreaming_ = false;
    QString conversationRuntimeLastSuccessSummary_ = QStringLiteral("No successful response yet.");
    QString conversationRuntimeLastErrorSummary_ = QStringLiteral("No error or refusal yet.");
    QString conversationRuntimeLastLatencySummary_ = QStringLiteral("No latency recorded.");
    ConversationHistorySummary conversationHistorySummary_;
    ConversationClearResult latestConversationClearResult_;
    ConversationSearchSummary latestConversationSearchSummary_;
    ConversationExportReadiness conversationExportReadiness_;
    ConversationExportResult latestConversationExportResult_;
    QString conversationExportDirectory_;
    QString activeConversationId_;
    LocalInferenceResponse latestLocalInferenceResponse_;
    LocalInferenceStreamResult latestLocalInferenceStreamResult_;
    bool piperFileOutputExecutionEnabled_ = false;
    PiperTtsResult latestPiperTtsResult_;
    QString piperBinaryPath_;
    QString piperModelPath_;
    QString whisperBinaryPath_;
    QString whisperModelPath_;
};

} // namespace sentinel::core
