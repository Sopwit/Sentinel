#include "sentinel/core/AgentTaskRuntime.h"

#include <QtTest>

using sentinel::core::AgentTaskPriority;
using sentinel::core::agentTaskRuntimeStateName;
using sentinel::core::AgentTaskSource;
using sentinel::core::AgentTaskStatus;
using sentinel::core::agentTaskStatusName;
using sentinel::core::AgentTaskTrace;
using sentinel::core::agentTaskTraceSummaries;
using sentinel::core::AgentTaskType;
using sentinel::core::StaticAgentTaskRuntime;

class AgentTaskRuntimeTest final : public QObject {
    Q_OBJECT

private slots:
    void createsDeterministicMetadataTasks();
    void refusesExecutionWithoutSideEffects();
    void ordersTraceSummariesDeterministically();
};

void AgentTaskRuntimeTest::createsDeterministicMetadataTasks() {
    StaticAgentTaskRuntime runtime;

    QCOMPARE(runtime.name(), QStringLiteral("StaticAgentTaskRuntime"));
    QCOMPARE(agentTaskRuntimeStateName(runtime.runtimeStatus().state),
             QStringLiteral("Refusing Execution"));
    QCOMPARE(runtime.runtimeStatus().taskCount, 6);

    const auto task = runtime.createTask(
        AgentTaskType::PrepareRetrievalContext, AgentTaskSource::ConversationMetadata,
        AgentTaskPriority::High, QStringLiteral("Prepare retrieval context metadata."));
    QCOMPARE(task.id.value, QStringLiteral("agent-task-7"));
    QCOMPARE(agentTaskStatusName(task.plan.status), QStringLiteral("Planned"));
    QCOMPARE(task.plan.steps.size(), 3);
    QCOMPARE(task.result.executionAttempted, false);
    QCOMPARE(runtime.runtimeStatus().taskCount, 7);
}

void AgentTaskRuntimeTest::refusesExecutionWithoutSideEffects() {
    StaticAgentTaskRuntime runtime;
    const auto beforeCount = runtime.runtimeStatus().taskCount;
    const auto task = runtime.tasks().first();
    const auto result = runtime.refuseExecution(task);

    QCOMPARE(result.status, AgentTaskStatus::Refused);
    QCOMPARE(result.executionAttempted, false);
    QVERIFY(result.summary.contains(QStringLiteral("refused execution")));
    QCOMPARE(runtime.runtimeStatus().taskCount, beforeCount);
}

void AgentTaskRuntimeTest::ordersTraceSummariesDeterministically() {
    const QList<AgentTaskTrace> traces{
        AgentTaskTrace{3, QStringLiteral("Execution Boundary"), AgentTaskStatus::Refused,
                       QStringLiteral("Execution refused.")},
        AgentTaskTrace{1, QStringLiteral("Task Created"), AgentTaskStatus::Planned,
                       QStringLiteral("Task created.")},
        AgentTaskTrace{2, QStringLiteral("Plan Prepared"), AgentTaskStatus::Planned,
                       QStringLiteral("Plan prepared.")},
    };

    const auto summaries = agentTaskTraceSummaries(traces);
    QCOMPARE(summaries.size(), 3);
    QVERIFY(summaries.at(0).startsWith(QStringLiteral("1. Task Created")));
    QVERIFY(summaries.at(1).startsWith(QStringLiteral("2. Plan Prepared")));
    QVERIFY(summaries.at(2).startsWith(QStringLiteral("3. Execution Boundary")));
}

QTEST_MAIN(AgentTaskRuntimeTest)

#include "test_agent_task_runtime.moc"
