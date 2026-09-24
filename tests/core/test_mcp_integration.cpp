// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/agent/AgentLoop.h"
#include "sentinel/core/agent/AgentRuntime.h"
#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/mcp/McpToolProvider.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/runtime/ToolHookService.h"
#include "sentinel/core/security/PermissionPolicyService.h"
#include "sentinel/core/security/StaticApprovalPolicy.h"
#include "sentinel/core/security/StaticSandboxPolicy.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace sentinel::core;

namespace {
class Planner final : public IAgentStepPlanner {
public:
    explicit Planner(const IToolRegistry& registry,
                     QString remoteName = QStringLiteral("echo_value"))
        : registry_(&registry), remoteName_(std::move(remoteName)) {}
    Planner() = default;
    void setRegistry(const IToolRegistry& registry) {
        registry_ = &registry;
    }
    mutable int calls = 0;
    mutable QString observation;
    AgentStepDecision nextStep(const QString&,
                               const QList<AgentStepRecord>& history) const override {
        ++calls;
        if (!history.isEmpty()) {
            observation = history.last().observation;
            AgentStepDecision final;
            final.kind = AgentStepDecision::Kind::FinalAnswer;
            final.answer = QStringLiteral("MCP integration completed");
            return final;
        }
        AgentStepDecision tool;
        tool.kind = AgentStepDecision::Kind::ToolCall;
        for (const auto& descriptor : registry_->enabledTools()) {
            if (descriptor.source == ToolSource::MCP && descriptor.name == remoteName_) {
                tool.toolId = descriptor.id;
                tool.toolName = descriptor.name;
                tool.riskLevel = descriptor.riskLevel;
                break;
            }
        }
        tool.arguments = {{QStringLiteral("value"), QStringLiteral("Sentinel MCP integration")}};
        return tool;
    }

private:
    const IToolRegistry* registry_ = nullptr;
    QString remoteName_ = QStringLiteral("echo_value");
};
class NoFallback final : public IToolExecutor {
public:
    ToolExecutionResult execute(const ToolExecutionRequest&) const override {
        return {ToolExecutionStatus::Blocked, QStringLiteral("fallback used")};
    }
};
QStringList calls(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
}
} // namespace

class McpIntegrationTest final : public QObject {
    Q_OBJECT
private slots:
    void realStdioAgentLoop();
    void realDenialSandboxCrashAndCancellation();
    void runtimeEventsAndCorrelation();
};

