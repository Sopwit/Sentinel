#include "sentinel/core/AgentTaskRuntime.h"

#include <QtTest>

using sentinel::core::agentCapabilityRegistryStatusName;
using sentinel::core::agentCapabilitySummaries;
using sentinel::core::agentPlanningSessionStatusName;
using sentinel::core::AgentTaskLifecycle;
using sentinel::core::AgentTaskLifecycleEvent;
using sentinel::core::agentTaskLifecycleSummaries;
using sentinel::core::AgentTaskPriority;
using sentinel::core::agentTaskRuntimeStateName;
using sentinel::core::AgentTaskSource;
using sentinel::core::AgentTaskStatus;
using sentinel::core::agentTaskStatusName;
using sentinel::core::AgentTaskTrace;
using sentinel::core::agentTaskTraceSummaries;
using sentinel::core::AgentTaskType;
using sentinel::core::StaticAgentTaskRuntime;
using sentinel::core::toolContractRegistryStatusName;
using sentinel::core::toolContractSummaries;

class AgentTaskRuntimeTest final : public QObject {
    Q_OBJECT

private slots:
    void createsDeterministicMetadataTasks();
    void ordersQueueByPriorityTimeAndId();
    void recordsLifecycleTransitions();
    void refusesExecutionWithoutSideEffects();
    void ordersTraceSummariesDeterministically();
    void ordersLifecycleSummariesDeterministically();
    void createsDeterministicPlanningSession();
    void boundsPlanningBudgetDeterministically();
    void refusesUnsafePlanningMetadata();
    void createsDeterministicCapabilityRegistry();
    void keepsDisabledAndRestrictedCapabilitiesNonExecutable();
    void exposesCapabilityReadinessAndSafetySummaries();
    void createsDeterministicToolContractRegistry();
    void exposesToolContractPermissionAndSandboxMetadata();
    void refusesUnsafeToolContractScopesWithoutExecution();
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
    const auto task =
        runtime.createTask(AgentTaskType::PlanResponse, AgentTaskSource::DesktopReadiness,
                           AgentTaskPriority::High, QStringLiteral("Plan response metadata."));

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
            AgentTaskLifecycleEvent{1, QStringLiteral("agent-task-test"), AgentTaskStatus::Queued,
                                    QStringLiteral("Queued.")},
            AgentTaskLifecycleEvent{2, QStringLiteral("agent-task-test"), AgentTaskStatus::Planned,
                                    QStringLiteral("Planned.")},
        },
        QStringLiteral("Lifecycle summary."),
    };

    const auto summaries = agentTaskLifecycleSummaries(lifecycle);
    QCOMPARE(summaries.size(), 3);
    QVERIFY(summaries.at(0).startsWith(QStringLiteral("1. agent-task-test")));
    QVERIFY(summaries.at(1).contains(QStringLiteral("[Planned]")));
    QVERIFY(summaries.at(2).contains(QStringLiteral("[Completed Metadata]")));
}

void AgentTaskRuntimeTest::createsDeterministicPlanningSession() {
    StaticAgentTaskRuntime runtime;

    const auto session = runtime.planningSession();
    QCOMPARE(session.id.value, QStringLiteral("agent-planning-session-local"));
    QCOMPARE(agentPlanningSessionStatusName(session.status), QStringLiteral("Ready"));
    QCOMPARE(session.summary.candidateCount, 6);
    QCOMPARE(session.summary.refusedCount, 0);
    QCOMPARE(session.result.executionAttempted, false);
    QCOMPARE(session.result.candidates.at(0).taskType, AgentTaskType::PlanResponse);
    QCOMPARE(session.result.candidates.at(1).taskType, AgentTaskType::PrepareRetrievalContext);
    QVERIFY(session.result.arbitration.summary.contains(QStringLiteral("priority/queue/id")));
    QCOMPARE(session.result.fallback.used, false);
}

void AgentTaskRuntimeTest::boundsPlanningBudgetDeterministically() {
    StaticAgentTaskRuntime runtime;
    runtime.createTask(AgentTaskType::PrepareRetrievalContext,
                       AgentTaskSource::ConversationMetadata, AgentTaskPriority::High,
                       QStringLiteral("Extra retrieval preparation metadata."));

    const auto session = runtime.planningSession();
    QCOMPARE(session.status, sentinel::core::AgentPlanningSessionStatus::Bounded);
    QCOMPARE(session.summary.candidateCount, 6);
    QCOMPARE(session.result.arbitration.omittedCount, 1);
    QCOMPARE(session.result.fallback.used, true);
    QVERIFY(session.result.fallback.summary.contains(QStringLiteral("omitted")));
    QCOMPARE(session.result.executionAttempted, false);
}

