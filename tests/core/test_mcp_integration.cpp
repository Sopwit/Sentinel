// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/agent/AgentLoop.h"
#include "sentinel/core/agent/AgentRuntime.h"
#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/agent/ObservationPolicy.h"
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/mcp/McpToolProvider.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/runtime/ToolExecutionGateway.h"
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
class DenyApprovalPolicy final : public IApprovalPolicy {
public:
    ApprovalDecision evaluate(const ToolInvocationPlan&) const override {
        return {ApprovalStatus::Denied, QStringLiteral("Denied for this test."), {}};
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
    void certificationDirectValidationAndFaults();
    void certificationPersistenceAndOwnedShutdown();
};

void McpIntegrationTest::certificationPersistenceAndOwnedShutdown() {
    QTemporaryDir directory;
    const auto path = directory.filePath("settings.json");
    const QString json = QString::fromUtf8(
        QJsonDocument(
            QJsonObject{
                {"mcpServers",
                 QJsonObject{{"certification",
                              QJsonObject{{"command", QString::fromUtf8(TEST_MCP_SERVER_PATH)},
                                          {"args", QJsonArray{"", "--certification"}}}}}}})
            .toJson(QJsonDocument::Compact));
    QString persisted;
    {
        AppSettings settings(std::make_unique<JsonSettingsStore>(path));
        settings.setMcpServersJson(json);
        persisted = settings.mcpServersJson();
    }
    AppSettings restored(std::make_unique<JsonSettingsStore>(path));
    QCOMPARE(restored.mcpServersJson(), persisted);
    const auto configJson = QJsonDocument::fromJson(restored.mcpServersJson().toUtf8())
                                .object()
                                .value("mcpServers")
                                .toObject()
                                .value("certification")
                                .toObject();
    McpServerConfig config;
    config.name = "certification";
    config.type = "local";
    config.command = configJson.value("command").toString();
    for (const auto& argument : configJson.value("args").toArray())
        config.arguments.append(argument.toString());
    QPointer<QProcess> process;
    {
        auto service = std::make_shared<McpService>();
        QVERIFY(service->addServer(config));
        QVERIFY(service->connectToAll());
        QCOMPARE(service->tools().size(), 8);
        const auto children = service->findChildren<QProcess*>();
        QVERIFY(!children.isEmpty());
        process = children.first();
        QCOMPARE(process->state(), QProcess::Running);
    }
    QVERIFY(process.isNull());
    auto restarted = std::make_shared<McpService>();
    QVERIFY(restarted->addServer(config));
    QVERIFY(restarted->connectToAll());
    QCOMPARE(restarted->tools().size(), 8);
    restarted->disconnectFromAll();
    QCOMPARE(restarted->connectionState(config.name), McpConnectionState::Disconnected);
    QVERIFY(restarted->tools().isEmpty());
}

void McpIntegrationTest::certificationDirectValidationAndFaults() {
    auto service = std::make_shared<McpService>();
    McpServerConfig config;
    config.name = "certification";
    config.type = "local";
    config.command = QString::fromUtf8(TEST_MCP_SERVER_PATH);
    config.arguments = {"", "--certification"};
    QVERIFY(service->addServer(config));
    QVERIFY(!service->addServer(config));
    InMemoryToolRegistry registry;
    McpToolProvider provider(service, registry);
    QVERIFY2(service->connectToServer(config.name), qPrintable(service->lastError(config.name)));
    QCOMPARE(registry.enabledTools().size(), 8);
    const auto initial = registry.enabledTools();
    QVERIFY(service->refreshTools(config.name));
    QVERIFY(provider.refresh(config.name));
    QCOMPARE(registry.enabledTools().size(), 8);
    for (const auto& descriptor : initial)
        QVERIFY(registry.findRegistration(descriptor.id));
    NoFallback fallback;
    ToolExecutionGateway gateway(&registry);
    PermissionPolicyService permissions;
    gateway.setPermissionPolicy(&permissions, "enabled");
    auto execute = [&](const QString& id, const QJsonObject& arguments,
                       ApprovalStatus approval = ApprovalStatus::Approved) {
        ToolExecutionRequest request;
        request.plan.status = ToolInvocationPlanStatus::Planned;
        PlannedToolInvocation invocation;
        invocation.toolId = id;
        for (auto it = arguments.begin(); it != arguments.end(); ++it)
            invocation.arguments.append({it.key(), {}, it.value()});
        request.plan.invocations = {invocation};
        request.approval.status = approval;
        request.sandbox.status = SandboxStatus::Allowed;
        ToolExecutionResult result;
        int callbacks = 0;
        gateway.executeAsync(request, fallback, {}, {}, {}, [&](auto value) {
            result = std::move(value);
            ++callbacks;
        });
        const bool done = QTest::qWaitFor([&] { return callbacks == 1; }, 35000);
        if (!done)
            QTest::qFail("MCP gateway did not settle", __FILE__, __LINE__);
        return result;
    };
    const QString echo = "mcp.certification.echo_5f_value";
    auto result = execute(echo, {{"value", "MERHABA"}});
    QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
    QVERIFY(result.summary.contains("ECHO: MERHABA"));
    result = execute("mcp.certification.add", {{"a", 2}, {"b", 3}});
    QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
    QVERIFY(result.structuredObservation);
    QCOMPARE(result.structuredObservation->data.value("sum").toDouble(), 5.0);
    const auto descriptor = registry.findRegistration("mcp.certification.add")->descriptor;
    PlannedToolInvocation invocation;
    invocation.toolId = descriptor.id;
    const auto evidence =
        EvidencePolicy::record(descriptor, invocation, result.status, result.summary, 1,
                               "real-call", result.structuredObservation);
    ObservationIntent intent;
    intent.requirements = {{ObservationDomain::ExternalService, "certification",
                            EvidenceFreshness::TurnScoped, ObservationPurpose::Inspect}};
    QVERIFY(EvidencePolicy::evaluate(intent, evidence, GroundingMode::Verified, true).accepted);
    intent.requirements = {{ObservationDomain::ExternalService,
                            {},
                            EvidenceFreshness::TurnScoped,
                            ObservationPurpose::Operate}};
    QVERIFY(EvidencePolicy::evaluate(intent, evidence, GroundingMode::Verified, true).accepted);
    intent.requirements = {{ObservationDomain::FileSystem,
                            {},
                            EvidenceFreshness::TurnScoped,
                            ObservationPurpose::Inspect}};
    QVERIFY(!EvidencePolicy::evaluate(intent, evidence, GroundingMode::Verified, true).accepted);
    intent.requirements = {{ObservationDomain::ExternalService, "certification",
                            EvidenceFreshness::TurnScoped, ObservationPurpose::Inspect}};
    for (const auto status : {ToolExecutionStatus::Failed, ToolExecutionStatus::Blocked,
                              ToolExecutionStatus::Cancelled})
        QVERIFY(!EvidencePolicy::evaluate(intent,
                                          EvidencePolicy::record(descriptor, invocation, status,
                                                                 "failure", 1, "failed-call"),
                                          GroundingMode::Verified, true)
                     .accepted);
    for (const auto& invalid :
         QList<QJsonObject>{{}, {{"value", 3}}, {{"value", "MERHABA"}, {"unknown", true}}})
        QCOMPARE(execute(echo, invalid).status, ToolExecutionStatus::InvalidArguments);
    QCOMPARE(execute(echo, {{"value", "MERHABA"}}, ApprovalStatus::Denied).status,
             ToolExecutionStatus::Blocked);
    gateway.setPermissionPolicy(&permissions, "disabled");
    QCOMPARE(execute(echo, {{"value", "MERHABA"}}).status, ToolExecutionStatus::Blocked);
    gateway.setPermissionPolicy(&permissions, "enabled");
    for (const auto& name :
         QStringList{"malformed_5f_json", "missing_5f_result", "wrong_5f_id", "server_5f_error"}) {
        if (service->connectionState(config.name) != McpConnectionState::Connected)
            QVERIFY(service->connectToServer(config.name));
        result = execute("mcp.certification." + name, {{"value", "fault"}});
        QVERIFY(result.status != ToolExecutionStatus::Succeeded);
        QCOMPARE(result.failureCategory, name == "wrong_5f_id" ? ToolFailureCategory::Timeout
                                         : name == "server_5f_error"
                                             ? ToolFailureCategory::RemoteExecutionFailure
                                             : ToolFailureCategory::ProtocolError);
    }
    QVERIFY(service->disconnectFromServer(config.name));
    QVERIFY(registry.enabledTools().isEmpty());
    QVERIFY(service->connectToServer(config.name));
    QCOMPARE(registry.enabledTools().size(), 8);
    QVERIFY(service->disconnectFromServer(config.name));
    McpServerConfig unavailable = config;
    unavailable.name = "unavailable";
    unavailable.command = "/definitely-unavailable-sentinel-mcp";
    QVERIFY(service->addServer(unavailable));
    QVERIFY(!service->connectToServer(unavailable.name));
    QVERIFY(service->connectionState(unavailable.name) != McpConnectionState::Connected);
    QVERIFY(!service->lastError(unavailable.name).isEmpty());
}

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
    QVERIFY2(service->connectToServer(config.name), qPrintable(service->lastError(config.name)));
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
    // The child may not write the parent's fixture directory. Its successful
    // echo observation proves execution; only in-process hooks can log here.
    QCOMPARE(calls(logPath), (QStringList{QStringLiteral("before"), QStringLiteral("after")}));
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
    DenyApprovalPolicy denial;
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
    QCOMPARE(deniedState.phase, AgentLoopPhase::Failed);
    QCOMPARE(deniedState.terminalReason, AgentTerminalReason::SecurityDenied);
    QCOMPARE(calls(logPath).size(), 0);

