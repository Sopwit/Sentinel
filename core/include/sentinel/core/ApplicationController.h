#pragma once

#include "sentinel/core/AgentActivityLog.h"
#include "sentinel/core/AgentPipelineResult.h"
#include "sentinel/core/AgentRuntimeContext.h"
#include "sentinel/core/ChatSession.h"
#include "sentinel/core/ConversationSession.h"
#include "sentinel/core/ConversationStateGraph.h"
#include "sentinel/core/ExecutionLifecycle.h"
#include "sentinel/core/IAgentRegistry.h"
#include "sentinel/core/IAgentRuntime.h"
#include "sentinel/core/IApprovalPolicy.h"
#include "sentinel/core/IChatHistoryStore.h"
#include "sentinel/core/IChatProvider.h"
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
#include "sentinel/core/OllamaRuntime.h"
#include "sentinel/core/OrchestrationDiagnostics.h"
#include "sentinel/core/OrchestrationSnapshot.h"
#include "sentinel/core/RuntimeCapabilities.h"
#include "sentinel/core/RuntimeIntegration.h"
#include "sentinel/core/RuntimePermissions.h"
#include "sentinel/core/RuntimePipeline.h"
#include "sentinel/core/RuntimeSafety.h"

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
    Q_PROPERTY(QStringList ollamaModelSummaries READ ollamaModelSummaries CONSTANT)
    Q_PROPERTY(QString selectedLocalModel READ selectedLocalModel WRITE setSelectedLocalModel NOTIFY
                   localModelSelectionChanged)
    Q_PROPERTY(QString selectedLocalModelSummary READ selectedLocalModelSummary NOTIFY
                   localModelSelectionChanged)
    Q_PROPERTY(QString activeLocalRuntimeBadge READ activeLocalRuntimeBadge NOTIFY
                   localModelSelectionChanged)
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
    Q_PROPERTY(bool localInferenceStreamingAvailable READ localInferenceStreamingAvailable CONSTANT)
    Q_PROPERTY(QString localInferenceStreamStatus READ localInferenceStreamStatus CONSTANT)
    Q_PROPERTY(QString localInferenceStreamSummary READ localInferenceStreamSummary CONSTANT)
    Q_PROPERTY(int availableToolCount READ availableToolCount CONSTANT)
    Q_PROPERTY(QStringList availableToolIds READ availableToolIds CONSTANT)
    Q_PROPERTY(QStringList chatMessages READ chatMessages NOTIFY chatMessagesChanged)
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
        QObject* parent = nullptr);

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
    QStringList ollamaModelSummaries() const;
    QString selectedLocalModel() const;
    void setSelectedLocalModel(const QString& model);
    QString selectedLocalModelSummary() const;
    QString activeLocalRuntimeBadge() const;
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
    int availableToolCount() const;
    QStringList availableToolIds() const;
    QString memoryStatus() const;
    QString chatHistoryStatus() const;
    QString memoryMaintenanceStatus() const;
    QString chatMaintenanceStatus() const;
    const QList<ChatMessage>& chatHistory() const;
    QStringList chatMessages() const;
    QStringList memoryEntries() const;

    Q_INVOKABLE bool sendMessage(const QString& message);
    Q_INVOKABLE bool runLocalInference(const QString& prompt, const QString& model);
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
    void agentActivityChanged();
    void modelRoutingChanged();
    void taskPlanChanged();
    void orchestrationSnapshotChanged();
    void localModelSelectionChanged();
    void localInferenceChanged();

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
    std::unique_ptr<ILocalInferenceClient> localInferenceClient_;
    std::unique_ptr<IMemoryStore> memoryStore_;
    std::unique_ptr<ChatSession> chatSession_;
    std::unique_ptr<IChatHistoryStore> chatHistoryStore_;
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
    bool localInferenceBusy_ = false;
    LocalInferenceResponse latestLocalInferenceResponse_;
    LocalInferenceStreamResult latestLocalInferenceStreamResult_;
};

} // namespace sentinel::core
