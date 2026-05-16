#include "sentinel/core/ApplicationController.h"

#include "sentinel/core/LocalRuntime.h"
#include "sentinel/core/NullToolExecutor.h"
#include "sentinel/core/StaticAgentRegistry.h"
#include "sentinel/core/StaticApprovalPolicy.h"
#include "sentinel/core/StaticMemoryCatalog.h"
#include "sentinel/core/StaticModelRouter.h"
#include "sentinel/core/StaticProviderCatalog.h"
#include "sentinel/core/StaticSandboxPolicy.h"
#include "sentinel/core/StaticTaskPlanner.h"

#include <QElapsedTimer>

namespace sentinel::core {

namespace {

AgentActivityStatus planActivityStatus(ToolInvocationPlanStatus status) {
    return status == ToolInvocationPlanStatus::Planned ? AgentActivityStatus::Completed
                                                       : AgentActivityStatus::Blocked;
}

AgentActivityStatus approvalActivityStatus(ApprovalStatus status) {
    return status == ApprovalStatus::Denied || status == ApprovalStatus::RequiresApproval
               ? AgentActivityStatus::Blocked
               : AgentActivityStatus::Completed;
}

AgentActivityStatus sandboxActivityStatus(SandboxStatus status) {
    return status == SandboxStatus::Allowed ? AgentActivityStatus::Completed
                                            : AgentActivityStatus::Blocked;
}

AgentActivityStatus executionActivityStatus(ToolExecutionStatus status) {
    return status == ToolExecutionStatus::PlaceholderSucceeded ? AgentActivityStatus::Completed
                                                               : AgentActivityStatus::Blocked;
}

OrchestrationHealthStatus healthStatusFor(const QString& routingStatus,
                                          const QString& taskPlanStatus) {
    if (routingStatus == QStringLiteral("Unavailable") ||
        taskPlanStatus == QStringLiteral("Blocked")) {
        return OrchestrationHealthStatus::Degraded;
    }

    if (routingStatus.isEmpty() || taskPlanStatus.isEmpty()) {
        return OrchestrationHealthStatus::Unknown;
    }

    return OrchestrationHealthStatus::Ready;
}

} // namespace

ApplicationController::ApplicationController(
    std::unique_ptr<IChatProvider> provider, std::unique_ptr<IMemoryStore> memoryStore,
    std::unique_ptr<ChatSession> chatSession, std::unique_ptr<IChatHistoryStore> chatHistoryStore,
    std::unique_ptr<IAgentRuntime> agentRuntime, std::unique_ptr<IApprovalPolicy> approvalPolicy,
    std::unique_ptr<ISandboxPolicy> sandboxPolicy, std::unique_ptr<IToolExecutor> toolExecutor,
    std::unique_ptr<IModelRouter> modelRouter, std::unique_ptr<IProviderCatalog> providerCatalog,
    std::unique_ptr<ITaskPlanner> taskPlanner, std::unique_ptr<IAgentRegistry> agentRegistry,
    std::unique_ptr<IMemoryCatalog> memoryCatalog, std::unique_ptr<ILocalRuntime> localRuntime,
    std::unique_ptr<ILocalRuntimeSessionManager> localRuntimeSessions,
    std::unique_ptr<IRuntimeCapabilityRegistry> runtimeCapabilities,
    std::unique_ptr<IRuntimePermissionPolicy> runtimePermissionPolicy,
    std::unique_ptr<IRuntimeSafetyPolicy> runtimeSafetyPolicy,
    std::unique_ptr<IRuntimePipeline> runtimePipeline,
    std::unique_ptr<IExecutionLifecycle> executionLifecycle,
    std::unique_ptr<ExecutionCoordinator> executionCoordinator,
    std::unique_ptr<ILocalRuntimeAdapter> localRuntimeAdapter,
    std::unique_ptr<IProviderRuntimeBridge> providerRuntimeBridge,
    std::unique_ptr<StaticRuntimeIntegrationReadiness> runtimeIntegrationReadiness,
    std::unique_ptr<IOllamaRuntimeClient> ollamaRuntimeClient,
    std::unique_ptr<ILocalInferenceClient> localInferenceClient, QObject* parent)
    : QObject(parent), provider_(std::move(provider)), agentRuntime_(std::move(agentRuntime)),
      approvalPolicy_(approvalPolicy ? std::move(approvalPolicy)
                                     : std::make_unique<StaticApprovalPolicy>()),
      sandboxPolicy_(sandboxPolicy ? std::move(sandboxPolicy)
                                   : std::make_unique<StaticSandboxPolicy>()),
      toolExecutor_(toolExecutor ? std::move(toolExecutor) : std::make_unique<NullToolExecutor>()),
      providerCatalog_(providerCatalog ? std::move(providerCatalog)
                                       : std::make_unique<StaticProviderCatalog>()),
      modelRouter_(modelRouter ? std::move(modelRouter)
                               : std::make_unique<StaticModelRouter>(*providerCatalog_)),
      taskPlanner_(taskPlanner ? std::move(taskPlanner) : std::make_unique<StaticTaskPlanner>()),
      agentRegistry_(agentRegistry ? std::move(agentRegistry)
                                   : std::make_unique<StaticAgentRegistry>()),
      memoryCatalog_(memoryCatalog ? std::move(memoryCatalog)
                                   : std::make_unique<StaticMemoryCatalog>()),
      localRuntime_(localRuntime ? std::move(localRuntime) : std::make_unique<NullLocalRuntime>()),
      localRuntimeSessions_(localRuntimeSessions
                                ? std::move(localRuntimeSessions)
                                : std::make_unique<NullLocalRuntimeSessionManager>()),
      runtimeCapabilities_(runtimeCapabilities
                               ? std::move(runtimeCapabilities)
                               : std::make_unique<StaticRuntimeCapabilityRegistry>()),
      runtimePermissionPolicy_(runtimePermissionPolicy
                                   ? std::move(runtimePermissionPolicy)
                                   : std::make_unique<StaticRuntimePermissionPolicy>()),
      runtimeSafetyPolicy_(runtimeSafetyPolicy ? std::move(runtimeSafetyPolicy)
                                               : std::make_unique<StaticRuntimeSafetyPolicy>()),
      runtimePipeline_(runtimePipeline ? std::move(runtimePipeline)
                                       : std::make_unique<StaticRuntimePipeline>()),
      executionLifecycle_(executionLifecycle ? std::move(executionLifecycle)
                                             : std::make_unique<StaticExecutionLifecycle>()),
      executionCoordinator_(executionCoordinator ? std::move(executionCoordinator)
                                                 : std::make_unique<ExecutionCoordinator>()),
      localRuntimeAdapter_(localRuntimeAdapter ? std::move(localRuntimeAdapter)
                                               : std::make_unique<StaticLocalRuntimeAdapter>()),
      providerRuntimeBridge_(providerRuntimeBridge
                                 ? std::move(providerRuntimeBridge)
                                 : std::make_unique<StaticProviderRuntimeBridge>()),
      runtimeIntegrationReadiness_(runtimeIntegrationReadiness
                                       ? std::move(runtimeIntegrationReadiness)
                                       : std::make_unique<StaticRuntimeIntegrationReadiness>()),
      ollamaRuntimeClient_(ollamaRuntimeClient ? std::move(ollamaRuntimeClient)
                                               : std::make_unique<NullOllamaRuntimeClient>()),
      localInferenceClient_(localInferenceClient ? std::move(localInferenceClient)
                                                 : std::make_unique<OllamaLocalInferenceClient>(
                                                       ollamaRuntimeClient_->config())),
      memoryStore_(std::move(memoryStore)),
      chatSession_(chatSession ? std::move(chatSession)
                               : std::make_unique<ChatSession>(std::make_unique<SystemClock>())),
      chatHistoryStore_(std::move(chatHistoryStore)) {
    const auto persistedMessages = chatHistoryStore_ && chatHistoryStore_->isAvailable()
                                       ? chatHistoryStore_->loadMessages()
                                       : QList<ChatMessage>{};
    if (!persistedMessages.isEmpty()) {
        chatSession_->loadMessages(persistedMessages);
    } else {
        const auto message = chatSession_->appendSystemMessage(
            QStringLiteral("Sentinel Core online."), ChatMessageStatus::Received);
        if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
            chatHistoryStore_->appendMessage(message);
        }
    }
    refreshLatestTaskPlan();
    refreshConversationSession();
}

