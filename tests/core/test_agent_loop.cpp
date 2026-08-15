// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "sentinel/core/agent/AgentLoop.h"
#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/security/StaticApprovalPolicy.h"
#include "sentinel/core/security/StaticSandboxPolicy.h"

#include <functional>

using namespace sentinel::core;

namespace {

class ScriptedPlanner final : public IAgentStepPlanner {
public:
    QVector<AgentStepDecision> decisions;
    mutable QVector<QList<AgentStepRecord>> observedHistories;
    mutable int calls = 0;
    bool repeatLast = false;
    std::function<void(int)> onCall;

    AgentStepDecision nextStep(const QString& goal,
                               const QList<AgentStepRecord>& history) const override {
        Q_UNUSED(goal);
        observedHistories.append(history);
        const int index = calls++;
        if (onCall) {
            onCall(index);
        }
        if (index < decisions.size()) {
            return decisions.at(index);
        }
        if (repeatLast && !decisions.isEmpty()) {
            return decisions.last();
        }
        AgentStepDecision fallback;
        fallback.kind = AgentStepDecision::Kind::FinalAnswer;
        fallback.answer = QStringLiteral("fallback final answer");
        return fallback;
    }
};

class RecordingExecutor final : public IToolExecutor {
public:
    mutable QVector<ToolExecutionRequest> requests;
    QString summary = QStringLiteral("observation-from-tool");
    ToolExecutionStatus status = ToolExecutionStatus::Succeeded;

    ToolExecutionResult execute(const ToolExecutionRequest& request) const override {
        requests.append(request);
        return {status, summary};
    }
};

AgentStepDecision toolDecision(const QString& toolId, const QString& argValue = QStringLiteral("x"),
                               ToolRiskLevel risk = ToolRiskLevel::Low) {
    AgentStepDecision decision;
    decision.kind = AgentStepDecision::Kind::ToolCall;
    decision.toolId = toolId;
    decision.toolName = toolId;
    decision.riskLevel = risk;
    decision.thought = QStringLiteral("use %1").arg(toolId);
    decision.arguments.append(ToolInvocationArgument{QStringLiteral("command"), argValue});
    return decision;
}

AgentStepDecision finalDecision(const QString& answer) {
    AgentStepDecision decision;
    decision.kind = AgentStepDecision::Kind::FinalAnswer;
    decision.answer = answer;
    return decision;
}

StaticSandboxPolicy permissiveSandbox() {
    return StaticSandboxPolicy(QSet<QString>{QStringLiteral("tool.metadata.read"),
                                             QStringLiteral("tool.risk.medium"),
                                             QStringLiteral("tool.risk.high")});
}

} // namespace

