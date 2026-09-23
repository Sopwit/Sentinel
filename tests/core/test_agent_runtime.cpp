// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "sentinel/core/agent/AgentRuntime.h"
#include "sentinel/core/agent/LlmAgentRuntime.h"
#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/runtime/NullToolExecutor.h"
#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/security/StaticApprovalPolicy.h"
#include "sentinel/core/security/StaticSandboxPolicy.h"

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

using namespace sentinel::core;

namespace {

class Planner final : public IAgentStepPlanner {
public:
    mutable int calls = 0;
    mutable std::function<void()> onStep;
    AgentStepDecision decision;

    AgentStepDecision nextStep(const QString&, const QList<AgentStepRecord>&) const override {
        ++calls;
        if (onStep) {
            onStep();
        }
        if (calls > 1 && decision.kind == AgentStepDecision::Kind::ToolCall) {
            AgentStepDecision final;
            final.kind = AgentStepDecision::Kind::FinalAnswer;
            final.answer = QStringLiteral("finished");
            return final;
        }
        return decision;
    }
};

class StreamingPlannerProvider final : public IChatProvider {
public:
    QString name() const override {
        return QStringLiteral("streaming-test");
    }
    ChatProviderStatus status() const override {
        return ChatProviderStatus::Ready;
    }
    ChatProviderReply sendMessage(const QString&) override {
        return {false, {}, QStringLiteral("non-streaming path used")};
    }
    bool supportsStreaming() const override {
        return true;
    }
    ChatProviderReply sendMessageStreaming(const QString&,
                                           const std::function<void(const QString&)>& onDelta,
                                           const std::shared_ptr<std::atomic_bool>&) override {
        onDelta(QStringLiteral("{\"action\":\"final\","));
        onDelta(QStringLiteral("\"answer\":\"done\"}"));
        return {true, QStringLiteral("{\"action\":\"final\",\"answer\":\"done\"}"), {}};
    }
};

class SubagentPlanner final : public IAgentStepPlanner {
public:
    AgentStepDecision nextStep(const QString& goal,
                               const QList<AgentStepRecord>& history) const override {
        AgentStepDecision decision;
        if (goal == QStringLiteral("child task")) {
            decision.kind = AgentStepDecision::Kind::FinalAnswer;
            decision.answer = QStringLiteral("child answer");
        } else if (history.isEmpty()) {
            decision.kind = AgentStepDecision::Kind::ToolCall;
            decision.toolId = QStringLiteral("spawn-agent");
            decision.toolName = QStringLiteral("spawn-agent");
            decision.arguments.append({QStringLiteral("task"), QStringLiteral("child task")});
        } else {
            decision.kind = AgentStepDecision::Kind::FinalAnswer;
            decision.answer = history.first().observation;
        }
        return decision;
    }
};

class FixedPlanRuntime final : public IAgentRuntime {
public:
    QString name() const override {
        return QStringLiteral("fixed");
    }
    AgentStatus status() const override {
        return AgentStatus::Ready;
    }
    QList<AgentCapabilityDescriptor> capabilities() const override {
        return {};
    }
    QList<ToolDescriptor> availableTools() const override {
        return {{QStringLiteral("run-command"),
                 QStringLiteral("Run Command"),
                 {},
                 ToolRiskLevel::Low,
                 ToolExecutionMode::Local}};
    }
    ToolInvocationPlan plan(const AgentRequest&) const override {
        ToolInvocationPlan plan;
        plan.status = ToolInvocationPlanStatus::Planned;
        plan.invocations.append({QStringLiteral("run-command"), QStringLiteral("Run Command")});
        return plan;
    }
    AgentResponse execute(const AgentRequest&) override {
        return {};
    }
};

class RecordingExecutor final : public IToolExecutor {
public:
    mutable int calls = 0;
    mutable ToolExecutionRequest lastRequest;
    ToolExecutionResult execute(const ToolExecutionRequest& request) const override {
        ++calls;
        lastRequest = request;
        return {ToolExecutionStatus::Succeeded, QStringLiteral("executed")};
    }
};

} // namespace