QString ApplicationController::providerName() const {
    return provider_ ? provider_->name() : QStringLiteral("No Provider");
}

QString ApplicationController::providerStatus() const {
    return provider_ ? chatProviderStatusName(provider_->status()) : QStringLiteral("Unavailable");
}

QString ApplicationController::agentStatus() const {
    return agentRuntime_ ? agentStatusName(agentRuntime_->status()) : QStringLiteral("Unavailable");
}

QString ApplicationController::lastAgentResponse() const {
    return lastAgentResponse_;
}

QString ApplicationController::latestToolPlanStatus() const {
    return toolInvocationPlanStatusName(latestAgentPipelineResult_.planningStatus());
}

QString ApplicationController::latestToolPlanSummary() const {
    return safeToolPlanSummary(latestAgentPipelineResult_.plan);
}

QString ApplicationController::latestApprovalStatus() const {
    return approvalStatusName(latestAgentPipelineResult_.approvalStatus());
}

QString ApplicationController::latestApprovalSummary() const {
    return safeApprovalSummary(latestAgentPipelineResult_.approval);
}

QString ApplicationController::latestSandboxStatus() const {
    return sandboxStatusName(latestAgentPipelineResult_.sandboxStatus());
}

QString ApplicationController::latestSandboxSummary() const {
    return safeSandboxSummary(latestAgentPipelineResult_.sandbox);
}

QString ApplicationController::latestToolExecutionStatus() const {
    return toolExecutionStatusName(latestAgentPipelineResult_.executionStatus());
}

QString ApplicationController::latestToolExecutionSummary() const {
    return safeToolExecutionSummary(latestAgentPipelineResult_.execution);
}

QString ApplicationController::latestAgentPipelineStatus() const {
    return agentPipelineStatusName(latestAgentPipelineResult_);
}

QString ApplicationController::latestAgentPipelineSummary() const {
    return safeAgentPipelineSummary(latestAgentPipelineResult_);
}

QString ApplicationController::runtimeSessionId() const {
    return runtimeSession_.context().sessionId.value;
}

QString ApplicationController::runtimeContextStatus() const {
    return runtimeSession_.context().statusName();
}

QString ApplicationController::runtimeContextSummary() const {
    return safeAgentRuntimeContextSummary(runtimeSession_.context());
}

QStringList ApplicationController::runtimeContextActiveToolIds() const {
    return runtimeSession_.context().activePlannedToolIds;
}

const ConversationSession& ApplicationController::currentConversationSession() const {
    return conversationSession_.session();
}

QString ApplicationController::conversationSessionId() const {
    return conversationSession_.session().id.value;
}

QString ApplicationController::conversationSessionStatus() const {
    return conversationSession_.session().statusName();
}

QString ApplicationController::interactionMode() const {
    return conversationSession_.session().interactionModeName();
}

QString ApplicationController::attentionState() const {
    return conversationSession_.session().attentionStateName();
}

QString ApplicationController::contextWindowSummary() const {
    return safeRuntimeContextWindowSummary(conversationSession_.session().contextWindow);
}

QString ApplicationController::conversationState() const {
    return conversationStateName(conversationStateGraph_.currentState());
}

QString ApplicationController::conversationTransitionStatus() const {
    return conversationTransitionStatusName(conversationStateGraph_.lastTransitionResult().status);
}

QString ApplicationController::conversationTransitionSummary() const {
    return safeConversationTransitionSummary(conversationStateGraph_.lastTransitionResult());
}

int ApplicationController::agentActivityCount() const {
    return agentActivityLog_.count();
}

QString ApplicationController::latestAgentActivitySummary() const {
    return agentActivityLog_.latestSummary();
}

QString ApplicationController::currentRoutingMode() const {
    return modelRouter_ ? routingModeName(modelRouter_->routingMode())
                        : routingModeName(RoutingMode::LocalOnly);
}

void ApplicationController::setRoutingModeByName(const QString& routingModeName) {
    if (!modelRouter_) {
        return;
    }

    const auto nextMode = routingModeFromName(routingModeName);
    if (nextMode == modelRouter_->routingMode()) {
        return;
    }

    modelRouter_->setRoutingMode(nextMode);
    refreshLatestTaskPlan();
    refreshConversationSession();
    emit modelRoutingChanged();
    emit taskPlanChanged();
    emit conversationSessionChanged();
    emit orchestrationSnapshotChanged();
}

QString ApplicationController::modelRoutingStatus() const {
    if (!modelRouter_) {
        return modelRoutingStatusName(ModelRoutingStatus::Unavailable);
    }
    return modelRoutingStatusName(
        modelRouter_->route(TaskClassification{TaskType::Unknown}).status);
}

QString ApplicationController::selectedModelProviderSummary() const {
    if (!modelRouter_) {
        return QStringLiteral("No model router available.");
    }
    return safeModelRouteSummary(modelRouter_->route(TaskClassification{TaskType::Unknown}));
}

QString ApplicationController::latestTaskPlanStatus() const {
    return taskPlanStatusName(latestTaskPlan_.status);
}

QString ApplicationController::latestTaskPlanSummary() const {
    return safeTaskPlanSummary(latestTaskPlan_);
}

int ApplicationController::plannedTaskStepCount() const {
    return static_cast<int>(latestTaskPlan_.steps.size());
}

int ApplicationController::registeredAgentCount() const {
    return agentRegistry_ ? static_cast<int>(agentRegistry_->agents().size()) : 0;
}

QStringList ApplicationController::activeAgentSummaries() const {
    QStringList summaries;
    if (!agentRegistry_) {
        return summaries;
    }

    for (const auto& agent : agentRegistry_->agents()) {
        if (isAgentAvailable(agent.state)) {
            summaries.append(agentDescriptorSummary(agent));
        }
    }
    return summaries;
}

QString ApplicationController::currentAgentSummary() const {
    return latestTaskPlan_.preferredAgentSummary.isEmpty()
               ? QStringLiteral("No agent metadata selected.")
               : latestTaskPlan_.preferredAgentSummary;
}

QString ApplicationController::currentMemoryAffinitySummary() const {
    return latestTaskPlan_.preferredMemorySummary.isEmpty()
               ? QStringLiteral("No memory taxonomy metadata selected.")
               : latestTaskPlan_.preferredMemorySummary;
}

int ApplicationController::providerCatalogCount() const {
    return providerCatalog_ ? static_cast<int>(providerCatalog_->entries().size()) : 0;
}

QStringList ApplicationController::providerCatalogSummaries() const {
    QStringList summaries;
    if (!providerCatalog_) {
        return summaries;
    }

    for (const auto& entry : providerCatalog_->entries()) {
        summaries.append(providerCatalogEntrySummary(entry));
    }
    return summaries;
}

int ApplicationController::memoryCatalogCount() const {
    return memoryCatalog_ ? static_cast<int>(memoryCatalog_->shards().size()) : 0;
}

QStringList ApplicationController::memoryCatalogSummaries() const {
    QStringList summaries;
    if (!memoryCatalog_) {
        return summaries;
    }

    for (const auto& shard : memoryCatalog_->shards()) {
        summaries.append(memoryShardSummary(shard));
    }
    return summaries;
}