void McpIntegrationTest::realStdioAgentLoop() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto logPath = directory.filePath(QStringLiteral("calls.log"));
    auto service = std::make_shared<McpService>();
    McpServerConfig config;
    config.name = QStringLiteral("test-server");
    config.type = QStringLiteral("local");
    config.command = QString::fromUtf8(TEST_MCP_SERVER_PATH);
    config.arguments = {logPath};
    QVERIFY(service->addServer(config));
    InMemoryToolRegistry registry;
    McpToolProvider provider(service, registry);
    QVERIFY(service->connectToServer(config.name));
    QCOMPARE(service->connectionState(config.name), McpConnectionState::Connected);
    const auto id = QStringLiteral("mcp.test_2d_server.echo_5f_value");
    const auto registration = registry.findRegistration(id);
    QVERIFY(registration && registration->handler);
    QCOMPARE(registration->descriptor.source, ToolSource::MCP);
    QCOMPARE(registration->descriptor.providerId, QStringLiteral("mcp:test-server"));
    QCOMPARE(registration->descriptor.inputSchema.value(QStringLiteral("required")).toArray(),
             QJsonArray{QStringLiteral("value")});
    Planner planner(registry);
    NoFallback fallback;
    StaticApprovalPolicy approval;
    StaticSandboxPolicy sandbox(
        {QStringLiteral("tool.metadata.read"), QStringLiteral("tool.risk.medium")});
    AgentLoop loop(planner, fallback, approval, sandbox, {id});
    loop.setToolRegistry(&registry);
    ToolHookService hooks;
    ToolHook hook;
    hook.beforeExecute = [&](QJsonObject&) {
        QFile log(logPath);
        if (log.open(QIODevice::Append))
            log.write("before\n");
    };
    hook.afterExecute = [&](QJsonObject&) {
        QFile log(logPath);
        if (log.open(QIODevice::Append))
            log.write("after\n");
    };
    hooks.registerHook(id, std::move(hook));
    loop.setToolHookService(&hooks);
    QObject context;
    AgentLoopState state;
    int completions = 0;
    loop.runAsync(QStringLiteral("echo test"), QStringLiteral("mcp-session"), &context,
                  [&](const auto& result) {
                      state = result;
                      ++completions;
                  });
    QVERIFY(QTest::qWaitFor([&] { return completions == 1; }, 5000));
    QCOMPARE(state.phase, AgentLoopPhase::AwaitingApproval);
    QCOMPARE(calls(logPath).size(), 0);
    loop.resumeAsync(state, true, &context, [&](const auto& result) {
        state = result;
        ++completions;
    });
    QVERIFY(QTest::qWaitFor([&] { return completions == 2; }, 5000));
    QCOMPARE(state.phase, AgentLoopPhase::Completed);
    QCOMPARE(state.finalAnswer, QStringLiteral("MCP integration completed"));
    QCOMPARE(state.steps.size(), 1);
    QVERIFY(
        state.steps.first().observation.contains(QStringLiteral("ECHO: Sentinel MCP integration")));
    QCOMPARE(planner.observation, state.steps.first().observation);
    QCOMPARE(planner.calls, 2);
    QCOMPARE(calls(logPath),
             (QStringList{QStringLiteral("before"), QStringLiteral("echo_value"),
                          QStringLiteral("response:echo_value"), QStringLiteral("after")}));
    QVERIFY(service->disconnectFromServer(config.name));
    QVERIFY(!registry.findRegistration(id));
    QVERIFY(service->connectToServer(config.name));
    QVERIFY(registry.findRegistration(id));
    QCOMPARE(registry.enabledTools().size(), 3);
    QVERIFY(service->disconnectFromServer(config.name));
}

