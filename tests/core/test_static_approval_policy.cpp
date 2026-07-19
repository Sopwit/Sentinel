#include "sentinel/core/StaticApprovalPolicy.h"

#include <QtTest>

#include <utility>

using sentinel::core::ApprovalStatus;
using sentinel::core::approvalStatusName;
using sentinel::core::PlannedToolInvocation;
using sentinel::core::StaticApprovalPolicy;
using sentinel::core::ToolExecutionMode;
using sentinel::core::ToolInvocationArgument;
using sentinel::core::ToolInvocationPlan;
using sentinel::core::ToolInvocationPlanStatus;
using sentinel::core::ToolRiskLevel;

class StaticApprovalPolicyTest final : public QObject {
    Q_OBJECT

private slots:
    void reportsEmptyPlanAsNotRequested();
    void allowsLowRiskMetadataPlansWithoutApproval();
    void requiresApprovalForRiskyPlans();
    void representsDeniedState();
    void representsApprovedState();
    void preservesRequestOrdering();
};

static PlannedToolInvocation makeInvocation(const QString& toolId, ToolRiskLevel riskLevel) {
    return PlannedToolInvocation{
        toolId,
        QStringLiteral("Tool %1").arg(toolId),
        QStringLiteral("Plan for %1").arg(toolId),
        QStringLiteral("metadata-only"),
        riskLevel,
        ToolExecutionMode::MetadataOnly,
        {
            ToolInvocationArgument{QStringLiteral("topic"), QStringLiteral("local")},
        },
    };
}

static ToolInvocationPlan makePlan(QList<PlannedToolInvocation> invocations) {
    return ToolInvocationPlan{
        ToolInvocationPlanStatus::Planned,
        QStringLiteral("Metadata-only tool plan prepared."),
        std::move(invocations),
    };
}

void StaticApprovalPolicyTest::reportsEmptyPlanAsNotRequested() {
    StaticApprovalPolicy policy;

    const auto decision = policy.evaluate(ToolInvocationPlan{});

    QCOMPARE(decision.status, ApprovalStatus::NotRequested);
    QCOMPARE(approvalStatusName(decision.status), QStringLiteral("Not Requested"));
    QCOMPARE(decision.summary, QStringLiteral("No planned tool invocation to approve."));
    QVERIFY(decision.requests.isEmpty());
}

void StaticApprovalPolicyTest::allowsLowRiskMetadataPlansWithoutApproval() {
    StaticApprovalPolicy policy;

    const auto decision = policy.evaluate(makePlan({
        makeInvocation(QStringLiteral("safe-tool"), ToolRiskLevel::Low),
    }));

    QCOMPARE(decision.status, ApprovalStatus::NotRequired);
    QCOMPARE(decision.summary, QStringLiteral("Planned tool invocations do not require approval."));
    QVERIFY(decision.requests.isEmpty());
}

void StaticApprovalPolicyTest::requiresApprovalForRiskyPlans() {
    StaticApprovalPolicy policy;

    const auto decision = policy.evaluate(makePlan({
        makeInvocation(QStringLiteral("risky-tool"), ToolRiskLevel::High),
    }));

    QCOMPARE(decision.status, ApprovalStatus::RequiresApproval);
    QCOMPARE(decision.summary,
             QStringLiteral("One or more planned tool invocations require approval."));
    QCOMPARE(decision.requests.size(), 1);
    QCOMPARE(decision.requests.first().toolId, QStringLiteral("risky-tool"));
    QCOMPARE(decision.requests.first().riskLevel, ToolRiskLevel::High);
    QCOMPARE(decision.requests.first().permissions.size(), 1);
    QCOMPARE(decision.requests.first().permissions.first().id,
             QStringLiteral("tool.metadata.approval"));
}

void StaticApprovalPolicyTest::representsDeniedState() {
    StaticApprovalPolicy policy(QMap<QString, ApprovalStatus>{
        {QStringLiteral("safe-tool"), ApprovalStatus::Denied},
    });

    const auto decision = policy.evaluate(makePlan({
        makeInvocation(QStringLiteral("safe-tool"), ToolRiskLevel::Low),
    }));

    QCOMPARE(decision.status, ApprovalStatus::Denied);
    QCOMPARE(decision.summary,
             QStringLiteral("At least one planned tool invocation is denied by policy."));
}

void StaticApprovalPolicyTest::representsApprovedState() {
    StaticApprovalPolicy policy(QMap<QString, ApprovalStatus>{
        {QStringLiteral("safe-tool"), ApprovalStatus::Approved},
    });

    const auto decision = policy.evaluate(makePlan({
        makeInvocation(QStringLiteral("safe-tool"), ToolRiskLevel::Low),
    }));

    QCOMPARE(decision.status, ApprovalStatus::Approved);
    QCOMPARE(decision.summary,
             QStringLiteral("Planned tool invocations are approved by policy metadata."));
    QVERIFY(decision.requests.isEmpty());
}

void StaticApprovalPolicyTest::preservesRequestOrdering() {
    StaticApprovalPolicy policy;

    const auto decision = policy.evaluate(makePlan({
        makeInvocation(QStringLiteral("tool-b"), ToolRiskLevel::Medium),
        makeInvocation(QStringLiteral("tool-a"), ToolRiskLevel::High),
    }));

    QCOMPARE(decision.status, ApprovalStatus::RequiresApproval);
    QCOMPARE(decision.requests.size(), 2);
    QCOMPARE(decision.requests.at(0).toolId, QStringLiteral("tool-b"));
    QCOMPARE(decision.requests.at(1).toolId, QStringLiteral("tool-a"));
}

QTEST_MAIN(StaticApprovalPolicyTest)

#include "test_static_approval_policy.moc"