OrchestrationSnapshot ApplicationController::currentOrchestrationSnapshot() const {
    WorkspaceStateSummary workspace{
        currentRoutingMode(),           modelRoutingStatus(),    selectedModelProviderSummary(),
        latestTaskPlanStatus(),         latestTaskPlanSummary(), currentAgentSummary(),
        currentMemoryAffinitySummary(), runtimeContextStatus(),  runtimeContextSummary(),
        latestAgentActivitySummary(),   providerCatalogCount(),  registeredAgentCount(),
        memoryCatalogCount(),           agentActivityCount(),
    };

    OrchestrationSnapshot snapshot;
    snapshot.healthStatus = healthStatusFor(workspace.routingStatus, workspace.taskPlanStatus);
    snapshot.workspace = workspace;
    snapshot.summary = QStringLiteral("%1 orchestration snapshot: %2 route, %3 task plan, %4 "
                                      "provider entries, %5 agents, %6 memory categories.")
                           .arg(orchestrationHealthStatusName(snapshot.healthStatus),
                                workspace.routingMode, workspace.taskPlanStatus)
                           .arg(workspace.providerCatalogCount)
                           .arg(workspace.registeredAgentCount)
                           .arg(workspace.memoryCatalogCount);
    snapshot.executionEnabled = false;
    snapshot.signalList = {
        OrchestrationSignal{
            QStringLiteral("routing"), QStringLiteral("Routing"),
            QStringLiteral("%1 / %2").arg(workspace.routingMode, workspace.routingStatus)},
        OrchestrationSignal{QStringLiteral("provider-model"), QStringLiteral("Provider Model"),
                            workspace.selectedProviderModelSummary},
        OrchestrationSignal{
            QStringLiteral("task-plan"), QStringLiteral("Task Plan"),
            QStringLiteral("%1 / %2").arg(workspace.taskPlanStatus, workspace.taskPlanSummary)},
        OrchestrationSignal{QStringLiteral("agent"), QStringLiteral("Agent"),
                            workspace.preferredAgentSummary},
        OrchestrationSignal{QStringLiteral("memory"), QStringLiteral("Memory"),
                            workspace.memoryAffinitySummary},
        OrchestrationSignal{QStringLiteral("catalogs"), QStringLiteral("Catalogs"),
                            QStringLiteral("%1 providers / %2 agents / %3 memory")
                                .arg(workspace.providerCatalogCount)
                                .arg(workspace.registeredAgentCount)
                                .arg(workspace.memoryCatalogCount)},
        OrchestrationSignal{QStringLiteral("runtime"), QStringLiteral("Runtime"),
                            QStringLiteral("%1 / activity %2")
                                .arg(workspace.runtimeContextStatus)
                                .arg(workspace.activityCount)},
    };
    return snapshot;
}

QString ApplicationController::orchestrationSnapshotStatus() const {
    return orchestrationHealthStatusName(currentOrchestrationSnapshot().healthStatus);
}

QString ApplicationController::orchestrationSnapshotSummary() const {
    return safeOrchestrationSnapshotSummary(currentOrchestrationSnapshot());
}

QStringList ApplicationController::orchestrationSignals() const {
    return orchestrationSignalSummaries(currentOrchestrationSnapshot());
}

OrchestrationReadinessReport ApplicationController::currentOrchestrationReadinessReport() const {
    return StaticOrchestrationDiagnostics{}.generate(OrchestrationDiagnosticsInput{
        currentOrchestrationSnapshot(),
        providerCatalog_ ? providerCatalog_->entries() : QList<ProviderCatalogEntry>{},
        providerCatalog_ != nullptr,
        agentRegistry_ != nullptr,
        memoryCatalog_ != nullptr,
        taskPlanner_ != nullptr,
    });
}

QString ApplicationController::orchestrationReadinessStatus() const {
    return currentOrchestrationReadinessReport().status;
}

QString ApplicationController::orchestrationReadinessSummary() const {
    return safeOrchestrationReadinessSummary(currentOrchestrationReadinessReport());
}

QStringList ApplicationController::orchestrationDiagnostics() const {
    return orchestrationDiagnosticSummaries(currentOrchestrationReadinessReport());
}

QString ApplicationController::localRuntimeStatus() const {
    if (!localRuntime_) {
        return localRuntimeStatusName(LocalRuntimeStatus::Unavailable);
    }
    return localRuntimeStatusName(localRuntime_->descriptor().status);
}

QString ApplicationController::localRuntimeHealth() const {
    if (!localRuntime_) {
        return localRuntimeHealthName(LocalRuntimeHealth::Unavailable);
    }
    return localRuntimeHealthName(localRuntime_->descriptor().health);
}

QString ApplicationController::localRuntimeSummary() const {
    if (!localRuntime_) {
        return QStringLiteral("No local runtime boundary available.");
    }
    return safeLocalRuntimeSummary(localRuntime_->descriptor());
}

QStringList ApplicationController::localRuntimeCapabilities() const {
    if (!localRuntime_) {
        return {};
    }
    return localRuntimeCapabilitySummaries(localRuntime_->descriptor().capabilities);
}

QString ApplicationController::localRuntimeResponseStatus() const {
    if (!localRuntime_) {
        return QStringLiteral("Unavailable");
    }
    return localRuntime_->evaluate(LocalRuntimeRequest{}).status;
}

QString ApplicationController::localRuntimeResponseSummary() const {
    if (!localRuntime_) {
        return QStringLiteral("No local runtime boundary available.");
    }
    return safeLocalRuntimeResponseSummary(localRuntime_->evaluate(LocalRuntimeRequest{}));
}

int ApplicationController::localRuntimeSessionCount() const {
    return localRuntimeSessions_ ? static_cast<int>(localRuntimeSessions_->sessions().size()) : 0;
}

QString ApplicationController::localRuntimeSessionStatus() const {
    if (!localRuntimeSessions_) {
        return localRuntimeSessionStatusName(LocalRuntimeSessionStatus::NotStarted);
    }
    return localRuntimeSessionStatusName(localRuntimeSessions_->currentSession().status);
}

QString ApplicationController::localRuntimeSessionHealth() const {
    if (!localRuntimeSessions_) {
        return localRuntimeSessionHealthName(LocalRuntimeSessionHealth::Unavailable);
    }
    return localRuntimeSessionHealthName(localRuntimeSessions_->currentSession().health);
}

QString ApplicationController::localRuntimeSessionSummary() const {
    if (!localRuntimeSessions_) {
        return QStringLiteral("No local runtime session metadata.");
    }
    return safeLocalRuntimeSessionSummary(localRuntimeSessions_->currentSession());
}

QString ApplicationController::localRuntimeAllocationSummary() const {
    if (!localRuntimeSessions_) {
        return QStringLiteral("No local runtime allocation metadata.");
    }
    return safeLocalRuntimeAllocationSummary(localRuntimeSessions_->currentSession().allocation);
}

QString ApplicationController::localRuntimeReservationSummary() const {
    if (!localRuntimeSessions_) {
        return QStringLiteral("No local runtime reservation metadata.");
    }
    return safeLocalRuntimeReservationSummary(localRuntimeSessions_->currentSession().reservation);
}

QStringList ApplicationController::localRuntimeSessionSummaries() const {
    if (!localRuntimeSessions_) {
        return {};
    }
    return sentinel::core::localRuntimeSessionSummaries(localRuntimeSessions_->sessions());
}

int ApplicationController::runtimeCapabilityCount() const {
    return runtimeCapabilities_ ? static_cast<int>(runtimeCapabilities_->capabilities().size()) : 0;
}

QStringList ApplicationController::enabledRuntimeCapabilitySummaries() const {
    if (!runtimeCapabilities_) {
        return {};
    }
    return sentinel::core::enabledRuntimeCapabilitySummaries(runtimeCapabilities_->capabilities());
}

QStringList ApplicationController::disabledRuntimeCapabilitySummaries() const {
    if (!runtimeCapabilities_) {
        return {};
    }
    return sentinel::core::disabledRuntimeCapabilitySummaries(runtimeCapabilities_->capabilities());
}

QString ApplicationController::runtimeNegotiationProfileSummary() const {
    if (!runtimeCapabilities_) {
        return QStringLiteral("No runtime negotiation profile metadata.");
    }
    return safeRuntimeNegotiationProfileSummary(runtimeCapabilities_->negotiate().profile);
}