void McpIntegrationTest::realDenialSandboxCrashAndCancellation() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto logPath = directory.filePath(QStringLiteral("calls.log"));
    auto service = std::make_shared<McpService>();
    McpServerConfig config;
    config.name = QStringLiteral("test-server");
    config.type = QStringLiteral("local");
    config.command = QString::fromUtf8(TEST_MCP_SERVER_PATH);
    config.arguments = {logPath};
    QVERIFY(service->addServer(config));
    InMemoryToolRegistry registry;
    McpToolProvider provider(service, registry);
    QVERIFY(service->connectToServer(config.name));
    const auto id = QStringLiteral("mcp.test_2d_server.echo_5f_value");
    NoFallback fallback;
    QObject context;

    Planner deniedPlanner(registry);
    StaticApprovalPolicy denial({{id, ApprovalStatus::Denied}});
    StaticSandboxPolicy sandbox(
        {QStringLiteral("tool.metadata.read"), QStringLiteral("tool.risk.medium")});
    AgentLoop denied(deniedPlanner, fallback, denial, sandbox, {id});
    denied.setToolRegistry(&registry);
    AgentLoopState deniedState;
    bool deniedDone = false;
    denied.runAsync(QStringLiteral("deny"), {}, &context, [&](const auto& state) {
        deniedState = state;
        deniedDone = true;
    });
    QVERIFY(QTest::qWaitFor([&] { return deniedDone; }, 5000));
    QCOMPARE(deniedState.phase, AgentLoopPhase::Completed);
    QCOMPARE(calls(logPath).size(), 0);

    Planner sandboxPlanner(registry);
    StaticApprovalPolicy approval;
    StaticSandboxPolicy blockedSandbox;
    AgentLoop blocked(sandboxPlanner, fallback, approval, blockedSandbox, {id});
    blocked.setToolRegistry(&registry);
    AgentLoopState blockedState;
    int blockedCompletions = 0;
    blocked.runAsync(QStringLiteral("sandbox"), {}, &context, [&](const auto& state) {
        blockedState = state;
        ++blockedCompletions;
    });
    QVERIFY(QTest::qWaitFor([&] { return blockedCompletions == 1; }, 5000));
    QCOMPARE(blockedState.phase, AgentLoopPhase::AwaitingApproval);
    blocked.resumeAsync(blockedState, true, &context, [&](const auto& state) {
        blockedState = state;
        ++blockedCompletions;
    });
    QVERIFY(QTest::qWaitFor([&] { return blockedCompletions == 2; }, 5000));
    QVERIFY(!blockedState.steps.first().succeeded);
    QCOMPARE(calls(logPath).size(), 0);

    Planner permissionPlanner(registry);
    AgentLoop permissionDenied(permissionPlanner, fallback, approval, sandbox, {id});
    permissionDenied.setToolRegistry(&registry);
    PermissionPolicyService permissions;
    permissionDenied.setPermissionPolicy(&permissions, QStringLiteral("disabled"));
    AgentLoopState permissionState;
    int permissionCompletions = 0;
    permissionDenied.runAsync(QStringLiteral("permission"), {}, &context, [&](const auto& state) {
        permissionState = state;
        ++permissionCompletions;
    });
    QVERIFY(QTest::qWaitFor([&] { return permissionCompletions == 1; }, 5000));
    permissionDenied.resumeAsync(permissionState, true, &context, [&](const auto& state) {
        permissionState = state;
        ++permissionCompletions;
    });
    QVERIFY(QTest::qWaitFor([&] { return permissionCompletions == 2; }, 5000));
    QVERIFY(!permissionState.steps.first().succeeded);
    QVERIFY(permissionState.steps.first().observation.contains(QStringLiteral("permission")));
    QCOMPARE(calls(logPath).size(), 0);

    const auto crashId = QStringLiteral("mcp.test_2d_server.crash_5f_echo");
    const auto delayedId = QStringLiteral("mcp.test_2d_server.delayed_5f_echo");
    Planner crashPlanner(registry, QStringLiteral("crash_echo"));
    AgentLoop crashing(crashPlanner, fallback, approval, sandbox, {crashId});
    crashing.setToolRegistry(&registry);
    AgentLoopState crashedState;
    int crashCompletions = 0;
    crashing.runAsync(QStringLiteral("crash"), {}, &context, [&](const auto& state) {
        crashedState = state;
        ++crashCompletions;
    });
    QVERIFY(QTest::qWaitFor([&] { return crashCompletions == 1; }, 5000));
    crashing.resumeAsync(crashedState, true, &context, [&](const auto& state) {
        crashedState = state;
        ++crashCompletions;
    });
    QVERIFY(QTest::qWaitFor([&] { return calls(logPath).contains(QStringLiteral("crash_echo")); },
                            5000));
    QVERIFY(QTest::qWaitFor([&] { return crashCompletions == 2; }, 5000));
    QVERIFY(!crashedState.steps.first().succeeded);
    QCOMPARE(crashCompletions, 2);
    QVERIFY(!registry.findRegistration(crashId));
    QVERIFY(service->connectToServer(config.name));

    Planner cancelPlanner(registry, QStringLiteral("delayed_echo"));
    AgentLoop cancelling(cancelPlanner, fallback, approval, sandbox, {delayedId});
    cancelling.setToolRegistry(&registry);
    AgentLoopState cancelledState;
    int cancelCompletions = 0;
    cancelling.runAsync(QStringLiteral("cancel"), {}, &context, [&](const auto& state) {
        cancelledState = state;
        ++cancelCompletions;
    });
    QVERIFY(QTest::qWaitFor([&] { return cancelCompletions == 1; }, 5000));
    cancelling.resumeAsync(cancelledState, true, &context, [&](const auto& state) {
        cancelledState = state;
        ++cancelCompletions;
    });
    QVERIFY(QTest::qWaitFor(
        [&] { return calls(logPath).count(QStringLiteral("delayed_echo")) == 1; }, 5000));
    cancelling.cancelAsync();
    QVERIFY(QTest::qWaitFor([&] { return cancelCompletions == 2; }, 5000));
    QCOMPARE(cancelledState.phase, AgentLoopPhase::Cancelled);
    QVERIFY(QTest::qWaitFor(
        [&] { return calls(logPath).contains(QStringLiteral("response:delayed_echo")); }, 5000));
    QCOMPARE(cancelCompletions, 2);
    QVERIFY(service->disconnectFromServer(config.name));
}

