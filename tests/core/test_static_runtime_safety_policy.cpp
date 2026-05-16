#include "sentinel/core/RuntimeSafety.h"

#include <QtTest>

using sentinel::core::RuntimeSafetyDecision;
using sentinel::core::runtimeSafetyDecisionName;
using sentinel::core::runtimeSafetyRuleSummaries;
using sentinel::core::safeRuntimeSafetySummary;
using sentinel::core::StaticRuntimeSafetyPolicy;

class StaticRuntimeSafetyPolicyTest final : public QObject {
    Q_OBJECT

private slots:
    void namesSafetyDecision();
    void reportsDeterministicLocalOnlyNoExecutionPosture();
};

void StaticRuntimeSafetyPolicyTest::namesSafetyDecision() {
    QCOMPARE(runtimeSafetyDecisionName(RuntimeSafetyDecision::Compliant),
             QStringLiteral("Compliant"));
    QCOMPARE(runtimeSafetyDecisionName(RuntimeSafetyDecision::Blocked), QStringLiteral("Blocked"));
}

void StaticRuntimeSafetyPolicyTest::reportsDeterministicLocalOnlyNoExecutionPosture() {
    const StaticRuntimeSafetyPolicy policy;
    const auto report = policy.evaluate();

    QVERIFY(report.policy.localOnly);
    QVERIFY(report.policy.noExecution);
    QCOMPARE(report.rules.size(), 3);
    QCOMPARE(report.decision, RuntimeSafetyDecision::Compliant);
    QCOMPARE(safeRuntimeSafetySummary(report),
             QStringLiteral("Runtime safety policy report: local-only and no-execution posture is "
                            "enforced with deterministic metadata rules."));
    QVERIFY(runtimeSafetyRuleSummaries(report.rules)
                .contains(QStringLiteral(
                    "No Side Effects (Enforced): No process launch, networking, filesystem, or "
                    "system action is allowed.")));
}

QTEST_MAIN(StaticRuntimeSafetyPolicyTest)

#include "test_static_runtime_safety_policy.moc"