QString ApplicationController::runtimeNegotiationSummary() const {
    if (!runtimeCapabilities_) {
        return QStringLiteral("No runtime negotiation metadata.");
    }
    return safeRuntimeNegotiationSummary(runtimeCapabilities_->negotiate());
}

QString ApplicationController::localOnlyRuntimeEnforcementSummary() const {
    if (!runtimeCapabilities_) {
        return QStringLiteral("No local-only runtime enforcement metadata.");
    }
    return sentinel::core::localOnlyRuntimeEnforcementSummary(runtimeCapabilities_->negotiate());
}

QString ApplicationController::runtimePermissionDecision() const {
    return runtimePermissionDecisionStatusName(currentRuntimePermissionDecision().status);
}

QString ApplicationController::runtimePermissionSummary() const {
    return safeRuntimePermissionDecisionSummary(currentRuntimePermissionDecision());
}

QString ApplicationController::runtimeSafetyDecision() const {
    return runtimeSafetyDecisionName(currentRuntimeSafetyReport().decision);
}

QString ApplicationController::runtimeSafetySummary() const {
    return safeRuntimeSafetySummary(currentRuntimeSafetyReport());
}

QString ApplicationController::runtimePipelineStatus() const {
    return runtimePipelineStatusName(currentRuntimePipelineResult().status);
}

QString ApplicationController::runtimePipelineSummary() const {
    return safeRuntimePipelineSummary(currentRuntimePipelineResult());
}

QStringList ApplicationController::runtimePipelineTraceSummaries() const {
    return sentinel::core::runtimePipelineTraceSummaries(currentRuntimePipelineResult().traces);
}

QString ApplicationController::executionLifecycleState() const {
    return executionLifecycleStateName(currentExecutionLifecycleResult().state);
}

QString ApplicationController::executionLifecycleStatus() const {
    return executionLifecycleStatusName(currentExecutionLifecycleResult().status);
}

QString ApplicationController::executionLifecycleSummary() const {
    return safeExecutionLifecycleSummary(currentExecutionLifecycleResult());
}

QStringList ApplicationController::executionLifecycleTraceSummaries() const {
    return sentinel::core::executionLifecycleTraceSummaries(
        currentExecutionLifecycleResult().traces);
}

QString ApplicationController::executionSessionId() const {
    return executionCoordinator_ ? executionCoordinator_->session().id.value : QString{};
}

QString ApplicationController::executionSessionStatus() const {
    return executionCoordinator_
               ? executionSessionStatusName(executionCoordinator_->session().status)
               : QStringLiteral("Blocked");
}

QString ApplicationController::executionSessionOwnership() const {
    return executionCoordinator_
               ? executionOwnershipName(executionCoordinator_->session().ownership)
               : QStringLiteral("Application Controller");
}

QString ApplicationController::executionCoordinationMode() const {
    return executionCoordinator_
               ? executionCoordinationModeName(executionCoordinator_->session().coordinationMode)
               : QStringLiteral("Metadata Only");
}

QString ApplicationController::executionSessionSummary() const {
    return executionCoordinator_ ? safeExecutionSessionSummary(executionCoordinator_->session())
                                 : QStringLiteral("No execution session metadata.");
}

QString ApplicationController::executionCoordinationSnapshotSummary() const {
    return safeExecutionCoordinationSnapshotSummary(currentExecutionCoordinationSnapshot());
}

QString ApplicationController::localRuntimeAdapterStatus() const {
    if (!localRuntimeAdapter_) {
        return localRuntimeAdapterStatusName(LocalRuntimeAdapterStatus::Unavailable);
    }
    return localRuntimeAdapterStatusName(localRuntimeAdapter_->descriptor().status);
}

QString ApplicationController::localRuntimeAdapterHealth() const {
    if (!localRuntimeAdapter_) {
        return localRuntimeAdapterHealthName(LocalRuntimeAdapterHealth::NotExecutable);
    }
    return localRuntimeAdapterHealthName(localRuntimeAdapter_->descriptor().health);
}

QString ApplicationController::localRuntimeAdapterSummary() const {
    if (!localRuntimeAdapter_) {
        return QStringLiteral("No local runtime adapter metadata.");
    }
    return safeLocalRuntimeAdapterSummary(localRuntimeAdapter_->descriptor());
}

QStringList ApplicationController::localRuntimeAdapterCapabilitySummaries() const {
    if (!localRuntimeAdapter_) {
        return {};
    }
    return sentinel::core::localRuntimeAdapterCapabilitySummaries(
        localRuntimeAdapter_->descriptor().capabilities);
}

QString ApplicationController::providerRuntimeBridgeStatus() const {
    if (!providerRuntimeBridge_) {
        return providerRuntimeBridgeStatusName(ProviderRuntimeBridgeStatus::Unavailable);
    }
    return providerRuntimeBridgeStatusName(providerRuntimeBridge_->summary().status);
}

QString ApplicationController::providerRuntimeBridgeSummary() const {
    if (!providerRuntimeBridge_) {
        return QStringLiteral("No provider runtime bridge metadata.");
    }
    return safeProviderRuntimeBridgeSummary(providerRuntimeBridge_->summary());
}

QString ApplicationController::providerRuntimeBridgeResponseSummary() const {
    return safeProviderRuntimeBridgeResponseSummary(currentProviderRuntimeBridgeResponse());
}

QString ApplicationController::runtimeIntegrationReadinessStatus() const {
    return runtimeIntegrationReadinessName(currentRuntimeIntegrationReport().readiness);
}

QString ApplicationController::runtimeIntegrationReadinessSummary() const {
    return safeRuntimeIntegrationReportSummary(currentRuntimeIntegrationReport());
}

QStringList ApplicationController::runtimeIntegrationReadinessChecks() const {
    return sentinel::core::runtimeIntegrationCheckSummaries(
        currentRuntimeIntegrationReport().checks);
}

QString ApplicationController::ollamaEndpoint() const {
    return ollamaRuntimeClient_ ? ollamaRuntimeClient_->config().endpoint.toString()
                                : OllamaEndpoint::defaultEndpoint().toString();
}

QString ApplicationController::ollamaConnectionStatus() const {
    return ollamaConnectionStatusName(currentOllamaHealthCheck().connectionStatus);
}

QString ApplicationController::ollamaHealthStatus() const {
    return ollamaHealthStatusName(currentOllamaHealthCheck().healthStatus);
}

QString ApplicationController::ollamaHealthSummary() const {
    return safeOllamaHealthSummary(currentOllamaHealthCheck());
}

int ApplicationController::ollamaModelCount() const {
    return static_cast<int>(currentOllamaModels().size());
}

QStringList ApplicationController::ollamaModelSummaries() const {
    return sentinel::core::ollamaModelSummaries(currentOllamaModels());
}

QString ApplicationController::selectedLocalModel() const {
    return selectedLocalModel_;
}

void ApplicationController::setSelectedLocalModel(const QString& model) {
    const auto normalized = model.trimmed();
    if (normalized == selectedLocalModel_) {
        return;
    }

    selectedLocalModel_ = normalized;
    emit localModelSelectionChanged();
    emit localChatInferenceRoutingChanged();
}

QString ApplicationController::selectedLocalModelSummary() const {
    const auto models = currentOllamaModels();
    const auto selected = selectedLocalModel_.trimmed();
    if (selected.isEmpty()) {
        if (!models.isEmpty()) {
            return QStringLiteral("No local model selected; fallback candidate is %1.")
                .arg(models.first().name);
        }
        return QStringLiteral("No local model selected; inference requires an explicit model.");
    }

    if (!models.isEmpty() && !discoveredModelNamesContain(selected, models)) {
        return QStringLiteral("Selected local model %1 was not found in discovered local models.")
            .arg(selected);
    }

    if (models.isEmpty()) {
        return QStringLiteral("Selected local model %1 is configured; discovery metadata is "
                              "currently unavailable.")
            .arg(selected);
    }

    return QStringLiteral("Selected local model %1 is available in local discovery metadata.")
        .arg(selected);
}