class AgentRuntimeTest final : public QObject {
    Q_OBJECT

private slots:
    void streamsRealProviderDeltasWithStepCorrelation() {
        StreamingPlannerProvider provider;
        LlmAgentRuntime planner(NullAgentRuntime::standardTools(), &provider);
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        const auto session = runtime.createSession();
        QCOMPARE(runtime.submit(session, QStringLiteral("task")).phase, AgentLoopPhase::Completed);
        const auto events = runtime.eventHistory(session);
        QString stepId;
        QString turnId;
        int start = -1, firstDelta = -1, lastDelta = -1, completed = -1;
        for (int i = 0; i < events.size(); ++i) {
            const auto& event = events.at(i);
            if (event.type == AgentEventType::ModelRequestStarted)
                start = i;
            if (event.type == AgentEventType::ModelOutputDelta) {
                if (firstDelta < 0)
                    firstDelta = i;
                lastDelta = i;
                QVERIFY(!std::get<AgentTextEvent>(event.payload).text.isEmpty());
            }
            if (event.type == AgentEventType::ModelRequestCompleted)
                completed = i;
            if (event.type == AgentEventType::ModelRequestStarted ||
                event.type == AgentEventType::ModelOutputDelta ||
                event.type == AgentEventType::ModelRequestCompleted) {
                if (stepId.isEmpty()) {
                    stepId = event.stepId;
                    turnId = event.turnId;
                }
                QCOMPARE(event.stepId, stepId);
                QCOMPARE(event.turnId, turnId);
            }
        }
        QVERIFY(start >= 0 && firstDelta > start && lastDelta >= firstDelta &&
                completed > lastDelta);
    }

    void emitsCorrelatedTerminalEventsInOrder() {
        Planner planner;
        planner.decision.kind = AgentStepDecision::Kind::FinalAnswer;
        planner.decision.answer = QStringLiteral("done");
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        QList<AgentEvent> received;
        const auto token =
            runtime.subscribe([&](const AgentEvent& event) { received.append(event); });
        const auto session = runtime.createSession();
        QCOMPARE(runtime.submit(session, QStringLiteral("task")).phase, AgentLoopPhase::Completed);
        const auto history = runtime.eventHistory(session);
        QCOMPARE(received.size(), history.size());
        QCOMPARE(history.first().type, AgentEventType::SessionCreated);
        QCOMPARE(history.at(1).type, AgentEventType::RunStarted);
        QCOMPARE(history.at(history.size() - 2).type, AgentEventType::AgentCompleted);
        QCOMPARE(history.last().type, AgentEventType::RuntimeStateChanged);
        QString turnId;
        QSet<QString> eventIds;
        int terminalCount = 0;
        for (const auto& event : history) {
            QCOMPARE(event.sessionId, session);
            QVERIFY(!event.id.isEmpty());
            eventIds.insert(event.id);
            if (event.type == AgentEventType::AgentCompleted)
                ++terminalCount;
            if (event.type != AgentEventType::SessionCreated) {
                if (turnId.isEmpty())
                    turnId = event.turnId;
                QCOMPARE(event.turnId, turnId);
            }
        }
        QCOMPARE(eventIds.size(), history.size());
        QCOMPARE(terminalCount, 1);
        runtime.unsubscribe(token);
    }

