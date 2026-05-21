#pragma once

#include "sentinel/core/SemanticRetrieval.h"
#include "sentinel/desktop/ChatMessageListModel.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace sentinel::core {
class AppSettings;
class ApplicationController;
class ModeManager;
} // namespace sentinel::core

namespace sentinel::desktop {

class DesktopShellViewModel final : public QObject {
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
    Q_PROPERTY(QStringList availableRoutingModes READ availableRoutingModes CONSTANT)
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
    Q_PROPERTY(QString agentTaskRuntimeStatus READ agentTaskRuntimeStatus CONSTANT)
    Q_PROPERTY(QString agentTaskRuntimeSummary READ agentTaskRuntimeSummary CONSTANT)
    Q_PROPERTY(int agentTaskRuntimeTaskCount READ agentTaskRuntimeTaskCount CONSTANT)
    Q_PROPERTY(int agentTaskQueueCount READ agentTaskQueueCount CONSTANT)
    Q_PROPERTY(int agentTaskQueueActiveCount READ agentTaskQueueActiveCount CONSTANT)
    Q_PROPERTY(int agentTaskQueuePlannedCount READ agentTaskQueuePlannedCount CONSTANT)
    Q_PROPERTY(int agentTaskQueueBlockedCount READ agentTaskQueueBlockedCount CONSTANT)
    Q_PROPERTY(int agentTaskQueueCompletedCount READ agentTaskQueueCompletedCount CONSTANT)
    Q_PROPERTY(int agentTaskQueueRefusedCount READ agentTaskQueueRefusedCount CONSTANT)
    Q_PROPERTY(QString latestAgentTaskSummary READ latestAgentTaskSummary CONSTANT)
    Q_PROPERTY(QString latestAgentTaskLifecycleSummary READ latestAgentTaskLifecycleSummary CONSTANT)
    Q_PROPERTY(QStringList agentTaskQueueSummaries READ agentTaskQueueSummaries CONSTANT)
    Q_PROPERTY(QStringList agentTaskTraceSummaries READ agentTaskTraceSummaries CONSTANT)
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
    Q_PROPERTY(bool promptContextInjectionEnabled READ promptContextInjectionEnabled WRITE
                   setPromptContextInjectionEnabled NOTIFY promptContextInjectionChanged)
    Q_PROPERTY(QString promptContextInjectionStatus READ promptContextInjectionStatus NOTIFY
                   promptContextInjectionChanged)
    Q_PROPERTY(QString promptContextInjectionSummary READ promptContextInjectionSummary NOTIFY
                   promptContextInjectionChanged)
    Q_PROPERTY(int promptContextInjectedBlockCount READ promptContextInjectedBlockCount NOTIFY
                   promptContextInjectionChanged)
    Q_PROPERTY(QString promptContextSourceSummary READ promptContextSourceSummary NOTIFY
                   promptContextInjectionChanged)
    Q_PROPERTY(QString promptContextSizeSummary READ promptContextSizeSummary NOTIFY
                   promptContextInjectionChanged)
    Q_PROPERTY(QStringList promptContextBlockSummaries READ promptContextBlockSummaries NOTIFY
                   promptContextInjectionChanged)
    Q_PROPERTY(
        QString conversationWindowStatus READ conversationWindowStatus NOTIFY chatMessagesChanged)
    Q_PROPERTY(
        QString conversationWindowSummary READ conversationWindowSummary NOTIFY chatMessagesChanged)
    Q_PROPERTY(QString conversationWindowBudgetSummary READ conversationWindowBudgetSummary NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(int conversationWindowBudgetCharacters READ conversationWindowBudgetCharacters NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(int conversationWindowIncludedMessageCount READ
                   conversationWindowIncludedMessageCount NOTIFY chatMessagesChanged)
    Q_PROPERTY(int conversationWindowTruncatedMessageCount READ
                   conversationWindowTruncatedMessageCount NOTIFY chatMessagesChanged)
    Q_PROPERTY(int conversationWindowOmittedMessageCount READ conversationWindowOmittedMessageCount
                   NOTIFY chatMessagesChanged)
    Q_PROPERTY(
        QString conversationSummaryStatus READ conversationSummaryStatus NOTIFY chatMessagesChanged)
    Q_PROPERTY(
        QString conversationSummaryText READ conversationSummaryText NOTIFY chatMessagesChanged)
    Q_PROPERTY(QString conversationSummaryBudgetSummary READ conversationSummaryBudgetSummary NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(int conversationSummaryBudgetCharacters READ conversationSummaryBudgetCharacters
                   NOTIFY chatMessagesChanged)
    Q_PROPERTY(int conversationSummaryBlockCount READ conversationSummaryBlockCount NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(int conversationSummaryMessageCount READ conversationSummaryMessageCount NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(int conversationSummaryOmittedMessageCount READ
                   conversationSummaryOmittedMessageCount NOTIFY chatMessagesChanged)
    Q_PROPERTY(int conversationSummaryTruncatedBlockCount READ
                   conversationSummaryTruncatedBlockCount NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList conversationSummaryBlockSummaries READ conversationSummaryBlockSummaries
                   NOTIFY chatMessagesChanged)
    Q_PROPERTY(
        QString retrievalPlanningStatus READ retrievalPlanningStatus NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString retrievalPlanningSummary READ retrievalPlanningSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString retrievalPlanningReadiness READ retrievalPlanningReadiness NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString retrievalPlanningBudgetSummary READ retrievalPlanningBudgetSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString retrievalPlanningSourceSummary READ retrievalPlanningSourceSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int retrievalPlanningSelectedSourceCount READ retrievalPlanningSelectedSourceCount
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(int retrievalPlanningExcludedSourceCount READ retrievalPlanningExcludedSourceCount
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(int retrievalPlanningSelectedCandidateCount READ
                   retrievalPlanningSelectedCandidateCount NOTIFY contextAssemblyChanged)
    Q_PROPERTY(int retrievalPlanningExcludedCandidateCount READ
                   retrievalPlanningExcludedCandidateCount NOTIFY contextAssemblyChanged)
    Q_PROPERTY(int retrievalPlanningTruncatedCandidateCount READ
                   retrievalPlanningTruncatedCandidateCount NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QStringList retrievalPlanningSourceSummaries READ retrievalPlanningSourceSummaries
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(bool semanticRetrievalEnabled READ semanticRetrievalEnabled CONSTANT)
    Q_PROPERTY(QString semanticRetrievalStatus READ semanticRetrievalStatus CONSTANT)
    Q_PROPERTY(QString semanticRetrievalSummary READ semanticRetrievalSummary CONSTANT)
    Q_PROPERTY(QString semanticReadiness READ semanticReadiness CONSTANT)
    Q_PROPERTY(QString embeddingProviderReadiness READ embeddingProviderReadiness CONSTANT)
    Q_PROPERTY(QString embeddingProviderSummary READ embeddingProviderSummary CONSTANT)
    Q_PROPERTY(QString vectorIndexReadiness READ vectorIndexReadiness CONSTANT)
    Q_PROPERTY(QString vectorIndexSummary READ vectorIndexSummary CONSTANT)
    Q_PROPERTY(int vectorIndexedItemCount READ vectorIndexedItemCount CONSTANT)
    Q_PROPERTY(QString semanticProviderMode READ semanticProviderMode CONSTANT)
    Q_PROPERTY(QString selectedSemanticProviderName READ selectedSemanticProviderName CONSTANT)
    Q_PROPERTY(QString semanticProviderReadiness READ semanticProviderReadiness CONSTANT)
    Q_PROPERTY(QString semanticProviderHealth READ semanticProviderHealth CONSTANT)
    Q_PROPERTY(QString semanticProviderStatusSummary READ semanticProviderStatusSummary CONSTANT)
    Q_PROPERTY(QString semanticActivationReadiness READ semanticActivationReadiness CONSTANT)
    Q_PROPERTY(QString semanticActivationSummary READ semanticActivationSummary CONSTANT)
    Q_PROPERTY(QStringList semanticProviderCapabilitySummaries READ
                   semanticProviderCapabilitySummaries CONSTANT)
    Q_PROPERTY(
        QStringList semanticActivationRequiredSteps READ semanticActivationRequiredSteps CONSTANT)
    Q_PROPERTY(
        QStringList semanticRetrievalReadinessChecks READ semanticRetrievalReadinessChecks CONSTANT)
    Q_PROPERTY(
        QString semanticCandidateStatus READ semanticCandidateStatus NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticCandidateSummary READ semanticCandidateSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticCandidateBudgetSummary READ semanticCandidateBudgetSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticCandidateArbitrationSummary READ semanticCandidateArbitrationSummary
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(int semanticCandidateCount READ semanticCandidateCount NOTIFY contextAssemblyChanged)
    Q_PROPERTY(int semanticCandidateSelectedCount READ semanticCandidateSelectedCount NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int semanticCandidateExcludedCount READ semanticCandidateExcludedCount NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int semanticCandidateTruncatedCount READ semanticCandidateTruncatedCount NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QStringList semanticCandidateParticipationSummaries READ
                   semanticCandidateParticipationSummaries NOTIFY contextAssemblyChanged)
    Q_PROPERTY(
        QString hybridRetrievalStatus READ hybridRetrievalStatus NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString hybridRetrievalReadiness READ hybridRetrievalReadiness NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(
        QString hybridRetrievalSummary READ hybridRetrievalSummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QStringList hybridRetrievalReadinessChecks READ hybridRetrievalReadinessChecks NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticArbitrationStatus READ semanticArbitrationStatus NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticArbitrationReadiness READ semanticArbitrationReadiness NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticArbitrationSummary READ semanticArbitrationSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticArbitrationBudgetSummary READ semanticArbitrationBudgetSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QStringList semanticArbitrationSelectionSummaries READ
                   semanticArbitrationSelectionSummaries NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QStringList semanticArbitrationChecks READ semanticArbitrationChecks NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString embeddingRuntimeReadiness READ embeddingRuntimeReadiness NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(
        QString embeddingRuntimeSummary READ embeddingRuntimeSummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString embeddingRuntimeBudgetSummary READ embeddingRuntimeBudgetSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QStringList embeddingRuntimeRequirementSummaries READ
                   embeddingRuntimeRequirementSummaries NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QStringList embeddingRuntimeConstraintSummaries READ
                   embeddingRuntimeConstraintSummaries NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString isolatedEmbeddingRuntimeStatus READ isolatedEmbeddingRuntimeStatus NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString isolatedEmbeddingRuntimeHealth READ isolatedEmbeddingRuntimeHealth NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString isolatedEmbeddingRuntimeReadiness READ isolatedEmbeddingRuntimeReadiness
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString isolatedEmbeddingRuntimeSummary READ isolatedEmbeddingRuntimeSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString isolatedEmbeddingRuntimeBoundedState READ
                   isolatedEmbeddingRuntimeBoundedState NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QStringList isolatedEmbeddingRuntimeChecks READ isolatedEmbeddingRuntimeChecks NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(
        QString vectorPersistenceStatus READ vectorPersistenceStatus NOTIFY contextAssemblyChanged)
    Q_PROPERTY(
        QString vectorPersistenceHealth READ vectorPersistenceHealth NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString vectorPersistenceReadiness READ vectorPersistenceReadiness NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString vectorPersistenceSummary READ vectorPersistenceSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString vectorPersistenceBoundedState READ vectorPersistenceBoundedState NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int vectorPersistenceIndexedItemCount READ vectorPersistenceIndexedItemCount NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QStringList vectorPersistenceChecks READ vectorPersistenceChecks NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticSearchStatus READ semanticSearchStatus NOTIFY contextAssemblyChanged)
    Q_PROPERTY(
        QString semanticSearchReadiness READ semanticSearchReadiness NOTIFY contextAssemblyChanged)
    Q_PROPERTY(
        QString semanticSearchSummary READ semanticSearchSummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticSearchBudgetSummary READ semanticSearchBudgetSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticSearchRuntimeState READ semanticSearchRuntimeState NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int semanticSearchCandidateCount READ semanticSearchCandidateCount NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticSearchArbitrationSummary READ semanticSearchArbitrationSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QStringList semanticSearchCandidateSummaries READ semanticSearchCandidateSummaries
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(
        QStringList semanticSearchChecks READ semanticSearchChecks NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString hybridBridgeStatus READ hybridBridgeStatus NOTIFY contextAssemblyChanged)
    Q_PROPERTY(
        QString hybridBridgeReadiness READ hybridBridgeReadiness NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString hybridBridgeSummary READ hybridBridgeSummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString hybridBridgeBudgetSummary READ hybridBridgeBudgetSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString hybridBridgeSourceSummary READ hybridBridgeSourceSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString hybridBridgeArbitrationSummary READ hybridBridgeArbitrationSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString hybridBridgeFallbackSummary READ hybridBridgeFallbackSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int hybridBridgeCandidateCount READ hybridBridgeCandidateCount NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int hybridBridgeSemanticFillCount READ hybridBridgeSemanticFillCount NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QStringList hybridBridgeCandidateSummaries READ hybridBridgeCandidateSummaries NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QStringList hybridBridgeChecks READ hybridBridgeChecks NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticAcceptanceStatus READ semanticAcceptanceStatus NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticAcceptanceReadiness READ semanticAcceptanceReadiness NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticAcceptanceSummary READ semanticAcceptanceSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticAcceptanceBudgetSummary READ semanticAcceptanceBudgetSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticAcceptanceSourceSummary READ semanticAcceptanceSourceSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticAcceptanceArbitrationSummary READ
                   semanticAcceptanceArbitrationSummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticAcceptanceFallbackSummary READ semanticAcceptanceFallbackSummary
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(int semanticAcceptanceAcceptedCount READ semanticAcceptanceAcceptedCount NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int semanticAcceptanceBudgetCharacters READ semanticAcceptanceBudgetCharacters NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QStringList semanticAcceptanceCandidateSummaries READ
                   semanticAcceptanceCandidateSummaries NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QStringList semanticAcceptanceChecks READ semanticAcceptanceChecks NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticSupplementAssemblyStatus READ semanticSupplementAssemblyStatus NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticSupplementAssemblyReadiness READ semanticSupplementAssemblyReadiness
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticSupplementAssemblySummary READ semanticSupplementAssemblySummary
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticSupplementAssemblyBudgetSummary READ
                   semanticSupplementAssemblyBudgetSummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticSupplementAssemblySafetySummary READ
                   semanticSupplementAssemblySafetySummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(int semanticSupplementAssemblyBlockCount READ semanticSupplementAssemblyBlockCount
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(int semanticSupplementAssemblyBudgetCharacters READ
                   semanticSupplementAssemblyBudgetCharacters NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QStringList semanticSupplementAssemblyChecks READ semanticSupplementAssemblyChecks
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticPromptAuthorityStatus READ semanticPromptAuthorityStatus NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticPromptAuthorityDecisionSummary READ
                   semanticPromptAuthorityDecisionSummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticPromptAuthoritySafetySummary READ
                   semanticPromptAuthoritySafetySummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticPromptAuthorityReadinessSummary READ
                   semanticPromptAuthorityReadinessSummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticPromptAuthorityFallbackSummary READ
                   semanticPromptAuthorityFallbackSummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticPromptAuthorityAuditSummary READ semanticPromptAuthorityAuditSummary
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(int semanticPromptAuthorityWouldIncludeBlockCount READ
                   semanticPromptAuthorityWouldIncludeBlockCount NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QStringList semanticPromptAuthorityChecks READ semanticPromptAuthorityChecks NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(bool semanticPromptInclusionEnabled READ semanticPromptInclusionEnabled WRITE
                   setSemanticPromptInclusionEnabled NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticPromptInclusionStatus READ semanticPromptInclusionStatus NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString semanticPromptInclusionSummary READ semanticPromptInclusionSummary NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int semanticPromptInclusionIncludedCount READ semanticPromptInclusionIncludedCount
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticPromptInclusionBudgetSummary READ
                   semanticPromptInclusionBudgetSummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticPromptInclusionFallbackSummary READ
                   semanticPromptInclusionFallbackSummary NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString semanticPromptInclusionAuditSummary READ semanticPromptInclusionAuditSummary
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(
        bool semanticPromptInclusionDeterministicAuthorityPreserved READ
            semanticPromptInclusionDeterministicAuthorityPreserved NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QStringList semanticPromptInclusionChecks READ semanticPromptInclusionChecks NOTIFY
                   contextAssemblyChanged)
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
    Q_PROPERTY(QString memoryStatus READ memoryStatus CONSTANT)
    Q_PROPERTY(QString chatHistoryStatus READ chatHistoryStatus CONSTANT)
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
    Q_PROPERTY(QString activeConversationStateSummary READ activeConversationStateSummary NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(QStringList conversationIds READ conversationIds NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList conversationTitles READ conversationTitles NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList conversationActiveSummaries READ conversationActiveSummaries NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(QStringList conversationLastUpdatedSummaries READ conversationLastUpdatedSummaries
                   NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList conversationMessageCountSummaries READ conversationMessageCountSummaries
                   NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList conversationArchivedSummaries READ conversationArchivedSummaries NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(int activeConversationCount READ activeConversationCount NOTIFY chatMessagesChanged)
    Q_PROPERTY(
        int archivedConversationCount READ archivedConversationCount NOTIFY chatMessagesChanged)
    Q_PROPERTY(int userCreatedConversationCount READ userCreatedConversationCount NOTIFY
                   chatMessagesChanged)
    Q_PROPERTY(bool conversationBrowserEmptyStateVisible READ conversationBrowserEmptyStateVisible
                   NOTIFY chatMessagesChanged)
    Q_PROPERTY(QString conversationBrowserEmptyStateSummary READ
                   conversationBrowserEmptyStateSummary NOTIFY chatMessagesChanged)
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
    Q_PROPERTY(bool conversationDeleteAvailable READ conversationDeleteAvailable NOTIFY
                   conversationDeleteChanged)
    Q_PROPERTY(QString conversationDeletePolicyStatus READ conversationDeletePolicyStatus NOTIFY
                   conversationDeleteChanged)
    Q_PROPERTY(QString conversationDeletePolicySummary READ conversationDeletePolicySummary NOTIFY
                   conversationDeleteChanged)
    Q_PROPERTY(QStringList conversationDeletePolicyRequirements READ
                   conversationDeletePolicyRequirements NOTIFY conversationDeleteChanged)
    Q_PROPERTY(QString conversationDeleteReadinessStatus READ conversationDeleteReadinessStatus
                   NOTIFY conversationDeleteChanged)
    Q_PROPERTY(QString conversationDeleteReadinessSummary READ conversationDeleteReadinessSummary
                   NOTIFY conversationDeleteChanged)
    Q_PROPERTY(QStringList conversationDeleteReadinessChecks READ conversationDeleteReadinessChecks
                   NOTIFY conversationDeleteChanged)
    Q_PROPERTY(QString conversationDeleteLastStatus READ conversationDeleteLastStatus NOTIFY
                   conversationDeleteChanged)
    Q_PROPERTY(QString conversationDeleteLastResultSummary READ conversationDeleteLastResultSummary
                   NOTIFY conversationDeleteChanged)
    Q_PROPERTY(int memoryCandidateCount READ memoryCandidateCount NOTIFY memoryCandidatesChanged)
    Q_PROPERTY(int pendingMemoryCandidateCount READ pendingMemoryCandidateCount NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(int approvedMemoryCandidateCount READ approvedMemoryCandidateCount NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(int rejectedMemoryCandidateCount READ rejectedMemoryCandidateCount NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(int archivedMemoryCandidateCount READ archivedMemoryCandidateCount NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(int committedMemoryCandidateCount READ committedMemoryCandidateCount NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(
        QStringList memoryCandidateIds READ memoryCandidateIds NOTIFY memoryCandidatesChanged)
    Q_PROPERTY(QStringList memoryCandidateReviewStates READ memoryCandidateReviewStates NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(QStringList memoryCandidateCommitStatuses READ memoryCandidateCommitStatuses NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(QStringList memoryCandidateSummaries READ memoryCandidateSummaries NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(QStringList pendingMemoryCandidateSummaries READ pendingMemoryCandidateSummaries
                   NOTIFY memoryCandidatesChanged)
    Q_PROPERTY(QStringList approvedMemoryCandidateSummaries READ approvedMemoryCandidateSummaries
                   NOTIFY memoryCandidatesChanged)
    Q_PROPERTY(QStringList rejectedMemoryCandidateSummaries READ rejectedMemoryCandidateSummaries
                   NOTIFY memoryCandidatesChanged)
    Q_PROPERTY(QStringList archivedMemoryCandidateSummaries READ archivedMemoryCandidateSummaries
                   NOTIFY memoryCandidatesChanged)
    Q_PROPERTY(QString lastMemoryCandidateReviewStatus READ lastMemoryCandidateReviewStatus NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(QString lastMemoryCandidateReviewSummary READ lastMemoryCandidateReviewSummary NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(QString memoryCommitReadinessStatus READ memoryCommitReadinessStatus NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(QString memoryCommitReadinessSummary READ memoryCommitReadinessSummary NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(QStringList memoryCommitReadinessChecks READ memoryCommitReadinessChecks NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(int memoryCommitPlanCount READ memoryCommitPlanCount NOTIFY memoryCandidatesChanged)
    Q_PROPERTY(QString memoryCommitTargetSummary READ memoryCommitTargetSummary NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(QStringList memoryCommitCandidateSummaries READ memoryCommitCandidateSummaries NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(
        QString lastMemoryCommitStatus READ lastMemoryCommitStatus NOTIFY memoryCandidatesChanged)
    Q_PROPERTY(QString lastMemoryCommitResultSummary READ lastMemoryCommitResultSummary NOTIFY
                   memoryCandidatesChanged)
    Q_PROPERTY(QString memoryRecallPolicyStatus READ memoryRecallPolicyStatus CONSTANT)
    Q_PROPERTY(QString memoryRecallPolicySummary READ memoryRecallPolicySummary CONSTANT)
    Q_PROPERTY(QString memoryRecallQueryText READ memoryRecallQueryText NOTIFY memoryRecallChanged)
    Q_PROPERTY(QString memoryRecallStatus READ memoryRecallStatus NOTIFY memoryRecallChanged)
    Q_PROPERTY(
        QString memoryRecallSummaryText READ memoryRecallSummaryText NOTIFY memoryRecallChanged)
    Q_PROPERTY(int memoryRecallResultCount READ memoryRecallResultCount NOTIFY memoryRecallChanged)
    Q_PROPERTY(QStringList memoryRecallResultSummaries READ memoryRecallResultSummaries NOTIFY
                   memoryRecallChanged)
    Q_PROPERTY(QString contextAssemblyPolicyStatus READ contextAssemblyPolicyStatus CONSTANT)
    Q_PROPERTY(QString contextAssemblyPolicySummary READ contextAssemblyPolicySummary CONSTANT)
    Q_PROPERTY(
        QString contextAssemblyStatus READ contextAssemblyStatus NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString contextAssemblySummaryText READ contextAssemblySummaryText NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int contextAssemblySourceCount READ contextAssemblySourceCount NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int contextAssemblyAvailableSourceCount READ contextAssemblyAvailableSourceCount
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(int contextAssemblyCandidateBlockCount READ contextAssemblyCandidateBlockCount NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int contextAssemblyEstimatedSize READ contextAssemblyEstimatedSize NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString conversationContextAvailability READ conversationContextAvailability NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QString committedMemoryContextAvailability READ committedMemoryContextAvailability
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString runtimeMetadataContextAvailability READ runtimeMetadataContextAvailability
                   NOTIFY contextAssemblyChanged)
    Q_PROPERTY(QString orchestrationContextAvailability READ orchestrationContextAvailability NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QStringList contextAssemblySourceSummaries READ contextAssemblySourceSummaries NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(QStringList contextAssemblyReadinessChecks READ contextAssemblyReadinessChecks NOTIFY
                   contextAssemblyChanged)
    Q_PROPERTY(int memoryEntryCount READ memoryEntryCount NOTIFY memoryEntriesChanged)
    Q_PROPERTY(QString memoryMaintenanceStatus READ memoryMaintenanceStatus NOTIFY
                   maintenanceStatusChanged)
    Q_PROPERTY(
        QString chatMaintenanceStatus READ chatMaintenanceStatus NOTIFY maintenanceStatusChanged)
    Q_PROPERTY(QString currentModeName READ currentModeName NOTIFY currentModeChanged)
    Q_PROPERTY(QStringList availableModes READ availableModes CONSTANT)
    Q_PROPERTY(QString currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(QStringList availablePages READ availablePages CONSTANT)
    Q_PROPERTY(ChatMessageListModel* chatMessages READ chatMessages CONSTANT)
    Q_PROPERTY(QStringList memoryEntries READ memoryEntries NOTIFY memoryEntriesChanged)
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY themeNameChanged)
    Q_PROPERTY(QString configurationProfile READ configurationProfile WRITE setConfigurationProfile
                   NOTIFY configurationProfileChanged)

public:
    DesktopShellViewModel(core::ApplicationController& controller, core::ModeManager& modeManager,
                          core::AppSettings& settings, QObject* parent = nullptr);

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
    QStringList availableRoutingModes() const;
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
    QString orchestrationSnapshotStatus() const;
    QString orchestrationSnapshotSummary() const;
    QStringList orchestrationSignals() const;
    QString orchestrationReadinessStatus() const;
    QString orchestrationReadinessSummary() const;
    QStringList orchestrationDiagnostics() const;
    QString agentTaskRuntimeStatus() const;
    QString agentTaskRuntimeSummary() const;
    int agentTaskRuntimeTaskCount() const;
    int agentTaskQueueCount() const;
    int agentTaskQueueActiveCount() const;
    int agentTaskQueuePlannedCount() const;
    int agentTaskQueueBlockedCount() const;
    int agentTaskQueueCompletedCount() const;
    int agentTaskQueueRefusedCount() const;
    QString latestAgentTaskSummary() const;
    QString latestAgentTaskLifecycleSummary() const;
    QStringList agentTaskQueueSummaries() const;
    QStringList agentTaskTraceSummaries() const;
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
    QString voiceReadinessStatus() const;
    QString voiceReadinessSummary() const;
    QStringList voiceReadinessChecks() const;
    QStringList voiceCapabilitySummaries() const;
    QString textToSpeechStatus() const;
    QString textToSpeechSummary() const;
    QString speechToTextStatus() const;
    QString speechToTextSummary() const;
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
    bool promptContextInjectionEnabled() const;
    void setPromptContextInjectionEnabled(bool enabled);
    QString promptContextInjectionStatus() const;
    QString promptContextInjectionSummary() const;
    int promptContextInjectedBlockCount() const;
    QString promptContextSourceSummary() const;
    QString promptContextSizeSummary() const;
    QStringList promptContextBlockSummaries() const;
    QString conversationWindowStatus() const;
    QString conversationWindowSummary() const;
    QString conversationWindowBudgetSummary() const;
    int conversationWindowBudgetCharacters() const;
    int conversationWindowIncludedMessageCount() const;
    int conversationWindowTruncatedMessageCount() const;
    int conversationWindowOmittedMessageCount() const;
    QString conversationSummaryStatus() const;
    QString conversationSummaryText() const;
    QString conversationSummaryBudgetSummary() const;
    int conversationSummaryBudgetCharacters() const;
    int conversationSummaryBlockCount() const;
    int conversationSummaryMessageCount() const;
    int conversationSummaryOmittedMessageCount() const;
    int conversationSummaryTruncatedBlockCount() const;
    QStringList conversationSummaryBlockSummaries() const;
    QString retrievalPlanningStatus() const;
    QString retrievalPlanningSummary() const;
    QString retrievalPlanningReadiness() const;
    QString retrievalPlanningBudgetSummary() const;
    QString retrievalPlanningSourceSummary() const;
    int retrievalPlanningSelectedSourceCount() const;
    int retrievalPlanningExcludedSourceCount() const;
    int retrievalPlanningSelectedCandidateCount() const;
    int retrievalPlanningExcludedCandidateCount() const;
    int retrievalPlanningTruncatedCandidateCount() const;
    QStringList retrievalPlanningSourceSummaries() const;
    core::SemanticRetrievalPolicy semanticRetrievalPolicy() const;
    bool semanticRetrievalEnabled() const;
    QString semanticRetrievalStatus() const;
    QString semanticRetrievalSummary() const;
    QString semanticReadiness() const;
    QString embeddingProviderReadiness() const;
    QString embeddingProviderSummary() const;
    QString vectorIndexReadiness() const;
    QString vectorIndexSummary() const;
    int vectorIndexedItemCount() const;
    QString semanticProviderMode() const;
    QString selectedSemanticProviderName() const;
    QString semanticProviderReadiness() const;
    QString semanticProviderHealth() const;
    QString semanticProviderStatusSummary() const;
    QString semanticActivationReadiness() const;
    QString semanticActivationSummary() const;
    QStringList semanticProviderCapabilitySummaries() const;
    QStringList semanticActivationRequiredSteps() const;
    QStringList semanticRetrievalReadinessChecks() const;
    QString semanticCandidateStatus() const;
    QString semanticCandidateSummary() const;
    QString semanticCandidateBudgetSummary() const;
    QString semanticCandidateArbitrationSummary() const;
    int semanticCandidateCount() const;
    int semanticCandidateSelectedCount() const;
    int semanticCandidateExcludedCount() const;
    int semanticCandidateTruncatedCount() const;
    QStringList semanticCandidateParticipationSummaries() const;
    QString hybridRetrievalStatus() const;
    QString hybridRetrievalReadiness() const;
    QString hybridRetrievalSummary() const;
    QStringList hybridRetrievalReadinessChecks() const;
    QString semanticArbitrationStatus() const;
    QString semanticArbitrationReadiness() const;
    QString semanticArbitrationSummary() const;
    QString semanticArbitrationBudgetSummary() const;
    QStringList semanticArbitrationSelectionSummaries() const;
    QStringList semanticArbitrationChecks() const;
    QString embeddingRuntimeReadiness() const;
    QString embeddingRuntimeSummary() const;
    QString embeddingRuntimeBudgetSummary() const;
    QStringList embeddingRuntimeRequirementSummaries() const;
    QStringList embeddingRuntimeConstraintSummaries() const;
    QString isolatedEmbeddingRuntimeStatus() const;
    QString isolatedEmbeddingRuntimeHealth() const;
    QString isolatedEmbeddingRuntimeReadiness() const;
    QString isolatedEmbeddingRuntimeSummary() const;
    QString isolatedEmbeddingRuntimeBoundedState() const;
    QStringList isolatedEmbeddingRuntimeChecks() const;
    QString vectorPersistenceStatus() const;
    QString vectorPersistenceHealth() const;
    QString vectorPersistenceReadiness() const;
    QString vectorPersistenceSummary() const;
    QString vectorPersistenceBoundedState() const;
    int vectorPersistenceIndexedItemCount() const;
    QStringList vectorPersistenceChecks() const;
    QString semanticSearchStatus() const;
    QString semanticSearchReadiness() const;
    QString semanticSearchSummary() const;
    QString semanticSearchBudgetSummary() const;
    QString semanticSearchRuntimeState() const;
    int semanticSearchCandidateCount() const;
    QString semanticSearchArbitrationSummary() const;
    QStringList semanticSearchCandidateSummaries() const;
    QStringList semanticSearchChecks() const;
    QString hybridBridgeStatus() const;
    QString hybridBridgeReadiness() const;
    QString hybridBridgeSummary() const;
    QString hybridBridgeBudgetSummary() const;
    QString hybridBridgeSourceSummary() const;
    QString hybridBridgeArbitrationSummary() const;
    QString hybridBridgeFallbackSummary() const;
    int hybridBridgeCandidateCount() const;
    int hybridBridgeSemanticFillCount() const;
    QStringList hybridBridgeCandidateSummaries() const;
    QStringList hybridBridgeChecks() const;
    QString semanticAcceptanceStatus() const;
    QString semanticAcceptanceReadiness() const;
    QString semanticAcceptanceSummary() const;
    QString semanticAcceptanceBudgetSummary() const;
    QString semanticAcceptanceSourceSummary() const;
    QString semanticAcceptanceArbitrationSummary() const;
    QString semanticAcceptanceFallbackSummary() const;
    int semanticAcceptanceAcceptedCount() const;
    int semanticAcceptanceBudgetCharacters() const;
    QStringList semanticAcceptanceCandidateSummaries() const;
    QStringList semanticAcceptanceChecks() const;
    QString semanticSupplementAssemblyStatus() const;
    QString semanticSupplementAssemblyReadiness() const;
    QString semanticSupplementAssemblySummary() const;
    QString semanticSupplementAssemblyBudgetSummary() const;
    QString semanticSupplementAssemblySafetySummary() const;
    int semanticSupplementAssemblyBlockCount() const;
    int semanticSupplementAssemblyBudgetCharacters() const;
    QStringList semanticSupplementAssemblyChecks() const;
    QString semanticPromptAuthorityStatus() const;
    QString semanticPromptAuthorityDecisionSummary() const;
    QString semanticPromptAuthoritySafetySummary() const;
    QString semanticPromptAuthorityReadinessSummary() const;
    QString semanticPromptAuthorityFallbackSummary() const;
    QString semanticPromptAuthorityAuditSummary() const;
    int semanticPromptAuthorityWouldIncludeBlockCount() const;
    QStringList semanticPromptAuthorityChecks() const;
    bool semanticPromptInclusionEnabled() const;
    void setSemanticPromptInclusionEnabled(bool enabled);
    QString semanticPromptInclusionStatus() const;
    QString semanticPromptInclusionSummary() const;
    int semanticPromptInclusionIncludedCount() const;
    QString semanticPromptInclusionBudgetSummary() const;
    QString semanticPromptInclusionFallbackSummary() const;
    QString semanticPromptInclusionAuditSummary() const;
    bool semanticPromptInclusionDeterministicAuthorityPreserved() const;
    QStringList semanticPromptInclusionChecks() const;
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
    QString activeConversationStateSummary() const;
    QStringList conversationIds() const;
    QStringList conversationTitles() const;
    QStringList conversationActiveSummaries() const;
    QStringList conversationLastUpdatedSummaries() const;
    QStringList conversationMessageCountSummaries() const;
    QStringList conversationArchivedSummaries() const;
    int activeConversationCount() const;
    int archivedConversationCount() const;
    int userCreatedConversationCount() const;
    bool conversationBrowserEmptyStateVisible() const;
    QString conversationBrowserEmptyStateSummary() const;
    QString conversationHistorySummaryText() const;
    QStringList conversationHistorySummaryLines() const;
    int conversationHistoryMessageCount() const;
    QString conversationPersistenceStatus() const;
    QString conversationLastSavedStatus() const;
    QString conversationLastRestoredStatus() const;
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
    QString conversationCurrentStorageMode() const;
    QString conversationFutureStorageMode() const;
    QString conversationMigrationReadiness() const;
    QString conversationMigrationStatusSummary() const;
    QString conversationSchemaStatusSummary() const;
    QString conversationSearchQueryText() const;
    QString conversationSearchStatus() const;
    QString conversationSearchSummaryText() const;
    int conversationSearchResultCount() const;
    QStringList conversationSearchResultSummaries() const;
    bool conversationExportAvailable() const;
    QString conversationExportReadinessStatus() const;
    QString conversationExportReadinessSummary() const;
    QStringList conversationExportReadinessChecks() const;
    QString conversationExportLastResultSummary() const;
    QString conversationExportLastStatus() const;
    QString conversationExportLastFileName() const;
    int conversationExportLastMessageCount() const;
    QString conversationExportLastTimestamp() const;
    bool conversationDeleteAvailable() const;
    QString conversationDeletePolicyStatus() const;
    QString conversationDeletePolicySummary() const;
    QStringList conversationDeletePolicyRequirements() const;
    QString conversationDeleteReadinessStatus() const;
    QString conversationDeleteReadinessSummary() const;
    QStringList conversationDeleteReadinessChecks() const;
    QString conversationDeleteLastStatus() const;
    QString conversationDeleteLastResultSummary() const;
    int memoryCandidateCount() const;
    int pendingMemoryCandidateCount() const;
    int approvedMemoryCandidateCount() const;
    int rejectedMemoryCandidateCount() const;
    int archivedMemoryCandidateCount() const;
    int committedMemoryCandidateCount() const;
    QStringList memoryCandidateIds() const;
    QStringList memoryCandidateReviewStates() const;
    QStringList memoryCandidateCommitStatuses() const;
    QStringList memoryCandidateSummaries() const;
    QStringList pendingMemoryCandidateSummaries() const;
    QStringList approvedMemoryCandidateSummaries() const;
    QStringList rejectedMemoryCandidateSummaries() const;
    QStringList archivedMemoryCandidateSummaries() const;
    QString lastMemoryCandidateReviewStatus() const;
    QString lastMemoryCandidateReviewSummary() const;
    QString memoryCommitReadinessStatus() const;
    QString memoryCommitReadinessSummary() const;
    QStringList memoryCommitReadinessChecks() const;
    int memoryCommitPlanCount() const;
    QString memoryCommitTargetSummary() const;
    QStringList memoryCommitCandidateSummaries() const;
    QString lastMemoryCommitStatus() const;
    QString lastMemoryCommitResultSummary() const;
    QString memoryRecallPolicyStatus() const;
    QString memoryRecallPolicySummary() const;
    QString memoryRecallQueryText() const;
    QString memoryRecallStatus() const;
    QString memoryRecallSummaryText() const;
    int memoryRecallResultCount() const;
    QStringList memoryRecallResultSummaries() const;
    QString contextAssemblyPolicyStatus() const;
    QString contextAssemblyPolicySummary() const;
    QString contextAssemblyStatus() const;
    QString contextAssemblySummaryText() const;
    int contextAssemblySourceCount() const;
    int contextAssemblyAvailableSourceCount() const;
    int contextAssemblyCandidateBlockCount() const;
    int contextAssemblyEstimatedSize() const;
    QString conversationContextAvailability() const;
    QString committedMemoryContextAvailability() const;
    QString runtimeMetadataContextAvailability() const;
    QString orchestrationContextAvailability() const;
    QStringList contextAssemblySourceSummaries() const;
    QStringList contextAssemblyReadinessChecks() const;
    int memoryEntryCount() const;
    QString memoryMaintenanceStatus() const;
    QString chatMaintenanceStatus() const;
    QString currentModeName() const;
    QStringList availableModes() const;
    QString currentPage() const;
    void setCurrentPage(const QString& page);
    QStringList availablePages() const;
    ChatMessageListModel* chatMessages();
    QStringList memoryEntries() const;
    QString themeName() const;
    void setThemeName(const QString& themeName);
    QString configurationProfile() const;
    void setConfigurationProfile(const QString& configurationProfile);

    Q_INVOKABLE bool sendMessage(const QString& message);
    Q_INVOKABLE bool runLocalInference(const QString& prompt, const QString& model);
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
    Q_INVOKABLE bool requestPermanentDeleteConversation(const QString& conversationId);
    Q_INVOKABLE QString createMemoryCandidateFromConversationText(const QString& text);
    Q_INVOKABLE bool approveMemoryCandidate(const QString& candidateId);
    Q_INVOKABLE bool rejectMemoryCandidate(const QString& candidateId);
    Q_INVOKABLE bool resetMemoryCandidate(const QString& candidateId);
    Q_INVOKABLE bool archiveMemoryCandidate(const QString& candidateId);
    Q_INVOKABLE bool requestMemoryCandidateCommit(const QString& candidateId);
    Q_INVOKABLE bool recallLocalMemory(const QString& query);
    Q_INVOKABLE void clearLocalMemoryRecall();
    Q_INVOKABLE bool runAgentRequest(const QString& request);
    Q_INVOKABLE bool clearMemory();
    Q_INVOKABLE bool clearChat();
    Q_INVOKABLE void setModeByName(const QString& modeName);
    Q_INVOKABLE void remember(const QString& key, const QString& value);
    Q_INVOKABLE void applyVoiceConfigurationPaths(const QString& piperBinaryPath,
                                                  const QString& piperModelPath,
                                                  const QString& whisperBinaryPath,
                                                  const QString& whisperModelPath);

signals:
    void currentModeChanged();
    void chatMessagesChanged();
    void memoryEntriesChanged();
    void themeNameChanged();
    void configurationProfileChanged();
    void currentPageChanged();
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
    void conversationDeleteChanged();
    void memoryCandidatesChanged();
    void memoryRecallChanged();
    void contextAssemblyChanged();
    void agentActivityChanged();
    void modelRoutingChanged();
    void taskPlanChanged();
    void orchestrationSnapshotChanged();
    void localModelSelectionChanged();
    void localChatInferenceRoutingChanged();
    void localInferenceChanged();
    void voiceConfigurationChanged();
    void promptContextInjectionChanged();

private:
    static QString normalizedPageOrDefault(const QString& page);

    core::ApplicationController& controller_;
    core::ModeManager& modeManager_;
    core::AppSettings& settings_;
    ChatMessageListModel chatMessages_;
    QString currentPage_ = QStringLiteral("Dashboard");
};

} // namespace sentinel::desktop