QString ApplicationController::activeLocalRuntimeBadge() const {
    const auto model = effectiveLocalModel({});
    return model.isEmpty() ? QStringLiteral("Ollama Local / No Model")
                           : QStringLiteral("Ollama Local / %1").arg(model);
}

bool ApplicationController::localChatInferenceEnabled() const {
    return localChatInferenceEnabled_;
}

void ApplicationController::setLocalChatInferenceEnabled(bool enabled) {
    if (enabled == localChatInferenceEnabled_) {
        return;
    }

    localChatInferenceEnabled_ = enabled;
    emit localChatInferenceRoutingChanged();
}

QString ApplicationController::localChatInferenceStatus() const {
    if (!localChatInferenceEnabled_) {
        return QStringLiteral("Disabled");
    }
    if (!localInferenceEndpointAllowed()) {
        return QStringLiteral("Blocked");
    }
    if (effectiveLocalModel({}).isEmpty()) {
        return QStringLiteral("Missing Model");
    }
    const auto models = currentOllamaModels();
    const auto model = effectiveLocalModel({});
    if (!models.isEmpty() && !discoveredModelNamesContain(model, models)) {
        return QStringLiteral("Invalid Model");
    }
    return QStringLiteral("Enabled");
}

QString ApplicationController::localChatInferenceSummary() const {
    if (!localChatInferenceEnabled_) {
        return QStringLiteral("Local chat inference is disabled; chat uses the local safe "
                              "provider path.");
    }
    if (!localInferenceEndpointAllowed()) {
        return QStringLiteral("Local chat inference is blocked: Ollama endpoint must be local "
                              "loopback HTTP.");
    }
    if (effectiveLocalModel({}).isEmpty()) {
        return QStringLiteral("Local chat inference is enabled but no valid local model is "
                              "selected.");
    }
    if (localChatInferenceStatus() == QStringLiteral("Invalid Model")) {
        return QStringLiteral("Local chat inference is enabled but the selected model is not in "
                              "discovered local model metadata.");
    }
    return QStringLiteral(
        "Local chat inference is enabled for explicit local-only Ollama routing.");
}

bool ApplicationController::localInferenceBusy() const {
    return localInferenceBusy_;
}

QString ApplicationController::localInferenceRuntimeState() const {
    if (localInferenceBusy_) {
        return QStringLiteral("Busy");
    }
    if (latestLocalInferenceResponse_.status == LocalInferenceStatus::Error ||
        latestLocalInferenceResponse_.status == LocalInferenceStatus::Blocked ||
        latestLocalInferenceResponse_.status == LocalInferenceStatus::InvalidRequest ||
        latestLocalInferenceResponse_.status == LocalInferenceStatus::ModelUnavailable) {
        return QStringLiteral("Error");
    }
    return QStringLiteral("Idle");
}

QString ApplicationController::localInferenceStatus() const {
    return localInferenceBusy_ ? localInferenceStatusName(LocalInferenceStatus::Busy)
                               : localInferenceStatusName(latestLocalInferenceResponse_.status);
}

QString ApplicationController::localInferenceSummary() const {
    if (latestLocalInferenceResponse_.status == LocalInferenceStatus::NotRequested &&
        localInferenceClient_) {
        return localInferenceClient_->statusSummary();
    }

    return safeLocalInferenceSummary(latestLocalInferenceResponse_);
}

QString ApplicationController::localInferenceLastResponseSummary() const {
    return safeLocalInferenceResponseSummary(latestLocalInferenceResponse_);
}

QString ApplicationController::localInferenceLatencySummary() const {
    return latestLocalInferenceResponse_.latencyMs >= 0
               ? QStringLiteral("Last local inference latency: %1 ms.")
                     .arg(latestLocalInferenceResponse_.latencyMs)
               : QStringLiteral("No local inference latency recorded.");
}

QStringList ApplicationController::localInferenceTraceSummaries() const {
    return sentinel::core::localInferenceTraceSummaries(latestLocalInferenceResponse_.traces);
}

bool ApplicationController::localInferenceStreamingAvailable() const {
    return false;
}

QString ApplicationController::localInferenceStreamStatus() const {
    return localInferenceStreamStatusName(latestLocalInferenceStreamResult_.status);
}

QString ApplicationController::localInferenceStreamSummary() const {
    return latestLocalInferenceStreamResult_.summary;
}

int ApplicationController::availableToolCount() const {
    return agentRuntime_ ? static_cast<int>(agentRuntime_->availableTools().size()) : 0;
}

QStringList ApplicationController::availableToolIds() const {
    QStringList ids;
    if (!agentRuntime_) {
        return ids;
    }

    for (const auto& tool : agentRuntime_->availableTools()) {
        ids.append(tool.id);
    }
    return ids;
}

QString ApplicationController::memoryStatus() const {
    return memoryStore_ && memoryStore_->isAvailable() ? QStringLiteral("Available")
                                                       : QStringLiteral("Unavailable");
}

QString ApplicationController::chatHistoryStatus() const {
    return chatHistoryStore_ && chatHistoryStore_->isAvailable() ? QStringLiteral("Available")
                                                                 : QStringLiteral("Runtime Only");
}

QString ApplicationController::memoryMaintenanceStatus() const {
    return memoryMaintenanceStatus_;
}

QString ApplicationController::chatMaintenanceStatus() const {
    return chatMaintenanceStatus_;
}

const QList<ChatMessage>& ApplicationController::chatHistory() const {
    return chatSession_->messages();
}

QStringList ApplicationController::chatMessages() const {
    QStringList result;
    for (const auto& message : chatSession_->messages()) {
        if (message.role == ChatRole::User) {
            result.append(QStringLiteral("You: %1").arg(message.content));
        } else {
            result.append(QStringLiteral("Sentinel: %1").arg(message.content));
        }
    }
    return result;
}

QStringList ApplicationController::memoryEntries() const {
    QStringList result;
    if (!memoryStore_) {
        return result;
    }

    for (const auto& [key, value] : memoryStore_->entries()) {
        result.append(QStringLiteral("%1: %2").arg(key, value));
    }

    return result;
}

bool ApplicationController::sendMessage(const QString& message) {
    const auto trimmed = message.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    resetCompletedConversationState();
    transitionConversationState(ConversationState::Listening,
                                QStringLiteral("chat input metadata received"));
    transitionConversationState(ConversationState::Planning,
                                QStringLiteral("chat request metadata prepared"));
    transitionConversationState(ConversationState::Routing,
                                QStringLiteral("chat route metadata selected"));

    const auto userMessage = chatSession_->appendUserMessage(trimmed);
    if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
        chatHistoryStore_->appendMessage(userMessage);
    }

    if (localChatInferenceEnabled_) {
        transitionConversationState(ConversationState::ReadyToRespond,
                                    QStringLiteral("local chat inference metadata ready"));
        transitionConversationState(ConversationState::Responding,
                                    QStringLiteral("local chat inference metadata active"));

        const auto ranLocalInference = runLocalInference(trimmed, {});
        const auto localStatus = latestLocalInferenceResponse_.status;
        const auto assistantMessage = chatSession_->appendAssistantMessage(
            ranLocalInference ? latestLocalInferenceResponse_.text
                              : QStringLiteral("Local inference unavailable: %1")
                                    .arg(safeLocalInferenceSummary(latestLocalInferenceResponse_)),
            ranLocalInference ? ChatMessageStatus::Received : ChatMessageStatus::Error);
        if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
            chatHistoryStore_->appendMessage(assistantMessage);
        }
        transitionConversationState(
            ranLocalInference ? ConversationState::Completed : ConversationState::Error,
            ranLocalInference ? QStringLiteral("local chat inference metadata completed")
                              : QStringLiteral("local chat inference metadata blocked"));
        emit chatMessagesChanged();
        return localStatus == LocalInferenceStatus::Succeeded;
    }

    if (!provider_ || provider_->status() != ChatProviderStatus::Ready) {
        transitionConversationState(ConversationState::Error,
                                    QStringLiteral("provider metadata unavailable"));
        const auto errorMessage = chatSession_->appendAssistantMessage(
            QStringLiteral("Provider unavailable. Status: %1").arg(providerStatus()),
            ChatMessageStatus::Error);
        if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
            chatHistoryStore_->appendMessage(errorMessage);
        }
        emit chatMessagesChanged();
        return false;
    }

    transitionConversationState(ConversationState::ReadyToRespond,
                                QStringLiteral("chat response metadata ready"));
    transitionConversationState(ConversationState::Responding,
                                QStringLiteral("chat response metadata active"));
    const auto reply = provider_->sendMessage(trimmed);
    if (!reply.success) {
        transitionConversationState(ConversationState::Error,
                                    QStringLiteral("chat provider returned error metadata"));
    } else {
        transitionConversationState(ConversationState::Completed,
                                    QStringLiteral("chat response metadata completed"));
    }
    const auto assistantMessage = chatSession_->appendAssistantMessage(
        reply.success ? reply.message
                      : QStringLiteral("Provider error: %1").arg(reply.errorMessage),
        reply.success ? ChatMessageStatus::Received : ChatMessageStatus::Error);
    if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
        chatHistoryStore_->appendMessage(assistantMessage);
    }
    emit chatMessagesChanged();
    return reply.success;
}

