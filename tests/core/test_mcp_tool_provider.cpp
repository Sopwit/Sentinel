// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/mcp/McpToolProvider.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/ToolExecutionGateway.h"
#include <QtTest>

using namespace sentinel::core;

class FakeMcpService final : public IMcpService {
public:
    QMap<QString, QList<McpToolDefinition>> definitions;
    QMap<QString, McpConnectionState> states;
    QString calledServer;
    QString calledTool;
    QJsonObject calledArguments;
    ToolCompletion pending;
    bool addServer(const McpServerConfig&) override {
        return true;
    }
    bool removeServer(const QString& name) override {
        states.remove(name);
        return true;
    }
    QList<McpServerConfig> servers() const override {
        QList<McpServerConfig> result;
        for (const auto& name : states.keys())
            result.append(McpServerConfig{name});
        return result;
    }
    McpServerConfig serverConfig(const QString& name) const override {
        return McpServerConfig{name};
    }
    bool connectToServer(const QString& name) override {
        states[name] = McpConnectionState::Connected;
        return true;
    }
    bool disconnectFromServer(const QString& name) override {
        states[name] = McpConnectionState::Disconnected;
        return true;
    }
    McpConnectionState connectionState(const QString& name) const override {
        return states.value(name);
    }
    QList<McpToolDefinition> tools(const QString& name = {}) const override {
        return definitions.value(name);
    }
    QJsonObject callTool(const QString&, const QString&, const QJsonObject&) override {
        return {};
    }
    Cancel callToolAsync(const QString& server, const QString& tool, const QJsonObject& args,
                         ToolCompletion completion) override {
        calledServer = server;
        calledTool = tool;
        calledArguments = args;
        pending = std::move(completion);
        return [] {};
    }
    bool connectToAll() override {
        return true;
    }
    void disconnectFromAll() override {
        states.clear();
    }
    void serverConnected(const QString&) override {}
    void serverDisconnected(const QString&) override {}
    void serverError(const QString&, const QString&) override {}
    void toolsUpdated(const QString&) override {}
};

class NoFallback final : public IToolExecutor {
public:
    ToolExecutionResult execute(const ToolExecutionRequest&) const override {
        return {ToolExecutionStatus::Blocked, QStringLiteral("fallback")};
    }
};

class McpToolProviderTest final : public QObject {
    Q_OBJECT
private slots:
    void registrationExecutionAndLifecycle();
    void securityAndStaleCalls();
    void namesAndServerIsolation();
    void cancellationAndFailedRefresh();
};

void McpToolProviderTest::registrationExecutionAndLifecycle() {
    auto service = std::make_shared<FakeMcpService>();
    service->states[QStringLiteral("test")] = McpConnectionState::Connected;
    const QJsonObject schema{
        {"type", "object"},
        {"required", QJsonArray{"query"}},
        {"properties", QJsonObject{{"query", QJsonObject{{"type", "string"}}}}}};
    service->definitions[QStringLiteral("test")] = {
        {QStringLiteral("alpha"), QStringLiteral("Search"), QStringLiteral("test"), schema},
        {QStringLiteral("beta"), QStringLiteral("Other"), QStringLiteral("test"), {}}};
    InMemoryToolRegistry registry;
    McpToolProvider provider(service, registry);
    auto alpha = registry.findRegistration(QStringLiteral("mcp.test.alpha"));
    QVERIFY(alpha && alpha->handler);
    QCOMPARE(alpha->descriptor.inputSchema, schema);
    QCOMPARE(alpha->descriptor.riskLevel, ToolRiskLevel::Medium);
    QCOMPARE(registry.enabledTools().size(), 2);
    ToolExecutionRequest request;
    request.plan.status = ToolInvocationPlanStatus::Planned;
    PlannedToolInvocation invocation;
    invocation.toolId = QStringLiteral("mcp.test.alpha");
    invocation.arguments = {{QStringLiteral("query"), QStringLiteral("hello")}};
    request.plan.invocations = {invocation};
    request.approval.status = ApprovalStatus::Approved;
    request.sandbox.status = SandboxStatus::Allowed;
    NoFallback fallback;
    ToolExecutionGateway gateway(&registry);
    int completions = 0;
    ToolExecutionResult result;
    auto cancel = gateway.executeAsync(request, fallback, {}, {}, {}, [&](auto value) {
        ++completions;
        result = std::move(value);
    });
    QCOMPARE(service->calledServer, QStringLiteral("test"));
    QCOMPARE(service->calledTool, QStringLiteral("alpha"));
    QCOMPARE(service->calledArguments.value(QStringLiteral("query")).toString(),
             QStringLiteral("hello"));
    provider.disconnectServer(QStringLiteral("test"));
    QVERIFY(registry.enabledTools().isEmpty());
    service->pending(
        {{"result",
          QJsonObject{{"content", QJsonArray{QJsonObject{{"type", "text"}, {"text", "found"}}}}}}});
    QCOMPARE(completions, 1);
    QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
    QVERIFY(result.summary.contains(QStringLiteral("found")));
    Q_UNUSED(cancel)
    service->definitions[QStringLiteral("test")] = {
        {QStringLiteral("beta"), {}, QStringLiteral("test"), {}},
        {QStringLiteral("gamma"), {}, QStringLiteral("test"), {}}};
    QVERIFY(provider.refresh(QStringLiteral("test")));
    QVERIFY(!registry.findRegistration(QStringLiteral("mcp.test.alpha")));
    QVERIFY(registry.findRegistration(QStringLiteral("mcp.test.gamma")));
    QCOMPARE(registry.enabledTools().size(), 2);
}

