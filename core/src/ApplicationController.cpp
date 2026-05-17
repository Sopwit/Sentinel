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
#include <QFileInfo>

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

QString localInferenceChatFailureMessage(const LocalInferenceResponse& response) {
    switch (response.error) {
    case LocalInferenceError::BlankPrompt:
        return QStringLiteral("Local inference did not run: the prompt was blank.");
    case LocalInferenceError::MissingModel:
        return QStringLiteral("Local inference did not run: no local model is selected. Select a "
                              "discovered Ollama model in Settings or pass an explicit model.");
    case LocalInferenceError::EndpointBlocked:
        return QStringLiteral("Local inference blocked: Ollama must use local loopback HTTP.");
    case LocalInferenceError::ModelUnavailable:
        return QStringLiteral("Local inference did not run: the selected model is not available "
                              "in discovered Ollama metadata.");
    case LocalInferenceError::PermissionDenied:
        return QStringLiteral("Local inference blocked by runtime permission policy.");
    case LocalInferenceError::SafetyBlocked:
        return QStringLiteral("Local inference blocked by runtime safety policy.");
    case LocalInferenceError::ClientUnavailable:
        return QStringLiteral("Local inference unavailable: the local Ollama client is not ready.");
    case LocalInferenceError::Timeout:
        return QStringLiteral("Local inference failed: the local Ollama request timed out.");
    case LocalInferenceError::InvalidResponse:
        return QStringLiteral("Local inference failed: Ollama returned an invalid response.");
    case LocalInferenceError::RequestFailed:
        if (!response.summary.trimmed().isEmpty()) {
            return QStringLiteral("Local inference failed: %1").arg(response.summary.trimmed());
        }
        return QStringLiteral("Local inference failed before a response was produced.");
    case LocalInferenceError::None:
        break;
    }

    return safeLocalInferenceSummary(response);
}

bool hasConfiguredVoicePath(const QString& path) {
    return !path.trimmed().isEmpty();
}

QString displayedVoicePath(const QString& path) {
    return hasConfiguredVoicePath(path) ? path.trimmed() : QStringLiteral("not configured");
}

QString pathExistenceText(const QFileInfo& info) {
    return info.exists() ? QStringLiteral("exists") : QStringLiteral("missing");
}

QString pathReadabilityText(const QFileInfo& info) {
    return info.exists() && info.isReadable() ? QStringLiteral("readable")
                                              : QStringLiteral("unreadable");
}

QString pathExecutableText(const QFileInfo& info) {
    return info.exists() && info.isExecutable() ? QStringLiteral("executable")
                                                : QStringLiteral("non-executable");
}

QString configuredStatusBadge(const QString& label, const QString& path, bool requiresExecutable) {
    const auto configured = hasConfiguredVoicePath(path);
    if (!configured) {
        return QStringLiteral("%1: Missing").arg(label);
    }

    const QFileInfo info(path.trimmed());
    QStringList badges{QStringLiteral("Configured")};
    badges.append(info.exists() ? QStringLiteral("Valid") : QStringLiteral("Missing"));
    badges.append(info.exists() && info.isReadable() ? QStringLiteral("Readable")
                                                     : QStringLiteral("Unreadable"));
    if (requiresExecutable) {
        badges.append(info.exists() && info.isExecutable() ? QStringLiteral("Executable")
                                                           : QStringLiteral("Non-executable"));
    }
    return QStringLiteral("%1: %2").arg(label, badges.join(QStringLiteral(" / ")));
}

QString configuredPathValueSummary(const QString& label, const QString& path) {
    return QStringLiteral("%1 path: %2").arg(label, displayedVoicePath(path));
}

QString binaryHintSummary(const QString& label, const QString& configuredPath,
                          const QStringList& knownPaths) {
    const QFileInfo configuredInfo(configuredPath.trimmed());
    if (hasConfiguredVoicePath(configuredPath) && configuredInfo.exists() &&
        configuredInfo.isReadable() && configuredInfo.isExecutable()) {
        return QStringLiteral("%1 hint: configured path is executable; no suggestion needed.")
            .arg(label);
    }

    for (const auto& path : knownPaths) {
        const QFileInfo info(path);
        if (info.exists() && info.isReadable() && info.isExecutable()) {
            return QStringLiteral("%1 hint: %2 appears executable. Suggestion only; settings are "
                                  "not changed automatically.")
                .arg(label, path);
        }
    }

    return QStringLiteral("%1 hint: no executable found in known Homebrew/local locations.")
        .arg(label);
}

QString modelHintSummary(const QString& label, const QString& configuredPath,
                         const QString& missingSummary) {
    const QFileInfo info(configuredPath.trimmed());
    if (hasConfiguredVoicePath(configuredPath) && info.exists() && info.isReadable()) {
        return QStringLiteral("%1 hint: configured path is readable; no suggestion needed.")
            .arg(label);
    }

    return QStringLiteral("%1 hint: %2").arg(label, missingSummary);
}

