// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/security/PermissionPolicyService.h"

#include <QtTest>

using sentinel::core::PermissionPolicyService;

class PermissionPolicyServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesDefaultDisabledRegistry();
    void normalizesPermissionStates();
    void reportsPermissionBoundaries();
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

void PermissionPolicyServiceTest::reportsPermissionBoundaries() {
    const PermissionPolicyService service;
    const auto summaries = service.permissionSummaries(QStringLiteral("Trusted"));

    QCOMPARE(summaries.size(), 10);
    QVERIFY(summaries.first().diagnostics.contains(
        QStringLiteral("Execution decision: descriptor, risk, and resource policy")));
    QVERIFY(service.registrySummary(QStringLiteral("Trusted"))
                .developerDiagnostics.join(QStringLiteral("\n"))
                .contains(QStringLiteral("Subagents use a restricted tool set")));
    const sentinel::core::AuthorizationRequest external{
        sentinel::core::SecurityDomain::ExternalService,
        sentinel::core::AccessMode::Invoke, QStringLiteral("mcp:test-server")};
    QVERIFY(!service.allowsAuthorization(external, QStringLiteral("Disabled"), true));
    QVERIFY(!service.allowsAuthorization(external, QStringLiteral("Ask Every Time"), false));
    QVERIFY(service.allowsAuthorization(external, QStringLiteral("Ask Every Time"), true));
}

QTEST_MAIN(PermissionPolicyServiceTest)

#include "test_permission_policy_service.moc"
