// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/agent/AgentLoop.h"
#include "sentinel/core/agent/AgentRuntime.h"
#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include "sentinel/core/plugin/PluginManager.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/runtime/ToolArgumentValidator.h"
#include "sentinel/core/runtime/ToolExecutionGateway.h"
#include "sentinel/core/runtime/ToolHookService.h"
#include "sentinel/core/security/ExternalDirectoryGate.h"
#include "sentinel/core/security/PermissionService.h"
#include "sentinel/core/security/ResourceAuthorizationResolver.h"
#include "sentinel/core/security/StaticApprovalPolicy.h"
#include "sentinel/core/security/StaticSandboxPolicy.h"
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#ifdef Q_OS_UNIX
#include <signal.h>
#endif
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
    void separateHostCrashFailsInflightAndRemovesTools();
    void realExistingBrokersAndArithmetic();
    void cancellationTerminatesRealHostExactlyOnce();
    void stoppedHostTimesOutInflightWithoutLateSuccess();
};

void PluginToolIntegrationTest::stoppedHostTimesOutInflightWithoutLateSuccess() {
#ifndef Q_OS_UNIX
    QSKIP("POSIX stopped-child timeout fixture.");
#else
    QTemporaryDir storage;
    InMemoryToolRegistry registry;
    PluginManager manager("1.0.0", storage.path());
    manager.setToolRegistry(&registry);
    const QString pluginId = "dev.sentinel.plugin.custom-tool";
    QVERIFY(manager.discoverPlugins(
                QFileInfo(QString::fromUtf8(TEST_PLUGIN_PATH)).absolutePath()) >= 1);
    QVERIFY(manager.startPlugin(pluginId));
    const auto host = manager.descriptor(pluginId)->host;
    QProcess listing;
    listing.start("/bin/ps", {"-axo", "pid,ppid,comm"});
    QVERIFY(listing.waitForFinished(5000));
    qint64 pid = 0;
    for (const auto& line : QString::fromUtf8(listing.readAllStandardOutput()).split('\n')) {
        const auto fields = line.simplified().split(' ');
        if (fields.size() >= 3 && fields[1].toLongLong() == QCoreApplication::applicationPid() &&
            fields[2].endsWith("/sentinel-plugin-host"))
            pid = fields[0].toLongLong();
    }
    QVERIFY(pid > 0);
    QCOMPARE(::kill(static_cast<pid_t>(pid), SIGSTOP), 0);
    ToolExecutionGateway gateway(&registry);
    NoFallback fallback;
    int completions = 0;
    ToolExecutionResult result;
    gateway.executeAsync(
        requestFor("plugin.dev_2e_sentinel_2e_plugin_2e_custom_2d_tool.delayed_5f_echo"), fallback,
        {}, {}, {}, [&](auto value) {
            result = value;
            ++completions;
        });
    QCoreApplication::processEvents();
    const auto health = host->call("health", {}, 40);
    QCOMPARE(health.value("category").toString(), QString("PluginTimeout"));
    QTRY_COMPARE_WITH_TIMEOUT(completions, 1, 5000);
    QCOMPARE(result.status, ToolExecutionStatus::Failed);
    QCOMPARE(result.failureCategory, ToolFailureCategory::Timeout);
    QCOMPARE(manager.descriptor(pluginId)->failureCategory, QString("PluginTimeout"));
    QTRY_VERIFY_WITH_TIMEOUT(!host->isRunning(), 5000);
    QTest::qWait(300);
    QCOMPARE(completions, 1);
    QVERIFY(manager.unloadPlugin(pluginId));
#endif
}