    Planner sandboxPlanner(registry);
    StaticApprovalPolicy approval;
    StaticSandboxPolicy blockedSandbox(QSet<QString>{});
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
    QCOMPARE(permissionState.phase, AgentLoopPhase::Failed);
    QCOMPARE(permissionState.terminalReason, AgentTerminalReason::SecurityDenied);
    QCOMPARE(permissionCompletions, 1);
    QVERIFY(!permissionState.steps.first().succeeded);
    QVERIFY(permissionState.steps.first().observation.contains(QStringLiteral("authorization")));
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
    QVERIFY(QTest::qWaitFor(
        [&] { return service->connectionState(config.name) != McpConnectionState::Connected; },
        5000));
    QVERIFY(QTest::qWaitFor([&] { return crashCompletions == 2; }, 5000));
    QVERIFY(!crashedState.steps.first().succeeded);
    QCOMPARE(crashCompletions, 2);
    QVERIFY(!registry.findRegistration(crashId));
    QVERIFY(service->connectToServer(config.name));

    Planner cancelPlanner(registry, QStringLiteral("delayed_echo"));
    AgentLoop cancelling(cancelPlanner, fallback, approval, sandbox, {delayedId});
    cancelling.setToolRegistry(&registry);
    bool delayedStarted = false;
    cancelling.setToolCallback([&](AgentLoop::ToolTransition transition, int,
                                   const ToolInvocationPlan&, const AgentStepRecord*) {
        delayedStarted |= transition == AgentLoop::ToolTransition::ExecutionStarted;
    });
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
    QVERIFY(QTest::qWaitFor([&] { return delayedStarted; }, 5000));
    cancelling.cancelAsync();
    QVERIFY(QTest::qWaitFor([&] { return cancelCompletions == 2; }, 5000));
    QCOMPARE(cancelledState.phase, AgentLoopPhase::Cancelled);
    // The fixture answers after 400 ms even when its client cancels. A late
    // response must not publish another terminal result.
    QTest::qWait(600);
    QVERIFY(calls(logPath).isEmpty());
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
    QVERIFY(calls(logPath).isEmpty()); // Child filesystem confinement remains enforced.
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
