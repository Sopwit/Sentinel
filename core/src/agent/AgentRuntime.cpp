// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/AgentRuntime.h"
#include "sentinel/core/agent/AgentLoop.h"
#include "sentinel/core/agent/LlmAgentRuntime.h"
#include "sentinel/core/chat/IChatHistoryStore.h"
#include "sentinel/core/interfaces/IMemoryStore.h"
#include "sentinel/core/mcp/McpToolProvider.h"
#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/security/IApprovalPolicy.h"
#include "sentinel/core/security/ISandboxPolicy.h"

#include <QTimer>
#include <QUuid>

namespace sentinel::core {

namespace {
class ExecutorCompatibilityHandler final : public IToolHandler {
public:
    explicit ExecutorCompatibilityHandler(IToolExecutor& executor) : executor_(executor) {}
    IToolExecutor::Cancel execute(const ToolExecutionRequest& request, const QString& sessionId,
                                  const QString& toolCallId, IToolExecutor::Output output,
                                  IToolExecutor::Completion completion) override {
        return executor_.executeAsync(request, sessionId, toolCallId, std::move(output),
                                      std::move(completion));
    }

private:
    IToolExecutor& executor_;
};
} // namespace

AgentRuntime::AgentRuntime(std::unique_ptr<IAgentRuntime> metadata, IAgentStepPlanner* planner,
                           IToolExecutor& executor, const IApprovalPolicy& approval,
                           const ISandboxPolicy& sandbox, const IMemoryStore* memoryStore,
                           const IChatHistoryStore* chatHistoryStore)
    : metadata_(std::move(metadata)), planner_(planner), executor_(executor), approval_(approval),
      sandbox_(sandbox), memoryStore_(memoryStore), chatHistoryStore_(chatHistoryStore) {
    if (auto* nativeExecutor = dynamic_cast<RealToolExecutor*>(&executor_)) {
        nativeExecutor->setExternalDirectoryGate(&externalDirectoryGate_);
        BuiltInToolProvider::registerTools(toolRegistry_, *nativeExecutor);
    } else {
        auto compatibility = std::make_shared<ExecutorCompatibilityHandler>(executor_);
        for (auto descriptor : metadata_->availableTools())
            toolRegistry_.registerTool({std::move(descriptor), compatibility});
    }
    if (auto* llm = dynamic_cast<LlmAgentRuntime*>(planner_))
        llm->setToolRegistry(&toolRegistry_);
    if (auto* nativeExecutor = dynamic_cast<RealToolExecutor*>(&executor_))
        setMcpService(nativeExecutor->mcpService());
    pluginManager_.setToolRegistry(&toolRegistry_);
    if (dynamic_cast<RealToolExecutor*>(&executor_)) {
        pluginManager_.discoverPlugins({});
        pluginManager_.startAll();
    }
}

void AgentRuntime::setMcpService(std::shared_ptr<IMcpService> service) {
    mcpToolProvider_.reset();
    if (service)
        mcpToolProvider_ = std::make_unique<McpToolProvider>(std::move(service), toolRegistry_);
}

AgentRuntime::~AgentRuntime() {
    shutdown();
    pluginManager_.unloadAll();
}

QString AgentRuntime::name() const {
    return metadata_->name();
}
AgentStatus AgentRuntime::status() const {
    return metadata_->status();
}
QList<AgentCapabilityDescriptor> AgentRuntime::capabilities() const {
    return metadata_->capabilities();
}
QList<ToolDescriptor> AgentRuntime::availableTools() const {
    return toolRegistry_.enabledTools();
}
ToolInvocationPlan AgentRuntime::plan(const AgentRequest& request) const {
    return metadata_->plan(request);
}
AgentResponse AgentRuntime::execute(const AgentRequest& request) {
    const auto result = executePipeline(request, false);
    return {result.execution.status == ToolExecutionStatus::Succeeded, result.summary, status()};
}

bool AgentRuntime::supportsSessions() const {
    return planner_ != nullptr;
}

QString AgentRuntime::subscribe(AgentEventCallback callback) {
    if (!callback)
        return {};
    const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    std::lock_guard lock(eventMutex_);
    auto subscription = std::make_shared<Subscription>();
    subscription->callback = std::move(callback);
    subscribers_.insert(id, std::move(subscription));
    return id;
}

void AgentRuntime::unsubscribe(const QString& id) {
    std::shared_ptr<Subscription> subscription;
    {
        std::lock_guard lock(eventMutex_);
        subscription = subscribers_.take(id);
    }
    if (!subscription)
        return;
    std::unique_lock lock(subscription->mutex);
    subscription->active = false;
    if (subscription->callbackThread != std::this_thread::get_id()) {
        subscription->idle.wait(lock, [&] { return !subscription->inFlight; });
    }
}

QList<AgentEvent> AgentRuntime::eventHistory(const QString& sessionId) const {
    std::lock_guard lock(eventMutex_);
    return history_.value(sessionId);
}

void AgentRuntime::publish(const QString& sessionId, AgentEventType type, AgentEventPayload payload,
                           int stepIndex, bool toolCall) {
    if (cancelRequested_ &&
        (type == AgentEventType::ModelRequestStarted || type == AgentEventType::ModelOutputDelta ||
         type == AgentEventType::ModelRequestCompleted || type == AgentEventType::ToolRequested ||
         type == AgentEventType::ToolApprovalRequired ||
         type == AgentEventType::ToolApprovalResolved ||
         type == AgentEventType::ToolExecutionStarted || type == AgentEventType::ToolOutput ||
         type == AgentEventType::ToolExecutionCompleted ||
         type == AgentEventType::ToolExecutionFailed || type == AgentEventType::AgentStepCompleted))
        return;
    AgentEvent event;
    event.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    event.sessionId = sessionId;
    event.type = type;
    if (type == AgentEventType::ModelRequestStarted ||
        type == AgentEventType::ModelRequestCompleted) {
        if (const auto* llm = dynamic_cast<const LlmAgentRuntime*>(planner_)) {
            const auto binding = llm->modelBinding();
            event.providerId = binding.providerId;
            event.modelId = binding.modelId;
        }
    }
    event.timestamp = QDateTime::currentDateTimeUtc();
    event.payload = std::move(payload);
    {
        std::lock_guard lock(eventMutex_);
        auto& turn = turns_[sessionId];
        if (turn.terminal && type != AgentEventType::AgentCompleted &&
            type != AgentEventType::AgentFailed && type != AgentEventType::AgentCancelled &&
            type != AgentEventType::RuntimeStateChanged)
            return;
        event.turnId = turn.id;
        if (stepIndex > 0) {
            auto& stepId = turn.stepIds[stepIndex];
            if (stepId.isEmpty())
                stepId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            event.stepId = stepId;
            if (toolCall) {
                auto& callId = turn.toolCallIds[stepIndex];
                if (callId.isEmpty())
                    callId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                event.toolCallId = callId;
            }
        }
        auto& events = history_[sessionId];
        events.append(event);
        if (events.size() > 256)
            events.remove(0, events.size() - 256);
        pendingEvents_.append(event);
        if (dispatching_)
            return;
        dispatching_ = true;
    }
    while (true) {
        AgentEvent next;
        QList<std::shared_ptr<Subscription>> listeners;
        {
            std::lock_guard lock(eventMutex_);
            if (pendingEvents_.isEmpty()) {
                dispatching_ = false;
                break;
            }
            next = pendingEvents_.takeFirst();
            listeners = subscribers_.values();
        }
        for (const auto& listener : listeners) {
            {
                std::lock_guard lock(listener->mutex);
                if (!listener->active)
                    continue;
                listener->inFlight = true;
                listener->callbackThread = std::this_thread::get_id();
            }
            try {
                listener->callback(next);
            } catch (...) {
                // Subscriber failures must not stop delivery to other consumers.
            }
            {
                std::lock_guard lock(listener->mutex);
                listener->inFlight = false;
                listener->callbackThread = {};
            }
            listener->idle.notify_all();
        }
    }
}

void AgentRuntime::beginTurn(const QString& sessionId) {
    std::lock_guard lock(eventMutex_);
    turns_[sessionId] =
        TurnContext{QUuid::createUuid().toString(QUuid::WithoutBraces), {}, {}, false};
}

void AgentRuntime::finishTurn(const QString& sessionId, const AgentLoopState& state) {
    AgentRunEvent run;
    run.phase = state.phase;
    run.finalAnswer = state.finalAnswer;
    run.abortReason = state.abortReason;
    run.completedSteps = static_cast<int>(state.steps.size());
    run.pendingThought = state.pendingApprovalThought;
    if (!state.pendingApprovalPlan.invocations.isEmpty()) {
        const auto& invocation = state.pendingApprovalPlan.invocations.first();
        run.pendingTool = {invocation.toolId, invocation.arguments, invocation.riskLevel, {}};
    }
    if (state.phase == AgentLoopPhase::AwaitingApproval) {
        publish(sessionId, AgentEventType::RuntimeStateChanged, run);
        return;
    }
    AgentEventType type;
    if (state.phase == AgentLoopPhase::Completed)
        type = AgentEventType::AgentCompleted;
    else if (state.phase == AgentLoopPhase::Cancelled)
        type = AgentEventType::AgentCancelled;
    else
        type = AgentEventType::AgentFailed;
    {
        std::lock_guard lock(eventMutex_);
        auto& turn = turns_[sessionId];
        if (turn.terminal)
            return;
        turn.terminal = true;
    }
    publish(sessionId, type,
            AgentTextEvent{state.phase == AgentLoopPhase::Completed ? state.finalAnswer
                                                                    : state.abortReason});
    publish(sessionId, AgentEventType::RuntimeStateChanged, run);
}

QString AgentRuntime::createSession() {
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    AgentLoopState state;
    state.sessionId = id;
    {
        std::lock_guard lock(mutex_);
        sessions_.insert(id, state);
    }
    publish(id, AgentEventType::SessionCreated);
    return id;
}

AgentLoopState AgentRuntime::submit(const QString& sessionId, const QString& goal) {
    // Compatibility API for direct synchronous callers. Desktop sessions use
    // start(), whose AgentLoop continuation runs on the Qt worker event loop.
    return advance(sessionId, false, false, goal);
}

AgentLoopState AgentRuntime::resume(const QString& sessionId, bool approved) {
    return advance(sessionId, true, approved, {});
}

bool AgentRuntime::approve(const QString& sessionId, bool alwaysAllow) {
    std::lock_guard lock(mutex_);
    const auto state = sessions_.value(sessionId);
    if (state.phase != AgentLoopPhase::AwaitingApproval || !activeSessionId_.isEmpty()) {
        return false;
    }
    if (alwaysAllow) {
        for (const auto& invocation : state.pendingApprovalPlan.invocations) {
            if (!approvedToolIds_.contains(invocation.toolId)) {
                approvedToolIds_.append(invocation.toolId);
            }
        }
    }
    return true;
}

bool AgentRuntime::cancel(const QString& sessionId) {
    bool waitingForApproval = false;
    {
        std::lock_guard lock(mutex_);
        if (activeSessionId_ != sessionId) {
            if (!activeSessionId_.isEmpty() ||
                sessions_.value(sessionId).phase != AgentLoopPhase::AwaitingApproval)
                return false;
            waitingForApproval = true;
        } else if (cancelRequested_)
            return false;
        cancelRequested_ = true;
        modelCancellationToken_->store(true);
        cancellationEventPending_ = true;
        sessions_[sessionId].phase = AgentLoopPhase::Cancelling;
        if (workerContext_)
            QMetaObject::invokeMethod(
                workerContext_,
                [this] {
                    AgentLoop* loop = nullptr;
                    {
                        std::lock_guard lock(mutex_);
                        loop = activeLoop_;
                    }
                    if (loop)
                        loop->cancelAsync();
                },
                Qt::QueuedConnection);
    }
    publish(sessionId, AgentEventType::RuntimeStateChanged,
            AgentStateEvent{AgentLoopPhase::Cancelling});
    {
        std::lock_guard lock(mutex_);
        cancellationEventPending_ = false;
    }
    cancellationPublished_.notify_all();
    if (waitingForApproval) {
        AgentLoopState finalState;
        {
            std::lock_guard lock(mutex_);
            finalState = sessions_.value(sessionId);
            finalState.phase = AgentLoopPhase::Cancelled;
            finalState.abortReason = QStringLiteral("Agent run cancelled by user.");
            sessions_[sessionId] = finalState;
            errors_[sessionId] = {AgentRuntimeErrorCode::Cancelled, finalState.abortReason, false};
        }
        externalDirectoryGate_.clearPermissions();
        finishTurn(sessionId, finalState);
    }
    return true;
}

AgentLoopState AgentRuntime::sessionState(const QString& sessionId) const {
    std::lock_guard lock(mutex_);
    return sessions_.value(sessionId);
}

AgentRuntimeError AgentRuntime::error(const QString& sessionId) const {
    std::lock_guard lock(mutex_);
    return errors_.value(sessionId);
}

void AgentRuntime::configureSession(const QString& sessionId, AgentSessionOptions options) {
    std::lock_guard lock(mutex_);
    if (sessions_.contains(sessionId) && activeSessionId_.isEmpty()) {
        options_.insert(sessionId, std::move(options));
    }
}

bool AgentRuntime::start(const QString& sessionId, const QString& goal) {
    return launch(sessionId, false, false, goal);
}

bool AgentRuntime::continueSession(const QString& sessionId, bool approved) {
    return launch(sessionId, true, approved, {});
}

bool AgentRuntime::launch(const QString& sessionId, bool isResume, bool approved,
                          const QString& goal) {
    std::lock_guard workerLock(workerMutex_);
    {
        std::lock_guard lock(mutex_);
        if (shuttingDown_ || !planner_ || !activeSessionId_.isEmpty() ||
            !sessions_.contains(sessionId)) {
            if (sessions_.contains(sessionId) && activeSessionId_.isEmpty()) {
                auto& state = sessions_[sessionId];
                state.phase = AgentLoopPhase::Failed;
                state.abortReason = QStringLiteral("Agent runtime could not start.");
                errors_[sessionId] = {AgentRuntimeErrorCode::ExecutionFailed, state.abortReason,
                                      false};
            }
            return false;
        }
        const auto phase = sessions_.value(sessionId).phase;
        if (isResume ? phase != AgentLoopPhase::AwaitingApproval : phase != AgentLoopPhase::Idle) {
            auto& state = sessions_[sessionId];
            state.phase = AgentLoopPhase::Failed;
            state.abortReason = QStringLiteral("Agent session is in the wrong state.");
            errors_[sessionId] = {AgentRuntimeErrorCode::InvalidState, state.abortReason, false};
            return false;
        }
        activeSessionId_ = sessionId;
        sessions_[sessionId].phase = AgentLoopPhase::Running;
        cancelRequested_ = false;
        if (!isResume)
            modelCancellationToken_ = std::make_shared<std::atomic_bool>(false);
    }
    if (worker_) {
        worker_->wait();
        delete worker_;
        worker_ = nullptr;
    }
    if (!isResume) {
        externalDirectoryGate_.clearPermissions();
        beginTurn(sessionId);
    }
    if (isResume) {
        const auto state = sessionState(sessionId);
        const int index = static_cast<int>(state.steps.size()) + 1;
        publish(sessionId, AgentEventType::ToolApprovalResolved,
                AgentTextEvent{approved ? QStringLiteral("Approved") : QStringLiteral("Denied")},
                index, true);
    } else {
        publish(sessionId, AgentEventType::RunStarted);
    }
    publish(sessionId, AgentEventType::RuntimeStateChanged,
            AgentStateEvent{AgentLoopPhase::Running});
    worker_ = new QThread;
    auto* context = new QObject;
    context->moveToThread(worker_);
    {
        std::lock_guard lock(mutex_);
        workerContext_ = context;
    }
    QObject::connect(worker_, &QThread::started, context,
                     [this, sessionId, isResume, approved, goal, context] {
                         advanceAsync(sessionId, isResume, approved, goal, context);
                     });
    worker_->start();
    return true;
}

void AgentRuntime::shutdown() {
    QStringList sessionsToCancel;
    {
        std::lock_guard lock(mutex_);
        if (!activeSessionId_.isEmpty())
            sessionsToCancel.append(activeSessionId_);
        for (auto it = sessions_.cbegin(); it != sessions_.cend(); ++it) {
            if (it.value().phase == AgentLoopPhase::AwaitingApproval) {
                sessionsToCancel.append(it.key());
            }
        }
    }
    if (!sessionsToCancel.isEmpty())
        cancel(sessionsToCancel.first());
    std::lock_guard workerLock(workerMutex_);
    QList<std::function<void()>> controlledCancels;
    {
        std::lock_guard lock(mutex_);
        shuttingDown_ = true;
        cancelRequested_ = true;
        modelCancellationToken_->store(true);
        callbacksAlive_->store(false);
        controlledCancels = controlledCancels_.values();
        controlledCancels_.clear();
    }
    for (const auto& cancelControlled : controlledCancels)
        if (cancelControlled)
            cancelControlled();
    if (worker_) {
        worker_->wait();
        delete worker_;
        worker_ = nullptr;
    }
    for (const auto& session : sessionsToCancel)
        cancel(session);
    QStringList subscriberIds;
    {
        std::lock_guard eventLock(eventMutex_);
        subscriberIds = subscribers_.keys();
    }
    for (const auto& id : subscriberIds)
        unsubscribe(id);
}

QStringList AgentRuntime::toolIds() const {
    QStringList ids;
    for (const auto& tool : toolRegistry_.enabledTools()) {
        ids.append(tool.id);
    }
    return ids;
}

void AgentRuntime::prepareExecution(const QStringList& availableToolIds) {
    std::lock_guard executionLock(executionMutex_);
    auto* executor = dynamic_cast<RealToolExecutor*>(&executor_);
    if (!executor) {
        return;
    }
    executor->setMemorySnapshot(
        memoryStore_ && memoryStore_->isAvailable() ? memoryStore_->entries() : MemoryEntries{});
    QStringList historyLines;
    if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
        const auto messages = chatHistoryStore_->loadMessages();
        const int first = qMax(0, static_cast<int>(messages.size()) - 200);
        for (int i = first; i < messages.size(); ++i) {
            const auto& message = messages.at(i);
            QString role = QStringLiteral("user");
            if (message.role == ChatRole::Assistant) {
                role = QStringLiteral("assistant");
            } else if (message.role == ChatRole::System) {
                role = QStringLiteral("system");
            }
            const QString content = message.content.simplified();
            if (!content.isEmpty()) {
                historyLines.append(QStringLiteral("[%1] %2").arg(role, content.left(1000)));
            }
        }
    }
    executor->setHistorySnapshot(std::move(historyLines));