VoiceBinaryDescriptor configuredVoiceBinaryDescriptor(const QString& id, const QString& name,
                                                      VoiceCapability capability,
                                                      const QString& path,
                                                      const QString& runtimeName) {
    const QFileInfo info(path.trimmed());
    const auto configured = hasConfiguredVoicePath(path);
    const auto exists = configured && info.exists();
    return VoiceBinaryDescriptor{
        id,
        name,
        capability,
        exists ? VoiceBinaryStatus::PresentMetadata : VoiceBinaryStatus::Missing,
        displayedVoicePath(path),
        exists && info.isReadable() && info.isExecutable(),
        configured
            ? QStringLiteral("%1 binary metadata only: path %2, %3, %4. Sentinel will not execute "
                             "%1 in this phase.")
                  .arg(runtimeName, pathExistenceText(info), pathReadabilityText(info),
                       pathExecutableText(info))
            : QStringLiteral("%1 binary path is not configured; Sentinel will not execute %1.")
                  .arg(runtimeName),
    };
}

VoiceModelDescriptor configuredVoiceModelDescriptor(const QString& id, const QString& name,
                                                    VoiceCapability capability, const QString& path,
                                                    const QString& runtimeName) {
    const QFileInfo info(path.trimmed());
    const auto configured = hasConfiguredVoicePath(path);
    const auto exists = configured && info.exists();
    return VoiceModelDescriptor{
        id,
        name,
        capability,
        exists ? VoiceModelStatus::PresentMetadata : VoiceModelStatus::Missing,
        displayedVoicePath(path),
        exists && info.isReadable(),
        configured
            ? QStringLiteral("%1 model metadata only: path %2, %3. Sentinel will not load or scan "
                             "%1 models in this phase.")
                  .arg(runtimeName, pathExistenceText(info), pathReadabilityText(info))
            : QStringLiteral("%1 model path is not configured or loaded.").arg(runtimeName),
    };
}

bool hasConfiguredVoicePaths(const QString& piperBinaryPath, const QString& piperModelPath,
                             const QString& whisperBinaryPath, const QString& whisperModelPath) {
    return hasConfiguredVoicePath(piperBinaryPath) || hasConfiguredVoicePath(piperModelPath) ||
           hasConfiguredVoicePath(whisperBinaryPath) || hasConfiguredVoicePath(whisperModelPath);
}

PiperTtsConfig configuredPiperTtsConfig(const QString& piperBinaryPath,
                                        const QString& piperModelPath) {
    auto config = defaultDisabledPiperTtsConfig();
    const auto binary = configuredVoiceBinaryDescriptor(
        QStringLiteral("piper-binary"), QStringLiteral("Piper Binary"),
        VoiceCapability::TextToSpeech, piperBinaryPath, QStringLiteral("Piper"));
    const auto model = configuredVoiceModelDescriptor(
        QStringLiteral("piper-voice-model"), QStringLiteral("Piper Voice Model"),
        VoiceCapability::TextToSpeech, piperModelPath, QStringLiteral("Piper"));

    config.enabled =
        hasConfiguredVoicePath(piperBinaryPath) || hasConfiguredVoicePath(piperModelPath);
    config.processExecutionAllowed = false;
    config.fileOutputAllowed = false;
    config.audioPlaybackAllowed = false;
    config.binary = binary;
    config.voiceModel = PiperVoiceModelDescriptor{
        QStringLiteral("piper-voice-model"),
        QStringLiteral("Piper Voice Model"),
        model.status,
        model.expectedPath,
        {},
        {},
        model.loadAllowed,
        model.summary,
    };
    config.safetyReport = NullVoiceRuntimeEnvironment{}.safetyReport();
    config.summary = config.enabled
                         ? QStringLiteral("Piper path configuration is stored as metadata only; "
                                          "Piper execution and playback remain blocked.")
                         : QStringLiteral("Piper TTS adapter is disabled and not configured; it "
                                          "exposes readiness metadata only.");
    return config;
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
    std::unique_ptr<ILocalInferenceClient> localInferenceClient,
    std::unique_ptr<ILocalInferenceStreamClient> localInferenceStreamClient,
    std::unique_ptr<IModelManagementService> modelManagementService,
    std::unique_ptr<ITextToSpeechProvider> textToSpeechProvider,
    std::unique_ptr<ISpeechToTextProvider> speechToTextProvider,
    std::unique_ptr<IVoiceRuntimeCoordinator> voiceRuntimeCoordinator,
    std::unique_ptr<IVoiceRuntimeEnvironment> voiceRuntimeEnvironment,
    std::unique_ptr<PiperTextToSpeechProvider> piperTextToSpeechProvider, QObject* parent)
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
      localInferenceStreamClient_(
          localInferenceStreamClient
              ? std::move(localInferenceStreamClient)
              : std::make_unique<OllamaLocalInferenceStreamClient>(ollamaRuntimeClient_->config())),
      modelManagementService_(modelManagementService
                                  ? std::move(modelManagementService)
                                  : std::make_unique<StaticModelManagementService>()),
      textToSpeechProvider_(textToSpeechProvider ? std::move(textToSpeechProvider)
                                                 : std::make_unique<NullTextToSpeechProvider>()),
      speechToTextProvider_(speechToTextProvider ? std::move(speechToTextProvider)
                                                 : std::make_unique<NullSpeechToTextProvider>()),
      voiceRuntimeCoordinator_(voiceRuntimeCoordinator
                                   ? std::move(voiceRuntimeCoordinator)
                                   : std::make_unique<StaticVoiceRuntimeCoordinator>()),
      voiceRuntimeEnvironment_(voiceRuntimeEnvironment
                                   ? std::move(voiceRuntimeEnvironment)
                                   : std::make_unique<NullVoiceRuntimeEnvironment>()),
      piperTextToSpeechProvider_(piperTextToSpeechProvider
                                     ? std::move(piperTextToSpeechProvider)
                                     : std::make_unique<PiperTextToSpeechProvider>()),
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