void McpIntegrationTest::runtimeEventsAndCorrelation() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto logPath = directory.filePath(QStringLiteral("calls.log"));
    auto service = std::make_shared<McpService>();
    McpServerConfig config;
    config.name = QStringLiteral("test-server");
    config.type = QStringLiteral("local");
    config.command = QString::fromUtf8(TEST_MCP_SERVER_PATH);
    config.arguments = {logPath};
    QVERIFY(service->addServer(config));
    QVERIFY(service->connectToServer(config.name));
    RealToolExecutor executor;
    executor.setMcpService(service);
    Planner planner;
    StaticApprovalPolicy approval;
    StaticSandboxPolicy sandbox(
        {QStringLiteral("tool.metadata.read"), QStringLiteral("tool.risk.medium")});
    AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), planner, executor, approval,
                         sandbox);
    planner.setRegistry(runtime.toolRegistry());
    const auto id = QStringLiteral("mcp.test_2d_server.echo_5f_value");
    QVERIFY(runtime.toolRegistry().findRegistration(id));
    const auto session = runtime.createSession();
    AgentSessionOptions options;
    options.availableToolIds = {id};
    runtime.configureSession(session, std::move(options));
    QVERIFY(runtime.start(session, QStringLiteral("echo test")));
    QTRY_COMPARE_WITH_TIMEOUT(runtime.sessionState(session).phase, AgentLoopPhase::AwaitingApproval,
                              5000);
    QCOMPARE(calls(logPath).size(), 0);
    QVERIFY(runtime.continueSession(session, true));
    QTRY_COMPARE_WITH_TIMEOUT(runtime.sessionState(session).phase, AgentLoopPhase::Completed, 5000);
    const auto state = runtime.sessionState(session);
    QCOMPARE(state.finalAnswer, QStringLiteral("MCP integration completed"));
    QVERIFY(
        state.steps.first().observation.contains(QStringLiteral("ECHO: Sentinel MCP integration")));
    QCOMPARE(calls(logPath).count(QStringLiteral("echo_value")), 1);
    const auto events = runtime.eventHistory(session);
    QString callId, stepId, turnId;
    int requested = -1, required = -1, resolved = -1, started = -1, finished = -1;
    int stepCompleted = -1, completed = -1, terminalCount = 0;
    for (int index = 0; index < events.size(); ++index) {
        const auto& event = events.at(index);
        if (event.type == AgentEventType::ToolRequested) {
            requested = index;
            callId = event.toolCallId;
            stepId = event.stepId;
            turnId = event.turnId;
        }
        if (event.type == AgentEventType::ToolApprovalRequired)
            required = index;
        if (event.type == AgentEventType::ToolApprovalResolved)
            resolved = index;
        if (event.type == AgentEventType::ToolExecutionStarted)
            started = index;
        if (event.type == AgentEventType::ToolExecutionCompleted)
            finished = index;
        if (event.type == AgentEventType::AgentStepCompleted)
            stepCompleted = index;
        if (event.type == AgentEventType::AgentCompleted) {
            completed = index;
            ++terminalCount;
        }
        if (event.type == AgentEventType::ToolRequested ||
            event.type == AgentEventType::ToolApprovalRequired ||
            event.type == AgentEventType::ToolApprovalResolved ||
            event.type == AgentEventType::ToolExecutionStarted ||
            event.type == AgentEventType::ToolExecutionCompleted ||
            event.type == AgentEventType::AgentStepCompleted) {
            QCOMPARE(event.sessionId, session);
            if (!callId.isEmpty())
                QCOMPARE(event.toolCallId, callId);
            if (!stepId.isEmpty())
                QCOMPARE(event.stepId, stepId);
            if (!turnId.isEmpty())
                QCOMPARE(event.turnId, turnId);
        }
    }
    QVERIFY(requested >= 0 && required > requested && resolved > required && started > resolved &&
            finished > started && stepCompleted > finished && completed > stepCompleted);
    QCOMPARE(terminalCount, 1);
    QVERIFY(!callId.isEmpty() && !stepId.isEmpty() && !turnId.isEmpty());
    QVERIFY(service->disconnectFromServer(config.name));
    QVERIFY(!runtime.toolRegistry().findRegistration(id));
}

QTEST_MAIN(McpIntegrationTest)
#include "test_mcp_integration.moc"
