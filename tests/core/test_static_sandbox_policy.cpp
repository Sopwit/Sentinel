#include "sentinel/core/StaticSandboxPolicy.h"

#include <QtTest>

#include <utility>

using sentinel::core::ApprovalDecision;
using sentinel::core::ApprovalStatus;
using sentinel::core::CapabilityDescriptor;
using sentinel::core::PlannedToolInvocation;
using sentinel::core::SandboxStatus;
using sentinel::core::sandboxStatusName;
using sentinel::core::StaticSandboxPolicy;
using sentinel::core::ToolExecutionMode;
using sentinel::core::ToolInvocationPlan;
using sentinel::core::ToolInvocationPlanStatus;
using sentinel::core::ToolRiskLevel;

class StaticSandboxPolicyTest final : public QObject {
    Q_OBJECT

private slots:
    void reportsEmptyPlanAsNotEvaluated();
    void allowsMetadataOnlyCapability();
    void deniesUnknownCapability();
    void blocksWhenApprovalIsRequired();
    void approvedButNotCapableRemainsDenied();
    void preservesDeterministicCapabilityOrdering();
};

static PlannedToolInvocation
makeInvocation(const QString& toolId, ToolRiskLevel riskLevel,
               QList<CapabilityDescriptor> capabilities = QList<CapabilityDescriptor>{}) {
    return PlannedToolInvocation{
        toolId,
        QStringLiteral("Tool %1").arg(toolId),
        QStringLiteral("Plan for %1").arg(toolId),
        QStringLiteral("metadata-only"),
        riskLevel,
        ToolExecutionMode::MetadataOnly,
        {},
        std::move(capabilities),
    };
}

static ToolInvocationPlan makePlan(QList<PlannedToolInvocation> invocations) {
    return ToolInvocationPlan{
        ToolInvocationPlanStatus::Planned,
        QStringLiteral("Metadata-only tool plan prepared."),
        std::move(invocations),
    };
}

static ApprovalDecision approved() {
    return {
        ApprovalStatus::Approved,
        QStringLiteral("Approved metadata."),
        {},
    };
}

void StaticSandboxPolicyTest::reportsEmptyPlanAsNotEvaluated() {
    StaticSandboxPolicy policy;

    const auto decision = policy.evaluate(ToolInvocationPlan{}, approved());

    QCOMPARE(decision.status, SandboxStatus::NotEvaluated);
    QCOMPARE(sandboxStatusName(decision.status), QStringLiteral("Not Evaluated"));
    QCOMPARE(decision.summary,
             QStringLiteral("No planned tool invocation to evaluate for sandbox capabilities."));
    QVERIFY(decision.capabilityDecisions.isEmpty());
}

void StaticSandboxPolicyTest::allowsMetadataOnlyCapability() {
    StaticSandboxPolicy policy;

    const auto decision = policy.evaluate(
        makePlan({makeInvocation(QStringLiteral("safe-tool"), ToolRiskLevel::Low)}), approved());

    QCOMPARE(decision.status, SandboxStatus::Allowed);
    QCOMPARE(decision.summary,
             QStringLiteral("Planned tool capabilities are allowed by sandbox metadata policy."));
    QCOMPARE(decision.capabilityDecisions.size(), 1);
    QCOMPARE(decision.capabilityDecisions.first().toolId, QStringLiteral("safe-tool"));
    QCOMPARE(decision.capabilityDecisions.first().capability.id,
             QStringLiteral("tool.metadata.read"));
    QVERIFY(decision.capabilityDecisions.first().allowed);
}

void StaticSandboxPolicyTest::deniesUnknownCapability() {
    StaticSandboxPolicy policy;

    const auto decision = policy.evaluate(
        makePlan({makeInvocation(
            QStringLiteral("unknown-tool"), ToolRiskLevel::Low,
            {CapabilityDescriptor{QStringLiteral("tool.unknown.future"),
                                  QStringLiteral("Future unsupported capability.")}})}),
        approved());

    QCOMPARE(decision.status, SandboxStatus::Denied);
    QCOMPARE(
        decision.summary,
        QStringLiteral("One or more planned capabilities are outside sandbox metadata policy."));
    QCOMPARE(decision.capabilityDecisions.size(), 1);
    QCOMPARE(decision.capabilityDecisions.first().capability.id,
             QStringLiteral("tool.unknown.future"));
    QVERIFY(!decision.capabilityDecisions.first().allowed);
}

void StaticSandboxPolicyTest::blocksWhenApprovalIsRequired() {
    StaticSandboxPolicy policy;

    const auto decision = policy.evaluate(
        makePlan({makeInvocation(QStringLiteral("risky-tool"), ToolRiskLevel::High)}),
        ApprovalDecision{
            ApprovalStatus::RequiresApproval,
            QStringLiteral("Requires approval."),
            {},
        });

    QCOMPARE(decision.status, SandboxStatus::BlockedByApproval);
    QCOMPARE(decision.summary,
             QStringLiteral("Sandbox capability evaluation is blocked by approval metadata."));
    QVERIFY(decision.capabilityDecisions.isEmpty());
}

void StaticSandboxPolicyTest::approvedButNotCapableRemainsDenied() {
    StaticSandboxPolicy policy;

    const auto decision = policy.evaluate(
        makePlan({makeInvocation(QStringLiteral("risky-tool"), ToolRiskLevel::High)}), approved());

    QCOMPARE(decision.status, SandboxStatus::Denied);
    QVERIFY(!decision.capabilityDecisions.isEmpty());
    QCOMPARE(decision.capabilityDecisions.last().capability.id, QStringLiteral("tool.risk.high"));
    QVERIFY(!decision.capabilityDecisions.last().allowed);
}

void StaticSandboxPolicyTest::preservesDeterministicCapabilityOrdering() {
    StaticSandboxPolicy policy(QSet<QString>{
        QStringLiteral("tool.metadata.read"),
        QStringLiteral("tool.alpha"),
    });

    const auto decision = policy.evaluate(
        makePlan({makeInvocation(
            QStringLiteral("ordered-tool"), ToolRiskLevel::Low,
            {
                CapabilityDescriptor{QStringLiteral("tool.alpha"), QStringLiteral("Alpha")},
                CapabilityDescriptor{QStringLiteral("tool.beta"), QStringLiteral("Beta")},
            })}),
        approved());

    QCOMPARE(decision.status, SandboxStatus::Denied);
    QCOMPARE(decision.capabilityDecisions.size(), 2);
    QCOMPARE(decision.capabilityDecisions.at(0).capability.id, QStringLiteral("tool.alpha"));
    QCOMPARE(decision.capabilityDecisions.at(1).capability.id, QStringLiteral("tool.beta"));
    QVERIFY(decision.capabilityDecisions.at(0).allowed);
    QVERIFY(!decision.capabilityDecisions.at(1).allowed);
}

QTEST_MAIN(StaticSandboxPolicyTest)

#include "test_static_sandbox_policy.moc"