QStringList ApplicationController::ollamaModelNames() const {
    QStringList names;
    const auto models = currentOllamaModels();
    for (const auto& model : models) {
        names.append(model.name);
    }
    return names;
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

QString ApplicationController::selectedLocalModelStatus() const {
    const auto models = currentOllamaModels();
    const auto selected = selectedLocalModel_.trimmed();
    if (selected.isEmpty()) {
        return models.isEmpty() ? QStringLiteral("Missing") : QStringLiteral("Fallback");
    }
    if (models.isEmpty()) {
        return QStringLiteral("Unverified");
    }
    return discoveredModelNamesContain(selected, models) ? QStringLiteral("Available")
                                                         : QStringLiteral("Invalid");
}

QString ApplicationController::selectedLocalModelSummary() const {
    const auto models = currentOllamaModels();
    const auto selected = selectedLocalModel_.trimmed();
    if (selected.isEmpty()) {
        if (!models.isEmpty()) {
            return QStringLiteral("No local model selected; fallback candidate is %1 from "
                                  "discovered Ollama metadata.")
                .arg(models.first().name);
        }
        return QStringLiteral("No local model selected and no discovered Ollama models are "
                              "available; local inference requires a selected or explicit model.");
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

QString ApplicationController::selectedLocalModelMetadataSummary() const {
    const auto models = currentOllamaModels();
    const auto selected = selectedLocalModel_.trimmed();
    const auto effective = effectiveLocalModel({});

    if (effective.isEmpty()) {
        return QStringLiteral("No local model metadata available.");
    }

    for (const auto& model : models) {
        if (model.name == effective) {
            const auto prefix =
                selected.isEmpty() ? QStringLiteral("Fallback") : QStringLiteral("Selected");
            return QStringLiteral("%1 model: %2").arg(prefix, ollamaModelSummary(model));
        }
    }

    if (!selected.isEmpty() && !models.isEmpty()) {
        return QStringLiteral("Invalid selection: %1 is not in discovered local model metadata.")
            .arg(selected);
    }

    return QStringLiteral("Selected model: %1 (Local Only, discovery metadata unavailable).")
        .arg(effective);
}

QString ApplicationController::activeLocalRuntimeBadge() const {
    const auto model = effectiveLocalModel({});
    return model.isEmpty() ? QStringLiteral("Ollama Local / No Model")
                           : QStringLiteral("Ollama Local / %1").arg(model);
}

QString ApplicationController::modelManagementStatus() const {
    return modelManagementService_ ? modelManagementStatusName(modelManagementService_->status())
                                   : modelManagementStatusName(ModelManagementStatus::Unavailable);
}

QString ApplicationController::modelManagementSummary() const {
    if (!modelManagementService_) {
        return QStringLiteral("No model management readiness metadata available.");
    }
    return modelManagementService_->statusSummary(ollamaModelCount(), effectiveLocalModel({}));
}

QString ApplicationController::modelManagementActionAvailability() const {
    if (!modelManagementService_) {
        return QStringLiteral("Model management actions are unavailable.");
    }

    const auto model =
        effectiveLocalModel({}).isEmpty() ? QStringLiteral("local model") : effectiveLocalModel({});
    const QStringList summaries{
        safeModelManagementResultSummary(modelManagementService_->evaluate(
            ModelManagementRequest{ModelManagementAction::Pull, model})),
        safeModelManagementResultSummary(modelManagementService_->evaluate(
            ModelManagementRequest{ModelManagementAction::Delete, model})),
        safeModelManagementResultSummary(modelManagementService_->evaluate(
            ModelManagementRequest{ModelManagementAction::Install, model})),
    };
    return summaries.join(QStringLiteral(" "));
}

QStringList ApplicationController::modelRecommendationSummaries() const {
    return modelManagementService_ ? sentinel::core::modelRecommendationSummaries(
                                         modelManagementService_->recommendations())
                                   : QStringList{};
}

QStringList ApplicationController::modelRequirementSummaries() const {
    return modelManagementService_ ? sentinel::core::modelRequirementSummaries(
                                         modelManagementService_->requirementSummaries())
                                   : QStringList{};
}

QString ApplicationController::voiceRuntimeMode() const {
    return voiceRuntimeModeName(VoiceRuntimeMode::Disabled);
}

bool ApplicationController::voiceEnabled() const {
    return false;
}

VoiceReadinessReport ApplicationController::currentVoiceReadinessReport() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        const PiperTextToSpeechProvider piperProvider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return buildVoiceReadinessReport(piperProvider.descriptor(),
                                         speechToTextProvider_ ? speechToTextProvider_->descriptor()
                                                               : VoiceProviderDescriptor{});
    }

    return buildVoiceReadinessReport(
        textToSpeechProvider_ ? textToSpeechProvider_->descriptor() : VoiceProviderDescriptor{},
        speechToTextProvider_ ? speechToTextProvider_->descriptor() : VoiceProviderDescriptor{});
}

QString ApplicationController::voiceReadinessStatus() const {
    return currentVoiceReadinessReport().status;
}

QString ApplicationController::voiceReadinessSummary() const {
    return currentVoiceReadinessReport().summary;
}

QStringList ApplicationController::voiceReadinessChecks() const {
    return currentVoiceReadinessReport().checks;
}

QStringList ApplicationController::voiceCapabilitySummaries() const {
    QStringList summaries;
    if (textToSpeechProvider_) {
        summaries.append(voiceProviderCapabilitySummaries(textToSpeechProvider_->descriptor()));
    }
    if (speechToTextProvider_) {
        summaries.append(voiceProviderCapabilitySummaries(speechToTextProvider_->descriptor()));
    }
    return summaries;
}

QString ApplicationController::textToSpeechStatus() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        const PiperTextToSpeechProvider piperProvider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return voiceProviderStatusName(piperProvider.descriptor().status);
    }

    return textToSpeechProvider_
               ? voiceProviderStatusName(textToSpeechProvider_->descriptor().status)
               : voiceProviderStatusName(VoiceProviderStatus::Unavailable);
}

