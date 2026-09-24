// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/agent/AgentLoop.h"
#include "sentinel/core/agent/AgentRuntime.h"
#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/plugin/PluginManager.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/runtime/ToolExecutionGateway.h"
#include "sentinel/core/runtime/ToolHookService.h"
#include "sentinel/core/security/StaticApprovalPolicy.h"
#include "sentinel/core/security/StaticSandboxPolicy.h"
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

using namespace sentinel::core;
using namespace sentinel::core::plugin;

namespace {
class Planner final : public IAgentStepPlanner {
public:
    explicit Planner(const IToolRegistry& registry) : registry_(&registry) {}
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
            AgentStepDecision answer;
            answer.kind = AgentStepDecision::Kind::FinalAnswer;
            answer.answer = QStringLiteral("Plugin integration completed");
            return answer;
        }
        AgentStepDecision tool;
        tool.kind = AgentStepDecision::Kind::ToolCall;
        for (const auto& descriptor : registry_->enabledTools()) {
            if (descriptor.source == ToolSource::Plugin &&
                descriptor.name == QStringLiteral("Echo")) {
                tool.toolId = descriptor.id;
                tool.toolName = descriptor.name;
                tool.riskLevel = descriptor.riskLevel;
                break;
            }
        }
        tool.arguments = {{QStringLiteral("text"), QStringLiteral("Sentinel Plugin integration")}};
        return tool;
    }

private:
    const IToolRegistry* registry_ = nullptr;
};
class NoFallback final : public IToolExecutor {
public:
    ToolExecutionResult execute(const ToolExecutionRequest&) const override {
        return {ToolExecutionStatus::Blocked, QStringLiteral("fallback used")};
    }
};
class FixedHandler final : public IToolHandler {
public:
    IToolExecutor::Cancel execute(const ToolExecutionRequest&, const QString&, const QString&,
                                  IToolExecutor::Output,
                                  IToolExecutor::Completion completion) override {
        completion({ToolExecutionStatus::Succeeded, QStringLiteral("fixed")});
        return {};
    }
};
ToolExecutionRequest requestFor(const QString& id) {
    ToolExecutionRequest request;
    request.plan.status = ToolInvocationPlanStatus::Planned;
    PlannedToolInvocation invocation;
    invocation.toolId = id;
    invocation.arguments = {{QStringLiteral("text"), QStringLiteral("active")}};
    request.plan.invocations = {invocation};
    request.approval.status = ApprovalStatus::Approved;
    request.sandbox.status = SandboxStatus::Allowed;
    return request;
}
} // namespace

class PluginToolIntegrationTest final : public QObject {
    Q_OBJECT
private slots:
    void loadedPluginExecutesAndUnloadsSafely();
    void failedReloadLeavesNoTool();
    void runtimeEventsAndCorrelation();
    void permissionDeniedAtRegistrationAndShutdownCleanup();
};