class AgentLoopTest final : public QObject {
    Q_OBJECT

private slots:
    void runsToolThenFinalAnswer() {
        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("run-command")),
                             finalDecision(QStringLiteral("all done"))};
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();

        AgentLoop loop(planner, executor, approval, sandbox,
                       QStringList{QStringLiteral("run-command")});
        const auto state = loop.run(QStringLiteral("list files"));

        QCOMPARE(state.phase, AgentLoopPhase::Completed);
        QCOMPARE(state.steps.size(), 1);
        QCOMPARE(state.finalAnswer, QStringLiteral("all done"));
        QCOMPARE(executor.requests.size(), 1);
        QCOMPARE(state.steps.first().statusText, QStringLiteral("Succeeded"));
        QVERIFY(state.steps.first().succeeded);
    }

    void feedsObservationToNextPlanningCall() {
        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("run-command")),
                             finalDecision(QStringLiteral("done"))};
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();

        AgentLoop loop(planner, executor, approval, sandbox,
                       QStringList{QStringLiteral("run-command")});
        loop.run(QStringLiteral("goal"));

        QCOMPARE(planner.observedHistories.size(), 2);
        QCOMPARE(planner.observedHistories.at(1).size(), 1);
        QVERIFY(planner.observedHistories.at(1).first().observation.contains(
            QStringLiteral("observation-from-tool")));
    }

    void stopsAtIterationLimit() {
        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("run-command"))};
        planner.repeatLast = true;
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();

        AgentLoop::Config config;
        config.maxIterations = 3;
        AgentLoop loop(planner, executor, approval, sandbox,
                       QStringList{QStringLiteral("run-command")}, config);
        const auto state = loop.run(QStringLiteral("goal"));

        QCOMPARE(state.phase, AgentLoopPhase::Failed);
        QCOMPARE(state.steps.size(), 3);
        QVERIFY(state.abortReason.contains(QStringLiteral("Iteration limit")));
    }

    void pausesForApprovalAndResumes() {
        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("run-command"),
                                          QStringLiteral("rm -rf /tmp/probe"), ToolRiskLevel::High),
                             finalDecision(QStringLiteral("finished"))};
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();

        AgentLoop loop(planner, executor, approval, sandbox,
                       QStringList{QStringLiteral("run-command")});
        auto state = loop.run(QStringLiteral("clean temp"));

        QCOMPARE(state.phase, AgentLoopPhase::AwaitingApproval);
        QCOMPARE(executor.requests.size(), 0);
        QVERIFY(!state.pendingApprovalPlan.invocations.isEmpty());

        AgentLoop resumed(planner, executor, approval, sandbox,
                          QStringList{QStringLiteral("run-command")});
        state = resumed.resume(state, true);

        QCOMPARE(state.phase, AgentLoopPhase::Completed);
        QCOMPARE(executor.requests.size(), 1);
        QCOMPARE(state.steps.size(), 1);
        QVERIFY(state.steps.first().succeeded);
    }

    void recordsDenialObservationWhenUserDenies() {
        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("run-command"), QStringLiteral(" risky "),
                                          ToolRiskLevel::High),
                             finalDecision(QStringLiteral("aborted by user denial"))};
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();

        AgentLoop loop(planner, executor, approval, sandbox,
                       QStringList{QStringLiteral("run-command")});
        auto state = loop.run(QStringLiteral("risky goal"));

        QCOMPARE(state.phase, AgentLoopPhase::AwaitingApproval);

        AgentLoop resumed(planner, executor, approval, sandbox,
                          QStringList{QStringLiteral("run-command")});
        state = resumed.resume(state, false);

        QCOMPARE(state.phase, AgentLoopPhase::Completed);
        QCOMPARE(executor.requests.size(), 0);
        QCOMPARE(state.steps.size(), 1);
        QVERIFY(!state.steps.first().succeeded);
        QVERIFY(state.steps.first().observation.contains(
            QStringLiteral("User denied execution in chat.")));
        QVERIFY(planner.observedHistories.at(1).first().observation.contains(
            QStringLiteral("User denied execution in chat.")));
    }

    void detectsDoomLoop() {
        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("run-command"))};
        planner.repeatLast = true;
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();

        AgentLoop loop(planner, executor, approval, sandbox,
                       QStringList{QStringLiteral("run-command")});
        const auto state = loop.run(QStringLiteral("goal"));

        QCOMPARE(state.phase, AgentLoopPhase::Stuck);
        QVERIFY(state.abortReason.contains(QStringLiteral("Doom loop")));
        QVERIFY(state.steps.size() >= 2);
    }

    void honorsCancelQueryBetweenSteps() {
        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("run-command"))};
        planner.repeatLast = true;
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();

        bool cancelNow = false;
        planner.onCall = [&cancelNow](int index) {
            if (index == 1) {
                cancelNow = true;
            }
        };

        AgentLoop loop(planner, executor, approval, sandbox,
                       QStringList{QStringLiteral("run-command")});
        loop.setCancelQuery([&cancelNow] { return cancelNow; });
        const auto state = loop.run(QStringLiteral("goal"));

        QCOMPARE(state.phase, AgentLoopPhase::Cancelled);
        QCOMPARE(state.steps.size(), 2);
        QVERIFY(state.abortReason.contains(QStringLiteral("cancelled")));
    }

    void recordsUnknownToolObservation() {
        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("warp-drive")),
                             finalDecision(QStringLiteral("recovered"))};
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();

        AgentLoop loop(planner, executor, approval, sandbox,
                       QStringList{QStringLiteral("run-command")});
        const auto state = loop.run(QStringLiteral("goal"));

        QCOMPARE(state.phase, AgentLoopPhase::Completed);
        QCOMPARE(executor.requests.size(), 0);
        QCOMPARE(state.steps.size(), 1);
        QVERIFY(state.steps.first().observation.contains(
            QStringLiteral("Unknown tool requested: warp-drive")));
    }

    void executesWithoutApprovalInAutonomousMode() {
        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("run-command"), QStringLiteral("deploy"),
                                          ToolRiskLevel::High),
                             finalDecision(QStringLiteral("deployed"))};
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();

        AgentLoop::Config config;
        config.autonomousMode = true;
        AgentLoop loop(planner, executor, approval, sandbox,
                       QStringList{QStringLiteral("run-command")}, config);
        const auto state = loop.run(QStringLiteral("deploy"));

        QCOMPARE(state.phase, AgentLoopPhase::Completed);
        QCOMPARE(executor.requests.size(), 1);
        QCOMPARE(state.steps.size(), 1);
    }

    void truncatesLargeObservations() {
        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("run-command")),
                             finalDecision(QStringLiteral("done"))};

        QStringList bigLines;
        for (int i = 0; i < 500; ++i) {
            bigLines.append(QStringLiteral("line-%1-%2").arg(i).arg(QString(96, QChar('x'))));
        }
        RecordingExecutor executor;
        executor.summary = bigLines.join(QLatin1Char('\n'));

        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();

        AgentLoop::Config config;
        config.observationMaxBytes = 4096;
        AgentLoop loop(planner, executor, approval, sandbox,
                       QStringList{QStringLiteral("run-command")}, config);
        const auto state = loop.run(QStringLiteral("goal"));

        QCOMPARE(state.phase, AgentLoopPhase::Completed);
        QVERIFY(state.steps.first().observation.toUtf8().size() < 5000);
        QVERIFY(state.steps.first().observation.contains(QStringLiteral("omitted")));
    }
};

QTEST_MAIN(AgentLoopTest)
#include "test_agent_loop.moc"