QString ApplicationController::textToSpeechSummary() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        const PiperTextToSpeechProvider piperProvider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return piperProvider.statusSummary();
    }

    return textToSpeechProvider_ ? textToSpeechProvider_->statusSummary()
                                 : QStringLiteral("No text-to-speech provider metadata.");
}

QString ApplicationController::speechToTextStatus() const {
    return speechToTextProvider_
               ? voiceProviderStatusName(speechToTextProvider_->descriptor().status)
               : voiceProviderStatusName(VoiceProviderStatus::Unavailable);
}

QString ApplicationController::speechToTextSummary() const {
    return speechToTextProvider_ ? speechToTextProvider_->statusSummary()
                                 : QStringLiteral("No speech-to-text provider metadata.");
}

VoiceSession ApplicationController::currentVoiceSession() const {
    return voiceRuntimeCoordinator_ ? voiceRuntimeCoordinator_->currentSession() : VoiceSession{};
}

VoicePipelineResult ApplicationController::currentVoicePipelineResult() const {
    return voiceRuntimeCoordinator_
               ? voiceRuntimeCoordinator_->evaluate(VoiceSessionState::Completed)
               : VoicePipelineResult{};
}

VoiceRuntimeSummary ApplicationController::currentVoiceRuntimeSummary() const {
    return voiceRuntimeCoordinator_ ? voiceRuntimeCoordinator_->runtimeSummary()
                                    : VoiceRuntimeSummary{};
}

QString ApplicationController::voiceSessionId() const {
    return currentVoiceSession().id.value;
}

QString ApplicationController::voiceSessionStatus() const {
    return voiceSessionStateName(currentVoicePipelineResult().session.state);
}

QString ApplicationController::voiceSessionSummary() const {
    return currentVoicePipelineResult().session.summary;
}

QString ApplicationController::voicePipelineStatus() const {
    return voicePipelineStatusName(currentVoicePipelineResult().status);
}

QString ApplicationController::voicePipelineSummary() const {
    return safeVoicePipelineSummary(currentVoicePipelineResult());
}

QStringList ApplicationController::voicePipelineTraceSummaries() const {
    return sentinel::core::voicePipelineTraceSummaries(currentVoicePipelineResult().traces);
}

QString ApplicationController::voiceRuntimeStatus() const {
    return currentVoiceRuntimeSummary().status;
}

QString ApplicationController::voiceRuntimeSummary() const {
    return voiceRuntimeSummaryText(currentVoiceRuntimeSummary());
}

QStringList ApplicationController::voiceRuntimeCheckSummaries() const {
    return sentinel::core::voiceRuntimeCheckSummaries(currentVoiceRuntimeSummary());
}

bool ApplicationController::voiceRuntimeAvailable() const {
    return currentVoiceRuntimeSummary().runtimeAvailable;
}

bool ApplicationController::voiceTextToSpeechAvailable() const {
    return currentVoiceRuntimeSummary().textToSpeechAvailable;
}

bool ApplicationController::voiceSpeechToTextAvailable() const {
    return currentVoiceRuntimeSummary().speechToTextAvailable;
}

bool ApplicationController::voiceMicrophoneEnabled() const {
    return currentVoiceRuntimeSummary().microphoneEnabled;
}

bool ApplicationController::voicePlaybackEnabled() const {
    return currentVoiceRuntimeSummary().playbackEnabled;
}