void PluginToolIntegrationTest::cancellationTerminatesRealHostExactlyOnce() {
    QTemporaryDir storage;
    InMemoryToolRegistry registry;
    PluginManager manager("1.0.0", storage.path());
    manager.setToolRegistry(&registry);
    const QString pluginId = "dev.sentinel.plugin.custom-tool";
    QVERIFY(manager.discoverPlugins(
                QFileInfo(QString::fromUtf8(TEST_PLUGIN_PATH)).absolutePath()) >= 1);
    QVERIFY(manager.startPlugin(pluginId));
    const auto host = manager.descriptor(pluginId)->host;
    ToolExecutionGateway gateway(&registry);
    NoFallback fallback;
    int completions = 0;
    ToolExecutionResult result;
    auto cancel = gateway.executeAsync(
        requestFor("plugin.dev_2e_sentinel_2e_plugin_2e_custom_2d_tool.delayed_5f_echo"), fallback,
        {}, {}, {}, [&](auto value) {
            result = value;
            ++completions;
        });
    QVERIFY(cancel);
    cancel();
    QTRY_COMPARE_WITH_TIMEOUT(completions, 1, 5000);
    QCOMPARE(result.status, ToolExecutionStatus::Cancelled);
    QCOMPARE(result.failureCategory, ToolFailureCategory::Cancelled);
    QTRY_VERIFY_WITH_TIMEOUT(!host->isRunning(), 5000);
    QTest::qWait(300);
    QCOMPARE(completions, 1);
    QVERIFY(manager.unloadPlugin(pluginId));
}