    void approvalEventsKeepToolCallIdentity() {
        Planner planner;
        planner.decision.kind = AgentStepDecision::Kind::ToolCall;
        planner.decision.toolId = QStringLiteral("run-command");
        planner.decision.toolName = QStringLiteral("run-command");
        planner.decision.riskLevel = ToolRiskLevel::High;
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        const auto session = runtime.createSession();
        AgentSessionOptions options;
        options.availableToolIds = {QStringLiteral("run-command")};
        runtime.configureSession(session, std::move(options));
        QCOMPARE(runtime.submit(session, QStringLiteral("task")).phase,
                 AgentLoopPhase::AwaitingApproval);
        QCOMPARE(runtime.resume(session, true).phase, AgentLoopPhase::Completed);
        QString toolCallId;
        QString stepId;
        int terminalCount = 0;
        for (const auto& event : runtime.eventHistory(session)) {
            if (event.type == AgentEventType::ToolRequested) {
                toolCallId = event.toolCallId;
                stepId = event.stepId;
            }
            if (event.type == AgentEventType::ToolApprovalRequired ||
                event.type == AgentEventType::ToolApprovalResolved ||
                event.type == AgentEventType::ToolExecutionStarted ||
                event.type == AgentEventType::ToolExecutionCompleted ||
                event.type == AgentEventType::ToolExecutionFailed ||
                event.type == AgentEventType::AgentStepCompleted) {
                QCOMPARE(event.toolCallId, toolCallId);
                QCOMPARE(event.stepId, stepId);
            }
            if (event.type == AgentEventType::AgentCompleted)
                ++terminalCount;
        }
        QVERIFY(!toolCallId.isEmpty());
        QCOMPARE(terminalCount, 1);
    }

    void cancellingEmitsOneTerminalAndUnsubscribeStopsDelivery() {
        Planner planner;
        planner.decision.kind = AgentStepDecision::Kind::FinalAnswer;
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        int received = 0;
        const auto token = runtime.subscribe([&](const AgentEvent&) { ++received; });
        const auto session = runtime.createSession();
        runtime.unsubscribe(token);
        const int countAfterUnsubscribe = received;
        planner.onStep = [&] { QVERIFY(runtime.cancel(session)); };
        QCOMPARE(runtime.submit(session, QStringLiteral("task")).phase, AgentLoopPhase::Cancelled);
        QCOMPARE(received, countAfterUnsubscribe);
        int cancelled = 0;
        int completed = 0;
        int cancellingStates = 0;
        for (const auto& event : runtime.eventHistory(session)) {
            if (event.type == AgentEventType::AgentCancelled)
                ++cancelled;
            if (event.type == AgentEventType::AgentCompleted)
                ++completed;
            if (event.type == AgentEventType::RuntimeStateChanged) {
                if (const auto* state = std::get_if<AgentStateEvent>(&event.payload);
                    state && state->phase == AgentLoopPhase::Cancelling)
                    ++cancellingStates;
            }
        }
        QCOMPARE(cancelled, 1);
        QCOMPARE(completed, 0);
        QCOMPARE(cancellingStates, 1);
    }

    void createsIndependentSessionsAndReturnsFinalAnswer() {
        Planner planner;
        planner.decision.kind = AgentStepDecision::Kind::FinalAnswer;
        planner.decision.answer = QStringLiteral("done");
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        IAgentRuntime& api = runtime;
        QVERIFY(api.supportsSessions());

        const QString first = api.createSession();
        const QString second = api.createSession();
        QVERIFY(!first.isEmpty());
        QVERIFY(first != second);
        QCOMPARE(api.sessionState(first).phase, AgentLoopPhase::Idle);
        const auto result = api.submit(first, QStringLiteral("task"));
        QCOMPARE(result.sessionId, first);
        QCOMPARE(result.phase, AgentLoopPhase::Completed);
        QCOMPARE(result.finalAnswer, QStringLiteral("done"));
        QCOMPARE(api.sessionState(second).phase, AgentLoopPhase::Idle);
        QCOMPARE(api.error(first).code, AgentRuntimeErrorCode::None);
        QCOMPARE(planner.calls, 1);
    }

    void cancellationDelegatesToRunningLoop() {
        Planner planner;
        planner.decision.kind = AgentStepDecision::Kind::FinalAnswer;
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        const QString session = runtime.createSession();
        AgentSessionOptions options;
        options.onStatus = [&](const QString&) { QVERIFY(runtime.cancel(session)); };
        runtime.configureSession(session, std::move(options));

        const auto result = runtime.submit(session, QStringLiteral("task"));
        QCOMPARE(result.phase, AgentLoopPhase::Cancelled);
        QCOMPARE(runtime.error(session).code, AgentRuntimeErrorCode::Cancelled);
        QVERIFY(!runtime.cancel(session));
    }

