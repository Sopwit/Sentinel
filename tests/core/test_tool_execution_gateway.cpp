#include "sentinel/core/ToolExecutionGateway.h"

#include <QtTest>

using sentinel::core::PermissionPolicyService;
using sentinel::core::ToolExecutionGateway;

class ToolExecutionGatewayTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesPlaceholderToolRegistry();
    void consultsPermissionPolicyWithoutGrantingExecution();
    void reportsMetadataOnlyDiagnostics();
};

void ToolExecutionGatewayTest::exposesPlaceholderToolRegistry() {
    const ToolExecutionGateway gateway;
    const PermissionPolicyService permissions;
    const auto registry = gateway.registrySummary(QStringLiteral("Disabled"), permissions);

    QCOMPARE(registry.status, QStringLiteral("Operational"));
    QCOMPARE(registry.toolCount, 10);
    QCOMPARE(registry.metadataSafeCount, 10);
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
    QCOMPARE(enabledSummaries.size(), 10);
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

QTEST_MAIN(ToolExecutionGatewayTest)

#include "test_tool_execution_gateway.moc"