void PluginToolIntegrationTest::realExistingBrokersAndArithmetic() {
    QTemporaryDir storage, workspace;
    QTemporaryDir external(QDir::current().filePath("sentinel-broker-external-XXXXXX"));
    QVERIFY(external.isValid());
    InMemoryToolRegistry registry;
    PermissionService permissions;
    PermissionPolicyService policy;
    ExternalDirectoryGate gate;
    gate.setWorkingDirectory(workspace.path());
    PluginManager manager("1.0.0", storage.path());
    manager.setToolRegistry(&registry);
    manager.setExternalDirectoryGate(&gate);
    QCOMPARE(manager.discoverPlugins(QString::fromUtf8(TEST_BROKER_PLUGIN_DIR)), 1);
    QVERIFY(manager.startPlugin("dev.sentinel.test.broker"));
    NoFallback fallback;
    ToolExecutionGateway gateway(&registry);
    gateway.setResourceGate(&gate);
    gateway.setPermissionService(&permissions);
    gateway.setPermissionPolicy(&policy, "Ask");
    auto execute = [&](const QString& name, const QJsonObject& arguments) {
        const QString id = "plugin.dev_2e_sentinel_2e_test_2e_broker." + name;
        auto request = requestFor(id);
        const auto descriptor = registry.findToolById(id);
        if (!descriptor)
            return ToolExecutionResult{ToolExecutionStatus::UnknownTool, "missing fixture tool"};
        auto& invocation = request.plan.invocations.first();
        invocation.descriptorSnapshot = std::make_shared<ToolDescriptor>(*descriptor);
        invocation.arguments = ToolArgumentValidator::toInvocationArguments(arguments);
        const auto resources = ResourceAuthorizationResolver::resolve(*descriptor, invocation,
                                                                      workspace.path(), &gate);
        if (!resources.ok())
            return ToolExecutionResult{ToolExecutionStatus::Blocked, resources.reason};
        const auto authorized = ResourceAuthorizationResolver::authorize(
            resources.snapshot, &gate, &permissions, "broker-test");
        if (!authorized.ok())
            return ToolExecutionResult{ToolExecutionStatus::Blocked, authorized.reason};
        invocation.resourceSnapshot =
            std::make_shared<ResourceAuthorizationSnapshot>(authorized.snapshot);
        bool finished = false;
        ToolExecutionResult result;
        gateway.executeAsync(request, fallback, "broker-test", "broker-call", {}, [&](auto value) {
            result = value;
            finished = true;
        });
        if (!QTest::qWaitFor([&] { return finished; }, 10000))
            return ToolExecutionResult{ToolExecutionStatus::Failed, "fixture timeout"};
        return result;
    };
    auto arithmetic = execute("add", {{"a", 2}, {"b", 3}});
    QCOMPARE(arithmetic.status, ToolExecutionStatus::Succeeded);
    QCOMPARE(arithmetic.summary, QString("5"));
    for (const auto& path : {workspace.filePath("read.txt"), external.filePath("outside.txt")}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("BROKER_READ_OK");
    }
    auto read = execute("read", {{"path", workspace.filePath("read.txt")}});
    QCOMPARE(read.status, ToolExecutionStatus::Succeeded);
    QCOMPARE(read.summary, QString("BROKER_READ_OK"));
    auto outside = execute("read", {{"path", external.filePath("outside.txt")}});
    QCOMPARE(outside.status, ToolExecutionStatus::Blocked);
    if (gate.canRequestPermission(external.filePath("outside.txt"), workspace.path())) {
        AuthorizationRequest externalRead{
            SecurityDomain::FileSystem, AccessMode::Read,
            QFileInfo(external.filePath("outside.txt")).canonicalFilePath()};
        externalRead.resourceKind = AuthorizationResourceKind::FileSystemPath;
        externalRead.providerId = "plugin:dev.sentinel.test.broker";
        gate.setAuthorizationCheck(
            [&, externalRead](const QString& path, bool writing, const QString&) {
                auto request = externalRead;
                request.resource = path;
                request.access = writing ? AccessMode::Write : AccessMode::Read;
                return permissions.evaluateAuthorization(request, "broker-test") ==
                       PermissionEffect::Allow;
            });
        QVERIFY(permissions.grantAuthorization(externalRead, "broker-test", false));
        auto grantedRead = execute("read", {{"path", external.filePath("outside.txt")}});
        QVERIFY2(grantedRead.status == ToolExecutionStatus::Succeeded,
                 qPrintable(grantedRead.summary));
        QCOMPARE(grantedRead.status, ToolExecutionStatus::Succeeded);
        QCOMPARE(grantedRead.summary, QString("BROKER_READ_OK"));
        permissions.clearSessionGrants("broker-test");
        auto otherOwner = externalRead;
        otherOwner.providerId = "plugin:dev.sentinel.plugin.second-tool";
        QVERIFY(permissions.grantAuthorization(otherOwner, "broker-test", false));
        QCOMPARE(execute("read", {{"path", external.filePath("outside.txt")}}).status,
                 ToolExecutionStatus::Blocked);
        permissions.clearSessionGrants("broker-test");
        otherOwner.providerId = "sentinel:built-in";
        QVERIFY(permissions.grantAuthorization(otherOwner, "broker-test", false));
        QCOMPARE(execute("read", {{"path", external.filePath("outside.txt")}}).status,
                 ToolExecutionStatus::Blocked);
        permissions.clearSessionGrants("broker-test");
    }
#ifdef Q_OS_UNIX
    auto process = execute("process", {{"program", QFileInfo("/bin/pwd").canonicalFilePath()},
                                       {"arguments", QJsonArray{}}});
    QCOMPARE(process.status, ToolExecutionStatus::Succeeded);
    QVERIFY(process.summary.trimmed() ==
            QFileInfo(storage.filePath("dev.sentinel.test.broker")).canonicalFilePath());
#endif
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
#ifdef Q_OS_MACOS
    const auto containment = execute(
        "containment", {{"port", server.serverPort()}, {"path", workspace.filePath("read.txt")}});
    QCOMPARE(containment.status, ToolExecutionStatus::Succeeded);
    QCOMPARE(containment.summary,
             QString("forkDenied=1 spawnDenied=1 networkDenied=1 fileDenied=1"));
#endif
    int requests = 0;
    connect(&server, &QTcpServer::newConnection, &server, [&] {
        auto* socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
            socket->readAll();
            ++requests;
            socket->write(
                "HTTP/1.1 200 OK\r\nContent-Length: 14\r\nConnection: close\r\n\r\nBROKER_NETWORK");
            socket->disconnectFromHost();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    });
    auto& network = NetworkPolicyService::instance();
    const auto previous = network.mode();
    network.setMode(NetworkMode::LocalOnly);
    const QString url = QString("http://127.0.0.1:%1/").arg(server.serverPort());
    const auto online = execute("network", {{"url", url}});
    QCOMPARE(online.status, ToolExecutionStatus::Succeeded);
    QCOMPARE(online.summary, QString("BROKER_NETWORK"));
    QCOMPARE(requests, 1);
    const auto credential =
        execute("network", {{"url", url}, {"credential", "unrelated-reference"}});
    QVERIFY(credential.status != ToolExecutionStatus::Succeeded);
    QVERIFY(credential.summary.contains("Credential"));
    QCOMPARE(requests, 1);
    const auto externalNetwork = execute("network", {{"url", "https://example.com/"}});
    QVERIFY(externalNetwork.status != ToolExecutionStatus::Succeeded);
    QCOMPARE(externalNetwork.summary, QString("PluginNetworkDenied"));
    QCOMPARE(requests, 1);
    network.setMode(NetworkMode::Offline);
    // Existing Offline policy intentionally permits loopback. Probe an
    // external target, without ever sending a request to it.
    const auto offline = execute("network", {{"url", "https://example.com/"}});
    QVERIFY(offline.status != ToolExecutionStatus::Succeeded);
    QCOMPARE(offline.summary, QString("PluginNetworkOffline"));
    QCOMPARE(requests, 1);
    network.setMode(previous);
    QVERIFY(manager.unloadPlugin("dev.sentinel.test.broker"));
}

