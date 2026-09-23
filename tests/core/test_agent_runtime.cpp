// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "sentinel/core/agent/AgentRuntime.h"
#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/runtime/NullToolExecutor.h"
#include "sentinel/core/security/StaticApprovalPolicy.h"
#include "sentinel/core/security/StaticSandboxPolicy.h"

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

} // namespace

class AgentRuntimeTest final : public QObject {
    Q_OBJECT

private slots:
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
};

QTEST_MAIN(AgentRuntimeTest)
#include "test_agent_runtime.moc"