bool ApplicationController::runLocalInference(const QString& prompt, const QString& model) {
    LocalInferenceRequest request;
    request.prompt = prompt.trimmed();
    request.options.model = effectiveLocalModel(model);

    if (request.prompt.isEmpty()) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::BlankPrompt,
            QStringLiteral("Local inference request rejected: prompt is blank."));
        latestLocalInferenceResponse_.status = LocalInferenceStatus::InvalidRequest;
        emit localInferenceChanged();
        return false;
    }

    if (request.options.model.isEmpty()) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::MissingModel,
            QStringLiteral("Local inference request rejected: model is required."));
        latestLocalInferenceResponse_.status = LocalInferenceStatus::InvalidRequest;
        emit localInferenceChanged();
        return false;
    }

    if (!localInferenceEndpointAllowed()) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::EndpointBlocked,
            QStringLiteral("Local inference blocked: endpoint must be local loopback HTTP."));
        emit localInferenceChanged();
        return false;
    }

    const auto discoveredModels = currentOllamaModels();
    if (!discoveredModels.isEmpty() &&
        !discoveredModelNamesContain(request.options.model, discoveredModels)) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::ModelUnavailable,
            QStringLiteral("Local inference request rejected: selected model is not installed."));
        latestLocalInferenceResponse_.status = LocalInferenceStatus::ModelUnavailable;
        emit localInferenceChanged();
        return false;
    }

    const auto permissionDecision = currentRuntimePermissionDecision();
    if (permissionDecision.status != RuntimePermissionDecisionStatus::Allowed) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::PermissionDenied,
            QStringLiteral("Local inference blocked by runtime permission policy."));
        latestLocalInferenceResponse_.traces.append(LocalInferenceTrace{
            2,
            QStringLiteral("Permission Policy"),
            runtimePermissionDecisionStatusName(permissionDecision.status),
            safeRuntimePermissionDecisionSummary(permissionDecision),
        });
        emit localInferenceChanged();
        return false;
    }

    const auto safetyReport = currentRuntimeSafetyReport();
    if (safetyReport.decision != RuntimeSafetyDecision::Compliant) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::SafetyBlocked,
            QStringLiteral("Local inference blocked by runtime safety policy."));
        latestLocalInferenceResponse_.traces.append(LocalInferenceTrace{
            2,
            QStringLiteral("Permission Policy"),
            runtimePermissionDecisionStatusName(permissionDecision.status),
            safeRuntimePermissionDecisionSummary(permissionDecision),
        });
        latestLocalInferenceResponse_.traces.append(LocalInferenceTrace{
            3,
            QStringLiteral("Safety Policy"),
            runtimeSafetyDecisionName(safetyReport.decision),
            safeRuntimeSafetySummary(safetyReport),
        });
        emit localInferenceChanged();
        return false;
    }

    if (!localInferenceClient_) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::ClientUnavailable,
            QStringLiteral("Local inference blocked: client is unavailable."));
        emit localInferenceChanged();
        return false;
    }

    localInferenceBusy_ = true;
    latestLocalInferenceResponse_.status = LocalInferenceStatus::Busy;
    latestLocalInferenceResponse_.summary = QStringLiteral("Local inference request is running.");
    emit localInferenceChanged();

    QElapsedTimer latencyTimer;
    latencyTimer.start();
    auto clientResponse = localInferenceClient_->infer(request);
    clientResponse.latencyMs = latencyTimer.elapsed();
    localInferenceBusy_ = false;
    latestLocalInferenceResponse_ = clientResponse;
    latestLocalInferenceResponse_.traces = {
        LocalInferenceTrace{
            1,
            QStringLiteral("Permission Policy"),
            runtimePermissionDecisionStatusName(permissionDecision.status),
            safeRuntimePermissionDecisionSummary(permissionDecision),
        },
        LocalInferenceTrace{
            2,
            QStringLiteral("Safety Policy"),
            runtimeSafetyDecisionName(safetyReport.decision),
            safeRuntimeSafetySummary(safetyReport),
        },
    };
    latestLocalInferenceResponse_.traces.append(clientResponse.traces);
    for (int i = 0; i < latestLocalInferenceResponse_.traces.size(); ++i) {
        latestLocalInferenceResponse_.traces[i].sequence = i + 1;
    }
    emit localInferenceChanged();
    return latestLocalInferenceResponse_.status == LocalInferenceStatus::Succeeded;
}

bool ApplicationController::runAgentRequest(const QString& request) {
    const auto trimmed = request.trimmed();
    if (trimmed.isEmpty()) {
        if (lastAgentResponse_ != QStringLiteral("Agent request was empty.")) {
            lastAgentResponse_ = QStringLiteral("Agent request was empty.");
            emit agentResponseChanged();
        }
        return false;
    }

    resetCompletedConversationState();
    transitionConversationState(ConversationState::Listening,
                                QStringLiteral("agent request metadata received"));
    transitionConversationState(ConversationState::Planning,
                                QStringLiteral("agent planning metadata started"));

    agentActivityLog_.append(AgentActivityType::RequestReceived, AgentActivityStatus::Recorded,
                             QStringLiteral("Agent request received."));

    if (!agentRuntime_) {
        transitionConversationState(ConversationState::Error,
                                    QStringLiteral("agent runtime metadata unavailable"));
        agentActivityLog_.append(AgentActivityType::PipelineCompleted, AgentActivityStatus::Blocked,
                                 QStringLiteral("Agent pipeline blocked: runtime unavailable."));
        refreshConversationSession();
        emit agentActivityChanged();
        emit conversationSessionChanged();
        emit orchestrationSnapshotChanged();
        if (lastAgentResponse_ != QStringLiteral("Agent runtime unavailable.")) {
            lastAgentResponse_ = QStringLiteral("Agent runtime unavailable.");
            emit agentResponseChanged();
        }
        return false;
    }

    transitionConversationState(ConversationState::Routing,
                                QStringLiteral("agent route metadata selected"));
    latestAgentPipelineResult_ = buildAgentPipelineResult(AgentRequest{trimmed});
    appendPipelineActivity(latestAgentPipelineResult_);
    runtimeSession_.attachPipelineResult(latestAgentPipelineResult_);
    if (latestAgentPipelineResult_.approvalStatus() == ApprovalStatus::RequiresApproval) {
        transitionConversationState(ConversationState::WaitingForApproval,
                                    QStringLiteral("approval metadata required"));
    } else if (latestAgentPipelineResult_.executionStatus() ==
               ToolExecutionStatus::PlaceholderSucceeded) {
        transitionConversationState(ConversationState::ReadyToRespond,
                                    QStringLiteral("agent response metadata ready"));
        transitionConversationState(ConversationState::Responding,
                                    QStringLiteral("agent response metadata active"));
        transitionConversationState(ConversationState::Completed,
                                    QStringLiteral("agent response metadata completed"));
    } else {
        transitionConversationState(ConversationState::Error,
                                    QStringLiteral("agent pipeline metadata blocked"));
    }
    refreshConversationSession();
    emit toolPlanChanged();
    emit approvalChanged();
    emit sandboxChanged();
    emit toolExecutionChanged();
    emit agentPipelineChanged();
    emit runtimeContextChanged();
    emit conversationSessionChanged();
    emit agentActivityChanged();
    emit orchestrationSnapshotChanged();

    const auto response = agentRuntime_->execute(AgentRequest{trimmed});
    const auto nextMessage =
        response.message.isEmpty() ? QStringLiteral("No agent response.") : response.message;
    if (lastAgentResponse_ != nextMessage) {
        lastAgentResponse_ = nextMessage;
        emit agentResponseChanged();
    }
    emit agentStatusChanged();
    return response.success;
}

