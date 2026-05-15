#include "sentinel/core/AgentRuntimeContext.h"

#include <QtTest>

class AgentRuntimeContextTest final : public QObject {
    Q_OBJECT

private slots:
    void createsDeterministicEmptyContext();
    void attachesPipelineResultMetadata();
    void preservesPlannedToolOrder();
    void clearsContextWithoutChangingSessionId();
};

static sentinel::core::AgentPipelineResult makePipelineResult(QStringList toolIds) {
    QList<sentinel::core::PlannedToolInvocation> invocations;
    for (const auto& toolId : toolIds) {
        invocations.append(sentinel::core::PlannedToolInvocation{
            toolId,
            QStringLiteral("Tool"),
            QStringLiteral("Plan metadata."),
            QStringLiteral("Metadata-only rationale."),
            sentinel::core::ToolRiskLevel::Low,
            sentinel::core::ToolExecutionMode::MetadataOnly,
            {},
            {},
        });
    }

    return sentinel::core::AgentPipelineResult{
        sentinel::core::ToolInvocationPlan{
            sentinel::core::ToolInvocationPlanStatus::Planned,
            QStringLiteral("Metadata-only tool plan prepared."),
            invocations,
        },
        sentinel::core::ApprovalDecision{
            sentinel::core::ApprovalStatus::NotRequired,
            QStringLiteral("Planned tool invocations do not require approval."),
            {},
        },
        sentinel::core::SandboxEvaluationResult{
            sentinel::core::SandboxStatus::Allowed,
            QStringLiteral("Planned tool capabilities are allowed by sandbox metadata policy."),
            {},
        },
        sentinel::core::ToolExecutionResult{
            sentinel::core::ToolExecutionStatus::PlaceholderSucceeded,
            QStringLiteral("Placeholder tool execution completed without performing actions."),
        },
        QStringLiteral("Placeholder tool execution completed without performing actions."),
    };
}

void AgentRuntimeContextTest::createsDeterministicEmptyContext() {
    const sentinel::core::RuntimeSession session;
    const auto& context = session.context();

    QCOMPARE(context.sessionId.value, QStringLiteral("runtime-session-1"));
    QCOMPARE(context.status, sentinel::core::RuntimeContextStatus::Empty);
    QCOMPARE(context.statusName(), QStringLiteral("Empty"));
    QCOMPARE(context.revision, 0);
    QVERIFY(context.activePlannedToolIds.isEmpty());
    QCOMPARE(sentinel::core::safeAgentRuntimeContextSummary(context),
             QStringLiteral("No runtime context yet."));
}

void AgentRuntimeContextTest::attachesPipelineResultMetadata() {
    sentinel::core::RuntimeSession session;
    const auto pipeline = makePipelineResult({QStringLiteral("tool-a")});

    session.attachPipelineResult(pipeline);

    const auto& context = session.context();
    QCOMPARE(context.sessionId.value, QStringLiteral("runtime-session-1"));
    QCOMPARE(context.status, sentinel::core::RuntimeContextStatus::Active);
    QCOMPARE(context.statusName(), QStringLiteral("Active"));
    QCOMPARE(context.revision, 1);
    QCOMPARE(context.latestPipelineResult.executionStatus(),
             sentinel::core::ToolExecutionStatus::PlaceholderSucceeded);
    QCOMPARE(context.activePlannedToolIds, QStringList{QStringLiteral("tool-a")});
    QCOMPARE(context.summary,
             QStringLiteral("Runtime context captured pipeline result: Placeholder Succeeded"));
}

void AgentRuntimeContextTest::preservesPlannedToolOrder() {
    sentinel::core::RuntimeSession session;

    session.attachPipelineResult(
        makePipelineResult({QStringLiteral("tool-b"), QStringLiteral("tool-a")}));

    QCOMPARE(session.context().activePlannedToolIds,
             QStringList({QStringLiteral("tool-b"), QStringLiteral("tool-a")}));
}

void AgentRuntimeContextTest::clearsContextWithoutChangingSessionId() {
    sentinel::core::RuntimeSession session{
        sentinel::core::RuntimeSessionId{QStringLiteral("runtime-session-test")}};
    session.attachPipelineResult(makePipelineResult({QStringLiteral("tool-a")}));

    session.reset();

    const auto& context = session.context();
    QCOMPARE(context.sessionId.value, QStringLiteral("runtime-session-test"));
    QCOMPARE(context.status, sentinel::core::RuntimeContextStatus::Cleared);
    QCOMPARE(context.statusName(), QStringLiteral("Cleared"));
    QCOMPARE(context.revision, 0);
    QVERIFY(context.activePlannedToolIds.isEmpty());
    QCOMPARE(context.latestPipelineResult.executionStatus(),
             sentinel::core::ToolExecutionStatus::NotRequested);
    QCOMPARE(context.summary, QStringLiteral("Runtime context cleared."));
    QCOMPARE(sentinel::core::safeAgentRuntimeContextSummary(context),
             QStringLiteral("Runtime context cleared."));
}

QTEST_MAIN(AgentRuntimeContextTest)
#include "test_agent_runtime_context.moc"