bool ApplicationController::voiceLocalOnlyPolicy() const {
    return currentVoiceRuntimeSummary().localOnlyPolicy;
}

bool ApplicationController::voiceProcessExecutionEnabled() const {
    return currentVoiceRuntimeSummary().processExecutionEnabled;
}

QString ApplicationController::voiceRuntimeEnvironmentStatus() const {
    if (hasConfiguredVoicePaths(piperBinaryPath_, piperModelPath_, whisperBinaryPath_,
                                whisperModelPath_)) {
        return QStringLiteral("Configured Metadata");
    }

    return voiceRuntimeEnvironment_ ? voiceRuntimeEnvironment_->status()
                                    : QStringLiteral("Blocked");
}

QString ApplicationController::voiceRuntimeEnvironmentSummary() const {
    if (hasConfiguredVoicePaths(piperBinaryPath_, piperModelPath_, whisperBinaryPath_,
                                whisperModelPath_)) {
        return QStringLiteral("Voice configuration stores Piper and Whisper paths as metadata "
                              "only. Exact configured paths are checked for exists/readable and "
                              "binary executable state without running Piper, running Whisper, "
                              "opening audio devices, downloading models, or scanning the "
                              "filesystem.");
    }

    return voiceRuntimeEnvironment_
               ? voiceRuntimeEnvironment_->summary()
               : QStringLiteral("No voice runtime environment metadata available.");
}

QStringList ApplicationController::voiceBinarySummaries() const {
    if (hasConfiguredVoicePaths(piperBinaryPath_, piperModelPath_, whisperBinaryPath_,
                                whisperModelPath_)) {
        return voiceBinaryDescriptorSummaries({
            configuredVoiceBinaryDescriptor(
                QStringLiteral("piper-binary"), QStringLiteral("Piper Binary"),
                VoiceCapability::TextToSpeech, piperBinaryPath_, QStringLiteral("Piper")),
            configuredVoiceBinaryDescriptor(
                QStringLiteral("whisper-binary"), QStringLiteral("Whisper Binary"),
                VoiceCapability::SpeechToText, whisperBinaryPath_, QStringLiteral("Whisper")),
        });
    }

    return voiceRuntimeEnvironment_
               ? voiceBinaryDescriptorSummaries(voiceRuntimeEnvironment_->binaries())
               : QStringList{};
}

QStringList ApplicationController::voiceModelSummaries() const {
    if (hasConfiguredVoicePaths(piperBinaryPath_, piperModelPath_, whisperBinaryPath_,
                                whisperModelPath_)) {
        return voiceModelDescriptorSummaries({
            configuredVoiceModelDescriptor(
                QStringLiteral("piper-voice-model"), QStringLiteral("Piper Voice Model"),
                VoiceCapability::TextToSpeech, piperModelPath_, QStringLiteral("Piper")),
            configuredVoiceModelDescriptor(
                QStringLiteral("whisper-model"), QStringLiteral("Whisper Model"),
                VoiceCapability::SpeechToText, whisperModelPath_, QStringLiteral("Whisper")),
        });
    }

    return voiceRuntimeEnvironment_
               ? voiceModelDescriptorSummaries(voiceRuntimeEnvironment_->models())
               : QStringList{};
}

QStringList ApplicationController::voiceRuntimePermissionSummaries() const {
    return voiceRuntimeEnvironment_ ? sentinel::core::voiceRuntimePermissionSummaries(
                                          voiceRuntimeEnvironment_->permissions())
                                    : QStringList{};
}

QString ApplicationController::voiceRuntimeSafetyStatus() const {
    return voiceRuntimeEnvironment_ ? voiceRuntimeEnvironment_->safetyReport().status
                                    : QStringLiteral("Blocked");
}

QString ApplicationController::voiceRuntimeSafetySummary() const {
    return voiceRuntimeEnvironment_
               ? voiceRuntimeSafetySummaryText(voiceRuntimeEnvironment_->safetyReport())
               : QStringLiteral("Voice runtime safety metadata is unavailable.");
}

QStringList ApplicationController::voiceRuntimeSafetyChecks() const {
    return voiceRuntimeEnvironment_
               ? voiceRuntimeSafetyCheckSummaries(voiceRuntimeEnvironment_->safetyReport())
               : QStringList{};
}

bool ApplicationController::voiceRuntimeExecutionAllowed() const {
    return voiceRuntimeEnvironment_ && voiceRuntimeEnvironment_->safetyReport().executionAllowed;
}

QString ApplicationController::piperTtsStatus() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return piperTtsStatusName(provider.status());
    }

    return piperTextToSpeechProvider_ ? piperTtsStatusName(piperTextToSpeechProvider_->status())
                                      : piperTtsStatusName(PiperTtsStatus::Disabled);
}

QString ApplicationController::piperTtsSummary() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return provider.piperStatusSummary();
    }

    return piperTextToSpeechProvider_ ? piperTextToSpeechProvider_->piperStatusSummary()
                                      : QStringLiteral("No Piper TTS provider metadata available.");
}