void AgentTaskRuntimeTest::refusesUnsafePlanningMetadata() {
    StaticAgentTaskRuntime runtime;
    runtime.createTask(AgentTaskType::PlanResponse, AgentTaskSource::DesktopReadiness,
                       AgentTaskPriority::High,
                       QStringLiteral("Attempt shell execution with a tool plugin."));

    const auto session = runtime.planningSession();
    QCOMPARE(session.status, sentinel::core::AgentPlanningSessionStatus::Refused);
    QCOMPARE(session.summary.refusedCount, 1);
    QCOMPARE(session.result.refusals.size(), 1);
    QVERIFY(session.result.refusals.first().safeSummary.contains(QStringLiteral("refused")));
    QVERIFY(session.result.safetyReport.summary.contains(QStringLiteral("refused")));
    QCOMPARE(session.result.executionAttempted, false);
}

void AgentTaskRuntimeTest::createsDeterministicCapabilityRegistry() {
    StaticAgentTaskRuntime runtime;

    const auto registry = runtime.capabilityRegistry();
    QCOMPARE(agentCapabilityRegistryStatusName(registry.status),
             QStringLiteral("Refusing Unsafe Capabilities"));
    QCOMPARE(registry.summary.totalCount, 9);
    QCOMPARE(registry.summary.enabledCount, 6);
    QCOMPARE(registry.summary.disabledCount, 2);
    QCOMPARE(registry.summary.restrictedCount, 3);
    QCOMPARE(registry.summary.refusedCount, 1);
    QCOMPARE(registry.summary.executionAttempted, false);
    QCOMPARE(registry.capabilities.at(0).type,
             sentinel::core::AgentCapabilityType::ConversationSummarization);
    QCOMPARE(registry.capabilities.at(6).type,
             sentinel::core::AgentCapabilityType::FilesystemAccess);
    QCOMPARE(registry.capabilities.at(7).type, sentinel::core::AgentCapabilityType::ShellExecution);
    QCOMPARE(registry.capabilities.at(8).type, sentinel::core::AgentCapabilityType::PluginRuntime);
}

void AgentTaskRuntimeTest::keepsDisabledAndRestrictedCapabilitiesNonExecutable() {
    StaticAgentTaskRuntime runtime;
    const auto registry = runtime.capabilityRegistry();

    for (const auto& capability : registry.capabilities) {
        QCOMPARE(capability.policy.executionAllowed, false);
        QCOMPARE(capability.policy.toolExecutionAllowed, false);
        QCOMPARE(capability.policy.filesystemActionsAllowed, false);
        QCOMPARE(capability.policy.shellExecutionAllowed, false);
        QCOMPARE(capability.policy.pluginExecutionAllowed, false);
        QCOMPARE(capability.policy.cloudCallsAllowed, false);
        QCOMPARE(capability.safetyReport.executionAttempted, false);
        if (capability.type == sentinel::core::AgentCapabilityType::ShellExecution) {
            QCOMPARE(capability.status, sentinel::core::AgentCapabilityStatus::Refused);
            QCOMPARE(capability.readiness.refused, true);
            QVERIFY(capability.safetyReport.refusalSummary.contains(QStringLiteral("refused")));
        }
    }
}

void AgentTaskRuntimeTest::exposesCapabilityReadinessAndSafetySummaries() {
    StaticAgentTaskRuntime runtime;
    const auto registry = runtime.capabilityRegistry();

    const auto summaries = agentCapabilitySummaries(registry);
    QCOMPARE(summaries.size(), 9);
    QVERIFY(summaries.first().startsWith(QStringLiteral("1. Conversation Summarization")));
    QVERIFY(summaries.at(6).contains(QStringLiteral("[Disabled/Future Runtime]")));
    QVERIFY(summaries.at(7).contains(QStringLiteral("[Refused/Future Runtime]")));
    QCOMPARE(sentinel::core::agentCapabilityReadinessSummaries(registry).size(), 9);
    QCOMPARE(sentinel::core::agentCapabilitySafetySummaries(registry).size(), 9);
    QVERIFY(registry.readinessSummary.contains(QStringLiteral("local-only metadata")));
    QVERIFY(registry.safetySummary.contains(QStringLiteral("disabled or refused")));
}