void PluginToolIntegrationTest::loadedPluginExecutesAndUnloadsSafely() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto pluginId = QStringLiteral("dev.sentinel.plugin.custom-tool");
    const auto echoId = QStringLiteral("plugin.dev_2e_sentinel_2e_plugin_2e_custom_2d_tool.echo");
    const auto delayedId =
        QStringLiteral("plugin.dev_2e_sentinel_2e_plugin_2e_custom_2d_tool.delayed_5f_echo");
    const auto secondPluginId = QStringLiteral("dev.sentinel.plugin.second-tool");
    const auto secondEchoId =
        QStringLiteral("plugin.dev_2e_sentinel_2e_plugin_2e_second_2d_tool.echo");
    InMemoryToolRegistry registry;
    ToolDescriptor builtIn;
    builtIn.id = QStringLiteral("read-file");
    builtIn.source = ToolSource::BuiltIn;
    builtIn.providerId = QStringLiteral("sentinel:built-in");
    QVERIFY(registry.registerTool({builtIn, std::make_shared<FixedHandler>()}));
    PluginManager manager(QStringLiteral("1.0.0"), directory.path());
    manager.setToolRegistry(&registry);
    QVERIFY(manager.discoverPlugins(
                QFileInfo(QString::fromUtf8(TEST_PLUGIN_PATH)).absolutePath()) >= 2);
    QVERIFY(manager.initializePlugin(pluginId));
    QVERIFY(manager.startPlugin(pluginId));
    QVERIFY(manager.initializePlugin(secondPluginId));
    QVERIFY(manager.startPlugin(secondPluginId));
    QVERIFY(registry.findRegistration(secondEchoId));
    ToolDescriptor colliding;
    colliding.id = QStringLiteral("read-file");
    colliding.providerId = QStringLiteral("forged-provider");
    colliding.inputSchema = QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
                                        {QStringLiteral("additionalProperties"), false}};
    QVERIFY(manager.descriptor(pluginId)->context->registerTool(colliding,
                                                                std::make_shared<FixedHandler>()));
    QVERIFY(registry.findRegistration(
        QStringLiteral("plugin.dev_2e_sentinel_2e_plugin_2e_custom_2d_tool.read_2d_file")));
    QCOMPARE(registry.findRegistration(QStringLiteral("read-file"))->descriptor.source,
             ToolSource::BuiltIn);
    QVERIFY(!manager.descriptor(pluginId)->context->registerTool(colliding,
                                                                 std::make_shared<FixedHandler>()));
    auto registration = registry.findRegistration(echoId);
    QVERIFY(registration && registration->handler);
    QCOMPARE(registration->descriptor.source, ToolSource::Plugin);
    QCOMPARE(registration->descriptor.providerId, QStringLiteral("plugin:") + pluginId);
    QCOMPARE(registration->descriptor.version, QStringLiteral("1.0.0"));
    QCOMPARE(registration->descriptor.inputSchema.value(QStringLiteral("required")).toArray(),
             QJsonArray{QStringLiteral("text")});
    QVERIFY(registry.findRegistration(delayedId));
    Planner planner(registry);
    NoFallback fallback;
    StaticApprovalPolicy approval;
    StaticSandboxPolicy sandbox(
        {QStringLiteral("tool.metadata.read"), QStringLiteral("tool.risk.medium")});
    AgentLoop loop(planner, fallback, approval, sandbox, {echoId});
    loop.setToolRegistry(&registry);
    ToolHookService hooks;
    QStringList order;
    ToolHook hook;
    hook.beforeExecute = [&](QJsonObject&) { order.append(QStringLiteral("before")); };
    hook.afterExecute = [&](QJsonObject&) { order.append(QStringLiteral("after")); };
    hooks.registerHook(echoId, std::move(hook));
    loop.setToolHookService(&hooks);
    QObject context;
    AgentLoopState state;
    int completions = 0;
    loop.runAsync(QStringLiteral("echo"), {}, &context, [&](const auto& result) {
        state = result;
        ++completions;
    });
    QVERIFY(QTest::qWaitFor([&] { return completions == 1; }, 5000));
    QCOMPARE(state.phase, AgentLoopPhase::AwaitingApproval);
    loop.resumeAsync(state, true, &context, [&](const auto& result) {
        state = result;
        ++completions;
    });
    QVERIFY(QTest::qWaitFor([&] { return completions == 2; }, 5000));
    QCOMPARE(state.phase, AgentLoopPhase::Completed);
    QVERIFY(state.steps.first().observation.contains(
        QStringLiteral("PLUGIN ECHO: Sentinel Plugin integration")));
    QCOMPARE(planner.observation, state.steps.first().observation);
    QCOMPARE(planner.calls, 2);
    QCOMPARE(order, (QStringList{QStringLiteral("before"), QStringLiteral("after")}));

    AgentLoopState deniedState;
    int deniedCompletions = 0;
    loop.runAsync(QStringLiteral("echo"), {}, &context, [&](const auto& result) {
        deniedState = result;
        ++deniedCompletions;
    });
    QVERIFY(QTest::qWaitFor([&] { return deniedCompletions == 1; }, 5000));
    QCOMPARE(deniedState.phase, AgentLoopPhase::AwaitingApproval);
    loop.resumeAsync(deniedState, false, &context, [&](const auto& result) {
        deniedState = result;
        ++deniedCompletions;
    });
    QVERIFY(QTest::qWaitFor([&] { return deniedCompletions == 2; }, 5000));
    QVERIFY(!deniedState.steps.isEmpty());
    QCOMPARE(order, (QStringList{QStringLiteral("before"), QStringLiteral("after")}));

    ToolExecutionGateway gateway(&registry);
    auto echo = requestFor(echoId);
    manager.sandbox().revokePermission(pluginId, Permissions::ToolExecution);
    ToolExecutionResult denied;
    gateway.executeAsync(echo, fallback, {}, {}, {}, [&](auto result) { denied = result; });
    QCOMPARE(denied.status, ToolExecutionStatus::Blocked);
    manager.sandbox().grantPermission(pluginId, Permissions::ToolExecution);
    ToolExecutionResult allowed;
    gateway.executeAsync(echo, fallback, {}, {}, {}, [&](auto result) { allowed = result; });
    QCOMPARE(allowed.status, ToolExecutionStatus::Succeeded);
    auto delayed = requestFor(delayedId);
    auto loadedLibrary = manager.descriptor(pluginId)->loader;
    QVERIFY(loadedLibrary && loadedLibrary->isLoaded());
    int activeCompletions = 0;
    ToolExecutionResult activeResult;
    auto cancel = gateway.executeAsync(delayed, fallback, {}, {}, {}, [&](auto result) {
        activeResult = result;
        ++activeCompletions;
    });
    QCOMPARE(activeCompletions, 0);
    QVERIFY(manager.unloadPlugin(pluginId));
    QVERIFY(!registry.findRegistration(echoId));
    QVERIFY(!registry.findRegistration(delayedId));
    QVERIFY(registry.findRegistration(secondEchoId));
    QVERIFY(manager.isModuleResident(pluginId));
    QVERIFY(loadedLibrary->isLoaded());
    ToolExecutionResult stale;
    gateway.executeAsync(delayed, fallback, {}, {}, {}, [&](auto result) { stale = result; });
    QCOMPARE(stale.status, ToolExecutionStatus::UnknownTool);
    QVERIFY(QTest::qWaitFor([&] { return activeCompletions == 1; }, 5000));
    QCOMPARE(activeResult.status, ToolExecutionStatus::Succeeded);
    cancel = {};
    registration.reset();
    QVERIFY(QTest::qWaitFor([&] { return !manager.isModuleResident(pluginId); }, 5000));
    QVERIFY(QTest::qWaitFor([&] { return !loadedLibrary->isLoaded(); }, 5000));
    QVERIFY(manager.initializePlugin(pluginId));
    QVERIFY(manager.startPlugin(pluginId));
    QVERIFY(manager.reloadPlugin(pluginId));
    QVERIFY(QTest::qWaitFor([&] { return bool(registry.findRegistration(echoId)); }, 5000));
    QVERIFY(manager.unloadPlugin(pluginId));
    QVERIFY(registry.findRegistration(secondEchoId));
    QVERIFY(manager.unloadPlugin(secondPluginId));
}

