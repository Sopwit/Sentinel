// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/ToolExecutionGateway.h"

#include <QtTest>

using sentinel::core::PermissionPolicyService;
using sentinel::core::ToolExecutionGateway;

class ToolExecutionGatewayTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesPlaceholderToolRegistry();
    void consultsPermissionPolicyWithoutGrantingExecution();
    void reportsMetadataOnlyDiagnostics();
    void asyncPathPreservesApprovalGate();
    void dynamicHandlerUsesGatewayGates();
    void registeredPathRejectsUnknownWithoutFallback();
};

void ToolExecutionGatewayTest::exposesPlaceholderToolRegistry() {
    const ToolExecutionGateway gateway;
    const PermissionPolicyService permissions;
    const auto registry = gateway.registrySummary(QStringLiteral("Disabled"), permissions);

    QCOMPARE(registry.status, QStringLiteral("Operational"));
    QCOMPARE(registry.toolCount, sentinel::core::BuiltInToolProvider::descriptors().size());
    QCOMPARE(registry.metadataSafeCount, registry.toolCount);
    QCOMPARE(registry.unavailableCount, 0);
    QCOMPARE(registry.refusedCount, 0);
    QVERIFY(registry.summary.contains(QStringLiteral("fully operational")));
    QVERIFY(registry.toolSummaries.join(QStringLiteral("\n"))
                .contains(QStringLiteral("Run Command / Available")));
}

void ToolExecutionGatewayTest::consultsPermissionPolicyWithoutGrantingExecution() {
    const ToolExecutionGateway gateway;
    const PermissionPolicyService permissions;
    const auto trustedRegistry = gateway.registrySummary(QStringLiteral("Trusted"), permissions);
    const auto enabledSummaries = gateway.toolSummaries(QStringLiteral("Enabled"), permissions);

    QCOMPARE(trustedRegistry.permissionPosture, QStringLiteral("Trusted"));
    QCOMPARE(enabledSummaries.size(), sentinel::core::BuiltInToolProvider::descriptors().size());
    QVERIFY(enabledSummaries.first().permissionPosture == QStringLiteral("Enabled"));
    QVERIFY(enabledSummaries.at(2).availability == QStringLiteral("Available"));
}

void ToolExecutionGatewayTest::reportsMetadataOnlyDiagnostics() {
    const ToolExecutionGateway gateway;
    const PermissionPolicyService permissions;
    const auto summaries = gateway.toolSummaries(QStringLiteral("Ask Every Time"), permissions);
    const auto diagnostics = gateway.registrySummary(QStringLiteral("Ask Every Time"), permissions)
                                 .developerDiagnostics.join(QStringLiteral("\n"));

    QVERIFY(summaries.at(4).summary.contains(QStringLiteral("Available")));
    QVERIFY(diagnostics.contains(QStringLiteral("Gateway execution grant: allowed")));
    QVERIFY(diagnostics.contains(QStringLiteral("cloud-provider-access")));
    QVERIFY(diagnostics.contains(QStringLiteral("subprocess-execution")));
}

void ToolExecutionGatewayTest::asyncPathPreservesApprovalGate() {
    class Executor final : public sentinel::core::IToolExecutor {
    public:
        mutable int calls = 0;
        sentinel::core::ToolExecutionResult
        execute(const sentinel::core::ToolExecutionRequest&) const override {
            ++calls;
            return {sentinel::core::ToolExecutionStatus::Succeeded, QStringLiteral("executed")};
        }
    } executor;
    sentinel::core::ToolExecutionRequest request;
    request.plan.status = sentinel::core::ToolInvocationPlanStatus::Planned;
    request.plan.invocations.append({QStringLiteral("run-command"), QStringLiteral("Run Command")});
    request.approval.status = sentinel::core::ApprovalStatus::RequiresApproval;
    const ToolExecutionGateway gateway;
    sentinel::core::ToolExecutionResult result;
    gateway.executeAsync(request, executor, QStringLiteral("session"), QStringLiteral("call"), {},
                         [&](auto value) { result = std::move(value); });
    QCOMPARE(result.status, sentinel::core::ToolExecutionStatus::Blocked);
    QCOMPARE(executor.calls, 0);
    request.approval.status = sentinel::core::ApprovalStatus::Approved;
    gateway.executeAsync(request, executor, QStringLiteral("session"), QStringLiteral("call"), {},
                         [&](auto value) { result = std::move(value); });
    QCOMPARE(result.status, sentinel::core::ToolExecutionStatus::Succeeded);
    QCOMPARE(executor.calls, 1);
}