    void rejectsInvalidSessionAndResume() {
        Planner planner;
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        const auto missing = runtime.submit(QStringLiteral("missing"), QStringLiteral("task"));
        QCOMPARE(missing.phase, AgentLoopPhase::Failed);
        QCOMPARE(runtime.error(QStringLiteral("missing")).code,
                 AgentRuntimeErrorCode::InvalidSession);
        QVERIFY(!missing.abortReason.isEmpty());
        const QString session = runtime.createSession();
        QCOMPARE(runtime.resume(session, true).phase, AgentLoopPhase::Failed);
    }

    void approvalDelegatesToLoop() {
        Planner planner;
        planner.decision.kind = AgentStepDecision::Kind::ToolCall;
        planner.decision.toolId = QStringLiteral("run-command");
        planner.decision.toolName = QStringLiteral("run-command");
        planner.decision.riskLevel = ToolRiskLevel::High;
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        const QString session = runtime.createSession();
        AgentSessionOptions options;
        options.availableToolIds = {QStringLiteral("run-command")};
        runtime.configureSession(session, std::move(options));
        const auto pending = runtime.submit(session, QStringLiteral("task"));
        QCOMPARE(pending.phase, AgentLoopPhase::AwaitingApproval);
        QCOMPARE(runtime.sessionState(session).phase, AgentLoopPhase::AwaitingApproval);
        QVERIFY(runtime.approve(session, false));
        const auto finished = runtime.resume(session, false);
        QCOMPARE(finished.phase, AgentLoopPhase::Completed);
        QCOMPARE(finished.finalAnswer, QStringLiteral("finished"));
    }

    void propagatesPlannerFailure() {
        Planner planner;
        planner.decision.kind = AgentStepDecision::Kind::GiveUp;
        planner.decision.reason = QStringLiteral("planner failed");
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        const QString session = runtime.createSession();
        const auto result = runtime.submit(session, QStringLiteral("task"));
        QCOMPARE(result.phase, AgentLoopPhase::Failed);
        QCOMPARE(result.abortReason, QStringLiteral("planner failed"));
        QCOMPARE(runtime.error(session).code, AgentRuntimeErrorCode::ExecutionFailed);
        QCOMPARE(runtime.error(session).message, QStringLiteral("planner failed"));
    }

    void ownsWorkerAndCompletesAsynchronously() {
        Planner planner;
        planner.decision.kind = AgentStepDecision::Kind::FinalAnswer;
        planner.decision.answer = QStringLiteral("worker done");
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        const auto callerThread = std::this_thread::get_id();
        std::thread::id workerThread;
        std::atomic<int> completions{0};
        const QString session = runtime.createSession();
        AgentSessionOptions options;
        options.onStatus = [&](const QString&) { workerThread = std::this_thread::get_id(); };
        options.onFinished = [&](const AgentLoopState&) { ++completions; };
        runtime.configureSession(session, std::move(options));
        IAgentRuntime& api = runtime;
        QVERIFY(api.start(session, QStringLiteral("task")));
        QTRY_COMPARE(api.sessionState(session).phase, AgentLoopPhase::Completed);
        QTRY_COMPARE(completions.load(), 1);
        QVERIFY(workerThread != callerThread);
        QCOMPARE(api.sessionState(session).finalAnswer, QStringLiteral("worker done"));
    }