void McpToolProviderTest::securityAndStaleCalls() {
    auto service = std::make_shared<FakeMcpService>();
    service->states[QStringLiteral("test")] = McpConnectionState::Connected;
    service->definitions[QStringLiteral("test")] = {{QStringLiteral("alpha"), {}, {}, {}}};
    InMemoryToolRegistry registry;
    McpToolProvider provider(service, registry);
    ToolExecutionRequest request;
    request.plan.status = ToolInvocationPlanStatus::Planned;
    PlannedToolInvocation invocation;
    invocation.toolId = QStringLiteral("mcp.test.alpha");
    request.plan.invocations = {invocation};
    request.approval.status = ApprovalStatus::Denied;
    request.sandbox.status = SandboxStatus::Allowed;
    NoFallback fallback;
    ToolExecutionGateway gateway(&registry);
    ToolExecutionResult result;
    gateway.executeAsync(request, fallback, {}, {}, {}, [&](auto value) { result = value; });
    QCOMPARE(result.status, ToolExecutionStatus::Blocked);
    QVERIFY(service->calledTool.isEmpty());
    request.approval.status = ApprovalStatus::Approved;
    request.sandbox.status = SandboxStatus::Denied;
    gateway.executeAsync(request, fallback, {}, {}, {}, [&](auto value) { result = value; });
    QCOMPARE(result.status, ToolExecutionStatus::Blocked);
    QVERIFY(service->calledTool.isEmpty());
    provider.disconnectServer(QStringLiteral("test"));
    request.sandbox.status = SandboxStatus::Allowed;
    gateway.executeAsync(request, fallback, {}, {}, {}, [&](auto value) { result = value; });
    QCOMPARE(result.status, ToolExecutionStatus::UnknownTool);
}

void McpToolProviderTest::namesAndServerIsolation() {
    auto service = std::make_shared<FakeMcpService>();
    for (const auto& server : {QStringLiteral("one"), QStringLiteral("two")}) {
        service->states[server] = McpConnectionState::Connected;
        service->definitions[server] = {{QStringLiteral("search"), {}, server, {}}};
    }
    service->definitions[QStringLiteral("one")].append(
        {QStringLiteral("foo-tool"), {}, QStringLiteral("one"), {}});
    service->definitions[QStringLiteral("one")].append(
        {QStringLiteral("foo_tool"), {}, QStringLiteral("one"), {}});
    InMemoryToolRegistry registry;
    McpToolProvider provider(service, registry);
    QCOMPARE(registry.enabledTools().size(), 4);
    QVERIFY(registry.findRegistration(QStringLiteral("mcp.one.foo_2d_tool")));
    QVERIFY(registry.findRegistration(QStringLiteral("mcp.one.foo_5f_tool")));
    provider.disconnectServer(QStringLiteral("one"));
    QCOMPARE(registry.enabledTools().size(), 1);
    QVERIFY(registry.findRegistration(QStringLiteral("mcp.two.search")));
}

void McpToolProviderTest::cancellationAndFailedRefresh() {
    auto service = std::make_shared<FakeMcpService>();
    service->states[QStringLiteral("test")] = McpConnectionState::Connected;
    service->definitions[QStringLiteral("test")] = {{QStringLiteral("alpha"), {}, {}, {}}};
    InMemoryToolRegistry registry;
    McpToolProvider provider(service, registry);
    ToolExecutionRequest request;
    request.plan.status = ToolInvocationPlanStatus::Planned;
    PlannedToolInvocation invocation;
    invocation.toolId = QStringLiteral("mcp.test.alpha");
    request.plan.invocations = {invocation};
    request.approval.status = ApprovalStatus::Approved;
    request.sandbox.status = SandboxStatus::Allowed;
    NoFallback fallback;
    ToolExecutionGateway gateway(&registry);
    int completions = 0;
    auto cancel = gateway.executeAsync(request, fallback, {}, {}, {}, [&](auto) { ++completions; });
    cancel();
    service->pending({{"result", QJsonObject{{"content", QJsonArray{}}}}});
    QCOMPARE(completions, 0);
    service->definitions[QStringLiteral("test")].append({QStringLiteral("alpha"), {}, {}, {}});
    QVERIFY(!provider.refresh(QStringLiteral("test")));
    QVERIFY(registry.findRegistration(QStringLiteral("mcp.test.alpha")));
    QCOMPARE(registry.enabledTools().size(), 1);
}

QTEST_MAIN(McpToolProviderTest)
#include "test_mcp_tool_provider.moc"