void AgentTaskRuntimeTest::createsDeterministicToolContractRegistry() {
    StaticAgentTaskRuntime runtime;

    const auto registry = runtime.toolContractRegistry();
    QCOMPARE(toolContractRegistryStatusName(registry.status),
             QStringLiteral("Refusing Unsafe Contracts"));
    QCOMPARE(registry.summary.totalCount, 10);
    QCOMPARE(registry.summary.enabledCount, 6);
    QCOMPARE(registry.summary.disabledCount, 3);
    QCOMPARE(registry.summary.restrictedCount, 4);
    QCOMPARE(registry.summary.refusedCount, 1);
    QCOMPARE(registry.summary.executionAttempted, false);
    QCOMPARE(registry.contracts.at(0).type, sentinel::core::ToolContractType::ConversationSummary);
    QCOMPARE(registry.contracts.at(6).type,
             sentinel::core::ToolContractType::FutureFilesystemAccess);
    QCOMPARE(registry.contracts.at(7).type,
             sentinel::core::ToolContractType::FutureSubprocessExecution);
    QCOMPARE(registry.contracts.at(9).type, sentinel::core::ToolContractType::FutureExportAction);

    const auto summaries = toolContractSummaries(registry);
    QCOMPARE(summaries.size(), 10);
    QVERIFY(summaries.at(0).startsWith(QStringLiteral("1. Conversation Summary")));
    QVERIFY(summaries.at(7).contains(QStringLiteral("[Refused/Unsafe Runtime]")));
}

void AgentTaskRuntimeTest::exposesToolContractPermissionAndSandboxMetadata() {
    StaticAgentTaskRuntime runtime;
    const auto registry = runtime.toolContractRegistry();

    QCOMPARE(sentinel::core::toolContractPermissionSummaries(registry).size(), 10);
    QCOMPARE(sentinel::core::toolContractSandboxSummaries(registry).size(), 10);
    QCOMPARE(sentinel::core::toolContractReadinessSummaries(registry).size(), 10);
    QCOMPARE(sentinel::core::toolContractSafetySummaries(registry).size(), 10);
    QVERIFY(sentinel::core::toolContractPermissionSummaries(registry).at(6).contains(
        QStringLiteral("future filesystem access")));
    QVERIFY(sentinel::core::toolContractPermissionSummaries(registry).at(7).contains(
        QStringLiteral("future subprocess execution")));
    QVERIFY(sentinel::core::toolContractSandboxSummaries(registry).at(7).contains(
        QStringLiteral("Denied")));
    QVERIFY(registry.permissionSummary.contains(QStringLiteral("approval-required")));
    QVERIFY(registry.sandboxSummary.contains(QStringLiteral("no filesystem")));
}

void AgentTaskRuntimeTest::refusesUnsafeToolContractScopesWithoutExecution() {
    StaticAgentTaskRuntime runtime;
    const auto registry = runtime.toolContractRegistry();

    for (const auto& contract : registry.contracts) {
        QCOMPARE(contract.policy.executionAllowed, false);
        QCOMPARE(contract.policy.toolRuntimeAllowed, false);
        QCOMPARE(contract.policy.filesystemActionsAllowed, false);
        QCOMPARE(contract.policy.subprocessExecutionAllowed, false);
        QCOMPARE(contract.policy.pluginRuntimeAllowed, false);
        QCOMPARE(contract.policy.cloudCallsAllowed, false);
        QCOMPARE(contract.safetyReport.executionAttempted, false);
        if (contract.type == sentinel::core::ToolContractType::FutureSubprocessExecution) {
            QCOMPARE(contract.status, sentinel::core::ToolContractStatus::Refused);
            QCOMPARE(contract.scope, sentinel::core::ToolContractScope::UnsafeRuntime);
            QVERIFY(contract.safetyReport.unsafeScopeDenied);
            QVERIFY(contract.safetyReport.refusalSummary.contains(QStringLiteral("refused")));
        }
    }
}

QTEST_MAIN(AgentTaskRuntimeTest)

#include "test_agent_task_runtime.moc"
