#include "sentinel/core/PermissionPolicyService.h"

#include <QtTest>

using sentinel::core::PermissionPolicyService;

class PermissionPolicyServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesDefaultDisabledRegistry();
    void normalizesPermissionStates();
    void reportsMetadataOnlySafetyBoundaries();
};

void PermissionPolicyServiceTest::exposesDefaultDisabledRegistry() {
    const PermissionPolicyService service;
    const auto registry = service.registrySummary(QStringLiteral("Disabled"));

    QCOMPARE(registry.status, QStringLiteral("Operational"));
    QCOMPARE(registry.defaultState, QStringLiteral("Disabled"));
    QCOMPARE(service.permissionDomainIds().size(), 10);
    QCOMPARE(service.permissionDomainNames().size(), 10);
    QCOMPARE(registry.domainSummaries.size(), 10);
    QCOMPARE(registry.stateLabels,
             QStringList({QStringLiteral("Disabled"), QStringLiteral("Ask Every Time"),
                          QStringLiteral("Trusted"), QStringLiteral("Enabled")}));
    QVERIFY(registry.summary.contains(QStringLiteral("registry is operational")));
}

void PermissionPolicyServiceTest::normalizesPermissionStates() {
    const PermissionPolicyService service;

    QCOMPARE(service.normalizedState(QStringLiteral(" ask-every-time ")),
             QStringLiteral("Ask Every Time"));
    QCOMPARE(service.normalizedState(QStringLiteral("trusted")), QStringLiteral("Trusted"));
    QCOMPARE(service.normalizedState(QStringLiteral("ENABLED")), QStringLiteral("Enabled"));
    QCOMPARE(service.normalizedState(QStringLiteral("unknown")), QStringLiteral("Disabled"));
}

void PermissionPolicyServiceTest::reportsMetadataOnlySafetyBoundaries() {
    const PermissionPolicyService service;
    const auto summaries = service.permissionSummaries(QStringLiteral("Trusted"));

    QCOMPARE(summaries.size(), 10);
    QVERIFY(summaries.first().diagnostics.contains(QStringLiteral("Execution grant: allowed")));
    QVERIFY(service.registrySummary(QStringLiteral("Trusted"))
                .developerDiagnostics.join(QStringLiteral("\n"))
                .contains(QStringLiteral("No tool, plugin, MCP, or external command execution")));
    QVERIFY(service.registrySummary(QStringLiteral("Enabled"))
                .developerDiagnostics.join(QStringLiteral("\n"))
                .contains(QStringLiteral("No cloud request")));
}

QTEST_MAIN(PermissionPolicyServiceTest)

#include "test_permission_policy_service.moc"
