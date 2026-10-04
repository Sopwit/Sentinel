// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "sentinel/core/agent/AgentLoop.h"
#include "sentinel/core/agent/ClaimGroundingResolver.h"
#include "sentinel/core/agent/ObservationPolicy.h"
#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/security/StaticApprovalPolicy.h"
#include "sentinel/core/security/StaticSandboxPolicy.h"
#include <QFile>

#include <QFileInfo>
#include <QTemporaryDir>
#include <functional>

using namespace sentinel::core;

namespace {
class CallbackIntentPolicy final : public IObservationIntentPolicy {
public:
    std::function<void()> onClassify;
    ObservationIntent classify(const QString&, const QString&,
                               const QList<ToolDescriptor>&) const override {
        onClassify();
        return {};
    }
};

class FixedFilesystemIntent final : public IObservationIntentPolicy {
public:
    ObservationIntent intent;
    ObservationIntent classify(const QString&, const QString&,
                               const QList<ToolDescriptor>&) const override {
        return intent;
    }
};

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

class RecordingHandler final : public IToolHandler {
public:
    QList<ToolExecutionRequest> requests;

    IToolExecutor::Cancel execute(const ToolExecutionRequest& request, const QString&,
                                  const QString&, IToolExecutor::Output,
                                  IToolExecutor::Completion completion) override {
        requests.append(request);
        completion({ToolExecutionStatus::Succeeded, QStringLiteral("observation-from-handler")});
        return {};
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
    void cancellationDuringInitialClassificationIsTerminalOnce() {
        ScriptedPlanner planner;
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();
        AgentLoop loop(planner, executor, approval, sandbox, QStringList{});
        auto policy = std::make_shared<CallbackIntentPolicy>();
        policy->onClassify = [&] { loop.cancelAsync(); };
        loop.setObservationIntentPolicy(policy);
        int terminals = 0;
        loop.runAsync(QStringLiteral("List the workspace"), QStringLiteral("initial-cancel"), this,
                      [&](const AgentLoopState& state) {
                          ++terminals;
                          QCOMPARE(state.phase, AgentLoopPhase::Cancelled);
                      });
        QTest::qWait(25);
        QCOMPARE(terminals, 1);
        QCOMPARE(planner.calls, 0);
        QVERIFY(executor.requests.isEmpty());
    }

    void hiddenNegativeClaimReproduction_data() {
        QTest::addColumn<QString>("finalText");
        QTest::addColumn<bool>("contextMode");
        QTest::newRow("original") << QString("No hidden files were found.") << false;
        QTest::newRow("universal") << QString("There are no hidden files.") << false;
        QTest::newRow("dotfiles") << QString("No dotfiles were found.") << false;
        QTest::newRow("context-bypass") << QString("No hidden files exist.") << true;
    }
    void hiddenNegativeClaimReproduction() {
        QFETCH(QString, finalText);
        QFETCH(bool, contextMode);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        QDir root(workspace.path());
        QVERIFY(root.mkpath("normal"));
        QVERIFY(root.mkpath(".hidden-dir"));
        for (const auto& name :
             {"visible.txt", ".hidden.txt", "normal/file.txt", ".hidden-dir/nested.txt"}) {
            QFile file(root.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("fixture");
        }
        ScriptedPlanner planner;
        auto invocation = toolDecision("list-directory");
        invocation.arguments = {{"path", workspace.path()}};
        auto final = finalDecision(finalText);
        final.grounding = contextMode ? GroundingMode::Context : GroundingMode::Verified;
        final.groundingDeclared = true;
        planner.decisions = {invocation, final};
        planner.repeatLast = true;
        RealToolExecutor executor;
        InMemoryToolRegistry registry;
        QVERIFY(BuiltInToolProvider::registerTools(registry, executor));
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();
        AgentLoop loop(planner, executor, approval, sandbox, {"list-directory"});
        loop.setToolRegistry(&registry);
        loop.setResourceScope(workspace.path());
        AgentContextInput::WorkspaceContext context;
        context.rootPath = workspace.path();
        loop.setWorkspaceContext(context);
        const auto state = loop.run("List the workspace");
        QVERIFY(!state.evidence.isEmpty());
        const auto observation = state.evidence.first().structuredObservation;
        QVERIFY(observation);
        QCOMPARE(observation->kind, StructuredObservationKind::DirectoryListing);
        QCOMPARE(observation->data.value("includeHidden").toBool(), false);
        QVERIFY(QFileInfo::exists(root.filePath(".hidden.txt")));
        QVERIFY(state.phase != AgentLoopPhase::Completed);
        QVERIFY(state.finalAnswer != finalText);
    }
    void typedHiddenAbsenceThroughGateway_data() {
        QTest::addColumn<bool>("hiddenExists");
        QTest::addColumn<bool>("includeHidden");
        QTest::addColumn<bool>("accepted");
        QTest::newRow("A-hidden-excluded") << true << false << false;
        QTest::newRow("B-hidden-included-present") << true << true << false;
        QTest::newRow("C-included-complete-absent") << false << true << true;
    }
    void typedHiddenAbsenceThroughGateway() {
        QFETCH(bool, hiddenExists);
        QFETCH(bool, includeHidden);
        QFETCH(bool, accepted);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        if (hiddenExists) {
            QFile file(QDir(workspace.path()).filePath(".hidden.txt"));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("hidden");
        }
        auto policy = std::make_shared<FixedFilesystemIntent>();
        ObservationRequirement claim;
        claim.domain = ObservationDomain::FileSystem;
        claim.claimType = ClaimType::HiddenEntriesExist;
        claim.claimId = "hidden";
        claim.resourceHint = QFileInfo(workspace.path()).canonicalFilePath();
        policy->intent.requirements.append(claim);
        ScriptedPlanner planner;
        auto tool = toolDecision("list-directory");
        tool.arguments = {{"path", workspace.path()},
                          {"includeHidden", includeHidden ? "true" : "false"}};
        tool.arguments.last().jsonValue = QJsonValue(includeHidden);
        auto final = finalDecision("No hidden files exist.");
        final.grounding = GroundingMode::Verified;
        final.groundingDeclared = true;
        final.claims.append({claim.claimId, false});
        planner.decisions = {tool, final};
        planner.repeatLast = true;
        planner.onCall = [&](int index) {
            if (index == 0)
                return;
            const auto history = planner.observedHistories.last();
            QVERIFY(!history.isEmpty());
            EvidenceRecord evidence;
            evidence.structuredObservation = history.first().structuredObservation;
            evidence.domain = ObservationDomain::FileSystem;
            evidence.outcome = EvidenceOutcome::Verified;
            evidence.toolCallId = "observed";
            if (const auto representation = ClaimGroundingResolver::filesystemFinalAnswer(
                    policy->intent, {evidence}, final.answer))
                planner.decisions[1].answer = *representation;
        };
        RealToolExecutor executor;
        InMemoryToolRegistry registry;
        QVERIFY(BuiltInToolProvider::registerTools(registry, executor));
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();
        AgentLoop loop(planner, executor, approval, sandbox, {"list-directory"});
        loop.setToolRegistry(&registry);
        loop.setResourceScope(workspace.path());
        loop.setObservationIntentPolicy(policy);
        AgentContextInput::WorkspaceContext context;
        context.rootPath = workspace.path();
        loop.setWorkspaceContext(context);
        const auto state = loop.run("Are there no hidden entries in this exact directory?");
        QCOMPARE(state.phase == AgentLoopPhase::Completed, accepted);
        QVERIFY(!state.evidence.isEmpty());
        if (accepted) {
            QCOMPARE(state.finalClaims.size(), 1);
            QCOMPARE(state.finalClaims.first().value, false);
            QVERIFY(state.finalAnswer.contains("not found"));
        }
    }
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

    void bindsFilesystemAuthorizationToConfiguredWorkspaceRoot() {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto descriptors = BuiltInToolProvider::descriptors();
        const auto descriptor =
            std::find_if(descriptors.cbegin(), descriptors.cend(), [](const ToolDescriptor& item) {
                return item.id == QLatin1String("list-directory");
            });
        QVERIFY(descriptor != descriptors.cend());

        auto handler = std::make_shared<RecordingHandler>();
        InMemoryToolRegistry registry;
        QVERIFY(registry.registerTool({*descriptor, handler}));
        ScriptedPlanner planner;
        auto list = toolDecision(QStringLiteral("list-directory"), QStringLiteral("."));
        list.arguments.first().id = QStringLiteral("path");
        EvidenceRecord observation;
        observation.domain = ObservationDomain::FileSystem;
        observation.toolId = QStringLiteral("list-directory");
        observation.resource = QStringLiteral(".");
        observation.outcome = EvidenceOutcome::Verified;
        const auto representation = ClaimGroundingResolver::filesystemFinalAnswer(
            {}, {observation}, QStringLiteral("done"));
        QVERIFY(representation);
        planner.decisions = {list, finalDecision(*representation)};
        RecordingExecutor executor;
        StaticApprovalPolicy approval;
        auto sandbox = permissiveSandbox();

        AgentLoop loop(planner, executor, approval, sandbox,
                       QStringList{QStringLiteral("list-directory")});
        loop.setToolRegistry(&registry);
        AgentContextInput::WorkspaceContext context;
        context.id = QStringLiteral("workspace");
        context.rootPath = workspace.path();
        loop.setWorkspaceContext(context);
        const auto state = loop.run(QStringLiteral("list the workspace"));

        QCOMPARE(state.phase, AgentLoopPhase::Completed);
        QCOMPARE(handler->requests.size(), 1);
        QVERIFY(handler->requests.first().plan.invocations.first().resourceSnapshot);
        QCOMPARE(
            handler->requests.first().plan.invocations.first().resourceSnapshot->workingDirectory,
            QFileInfo(workspace.path()).canonicalFilePath());
    }

    void stopsAtIterationLimit() {
        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("run-command"), QStringLiteral("one")),
                             toolDecision(QStringLiteral("run-command"), QStringLiteral("two")),
                             toolDecision(QStringLiteral("run-command"), QStringLiteral("three"))};
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

        QCOMPARE(state.phase, AgentLoopPhase::Failed);
        QCOMPARE(executor.requests.size(), 0);
        QCOMPARE(state.steps.size(), 1);
        QVERIFY(!state.steps.first().succeeded);
        QVERIFY(state.steps.first().observation.contains(
            QStringLiteral("User denied execution in chat.")));
        QVERIFY(state.abortReason.contains(QStringLiteral("User denied")));
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
        QVERIFY(state.abortReason.contains(QStringLiteral("repeated the same")));
        QCOMPARE(state.steps.size(), 1);
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
        QCOMPARE(state.steps.size(), 1);
        QCOMPARE(executor.requests.size(), 1);
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