void PluginToolIntegrationTest::separateHostCrashFailsInflightAndRemovesTools() {
#ifndef Q_OS_UNIX
    QSKIP("PID inspection fixture uses POSIX ps/kill.");
#else
    QTemporaryDir directory;
    InMemoryToolRegistry registry;
    PluginManager manager("1.0.0", directory.path());
    manager.setToolRegistry(&registry);
    const QString pluginId = "dev.sentinel.plugin.custom-tool";
    QVERIFY(manager.discoverPlugins(
                QFileInfo(QString::fromUtf8(TEST_PLUGIN_PATH)).absolutePath()) >= 1);
    QVERIFY(manager.startPlugin(pluginId));
    QProcess listing;
    listing.start("/bin/ps", {"-axo", "pid,ppid,comm"});
    QVERIFY(listing.waitForFinished(5000));
    qint64 hostPid = 0;
    for (const auto& line : QString::fromUtf8(listing.readAllStandardOutput()).split('\n')) {
        const auto fields = line.simplified().split(' ');
        if (fields.size() >= 3 && fields[1].toLongLong() == QCoreApplication::applicationPid() &&
            fields[2].endsWith("/sentinel-plugin-host"))
            hostPid = fields[0].toLongLong();
    }
    QVERIFY(hostPid > 0);
    QVERIFY(hostPid != QCoreApplication::applicationPid());
    qInfo() << "Plugin host PID" << hostPid << "owner PID" << QCoreApplication::applicationPid();
    const QString toolId = "plugin.dev_2e_sentinel_2e_plugin_2e_custom_2d_tool.delayed_5f_echo";
    NoFallback fallback;
    ToolExecutionGateway gateway(&registry);
    int completions = 0;
    ToolExecutionResult result;
    gateway.executeAsync(requestFor(toolId), fallback, {}, {}, {}, [&](auto value) {
        result = value;
        ++completions;
    });
    QCoreApplication::processEvents();
    QCOMPARE(::kill(static_cast<pid_t>(hostPid), SIGKILL), 0);
    QTRY_COMPARE_WITH_TIMEOUT(completions, 1, 5000);
    QVERIFY(result.status != ToolExecutionStatus::Succeeded);
    QTRY_VERIFY_WITH_TIMEOUT(!registry.findRegistration(toolId), 5000);
    QCOMPARE(manager.descriptor(pluginId)->failureCategory, QString("PluginCrashed"));
    QTest::qWait(300);
    QCOMPARE(completions, 1);
    QVERIFY(manager.unloadPlugin(pluginId));
    QVERIFY(manager.startPlugin(pluginId));
    QVERIFY(registry.findRegistration(toolId));
    QVERIFY(manager.unloadPlugin(pluginId));
#endif
}

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
                QFileInfo(QString::fromUtf8(TEST_PLUGIN_PATH)).dir().filePath(
                    QStringLiteral(".."))) >= 2);
    if (!manager.initializePlugin(pluginId)) {
#ifdef Q_OS_MACOS
        QCOMPARE(manager.descriptor(pluginId)->failureCategory,
                 QStringLiteral("PluginSandboxUnavailable"));
        QSKIP("macOS cannot enforce the plugin host's detached-child restriction.");
#else
        QFAIL("Plugin host failed to initialize.");
#endif
    }
    if (!manager.startPlugin(pluginId)) {
#ifdef Q_OS_MACOS
        QCOMPARE(manager.descriptor(pluginId)->failureCategory,
                 QStringLiteral("PluginSandboxUnavailable"));
        QSKIP("macOS cannot enforce the plugin host's detached-child restriction.");
#else
        QFAIL("Plugin host failed to start.");
#endif
    }
    QVERIFY(manager.initializePlugin(secondPluginId));
    QVERIFY(manager.startPlugin(secondPluginId));
    QVERIFY(registry.findRegistration(secondEchoId));
    QCOMPARE(registry.findRegistration(QStringLiteral("read-file"))->descriptor.source,
             ToolSource::BuiltIn);
    QVERIFY(manager.pluginInstance(pluginId) == nullptr);
    auto registration = registry.findRegistration(echoId);
    QVERIFY(registration && registration->handler);
    QCOMPARE(registration->descriptor.structuredObservationKind,
             StructuredObservationKind::Generic);
    QCOMPARE(registration->descriptor.evidenceProduced.size(), 1);
    QCOMPARE(registration->descriptor.evidenceProduced.first().domain,
             ObservationDomain::ExternalService);
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
    bool allowedFinished = false;
    gateway.executeAsync(echo, fallback, {}, {}, {}, [&](auto result) {
        allowed = result;
        allowedFinished = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(allowedFinished, 5000);
    QCOMPARE(allowed.status, ToolExecutionStatus::Succeeded);
    QVERIFY(allowed.structuredObservation);
    QCOMPARE(allowed.structuredObservation->kind, StructuredObservationKind::Generic);
    auto delayed = requestFor(delayedId);
    auto host = manager.descriptor(pluginId)->host;
    QVERIFY(host && host->isRunning());
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
    QVERIFY(!host->isRunning());
    ToolExecutionResult stale;
    gateway.executeAsync(delayed, fallback, {}, {}, {}, [&](auto result) { stale = result; });
    QCOMPARE(stale.status, ToolExecutionStatus::UnknownTool);
    QVERIFY(QTest::qWaitFor([&] { return activeCompletions == 1; }, 5000));
    QVERIFY(activeResult.status != ToolExecutionStatus::Succeeded);
    cancel = {};
    registration.reset();
    QVERIFY(QTest::qWaitFor([&] { return !manager.isModuleResident(pluginId); }, 5000));
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
    QVERIFY(QFile::copy(QFileInfo(QString::fromUtf8(TEST_PLUGIN_PATH)).dir().filePath(
                            QStringLiteral("plugin.json")),
                        directory.filePath(QStringLiteral("plugin.json"))));
    InMemoryToolRegistry registry;
    PluginManager manager(QStringLiteral("1.0.0"), directory.path());
    manager.setToolRegistry(&registry);
    const auto pluginId = QStringLiteral("dev.sentinel.plugin.custom-tool");
    const auto echoId = QStringLiteral("plugin.dev_2e_sentinel_2e_plugin_2e_custom_2d_tool.echo");
    QVERIFY(manager.discoverPlugins(directory.path()) >= 1);
    if (!manager.startPlugin(pluginId)) {
#ifdef Q_OS_MACOS
        QCOMPARE(manager.descriptor(pluginId)->failureCategory,
                 QStringLiteral("PluginSandboxUnavailable"));
        QSKIP("macOS cannot enforce the plugin host's detached-child restriction.");
#else
        QFAIL("Plugin host failed to start.");
#endif
    }
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
    if (!manager.startPlugin(pluginId)) {
#ifdef Q_OS_MACOS
        QCOMPARE(manager.descriptor(pluginId)->failureCategory,
                 QStringLiteral("PluginSandboxUnavailable"));
        QSKIP("macOS cannot enforce the plugin host's detached-child restriction.");
#else
        QFAIL("Plugin host failed to start.");
#endif
    }
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
        QVERIFY(manager.initializePlugin(pluginId));
        QVERIFY(!manager.startPlugin(pluginId));
        QVERIFY(!registry.findRegistration(echoId));
    }
    {
        PluginManager manager(QStringLiteral("1.0.0"), directory.path());
        manager.setToolRegistry(&registry);
        QVERIFY(manager.discoverPlugins(
                    QFileInfo(QString::fromUtf8(TEST_PLUGIN_PATH)).absolutePath()) >= 1);
        if (!manager.startPlugin(pluginId)) {
#ifdef Q_OS_MACOS
            QCOMPARE(manager.descriptor(pluginId)->failureCategory,
                     QStringLiteral("PluginSandboxUnavailable"));
            QSKIP("macOS cannot enforce the plugin host's detached-child restriction.");
#else
            QFAIL("Plugin host failed to start.");
#endif
        }
        QVERIFY(registry.findRegistration(echoId));
    }
    QVERIFY(!registry.findRegistration(echoId));
}

QTEST_MAIN(PluginToolIntegrationTest)
#include "test_plugin_tool_integration.moc"