void PluginToolIntegrationTest::failedReloadLeavesNoTool() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto copiedPlugin =
        directory.filePath(QFileInfo(QString::fromUtf8(TEST_PLUGIN_PATH)).fileName());
    QVERIFY(QFile::copy(QString::fromUtf8(TEST_PLUGIN_PATH), copiedPlugin));
    InMemoryToolRegistry registry;
    PluginManager manager(QStringLiteral("1.0.0"), directory.path());
    manager.setToolRegistry(&registry);
    const auto pluginId = QStringLiteral("dev.sentinel.plugin.custom-tool");
    const auto echoId = QStringLiteral("plugin.dev_2e_sentinel_2e_plugin_2e_custom_2d_tool.echo");
    QVERIFY(manager.discoverPlugins(directory.path()) >= 1);
    QVERIFY(manager.startPlugin(pluginId));
    QVERIFY(registry.findRegistration(echoId));
    QSignalSpy failed(&manager, &PluginManager::pluginReloadFailed);
    QVERIFY(QFile::remove(copiedPlugin));
    QVERIFY(manager.reloadPlugin(pluginId));
    QVERIFY(!registry.findRegistration(echoId));
    QVERIFY(QTest::qWaitFor([&] { return failed.size() == 1; }, 5000));
    QVERIFY(!registry.findRegistration(echoId));
    QCOMPARE(manager.pluginState(pluginId), PluginState::Error);
}