QStringList ApplicationController::piperTtsReadinessChecks() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return provider.readinessChecks();
    }

    return piperTextToSpeechProvider_ ? piperTextToSpeechProvider_->readinessChecks()
                                      : QStringList{};
}

bool ApplicationController::piperTtsReady() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return provider.status() == PiperTtsStatus::Configured;
    }

    return piperTextToSpeechProvider_ &&
           piperTextToSpeechProvider_->status() == PiperTtsStatus::Configured;
}

QString ApplicationController::piperTtsFileOutputStatus() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return provider.fileOutputStatus();
    }

    return piperTextToSpeechProvider_ ? piperTextToSpeechProvider_->fileOutputStatus()
                                      : piperTtsStatusName(PiperTtsStatus::Disabled);
}

QString ApplicationController::piperTtsFileOutputSummary() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return provider.fileOutputSummary();
    }

    return piperTextToSpeechProvider_
               ? piperTextToSpeechProvider_->fileOutputSummary()
               : QStringLiteral("No Piper TTS file-output metadata available.");
}

QString ApplicationController::piperBinaryPath() const {
    return piperBinaryPath_;
}

void ApplicationController::setPiperBinaryPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == piperBinaryPath_) {
        return;
    }

    piperBinaryPath_ = normalized;
    emit voiceConfigurationChanged();
}

QString ApplicationController::piperModelPath() const {
    return piperModelPath_;
}

void ApplicationController::setPiperModelPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == piperModelPath_) {
        return;
    }

    piperModelPath_ = normalized;
    emit voiceConfigurationChanged();
}

QString ApplicationController::whisperBinaryPath() const {
    return whisperBinaryPath_;
}

void ApplicationController::setWhisperBinaryPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == whisperBinaryPath_) {
        return;
    }

    whisperBinaryPath_ = normalized;
    emit voiceConfigurationChanged();
}

QString ApplicationController::whisperModelPath() const {
    return whisperModelPath_;
}

void ApplicationController::setWhisperModelPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == whisperModelPath_) {
        return;
    }

    whisperModelPath_ = normalized;
    emit voiceConfigurationChanged();
}

QStringList ApplicationController::voiceConfigurationSummaries() const {
    return voiceBinarySummaries() + voiceModelSummaries();
}

QString ApplicationController::voiceConfigurationReadinessSummary() const {
    const auto piperBinaryConfigured = hasConfiguredVoicePath(piperBinaryPath_);
    const auto piperModelConfigured = hasConfiguredVoicePath(piperModelPath_);
    const auto whisperBinaryConfigured = hasConfiguredVoicePath(whisperBinaryPath_);
    const auto whisperModelConfigured = hasConfiguredVoicePath(whisperModelPath_);
    return QStringLiteral("Piper binary %1, Piper model %2, Whisper binary %3, Whisper model %4. "
                          "Configuration is metadata-only; Piper and Whisper are not executed.")
        .arg(piperBinaryConfigured ? QStringLiteral("configured") : QStringLiteral("missing"),
             piperModelConfigured ? QStringLiteral("configured") : QStringLiteral("missing"),
             whisperBinaryConfigured ? QStringLiteral("configured") : QStringLiteral("missing"),
             whisperModelConfigured ? QStringLiteral("configured") : QStringLiteral("missing"));
}

QStringList ApplicationController::voiceConfigurationStatusBadges() const {
    return {
        configuredStatusBadge(QStringLiteral("Piper binary"), piperBinaryPath_, true),
        configuredStatusBadge(QStringLiteral("Piper model"), piperModelPath_, false),
        configuredStatusBadge(QStringLiteral("Whisper binary"), whisperBinaryPath_, true),
        configuredStatusBadge(QStringLiteral("Whisper model"), whisperModelPath_, false),
    };
}

QStringList ApplicationController::voiceConfigurationHintSummaries() const {
    const QStringList piperBinaryKnownPaths{
        QStringLiteral("/opt/homebrew/bin/piper"),
        QStringLiteral("/usr/local/bin/piper"),
    };
    const QStringList whisperBinaryKnownPaths{
        QStringLiteral("/opt/homebrew/bin/whisper"),
        QStringLiteral("/usr/local/bin/whisper"),
    };

    return {
        configuredPathValueSummary(QStringLiteral("Piper binary"), piperBinaryPath_),
        binaryHintSummary(QStringLiteral("Piper binary"), piperBinaryPath_, piperBinaryKnownPaths),
        modelHintSummary(QStringLiteral("Piper model"), piperModelPath_,
                         QStringLiteral("configure a local Piper model path to validate it.")),
        configuredPathValueSummary(QStringLiteral("Whisper binary"), whisperBinaryPath_),
        binaryHintSummary(QStringLiteral("Whisper binary"), whisperBinaryPath_,
                          whisperBinaryKnownPaths),
        modelHintSummary(
            QStringLiteral("Whisper model"), whisperModelPath_,
            QStringLiteral(
                "only the configured path is checked; no model directories are scanned.")),
        QStringLiteral("Auto-detection is hint-only and checks only fixed known binary locations "
                       "plus configured paths. Piper and Whisper are not executed."),
    };
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
        return QStringLiteral("Local chat inference is disabled; chat stays on the local safe "
                              "provider path and no Ollama prompt is sent.");
    }
    if (!localInferenceEndpointAllowed()) {
        return QStringLiteral("Local chat inference is blocked: Ollama endpoint must be local "
                              "loopback HTTP.");
    }
    if (effectiveLocalModel({}).isEmpty()) {
        return QStringLiteral("Local chat inference is enabled but no local model is selected or "
                              "available from discovery metadata.");
    }
    if (localChatInferenceStatus() == QStringLiteral("Invalid Model")) {
        return QStringLiteral("Local chat inference is enabled but the selected model is missing "
                              "from discovered Ollama metadata.");
    }
    return QStringLiteral("Local chat inference is enabled for guarded local-only Ollama routing.");
}

