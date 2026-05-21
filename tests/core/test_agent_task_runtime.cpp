#include "sentinel/core/AgentTaskRuntime.h"

#include <QtTest>

using sentinel::core::AgentTaskPriority;
using sentinel::core::agentTaskRuntimeStateName;
using sentinel::core::AgentTaskSource;
using sentinel::core::AgentTaskStatus;
using sentinel::core::agentTaskStatusName;
using sentinel::core::AgentTaskLifecycle;
using sentinel::core::AgentTaskLifecycleEvent;
using sentinel::core::agentTaskLifecycleSummaries;
using sentinel::core::AgentTaskTrace;
using sentinel::core::agentTaskTraceSummaries;
using sentinel::core::AgentTaskType;
using sentinel::core::StaticAgentTaskRuntime;

class AgentTaskRuntimeTest final : public QObject {
    Q_OBJECT

private slots:
    void createsDeterministicMetadataTasks();
    void ordersQueueByPriorityTimeAndId();
    void recordsLifecycleTransitions();
    void refusesExecutionWithoutSideEffects();
    void ordersTraceSummariesDeterministically();
    void ordersLifecycleSummariesDeterministically();
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
    QCOMPARE(runtime.queue().summary.totalCount, 7);
    QCOMPARE(runtime.queue().summary.plannedCount, 7);
    QCOMPARE(runtime.queue().summary.activeCount, 0);
}

void AgentTaskRuntimeTest::ordersQueueByPriorityTimeAndId() {
    StaticAgentTaskRuntime runtime;

    const auto ordered = runtime.queue().tasks;
    QCOMPARE(ordered.size(), 6);
    QCOMPARE(ordered.at(0).type, AgentTaskType::PlanResponse);
    QCOMPARE(ordered.at(1).type, AgentTaskType::PrepareRetrievalContext);
    QCOMPARE(ordered.at(2).type, AgentTaskType::SummarizeConversation);
    QCOMPARE(ordered.at(3).type, AgentTaskType::InspectMemoryStatus);
    QCOMPARE(ordered.at(4).type, AgentTaskType::PrepareVoiceResponse);
    QCOMPARE(ordered.at(5).type, AgentTaskType::PrepareExportAction);
}

void AgentTaskRuntimeTest::recordsLifecycleTransitions() {
    StaticAgentTaskRuntime runtime;
    const auto task = runtime.createTask(AgentTaskType::PlanResponse,
                                         AgentTaskSource::DesktopReadiness,
                                         AgentTaskPriority::High,
                                         QStringLiteral("Plan response metadata."));

    const auto planned = runtime.markTaskPlanned(task.id);
    const auto blocked = runtime.markTaskBlocked(task.id, QStringLiteral("waiting on metadata"));
    const auto completed =
        runtime.completeTaskAsMetadata(task.id, QStringLiteral("Metadata complete."));

    QCOMPARE(planned.executionAttempted, false);
    QCOMPARE(blocked.status, AgentTaskStatus::Blocked);
    QCOMPARE(completed.status, AgentTaskStatus::CompletedMetadata);
    QCOMPARE(runtime.queue().summary.completedCount, 1);
    QVERIFY(runtime.queue().summary.latestLifecycleSummary.contains(
        QStringLiteral("Completed Metadata")));

    auto updated = runtime.tasks().first();
    for (const auto& queuedTask : runtime.tasks()) {
        if (queuedTask.id.value == task.id.value) {
            updated = queuedTask;
        }
    }
    QCOMPARE(updated.status, AgentTaskStatus::CompletedMetadata);
    QVERIFY(!agentTaskLifecycleSummaries(updated.lifecycle).isEmpty());
}

void AgentTaskRuntimeTest::refusesExecutionWithoutSideEffects() {
    StaticAgentTaskRuntime runtime;
    const auto beforeCount = runtime.runtimeStatus().taskCount;
    const auto task = runtime.tasks().first();
    const auto result = runtime.refuseExecution(task);
    const auto refused = runtime.refuseTask(task.id, QStringLiteral("execution request"));

    QCOMPARE(result.status, AgentTaskStatus::Refused);
    QCOMPARE(result.executionAttempted, false);
    QCOMPARE(refused.status, AgentTaskStatus::Refused);
    QCOMPARE(refused.executionAttempted, false);
    QVERIFY(result.summary.contains(QStringLiteral("refused execution")));
    QCOMPARE(runtime.runtimeStatus().taskCount, beforeCount);
    QCOMPARE(runtime.queue().summary.refusedCount, 1);
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

void AgentTaskRuntimeTest::ordersLifecycleSummariesDeterministically() {
    const AgentTaskLifecycle lifecycle{
        QStringLiteral("agent-task-test"),
        QList<AgentTaskLifecycleEvent>{
            AgentTaskLifecycleEvent{3, QStringLiteral("agent-task-test"),
                                    AgentTaskStatus::CompletedMetadata,
                                    QStringLiteral("Completed.")},
            AgentTaskLifecycleEvent{1, QStringLiteral("agent-task-test"),
                                    AgentTaskStatus::Queued, QStringLiteral("Queued.")},
            AgentTaskLifecycleEvent{2, QStringLiteral("agent-task-test"),
                                    AgentTaskStatus::Planned, QStringLiteral("Planned.")},
        },
        QStringLiteral("Lifecycle summary."),
    };

    const auto summaries = agentTaskLifecycleSummaries(lifecycle);
    QCOMPARE(summaries.size(), 3);
    QVERIFY(summaries.at(0).startsWith(QStringLiteral("1. agent-task-test")));
    QVERIFY(summaries.at(1).contains(QStringLiteral("[Planned]")));
    QVERIFY(summaries.at(2).contains(QStringLiteral("[Completed Metadata]")));
}

QTEST_MAIN(AgentTaskRuntimeTest)

#include "test_agent_task_runtime.moc"