void PluginToolIntegrationTest::runtimeEventsAndCorrelation() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    RealToolExecutor executor;
    Planner planner;
    StaticApprovalPolicy approval;
    StaticSandboxPolicy sandbox(
        {QStringLiteral("tool.metadata.read"), QStringLiteral("tool.risk.medium")});
    AgentRuntime runtime(std::make_unique<NullAgentRuntime>(), &planner, executor, approval,
                         sandbox);
    planner.setRegistry(runtime.toolRegistry());
    auto& manager = runtime.pluginManager();
    manager.setPluginStorageDir(directory.path());
    const auto pluginId = QStringLiteral("dev.sentinel.plugin.custom-tool");
    const auto echoId = QStringLiteral("plugin.dev_2e_sentinel_2e_plugin_2e_custom_2d_tool.echo");
    QVERIFY(manager.discoverPlugins(
                QFileInfo(QString::fromUtf8(TEST_PLUGIN_PATH)).absolutePath()) >= 1);
    QVERIFY(manager.startPlugin(pluginId));
    const auto session = runtime.createSession();
    AgentSessionOptions options;
    options.availableToolIds = {echoId};
    runtime.configureSession(session, std::move(options));
    QVERIFY(runtime.start(session, QStringLiteral("echo test")));
    QTRY_COMPARE_WITH_TIMEOUT(runtime.sessionState(session).phase, AgentLoopPhase::AwaitingApproval,
                              5000);
    QVERIFY(runtime.continueSession(session, true));
    QTRY_COMPARE_WITH_TIMEOUT(runtime.sessionState(session).phase, AgentLoopPhase::Completed, 5000);
    const auto events = runtime.eventHistory(session);
    QList<AgentEventType> types;
    QString callId, stepId, turnId;
    for (const auto& event : events) {
        types.append(event.type);
        if (event.type == AgentEventType::ToolRequested) {
            callId = event.toolCallId;
            stepId = event.stepId;
            turnId = event.turnId;
            QCOMPARE(std::get<AgentToolEvent>(event.payload).toolId, echoId);
        }
        if (event.type == AgentEventType::ToolApprovalRequired ||
            event.type == AgentEventType::ToolApprovalResolved ||
            event.type == AgentEventType::ToolExecutionStarted ||
            event.type == AgentEventType::ToolExecutionCompleted ||
            event.type == AgentEventType::AgentStepCompleted) {
            QCOMPARE(event.sessionId, session);
            QCOMPARE(event.toolCallId, callId);
            QCOMPARE(event.stepId, stepId);
            QCOMPARE(event.turnId, turnId);
        }
    }
    QVERIFY(!callId.isEmpty() && !stepId.isEmpty() && !turnId.isEmpty());
    const auto index = [&](AgentEventType type) { return types.indexOf(type); };
    QVERIFY(index(AgentEventType::ToolRequested) >= 0);
    QVERIFY(index(AgentEventType::ToolApprovalRequired) > index(AgentEventType::ToolRequested));
    QVERIFY(index(AgentEventType::ToolApprovalResolved) >
            index(AgentEventType::ToolApprovalRequired));
    QVERIFY(index(AgentEventType::ToolExecutionStarted) >
            index(AgentEventType::ToolApprovalResolved));
    QVERIFY(index(AgentEventType::ToolExecutionCompleted) >
            index(AgentEventType::ToolExecutionStarted));
    QVERIFY(index(AgentEventType::AgentStepCompleted) >
            index(AgentEventType::ToolExecutionCompleted));
    QVERIFY(runtime.sessionState(session).steps.first().observation.contains(
        QStringLiteral("PLUGIN ECHO: Sentinel Plugin integration")));
}

void PluginToolIntegrationTest::permissionDeniedAtRegistrationAndShutdownCleanup() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto pluginId = QStringLiteral("dev.sentinel.plugin.custom-tool");
    const auto echoId = QStringLiteral("plugin.dev_2e_sentinel_2e_plugin_2e_custom_2d_tool.echo");
    InMemoryToolRegistry registry;
    {
        PluginManager manager(QStringLiteral("1.0.0"), directory.path());
        manager.setToolRegistry(&registry);
        QVERIFY(manager.discoverPlugins(
                    QFileInfo(QString::fromUtf8(TEST_PLUGIN_PATH)).absolutePath()) >= 1);
        manager.sandbox().revokePermission(pluginId, Permissions::ToolExecution);
        QVERIFY(!manager.initializePlugin(pluginId));
        QVERIFY(!registry.findRegistration(echoId));
    }
    {
        PluginManager manager(QStringLiteral("1.0.0"), directory.path());
        manager.setToolRegistry(&registry);
        QVERIFY(manager.discoverPlugins(
                    QFileInfo(QString::fromUtf8(TEST_PLUGIN_PATH)).absolutePath()) >= 1);
        QVERIFY(manager.startPlugin(pluginId));
        QVERIFY(registry.findRegistration(echoId));
    }
    QVERIFY(!registry.findRegistration(echoId));
}

QTEST_MAIN(PluginToolIntegrationTest)
#include "test_plugin_tool_integration.moc"