bool ApplicationController::localInferenceStreamingEnabled() const {
    return localInferenceStreamingEnabled_;
}

void ApplicationController::setLocalInferenceStreamingEnabled(bool enabled) {
    if (enabled == localInferenceStreamingEnabled_) {
        return;
    }

    localInferenceStreamingEnabled_ = enabled;
    if (!enabled) {
        latestLocalInferenceStreamResult_ = LocalInferenceStreamResult{};
    }
    emit localInferenceChanged();
}

bool ApplicationController::localInferenceBusy() const {
    return localInferenceBusy_;
}

QString ApplicationController::localInferenceRuntimeState() const {
    if (localInferenceBusy_) {
        return latestLocalInferenceStreamResult_.status == LocalInferenceStreamStatus::Streaming
                   ? QStringLiteral("Streaming")
                   : QStringLiteral("Inferencing");
    }
    if (latestLocalInferenceResponse_.status == LocalInferenceStatus::Error ||
        latestLocalInferenceResponse_.status == LocalInferenceStatus::Refused ||
        latestLocalInferenceResponse_.status == LocalInferenceStatus::Blocked ||
        latestLocalInferenceResponse_.status == LocalInferenceStatus::InvalidRequest ||
        latestLocalInferenceResponse_.status == LocalInferenceStatus::ModelUnavailable) {
        return QStringLiteral("Failed");
    }
    if (latestLocalInferenceResponse_.status == LocalInferenceStatus::NotRequested &&
        currentOllamaHealthCheck().healthStatus != OllamaHealthStatus::Healthy) {
        return QStringLiteral("Unavailable");
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
    return localInferenceStreamingEnabled_ && localInferenceStreamClient_ &&
           localInferenceStreamClient_->isAvailable();
}

QString ApplicationController::localInferenceStreamStatus() const {
    return localInferenceStreamStatusName(latestLocalInferenceStreamResult_.status);
}

QString ApplicationController::localInferenceStreamSummary() const {
    if (!localInferenceStreamingEnabled_) {
        return QStringLiteral("Local inference streaming is disabled; responses finalize through "
                              "normal chat history.");
    }
    if (!localInferenceStreamClient_) {
        return QStringLiteral("Local inference streaming client is unavailable.");
    }
    if (localInferenceBusy_ &&
        latestLocalInferenceStreamResult_.status == LocalInferenceStreamStatus::Streaming) {
        return latestLocalInferenceStreamResult_.summary.isEmpty()
                   ? QStringLiteral("Local streaming response is active.")
                   : latestLocalInferenceStreamResult_.summary;
    }
    if (latestLocalInferenceStreamResult_.status == LocalInferenceStreamStatus::Disabled) {
        return localInferenceStreamClient_->statusSummary();
    }
    return latestLocalInferenceStreamResult_.summary;
}

QString ApplicationController::localInferenceStreamingText() const {
    if (!localInferenceBusy_ ||
        latestLocalInferenceStreamResult_.status != LocalInferenceStreamStatus::Streaming) {
        return {};
    }
    return latestLocalInferenceStreamResult_.accumulatedText;
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

        const auto ranLocalInference = localInferenceStreamingAvailable()
                                           ? runLocalInferenceStream(trimmed, {})
                                           : runLocalInference(trimmed, {});
        const auto localStatus = latestLocalInferenceResponse_.status;
        const auto assistantMessage = chatSession_->appendAssistantMessage(
            ranLocalInference ? latestLocalInferenceResponse_.text
                              : localInferenceChatFailureMessage(latestLocalInferenceResponse_),
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

bool ApplicationController::runLocalInferenceStream(const QString& prompt, const QString& model) {
    LocalInferenceRequest request;
    request.prompt = prompt.trimmed();
    request.options.model = effectiveLocalModel(model);
    request.options.streamingRequested = true;

    auto blockStream = [this,
                        &request](LocalInferenceError error, const QString& summary,
                                  LocalInferenceStatus status = LocalInferenceStatus::Blocked) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(request, error, summary);
        latestLocalInferenceResponse_.status = status;
        latestLocalInferenceStreamResult_ = LocalInferenceStreamResult{};
        latestLocalInferenceStreamResult_.status = LocalInferenceStreamStatus::Refused;
        latestLocalInferenceStreamResult_.error = error;
        latestLocalInferenceStreamResult_.summary = summary;
        emit localInferenceChanged();
        return false;
    };

    if (!localInferenceStreamingEnabled_) {
        return blockStream(LocalInferenceError::RequestFailed,
                           QStringLiteral("Local inference streaming is disabled; using "
                                          "non-streaming local inference when chat routing allows "
                                          "it."),
                           LocalInferenceStatus::Refused);
    }

    if (request.prompt.isEmpty()) {
        return blockStream(LocalInferenceError::BlankPrompt,
                           QStringLiteral("Local streaming request rejected: prompt is blank."),
                           LocalInferenceStatus::InvalidRequest);
    }

    if (request.options.model.isEmpty()) {
        return blockStream(LocalInferenceError::MissingModel,
                           QStringLiteral("Local streaming request rejected: model is required."),
                           LocalInferenceStatus::InvalidRequest);
    }

    if (!localInferenceEndpointAllowed()) {
        return blockStream(LocalInferenceError::EndpointBlocked,
                           QStringLiteral("Local streaming blocked: endpoint must be local "
                                          "loopback HTTP."));
    }

    const auto discoveredModels = currentOllamaModels();
    if (!discoveredModels.isEmpty() &&
        !discoveredModelNamesContain(request.options.model, discoveredModels)) {
        return blockStream(LocalInferenceError::ModelUnavailable,
                           QStringLiteral("Local streaming request rejected: selected model is "
                                          "not installed."),
                           LocalInferenceStatus::ModelUnavailable);
    }

    const auto permissionDecision = currentRuntimePermissionDecision();
    if (permissionDecision.status != RuntimePermissionDecisionStatus::Allowed) {
        const auto blocked =
            blockStream(LocalInferenceError::PermissionDenied,
                        QStringLiteral("Local streaming blocked by runtime permission policy."));
        latestLocalInferenceResponse_.traces.append(LocalInferenceTrace{
            2,
            QStringLiteral("Permission Policy"),
            runtimePermissionDecisionStatusName(permissionDecision.status),
            safeRuntimePermissionDecisionSummary(permissionDecision),
        });
        emit localInferenceChanged();
        return blocked;
    }

    const auto safetyReport = currentRuntimeSafetyReport();
    if (safetyReport.decision != RuntimeSafetyDecision::Compliant) {
        const auto blocked =
            blockStream(LocalInferenceError::SafetyBlocked,
                        QStringLiteral("Local streaming blocked by runtime safety policy."));
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
        return blocked;
    }

    if (!localInferenceStreamClient_ || !localInferenceStreamClient_->isAvailable()) {
        return blockStream(LocalInferenceError::ClientUnavailable,
                           QStringLiteral("Local streaming blocked: client is unavailable."));
    }

    localInferenceBusy_ = true;
    latestLocalInferenceResponse_.status = LocalInferenceStatus::Busy;
    latestLocalInferenceResponse_.model = request.options.model;
    latestLocalInferenceResponse_.text.clear();
    latestLocalInferenceResponse_.summary = QStringLiteral("Local streaming request is running.");
    latestLocalInferenceStreamResult_ = LocalInferenceStreamResult{};
    latestLocalInferenceStreamResult_.status = LocalInferenceStreamStatus::Streaming;
    latestLocalInferenceStreamResult_.model = request.options.model;
    latestLocalInferenceStreamResult_.summary =
        QStringLiteral("Local streaming request is running.");
    emit localInferenceChanged();

    QElapsedTimer latencyTimer;
    latencyTimer.start();
    auto streamResult = localInferenceStreamClient_->startStream(
        request, [this](const LocalInferenceStreamChunk& chunk) {
            latestLocalInferenceStreamResult_.chunks.append(chunk);
            if (!chunk.malformed) {
                latestLocalInferenceStreamResult_.accumulatedText.append(chunk.text);
                latestLocalInferenceResponse_.text =
                    latestLocalInferenceStreamResult_.accumulatedText;
            }
            latestLocalInferenceStreamResult_.summary = chunk.summary;
            emit localInferenceChanged();
        });

    localInferenceBusy_ = false;
    latestLocalInferenceStreamResult_ = streamResult;
    latestLocalInferenceStreamResult_.accumulatedText.clear();
    latestLocalInferenceResponse_.latencyMs = latencyTimer.elapsed();
    latestLocalInferenceResponse_.model = streamResult.model;
    latestLocalInferenceResponse_.endpoint = streamResult.endpoint;
    latestLocalInferenceResponse_.text = streamResult.accumulatedText;
    latestLocalInferenceResponse_.error = streamResult.error;
    latestLocalInferenceResponse_.summary = streamResult.summary;
    latestLocalInferenceResponse_.status =
        streamResult.status == LocalInferenceStreamStatus::Completed
            ? LocalInferenceStatus::Succeeded
        : streamResult.status == LocalInferenceStreamStatus::Refused ? LocalInferenceStatus::Refused
        : streamResult.status == LocalInferenceStreamStatus::Cancelled
            ? LocalInferenceStatus::Blocked
            : LocalInferenceStatus::Error;
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
    latestLocalInferenceResponse_.traces.append(streamResult.traces);
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