AgentPipelineResult
ApplicationController::buildAgentPipelineResult(const AgentRequest& request) const {
    AgentPipelineResult result;
    result.plan = agentRuntime_->plan(request);
    result.approval = approvalPolicy_->evaluate(result.plan);
    result.sandbox = sandboxPolicy_->evaluate(result.plan, result.approval);
    result.execution = toolExecutor_->execute(ToolExecutionRequest{
        result.plan,
        result.approval,
        result.sandbox,
        availableToolIds(),
    });
    result.summary = safeToolExecutionSummary(result.execution);
    return result;
}

void ApplicationController::appendPipelineActivity(const AgentPipelineResult& result) {
    agentActivityLog_.append(AgentActivityType::PlanCreated,
                             planActivityStatus(result.planningStatus()),
                             QStringLiteral("Tool planning evaluated: %1")
                                 .arg(toolInvocationPlanStatusName(result.planningStatus())));
    agentActivityLog_.append(AgentActivityType::ApprovalEvaluated,
                             approvalActivityStatus(result.approvalStatus()),
                             QStringLiteral("Approval metadata evaluated: %1")
                                 .arg(approvalStatusName(result.approvalStatus())));
    agentActivityLog_.append(AgentActivityType::SandboxEvaluated,
                             sandboxActivityStatus(result.sandboxStatus()),
                             QStringLiteral("Sandbox metadata evaluated: %1")
                                 .arg(sandboxStatusName(result.sandboxStatus())));
    agentActivityLog_.append(AgentActivityType::PlaceholderExecutionEvaluated,
                             executionActivityStatus(result.executionStatus()),
                             QStringLiteral("Placeholder execution evaluated: %1")
                                 .arg(toolExecutionStatusName(result.executionStatus())));
    agentActivityLog_.append(
        AgentActivityType::PipelineCompleted, executionActivityStatus(result.executionStatus()),
        QStringLiteral("Agent pipeline finished: %1").arg(agentPipelineStatusName(result)));
}

void ApplicationController::resetCompletedConversationState() {
    if (conversationStateGraph_.currentState() == ConversationState::Completed ||
        conversationStateGraph_.currentState() == ConversationState::Error) {
        transitionConversationState(ConversationState::Idle,
                                    QStringLiteral("conversation state reset for new metadata"));
    }
}

void ApplicationController::transitionConversationState(ConversationState nextState,
                                                        const QString& reason) {
    conversationStateGraph_.transitionTo(nextState, reason);
    emit conversationStateChanged();
}

void ApplicationController::refreshLatestTaskPlan() {
    if (!taskPlanner_ || !providerCatalog_) {
        latestTaskPlan_ = TaskPlan{
            TaskPlanStatus::Blocked,
            modelRouter_ ? modelRouter_->routingMode() : RoutingMode::LocalOnly,
            TaskClassification{TaskType::Unknown},
            {},
            {},
            QStringLiteral("Task planner metadata is unavailable."),
            {},
            {},
            {},
            {},
            {},
            false,
            false,
        };
        return;
    }

    latestTaskPlan_ = taskPlanner_->plan(TaskPlanningRequest{
        TaskClassification{TaskType::Unknown},
        modelRouter_ ? modelRouter_->routingMode() : RoutingMode::LocalOnly,
        providerCatalog_->entries(),
        agentRegistry_ ? agentRegistry_->agents() : QList<AgentDescriptor>{},
        memoryCatalog_ ? memoryCatalog_->shards() : QList<MemoryShardDescriptor>{},
    });
}

void ApplicationController::refreshConversationSession() {
    conversationSession_.refreshContext(ConversationSessionContextBuilder{}.build(
        currentRoutingMode(), currentAgentSummary(), currentMemoryAffinitySummary(),
        orchestrationSnapshotSummary()));
}

RuntimePermissionRequest ApplicationController::runtimePermissionRequest() const {
    return RuntimePermissionRequest{
        RuntimePermission::LocalInference,
        RuntimePermissionLevel::Execute,
        QStringLiteral("local-runtime-request"),
        QStringLiteral("Evaluate metadata-only permission for local runtime execution request."),
    };
}

RuntimePermissionDecision ApplicationController::currentRuntimePermissionDecision() const {
    if (!runtimePermissionPolicy_) {
        return RuntimePermissionDecision{
            RuntimePermissionDecisionStatus::Denied,
            runtimePermissionRequest(),
            QStringLiteral("No runtime permission policy available."),
        };
    }

    return runtimePermissionPolicy_->evaluate(runtimePermissionRequest());
}

RuntimeSafetyReport ApplicationController::currentRuntimeSafetyReport() const {
    if (!runtimeSafetyPolicy_) {
        RuntimeSafetyReport report;
        report.policy = RuntimeSafetyPolicy{
            QStringLiteral("runtime-safety-policy-missing"),
            QStringLiteral("Missing Runtime Safety Policy"),
            true,
            true,
            QStringLiteral("No runtime safety policy available."),
        };
        report.decision = RuntimeSafetyDecision::Blocked;
        report.summary = QStringLiteral("No runtime safety policy available.");
        return report;
    }

    return runtimeSafetyPolicy_->evaluate();
}

RuntimePipelineResult ApplicationController::currentRuntimePipelineResult() const {
    if (!runtimePipeline_) {
        RuntimePipelineResult result;
        result.status = RuntimePipelineStatus::Blocked;
        result.summary = QStringLiteral("No runtime pipeline available.");
        result.traces = {RuntimePipelineTrace{
            RuntimePipelineStage::ExecutionBoundary,
            QStringLiteral("Blocked"),
            QStringLiteral("Runtime pipeline is unavailable."),
        }};
        result.executionBlocked = true;
        return result;
    }

    return runtimePipeline_->evaluate(
        RuntimePipelineRequest{
            QStringLiteral("runtime-pipeline-request-1"),
            QStringLiteral("Runtime request pipeline metadata evaluation."),
            runtimePermissionRequest(),
        },
        currentRuntimePermissionDecision(), currentRuntimeSafetyReport());
}

ExecutionRequest ApplicationController::executionRequest() const {
    return ExecutionRequest{
        QStringLiteral("execution-request-1"),
        ExecutionIntent::RuntimePlaceholder,
        ExecutionPriority::Normal,
        QStringLiteral("Coordinate future execution lifecycle metadata without enabling "
                       "execution."),
    };
}

ExecutionLifecycleResult ApplicationController::currentExecutionLifecycleResult() const {
    if (!executionLifecycle_) {
        ExecutionLifecycleResult result;
        result.request = executionRequest();
        result.state = ExecutionLifecycleState::Blocked;
        result.status = ExecutionLifecycleStatus::Blocked;
        result.summary = QStringLiteral("No execution lifecycle available; execution is blocked.");
        result.executable = false;
        result.traces = {ExecutionLifecycleTrace{
            1,
            ExecutionLifecycleState::Blocked,
            ExecutionTraceLevel::Blocked,
            QStringLiteral("Execution lifecycle is unavailable."),
        }};
        return result;
    }

    return executionLifecycle_->evaluate(executionRequest());
}

