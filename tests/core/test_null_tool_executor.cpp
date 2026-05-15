#include "sentinel/core/NullToolExecutor.h"

#include <QtTest>

using sentinel::core::ApprovalDecision;
using sentinel::core::ApprovalStatus;
using sentinel::core::NullToolExecutor;
using sentinel::core::PlannedToolInvocation;
using sentinel::core::SandboxEvaluationResult;
using sentinel::core::SandboxStatus;
using sentinel::core::ToolExecutionMode;
using sentinel::core::ToolExecutionRequest;
using sentinel::core::ToolExecutionStatus;
using sentinel::core::toolExecutionStatusName;
using sentinel::core::ToolInvocationPlan;
using sentinel::core::ToolInvocationPlanStatus;
using sentinel::core::ToolRiskLevel;

class NullToolExecutorTest final : public QObject {
    Q_OBJECT

private slots:
    void returnsPlaceholderSuccessForApprovedSandboxAllowedPlan();
    void blocksWhenApprovalIsRequired();
    void blocksWhenSandboxDenied();
    void handlesEmptyPlan();
    void rejectsUnknownTool();
    void returnsDeterministicResults();
};

static PlannedToolInvocation makeInvocation(const QString& toolId) {
    return PlannedToolInvocation{
        toolId,
        QStringLiteral("Tool %1").arg(toolId),
        QStringLiteral("Plan for %1").arg(toolId),
        QStringLiteral("metadata-only"),
        ToolRiskLevel::Low,
        ToolExecutionMode::MetadataOnly,
        {},
        {},
    };
}

static ToolInvocationPlan makePlan(const QString& toolId) {
    return {
        ToolInvocationPlanStatus::Planned,
        QStringLiteral("Metadata-only tool plan prepared."),
        {makeInvocation(toolId)},
    };
}

static ApprovalDecision approved() {
    return {
        ApprovalStatus::Approved,
        QStringLiteral("Approved metadata."),
        {},
    };
}

static SandboxEvaluationResult sandboxAllowed() {
    return {
        SandboxStatus::Allowed,
        QStringLiteral("Allowed metadata."),
        {},
    };
}

void NullToolExecutorTest::returnsPlaceholderSuccessForApprovedSandboxAllowedPlan() {
    NullToolExecutor executor;

    const auto result = executor.execute(ToolExecutionRequest{
        makePlan(QStringLiteral("safe-tool")),
        approved(),
        sandboxAllowed(),
        {QStringLiteral("safe-tool")},
    });

    QCOMPARE(result.status, ToolExecutionStatus::PlaceholderSucceeded);
    QCOMPARE(toolExecutionStatusName(result.status), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(result.summary,
             QStringLiteral("Placeholder tool execution completed without performing actions."));
}

void NullToolExecutorTest::blocksWhenApprovalIsRequired() {
    NullToolExecutor executor;

    const auto result = executor.execute(ToolExecutionRequest{
        makePlan(QStringLiteral("safe-tool")),
        {
            ApprovalStatus::RequiresApproval,
            QStringLiteral("Requires approval."),
            {},
        },
        sandboxAllowed(),
        {QStringLiteral("safe-tool")},
    });

    QCOMPARE(result.status, ToolExecutionStatus::Blocked);
    QCOMPARE(result.summary, QStringLiteral("Execution boundary blocked by approval metadata."));
}

void NullToolExecutorTest::blocksWhenSandboxDenied() {
    NullToolExecutor executor;

    const auto result = executor.execute(ToolExecutionRequest{
        makePlan(QStringLiteral("safe-tool")),
        approved(),
        {
            SandboxStatus::Denied,
            QStringLiteral("Denied metadata."),
            {},
        },
        {QStringLiteral("safe-tool")},
    });

    QCOMPARE(result.status, ToolExecutionStatus::Blocked);
    QCOMPARE(result.summary,
             QStringLiteral("Execution boundary blocked by sandbox capability metadata."));
}

void NullToolExecutorTest::handlesEmptyPlan() {
    NullToolExecutor executor;

    const auto result = executor.execute(ToolExecutionRequest{
        ToolInvocationPlan{},
        approved(),
        sandboxAllowed(),
        {QStringLiteral("safe-tool")},
    });

    QCOMPARE(result.status, ToolExecutionStatus::EmptyPlan);
    QCOMPARE(result.summary,
             QStringLiteral("No planned tool invocation reached the execution boundary."));
}

void NullToolExecutorTest::rejectsUnknownTool() {
    NullToolExecutor executor;

    const auto result = executor.execute(ToolExecutionRequest{
        makePlan(QStringLiteral("missing-tool")),
        approved(),
        sandboxAllowed(),
        {QStringLiteral("safe-tool")},
    });

    QCOMPARE(result.status, ToolExecutionStatus::UnknownTool);
    QCOMPARE(result.summary,
             QStringLiteral("Execution boundary rejected unknown tool metadata: missing-tool"));
}

void NullToolExecutorTest::returnsDeterministicResults() {
    NullToolExecutor executor;
    const auto request = ToolExecutionRequest{
        makePlan(QStringLiteral("safe-tool")),
        approved(),
        sandboxAllowed(),
        {QStringLiteral("safe-tool")},
    };

    const auto first = executor.execute(request);
    const auto second = executor.execute(request);

    QCOMPARE(first.status, second.status);
    QCOMPARE(first.summary, second.summary);
}

QTEST_MAIN(NullToolExecutorTest)

#include "test_null_tool_executor.moc"