    void cancelsActiveWorkerAndJoinsOnShutdown() {
        Planner planner;
        planner.decision.kind = AgentStepDecision::Kind::FinalAnswer;
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        std::promise<void> entered;
        auto enteredFuture = entered.get_future();
        std::atomic<int> completions{0};
        {
            AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                                 sandbox);
            const QString session = runtime.createSession();
            AgentSessionOptions options;
            options.onStatus = [&](const QString&) {
                entered.set_value();
                std::this_thread::sleep_for(std::chrono::milliseconds(40));
            };
            options.onFinished = [&](const AgentLoopState&) { ++completions; };
            runtime.configureSession(session, std::move(options));
            QVERIFY(runtime.start(session, QStringLiteral("task")));
            QCOMPARE(enteredFuture.wait_for(std::chrono::seconds(1)), std::future_status::ready);
            QVERIFY(runtime.cancel(session));
            runtime.shutdown();
            QCOMPARE(runtime.sessionState(session).phase, AgentLoopPhase::Cancelled);
        }
        QCOMPARE(completions.load(), 0);
    }

    void resumesApprovalOnSameWorkerOwnedSession() {
        Planner planner;
        planner.decision.kind = AgentStepDecision::Kind::ToolCall;
        planner.decision.toolId = QStringLiteral("run-command");
        planner.decision.toolName = QStringLiteral("run-command");
        planner.decision.riskLevel = ToolRiskLevel::High;
        NullToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        const QString session = runtime.createSession();
        AgentSessionOptions options;
        options.availableToolIds = {QStringLiteral("run-command")};
        runtime.configureSession(session, std::move(options));
        QVERIFY(runtime.start(session, QStringLiteral("task")));
        QTRY_COMPARE(runtime.sessionState(session).phase, AgentLoopPhase::AwaitingApproval);
        QVERIFY(runtime.approve(session, false));
        QVERIFY(runtime.continueSession(session, false));
        QTRY_COMPARE(runtime.sessionState(session).phase, AgentLoopPhase::Completed);
        QCOMPARE(runtime.sessionState(session).sessionId, session);
        QCOMPARE(runtime.sessionState(session).steps.first().statusText, QStringLiteral("Denied"));
    }

    void runtimeWiresSubagentExecution() {
        SubagentPlanner planner;
        RealToolExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox(QSet<QString>{QStringLiteral("tool.metadata.read"),
                                                  QStringLiteral("tool.risk.medium"),
                                                  QStringLiteral("tool.risk.high")});
        AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                             sandbox);
        const QString session = runtime.createSession();
        AgentSessionOptions options;
        options.autonomousMode = true;
        options.availableToolIds = {QStringLiteral("spawn-agent")};
        runtime.configureSession(session, std::move(options));
        const auto result = runtime.submit(session, QStringLiteral("parent task"));
        QCOMPARE(result.phase, AgentLoopPhase::Completed);
        QCOMPARE(result.steps.size(), 1);
        QVERIFY(result.finalAnswer.contains(QStringLiteral("child answer")));
    }

    void controlledPipelineUsesRuntimeDependencies() {
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        StaticSandboxPolicy sandbox;
        AgentRuntime runtime(std::make_unique<FixedPlanRuntime>(), nullptr, executor, approval,
                             sandbox);
        IAgentRuntime& api = runtime;
        QVERIFY(!api.supportsSessions());
        const auto result = api.executeApprovedGoal(QStringLiteral("controlled task"));
        QCOMPARE(executor.calls, 1);
        QCOMPARE(executor.lastRequest.approval.status, ApprovalStatus::Approved);
        QCOMPARE(executor.lastRequest.plan.invocations.first().toolId,
                 QStringLiteral("run-command"));
        QCOMPARE(result.execution.status, ToolExecutionStatus::Succeeded);
        QCOMPARE(result.summary, QStringLiteral("executed"));

        const auto approvedPlan =
            api.executeApprovedPlan(result.plan, QStringLiteral("Approved in chat."));
        QCOMPARE(executor.calls, 2);
        QCOMPARE(executor.lastRequest.approval.summary, QStringLiteral("Approved in chat."));
        QCOMPARE(approvedPlan.execution.status, ToolExecutionStatus::Succeeded);
    }
};

QTEST_MAIN(AgentRuntimeTest)
#include "test_agent_runtime.moc"