ExecutionCoordinationSnapshot ApplicationController::currentExecutionCoordinationSnapshot() const {
    if (!executionCoordinator_) {
        ExecutionCoordinationSnapshot snapshot;
        snapshot.lifecycle = currentExecutionLifecycleResult();
        snapshot.readOnly = true;
        snapshot.executable = false;
        snapshot.summary = QStringLiteral("No execution coordinator available; execution is "
                                          "blocked.");
        return snapshot;
    }

    return executionCoordinator_->coordinate(currentExecutionLifecycleResult());
}

ProviderRuntimeBridgeRequest ApplicationController::providerRuntimeBridgeRequest() const {
    return ProviderRuntimeBridgeRequest{
        QStringLiteral("provider-runtime-bridge-request-1"),
        QStringLiteral("ollama-local"),
        localRuntimeAdapter_ ? localRuntimeAdapter_->descriptor().id
                             : QStringLiteral("local-runtime-adapter-unavailable"),
        QStringLiteral("Evaluate provider runtime bridge metadata without connecting or "
                       "executing."),
    };
}

ProviderRuntimeBridgeResponse ApplicationController::currentProviderRuntimeBridgeResponse() const {
    if (!providerRuntimeBridge_) {
        ProviderRuntimeBridgeResponse response;
        response.request = providerRuntimeBridgeRequest();
        response.status = ProviderRuntimeBridgeStatus::Unavailable;
        response.summary = QStringLiteral("Provider runtime bridge is unavailable.");
        response.connected = false;
        response.executable = false;
        return response;
    }

    return providerRuntimeBridge_->evaluate(providerRuntimeBridgeRequest());
}

RuntimeIntegrationReport ApplicationController::currentRuntimeIntegrationReport() const {
    if (!runtimeIntegrationReadiness_) {
        RuntimeIntegrationReport report;
        report.readiness = RuntimeIntegrationReadiness::Blocked;
        report.summary = QStringLiteral("No runtime integration readiness metadata available.");
        report.executable = false;
        return report;
    }

    const auto adapter =
        localRuntimeAdapter_ ? localRuntimeAdapter_->descriptor() : LocalRuntimeAdapterDescriptor{};
    const auto bridge =
        providerRuntimeBridge_ ? providerRuntimeBridge_->summary() : ProviderRuntimeBridgeSummary{};
    auto report = runtimeIntegrationReadiness_->evaluate(adapter, bridge);
    const auto ollamaConfig =
        ollamaRuntimeClient_ ? ollamaRuntimeClient_->config() : OllamaConfig{};
    for (auto& check : report.checks) {
        if (check.id == QStringLiteral("runtime-integration.endpoint")) {
            check.passed = ollamaConfig.endpoint.isLoopbackHttp();
            check.summary = QStringLiteral("Safe local Ollama endpoint is configured for "
                                           "loopback-only health checks.");
        } else if (check.id == QStringLiteral("runtime-integration.model-discovery")) {
            check.passed = ollamaConfig.modelDiscoveryEnabled;
            check.summary = QStringLiteral("Installed model discovery boundary is available for "
                                           "read-only local metadata.");
        }
    }
    report.summary = QStringLiteral("Runtime integration readiness is blocked: Ollama local "
                                    "health/discovery metadata is available, but provider bridge "
                                    "execution and inference remain disabled.");
    return report;
}

OllamaHealthCheckResult ApplicationController::currentOllamaHealthCheck() const {
    if (!ollamaRuntimeClient_) {
        return NullOllamaRuntimeClient{}.healthCheck();
    }

    return ollamaRuntimeClient_->healthCheck();
}

QList<OllamaModelSummary> ApplicationController::currentOllamaModels() const {
    return ollamaRuntimeClient_ ? ollamaRuntimeClient_->installedModels()
                                : QList<OllamaModelSummary>{};
}

QString ApplicationController::effectiveLocalModel(const QString& requestedModel) const {
    const auto explicitModel = requestedModel.trimmed();
    if (!explicitModel.isEmpty()) {
        return explicitModel;
    }

    const auto selectedModel = selectedLocalModel_.trimmed();
    if (!selectedModel.isEmpty()) {
        return selectedModel;
    }

    const auto models = currentOllamaModels();
    return models.isEmpty() ? QString() : models.first().name;
}

bool ApplicationController::discoveredModelNamesContain(
    const QString& model, const QList<OllamaModelSummary>& models) const {
    for (const auto& discoveredModel : models) {
        if (discoveredModel.name == model) {
            return true;
        }
    }
    return false;
}

bool ApplicationController::localInferenceEndpointAllowed() const {
    const auto config = ollamaRuntimeClient_ ? ollamaRuntimeClient_->config() : OllamaConfig{};
    return config.endpoint.isLoopbackHttp();
}

LocalInferenceResponse ApplicationController::blockedLocalInferenceResponse(
    const LocalInferenceRequest& request, LocalInferenceError error, const QString& summary) const {
    LocalInferenceResponse response;
    response.status = LocalInferenceStatus::Blocked;
    response.error = error;
    response.model = request.options.model.trimmed();
    response.summary = summary;
    response.traces = {
        LocalInferenceTrace{
            1,
            QStringLiteral("Request"),
            QStringLiteral("Blocked"),
            summary,
        },
    };
    return response;
}

bool ApplicationController::clearMemory() {
    if (!memoryStore_ || !memoryStore_->isAvailable()) {
        setMemoryMaintenanceStatus(QStringLiteral("Unavailable"));
        return false;
    }

    memoryStore_->clear();
    const auto succeeded = memoryStore_->lastError().isEmpty();
    setMemoryMaintenanceStatus(succeeded ? QStringLiteral("Clear completed")
                                         : QStringLiteral("Clear failed"));
    emit memoryEntriesChanged();
    return succeeded;
}

bool ApplicationController::clearChat() {
    const auto persistentAvailable = chatHistoryStore_ && chatHistoryStore_->isAvailable();
    auto persistentHealthy = persistentAvailable;
    if (persistentAvailable) {
        chatHistoryStore_->clear();
        if (!chatHistoryStore_->lastError().isEmpty()) {
            persistentHealthy = false;
        }
    }

    chatSession_->clear();
    const auto message = chatSession_->appendSystemMessage(QStringLiteral("Sentinel Core online."),
                                                           ChatMessageStatus::Received);
    if (persistentAvailable) {
        chatHistoryStore_->appendMessage(message);
        if (!chatHistoryStore_->lastError().isEmpty()) {
            persistentHealthy = false;
        }
    }

    if (!persistentAvailable) {
        setChatMaintenanceStatus(QStringLiteral("Runtime Only"));
    } else if (persistentHealthy) {
        setChatMaintenanceStatus(QStringLiteral("Clear completed"));
    } else {
        setChatMaintenanceStatus(QStringLiteral("Clear failed"));
    }
    emit chatMessagesChanged();
    return persistentHealthy;
}

void ApplicationController::remember(const QString& key, const QString& value) {
    if (!memoryStore_ || key.trimmed().isEmpty()) {
        return;
    }

    memoryStore_->put(key.trimmed(), value.trimmed());
    emit memoryEntriesChanged();
}

void ApplicationController::setMemoryMaintenanceStatus(const QString& status) {
    if (memoryMaintenanceStatus_ == status) {
        return;
    }
    memoryMaintenanceStatus_ = status;
    emit maintenanceStatusChanged();
}

void ApplicationController::setChatMaintenanceStatus(const QString& status) {
    if (chatMaintenanceStatus_ == status) {
        return;
    }
    chatMaintenanceStatus_ = status;
    emit maintenanceStatusChanged();
}

} // namespace sentinel::core