void ToolExecutionGatewayTest::dynamicHandlerUsesGatewayGates() {
    class Handler final : public sentinel::core::IToolHandler {
    public:
        int calls = 0;
        sentinel::core::IToolExecutor::Cancel
        execute(const sentinel::core::ToolExecutionRequest&, const QString&, const QString&,
                sentinel::core::IToolExecutor::Output,
                sentinel::core::IToolExecutor::Completion completion) override {
            ++calls;
            completion({sentinel::core::ToolExecutionStatus::Succeeded, QStringLiteral("dynamic")});
            return {};
        }
    };
    class Executor final : public sentinel::core::IToolExecutor {
    public:
        sentinel::core::ToolExecutionResult
        execute(const sentinel::core::ToolExecutionRequest&) const override {
            return {sentinel::core::ToolExecutionStatus::Blocked, QStringLiteral("legacy")};
        }
    } legacy;
    sentinel::core::InMemoryToolRegistry registry;
    auto handler = std::make_shared<Handler>();
    sentinel::core::ToolDescriptor descriptor;
    descriptor.id = QStringLiteral("plugin.test.dynamic");
    descriptor.source = sentinel::core::ToolSource::Plugin;
    descriptor.providerId = QStringLiteral("test");
    descriptor.inputSchema = QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
                                         {QStringLiteral("additionalProperties"), false}};
    descriptor.riskLevel = sentinel::core::ToolRiskLevel::High;
    QVERIFY(registry.registerTool({descriptor, handler}));
    ToolExecutionGateway gateway(&registry);
    sentinel::core::ToolExecutionRequest request;
    request.plan.status = sentinel::core::ToolInvocationPlanStatus::Planned;
    request.plan.invocations.append({descriptor.id, QStringLiteral("Dynamic")});
    request.approval.status = sentinel::core::ApprovalStatus::RequiresApproval;
    sentinel::core::ToolExecutionResult result;
    auto run = [&] {
        gateway.executeAsync(request, legacy, QStringLiteral("session"), QStringLiteral("call"), {},
                             [&](auto value) { result = std::move(value); });
    };
    run();
    QCOMPARE(result.status, sentinel::core::ToolExecutionStatus::Blocked);
    QCOMPARE(handler->calls, 0);
    request.approval.status = sentinel::core::ApprovalStatus::Approved;
    request.sandbox.status = sentinel::core::SandboxStatus::Denied;
    run();
    QCOMPARE(result.status, sentinel::core::ToolExecutionStatus::Blocked);
    QCOMPARE(handler->calls, 0);
    request.sandbox.status = sentinel::core::SandboxStatus::Allowed;
    run();
    QCOMPARE(result.status, sentinel::core::ToolExecutionStatus::Succeeded);
    QCOMPARE(result.summary, QStringLiteral("dynamic"));
    QCOMPARE(handler->calls, 1);
}

void ToolExecutionGatewayTest::registeredPathRejectsUnknownWithoutFallback() {
    class Executor final : public sentinel::core::IToolExecutor {
    public:
        mutable int calls = 0;
        sentinel::core::ToolExecutionResult
        execute(const sentinel::core::ToolExecutionRequest&) const override {
            ++calls;
            return {sentinel::core::ToolExecutionStatus::Succeeded, QStringLiteral("fallback")};
        }
    } executor;
    sentinel::core::InMemoryToolRegistry registry;
    ToolExecutionGateway gateway(&registry);
    sentinel::core::ToolExecutionRequest request;
    request.plan.status = sentinel::core::ToolInvocationPlanStatus::Planned;
    request.plan.invocations.append({QStringLiteral("does-not-exist"), QStringLiteral("Missing")});
    request.approval.status = sentinel::core::ApprovalStatus::Approved;
    request.sandbox.status = sentinel::core::SandboxStatus::Allowed;
    sentinel::core::ToolExecutionResult result;
    gateway.executeAsync(request, executor, {}, {}, {},
                         [&](auto value) { result = std::move(value); });
    QCOMPARE(result.status, sentinel::core::ToolExecutionStatus::UnknownTool);
    QCOMPARE(executor.calls, 0);
}

QTEST_MAIN(ToolExecutionGatewayTest)

#include "test_tool_execution_gateway.moc"