    executor->setSubagentRunner([this, availableToolIds](const QString& task) {
        AgentLoop::Config config;
        config.autonomousMode = true;
        config.maxIterations = 6;
        static const QSet<QString> kSubagentTools{
            QStringLiteral("read-file"),     QStringLiteral("grep"),
            QStringLiteral("glob"),          QStringLiteral("list-code-definitions"),
            QStringLiteral("web-search"),    QStringLiteral("web-fetch"),
            QStringLiteral("memory-search"), QStringLiteral("history-search"),
            QStringLiteral("current-time"),  QStringLiteral("system-info"),
            QStringLiteral("mcp-list"),
        };
        QStringList readOnlyToolIds;
        for (const auto& id : availableToolIds) {
            if (kSubagentTools.contains(id)) {
                readOnlyToolIds.append(id);
            }
        }
        AgentLoop loop(*planner_, executor_, approval_, sandbox_, readOnlyToolIds, config);
        loop.setToolRegistry(&toolRegistry_);
        loop.setExternalDirectoryGate(&externalDirectoryGate_);
        loop.setToolHookService(&toolHooks_);
        loop.setPermissionPolicy(&toolPermissionPolicy_, toolPermissionState_);
        const auto state = loop.run(
            task,
            QStringLiteral("subagent-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        if (state.phase == AgentLoopPhase::Completed) {
            return state.finalAnswer;
        }
        if (state.phase == AgentLoopPhase::Cancelled) {
            return QStringLiteral("Subagent run was cancelled.");
        }
        return QStringLiteral("Subagent could not finish: %1")
            .arg(state.abortReason.isEmpty() ? QStringLiteral("unknown reason")
                                             : state.abortReason);
    });
}

AgentPipelineResult AgentRuntime::executePipeline(const AgentRequest& request, bool autonomous) {
    // Retained for synchronous API compatibility. Desktop dispatch uses
    // executePipelineAsync() so process-backed tools do not block the UI.
    std::lock_guard executionLock(executionMutex_);
    AgentPipelineResult result;
    result.plan = metadata_->plan(request);
    if (result.plan.status == ToolInvocationPlanStatus::Planned) {
        ToolExecutionGateway gateway(&toolRegistry_);
        result.execution = gateway.validatePlan(result.plan);
        if (result.execution.status != ToolExecutionStatus::Succeeded) {
            result.summary = safeToolExecutionSummary(result.execution);
            return result;
        }
    }
    result.approval =
        autonomous ? ApprovalDecision{ApprovalStatus::Approved,
                                      QStringLiteral(
                                          "Autonomous Mode is enabled: user approval is bypassed."),
                                      {}}
                   : approval_.evaluate(result.plan);
    result.sandbox = sandbox_.evaluate(result.plan, result.approval);
    if (!autonomous && result.approval.status == ApprovalStatus::RequiresApproval) {
        result.execution.status = ToolExecutionStatus::Blocked;
        result.execution.summary =
            QStringLiteral("Execution paused: pending user approval in chat.");
    } else {
        ToolExecutionGateway gateway(&toolRegistry_);
        result.execution = gateway.execute(
            ToolExecutionRequest{result.plan, result.approval, result.sandbox, toolIds()},
            executor_);
    }
    result.summary = safeToolExecutionSummary(result.execution);
    return result;
}

void AgentRuntime::executePipelineAsync(const AgentRequest& request, bool autonomous,
                                        std::function<void(AgentPipelineResult)> completion) {
    AgentPipelineResult result;
    result.plan = metadata_->plan(request);
    if (result.plan.status == ToolInvocationPlanStatus::Planned) {
        ToolExecutionGateway gateway(&toolRegistry_);
        result.execution = gateway.validatePlan(result.plan);
        if (result.execution.status != ToolExecutionStatus::Succeeded) {
            result.summary = safeToolExecutionSummary(result.execution);
            completion(std::move(result));
            return;
        }
    }
    result.approval =
        autonomous ? ApprovalDecision{ApprovalStatus::Approved,
                                      QStringLiteral(
                                          "Autonomous Mode is enabled: user approval is bypassed."),
                                      {}}
                   : approval_.evaluate(result.plan);
    result.sandbox = sandbox_.evaluate(result.plan, result.approval);
    if (!autonomous && result.approval.status == ApprovalStatus::RequiresApproval) {
        result.execution.status = ToolExecutionStatus::Blocked;
        result.execution.summary =
            QStringLiteral("Execution paused: pending user approval in chat.");
        result.summary = safeToolExecutionSummary(result.execution);
        completion(std::move(result));
        return;
    }
    executePipelineResultAsync(std::move(result), std::move(completion));
}

AgentPipelineResult AgentRuntime::executeApprovedGoal(const QString& goal) {
    AgentPipelineResult result;
    const auto trimmed = goal.trimmed();
    if (trimmed.isEmpty()) {
        result.summary = QStringLiteral("Agent request was empty.");
        return result;
    }
    std::lock_guard executionLock(executionMutex_);
    result.plan = metadata_->plan(AgentRequest{trimmed, {}});
    return executeApprovedPlanLocked(
        result.plan,
        QStringLiteral("User approved this controlled task explicitly in Security settings."));
}

AgentPipelineResult AgentRuntime::executeApprovedPlan(const ToolInvocationPlan& plan,
                                                      const QString& approvalSummary) {
    std::lock_guard executionLock(executionMutex_);
    return executeApprovedPlanLocked(plan, approvalSummary);
}

void AgentRuntime::executeApprovedGoalAsync(const QString& goal,
                                            std::function<void(AgentPipelineResult)> completion) {
    const QString trimmed = goal.trimmed();
    if (trimmed.isEmpty()) {
        AgentPipelineResult result;
        result.summary = QStringLiteral("Agent request was empty.");
        completion(std::move(result));
        return;
    }
    executeApprovedPlanAsync(
        metadata_->plan(AgentRequest{trimmed, {}}),
        QStringLiteral("User approved this controlled task explicitly in Security settings."),
        std::move(completion));
}

void AgentRuntime::executeApprovedPlanAsync(const ToolInvocationPlan& plan,
                                            const QString& approvalSummary,
                                            std::function<void(AgentPipelineResult)> completion) {
    AgentPipelineResult result;
    result.plan = plan;
    result.approval = {ApprovalStatus::Approved, approvalSummary, {}};
    result.sandbox = sandbox_.evaluate(plan, result.approval);
    executePipelineResultAsync(std::move(result), std::move(completion));
}

void AgentRuntime::executePipelineResultAsync(AgentPipelineResult result,
                                              std::function<void(AgentPipelineResult)> completion) {
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto alive = callbacksAlive_;
    ToolExecutionGateway gateway(&toolRegistry_);
    auto completed = std::make_shared<bool>(false);
    ToolExecutionRequest request{result.plan, result.approval, result.sandbox, toolIds()};
    auto cancel = gateway.executeAsync(
        request, executor_, id, id, {},
        [this, id, alive, completed, result = std::move(result),
         completion = std::move(completion)](ToolExecutionResult execution) mutable {
            *completed = true;
            if (!alive->load())
                return;
            {
                std::lock_guard lock(mutex_);
                controlledCancels_.remove(id);
            }
            result.execution = std::move(execution);
            result.summary = safeToolExecutionSummary(result.execution);
            completion(std::move(result));
        });
    if (!*completed && cancel) {
        bool shuttingDown;
        {
            std::lock_guard lock(mutex_);
            shuttingDown = shuttingDown_;
            if (!shuttingDown)
                controlledCancels_.insert(id, cancel);
        }
        if (shuttingDown)
            cancel();
    }
}

AgentPipelineResult AgentRuntime::executeApprovedPlanLocked(const ToolInvocationPlan& plan,
                                                            const QString& approvalSummary) {
    AgentPipelineResult result;
    result.plan = plan;
    result.approval = {ApprovalStatus::Approved, approvalSummary, {}};
    result.sandbox = sandbox_.evaluate(result.plan, result.approval);
    ToolExecutionGateway gateway(&toolRegistry_);
    result.execution = gateway.execute(
        ToolExecutionRequest{result.plan, result.approval, result.sandbox, toolIds()}, executor_);
    result.summary = safeToolExecutionSummary(result.execution);
    return result;
}

void AgentRuntime::configureLoop(AgentLoop& loop, const QString& sessionId,
                                 const AgentSessionOptions& options, const QString& goal) {
    loop.setCancelQuery([this] { return cancelRequested_.load(); });
    loop.setStepCallback([this, sessionId, callback = options.onStep](const AgentStepRecord& step) {
        publish(sessionId, AgentEventType::AgentStepCompleted, AgentStepEvent{step}, step.index,
                !step.toolId.isEmpty());
        if (callback)
            callback(step);
    });
    loop.setStatusCallback(options.onStatus);
    loop.setPlanningCallback([this, sessionId, goal](int index, bool started) {
        const auto* llm = dynamic_cast<const LlmAgentRuntime*>(planner_);
        if (llm && llm->hasModelProvider() && !goal.trimmed().isEmpty()) {
            if (started) {
                llm->setStreamObserver(
                    [this, sessionId, index](const QString& delta) {
                        publish(sessionId, AgentEventType::ModelOutputDelta, AgentTextEvent{delta},
                                index);
                    },
                    modelCancellationToken_);
            } else {
                llm->setStreamObserver({});
            }
            publish(sessionId,
                    started ? AgentEventType::ModelRequestStarted
                            : AgentEventType::ModelRequestCompleted,
                    {}, index);
        }
    });
    loop.setToolCallback([this, sessionId](AgentLoop::ToolTransition transition, int index,
                                           const ToolInvocationPlan& plan,
                                           const AgentStepRecord* record) {
        if (plan.invocations.isEmpty())
            return;
        const auto& invocation = plan.invocations.first();
        AgentToolEvent payload{invocation.toolId, invocation.arguments, invocation.riskLevel,
                               record ? record->observation : QString{}};
        AgentEventType type;
        switch (transition) {
        case AgentLoop::ToolTransition::Requested:
            type = AgentEventType::ToolRequested;
            break;
        case AgentLoop::ToolTransition::ApprovalRequired:
            type = AgentEventType::ToolApprovalRequired;
            break;
        case AgentLoop::ToolTransition::ExecutionStarted:
            type = AgentEventType::ToolExecutionStarted;
            break;
        case AgentLoop::ToolTransition::ExecutionFinished:
            type = record && record->succeeded ? AgentEventType::ToolExecutionCompleted
                                               : AgentEventType::ToolExecutionFailed;
            break;
        }
        publish(sessionId, type, payload, index, true);
    });
    loop.setOutputCallback([this, sessionId](int index, const QString& processId,
                                             ProcessStream stream, const QByteArray& bytes) {
        publish(sessionId, AgentEventType::ToolOutput,
                AgentToolOutputEvent{processId, stream, bytes}, index, true);
    });
    loop.setToolCallIdProvider([this, sessionId](int index) {
        std::lock_guard lock(eventMutex_);
        return turns_.value(sessionId).toolCallIds.value(index);
    });
}

void AgentRuntime::commitResult(const QString& sessionId, AgentLoopState& result) {
    {
        std::unique_lock lock(mutex_);
        cancellationPublished_.wait(lock, [this] { return !cancellationEventPending_; });
        if (cancelRequested_) {
            result.phase = AgentLoopPhase::Cancelled;
            result.abortReason = QStringLiteral("Agent run cancelled by user.");
        }
        sessions_[sessionId] = result;
        AgentRuntimeErrorCode code = AgentRuntimeErrorCode::None;
        if (result.phase == AgentLoopPhase::Failed)
            code = AgentRuntimeErrorCode::ExecutionFailed;
        else if (result.phase == AgentLoopPhase::Cancelled)
            code = AgentRuntimeErrorCode::Cancelled;
        else if (result.phase == AgentLoopPhase::Stuck)
            code = AgentRuntimeErrorCode::Stuck;
        if (code != AgentRuntimeErrorCode::None)
            errors_[sessionId] = {code, result.abortReason, false};
        activeSessionId_.clear();
        activeLoop_ = nullptr;
        workerContext_ = nullptr;
    }
    if (result.phase != AgentLoopPhase::AwaitingApproval)
        externalDirectoryGate_.clearPermissions();
    finishTurn(sessionId, result);
}

void AgentRuntime::advanceAsync(const QString& sessionId, bool isResume, bool approved,
                                const QString& goal, QObject* context) {
    AgentLoopState seed;
    AgentSessionOptions options;
    QStringList approvedToolIds;
    {
        std::lock_guard lock(mutex_);
        seed = sessions_.value(sessionId);
        options = options_.value(sessionId);
        approvedToolIds = approvedToolIds_;
        errors_.remove(sessionId);
    }
    const auto availableToolIds =
        options.availableToolIds.isEmpty() ? toolIds() : options.availableToolIds;
    prepareExecution(availableToolIds);
    auto executionLock = std::make_shared<std::unique_lock<std::mutex>>(executionMutex_);
    AgentLoop::Config config;
    config.autonomousMode = options.autonomousMode;
    config.sessionApprovedToolIds = approvedToolIds;
    auto loop = std::make_shared<AgentLoop>(*planner_, executor_, approval_, sandbox_,
                                            availableToolIds, config);
    loop->setToolRegistry(&toolRegistry_);
    loop->setExternalDirectoryGate(&externalDirectoryGate_);
    loop->setToolHookService(&toolHooks_);
    loop->setPermissionPolicy(&toolPermissionPolicy_, toolPermissionState_);
    // The worker context owns the loop until its queued deletion on the same thread.
    QObject::connect(context, &QObject::destroyed, [loop] {});
    configureLoop(*loop, sessionId, options, seed.goal.isEmpty() ? goal : seed.goal);
    {
        std::lock_guard lock(mutex_);
        activeLoop_ = loop.get();
    }
    auto finish = [this, sessionId, context, executionLock,
                   callback = options.onFinished](const AgentLoopState& state) {
        AgentLoopState result = state;
        commitResult(sessionId, result);
        bool shuttingDown;
        {
            std::lock_guard lock(mutex_);
            shuttingDown = shuttingDown_;
        }
        if (!shuttingDown && callback)
            callback(result);
        context->deleteLater();
        worker_->quit();
    };
    if (isResume)
        loop->resumeAsync(seed, approved, context, std::move(finish));
    else
        loop->runAsync(goal, sessionId, context, std::move(finish));
}

AgentLoopState AgentRuntime::advance(const QString& sessionId, bool isResume, bool approved,
                                     const QString& goal, bool prepared) {
    if (!planner_) {
        AgentLoopState failure;
        failure.sessionId = sessionId;
        failure.phase = AgentLoopPhase::Failed;
        failure.abortReason = QStringLiteral("Agent step planner is unavailable.");
        return failure;
    }
    AgentLoopState seed;
    AgentSessionOptions options;
    QStringList approvedToolIds;
    {
        std::lock_guard lock(mutex_);
        if (!sessions_.contains(sessionId) ||
            (prepared ? activeSessionId_ != sessionId : !activeSessionId_.isEmpty())) {
            AgentLoopState failure;
            failure.sessionId = sessionId;
            failure.phase = AgentLoopPhase::Failed;
            failure.abortReason = QStringLiteral("Agent session is unavailable or busy.");
            errors_[sessionId] = {!activeSessionId_.isEmpty()
                                      ? AgentRuntimeErrorCode::Busy
                                      : AgentRuntimeErrorCode::InvalidSession,
                                  failure.abortReason, !activeSessionId_.isEmpty()};
            return failure;
        }
        seed = sessions_.value(sessionId);
        options = options_.value(sessionId);
        approvedToolIds = approvedToolIds_;
        const auto expectedPhase = prepared   ? AgentLoopPhase::Running
                                   : isResume ? AgentLoopPhase::AwaitingApproval
                                              : AgentLoopPhase::Idle;
        if (seed.phase != expectedPhase &&
            !(prepared && seed.phase == AgentLoopPhase::Cancelling)) {
            seed.phase = AgentLoopPhase::Failed;
            seed.abortReason = QStringLiteral("Agent session is in the wrong state.");
            errors_[sessionId] = {AgentRuntimeErrorCode::InvalidState, seed.abortReason, false};
            return seed;
        }
        if (!prepared) {
            activeSessionId_ = sessionId;
            cancelRequested_ = false;
            if (!isResume)
                modelCancellationToken_ = std::make_shared<std::atomic_bool>(false);
        }
        if (!cancelRequested_)
            sessions_[sessionId].phase = AgentLoopPhase::Running;
        errors_.remove(sessionId);
    }

    if (!prepared) {
        if (!isResume) {
            externalDirectoryGate_.clearPermissions();
            beginTurn(sessionId);
        }
        if (isResume) {
            const int index = static_cast<int>(seed.steps.size()) + 1;
            publish(
                sessionId, AgentEventType::ToolApprovalResolved,
                AgentTextEvent{approved ? QStringLiteral("Approved") : QStringLiteral("Denied")},
                index, true);
        } else
            publish(sessionId, AgentEventType::RunStarted);
        publish(sessionId, AgentEventType::RuntimeStateChanged,
                AgentStateEvent{AgentLoopPhase::Running});
    }

    const auto availableToolIds =
        options.availableToolIds.isEmpty() ? toolIds() : options.availableToolIds;
    AgentLoop::Config config;
    config.autonomousMode = options.autonomousMode;
    config.sessionApprovedToolIds = approvedToolIds;
    AgentLoopState result;
    try {
        prepareExecution(availableToolIds);
        std::lock_guard executionLock(executionMutex_);
        AgentLoop loop(*planner_, executor_, approval_, sandbox_, availableToolIds, config);
        loop.setToolRegistry(&toolRegistry_);
        loop.setExternalDirectoryGate(&externalDirectoryGate_);
        loop.setToolHookService(&toolHooks_);
        loop.setPermissionPolicy(&toolPermissionPolicy_, toolPermissionState_);
        configureLoop(loop, sessionId, options, seed.goal.isEmpty() ? goal : seed.goal);
        result = isResume ? loop.resume(seed, approved) : loop.run(goal, sessionId);
    } catch (...) {
        result = std::move(seed);
        result.phase = AgentLoopPhase::Failed;
        result.abortReason = QStringLiteral("Agent execution failed.");
    }
    {
        std::unique_lock lock(mutex_);
        cancellationPublished_.wait(lock, [this] { return !cancellationEventPending_; });
        if (cancelRequested_) {
            result.phase = AgentLoopPhase::Cancelled;
            result.abortReason = QStringLiteral("Agent run cancelled by user.");
        }
        sessions_[sessionId] = result;
        AgentRuntimeErrorCode code = AgentRuntimeErrorCode::None;
        if (result.phase == AgentLoopPhase::Failed) {
            code = AgentRuntimeErrorCode::ExecutionFailed;
        } else if (result.phase == AgentLoopPhase::Cancelled) {
            code = AgentRuntimeErrorCode::Cancelled;
        } else if (result.phase == AgentLoopPhase::Stuck) {
            code = AgentRuntimeErrorCode::Stuck;
        }
        if (code != AgentRuntimeErrorCode::None) {
            errors_[sessionId] = {code, result.abortReason, false};
        }
        activeSessionId_.clear();
    }
    if (result.phase != AgentLoopPhase::AwaitingApproval)
        externalDirectoryGate_.clearPermissions();
    finishTurn(sessionId, result);
    return result;
}

} // namespace sentinel::core
