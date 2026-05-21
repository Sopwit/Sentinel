#include "sentinel/core/AgentTaskRuntime.h"

#include <algorithm>

namespace sentinel::core {

namespace {

QString defaultTaskSummary(AgentTaskType type) {
    switch (type) {
    case AgentTaskType::SummarizeConversation:
        return QStringLiteral("Summarize conversation metadata for future response planning.");
    case AgentTaskType::InspectMemoryStatus:
        return QStringLiteral("Inspect memory status summaries without reading private payloads.");
    case AgentTaskType::PlanResponse:
        return QStringLiteral("Plan response metadata without provider or model execution.");
    case AgentTaskType::PrepareRetrievalContext:
        return QStringLiteral("Prepare retrieval context metadata with deterministic authority.");
    case AgentTaskType::PrepareVoiceResponse:
        return QStringLiteral("Prepare voice response metadata without recording or playback.");
    case AgentTaskType::PrepareExportAction:
        return QStringLiteral("Prepare export action metadata without executing an export.");
    }

    return QStringLiteral("Plan agent task metadata.");
}

QString plannedStepSummary(AgentTaskType type) {
    return QStringLiteral("Prepare %1 metadata only.").arg(agentTaskTypeName(type).toLower());
}

AgentTaskTrace traceFor(int order, const QString& stage, AgentTaskStatus status,
                        const QString& summary) {
    return AgentTaskTrace{order, stage, status, summary};
}

} // namespace

QString agentTaskTypeName(AgentTaskType type) {
    switch (type) {
    case AgentTaskType::SummarizeConversation:
        return QStringLiteral("Summarize Conversation");
    case AgentTaskType::InspectMemoryStatus:
        return QStringLiteral("Inspect Memory Status");
    case AgentTaskType::PlanResponse:
        return QStringLiteral("Plan Response");
    case AgentTaskType::PrepareRetrievalContext:
        return QStringLiteral("Prepare Retrieval Context");
    case AgentTaskType::PrepareVoiceResponse:
        return QStringLiteral("Prepare Voice Response");
    case AgentTaskType::PrepareExportAction:
        return QStringLiteral("Prepare Export Action");
    }

    return QStringLiteral("Plan Response");
}

QString agentTaskStatusName(AgentTaskStatus status) {
    switch (status) {
    case AgentTaskStatus::Planned:
        return QStringLiteral("Planned");
    case AgentTaskStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Planned");
}

QString agentTaskPriorityName(AgentTaskPriority priority) {
    switch (priority) {
    case AgentTaskPriority::Low:
        return QStringLiteral("Low");
    case AgentTaskPriority::Normal:
        return QStringLiteral("Normal");
    case AgentTaskPriority::High:
        return QStringLiteral("High");
    }

    return QStringLiteral("Normal");
}

QString agentTaskSourceName(AgentTaskSource source) {
    switch (source) {
    case AgentTaskSource::DesktopReadiness:
        return QStringLiteral("Desktop Readiness");
    case AgentTaskSource::ConversationMetadata:
        return QStringLiteral("Conversation Metadata");
    case AgentTaskSource::MemoryMetadata:
        return QStringLiteral("Memory Metadata");
    case AgentTaskSource::VoiceMetadata:
        return QStringLiteral("Voice Metadata");
    case AgentTaskSource::ExportMetadata:
        return QStringLiteral("Export Metadata");
    }

    return QStringLiteral("Desktop Readiness");
}

QString agentTaskRuntimeStateName(AgentTaskRuntimeState state) {
    switch (state) {
    case AgentTaskRuntimeState::Ready:
        return QStringLiteral("Ready");
    case AgentTaskRuntimeState::RefusingExecution:
        return QStringLiteral("Refusing Execution");
    }

    return QStringLiteral("Ready");
}

QString agentTaskSummary(const AgentTask& task) {
    return QStringLiteral("%1 [%2/%3]: %4")
        .arg(agentTaskTypeName(task.type), agentTaskStatusName(task.status),
             agentTaskPriorityName(task.priority),
             task.summary.isEmpty() ? defaultTaskSummary(task.type) : task.summary);
}

QString agentTaskTraceSummary(const AgentTaskTrace& trace) {
    return QStringLiteral("%1. %2 [%3]: %4")
        .arg(trace.order)
        .arg(trace.stage, agentTaskStatusName(trace.status),
             trace.summary.isEmpty() ? QStringLiteral("No trace summary.") : trace.summary);
}

QStringList agentTaskTraceSummaries(const QList<AgentTaskTrace>& traces) {
    auto ordered = traces;
    std::sort(ordered.begin(), ordered.end(),
              [](const AgentTaskTrace& left, const AgentTaskTrace& right) {
                  return left.order < right.order;
              });

    QStringList summaries;
    for (const auto& trace : ordered) {
        summaries.append(agentTaskTraceSummary(trace));
    }
    return summaries;
}

StaticAgentTaskRuntime::StaticAgentTaskRuntime() {
    tasks_.append(makeTask(AgentTaskType::SummarizeConversation,
                           AgentTaskSource::ConversationMetadata, AgentTaskPriority::Normal,
                           defaultTaskSummary(AgentTaskType::SummarizeConversation)));
    tasks_.append(makeTask(AgentTaskType::InspectMemoryStatus, AgentTaskSource::MemoryMetadata,
                           AgentTaskPriority::Normal,
                           defaultTaskSummary(AgentTaskType::InspectMemoryStatus)));
    tasks_.append(makeTask(AgentTaskType::PlanResponse, AgentTaskSource::DesktopReadiness,
                           AgentTaskPriority::High,
                           defaultTaskSummary(AgentTaskType::PlanResponse)));
    tasks_.append(makeTask(AgentTaskType::PrepareRetrievalContext,
                           AgentTaskSource::ConversationMetadata, AgentTaskPriority::High,
                           defaultTaskSummary(AgentTaskType::PrepareRetrievalContext)));
    tasks_.append(makeTask(AgentTaskType::PrepareVoiceResponse, AgentTaskSource::VoiceMetadata,
                           AgentTaskPriority::Low,
                           defaultTaskSummary(AgentTaskType::PrepareVoiceResponse)));
    tasks_.append(makeTask(AgentTaskType::PrepareExportAction, AgentTaskSource::ExportMetadata,
                           AgentTaskPriority::Low,
                           defaultTaskSummary(AgentTaskType::PrepareExportAction)));
}

QString StaticAgentTaskRuntime::name() const {
    return QStringLiteral("StaticAgentTaskRuntime");
}

AgentTaskRuntimeStatus StaticAgentTaskRuntime::runtimeStatus() const {
    return AgentTaskRuntimeStatus{
        AgentTaskRuntimeState::RefusingExecution,
        tasks_.isEmpty() ? QString() : tasks_.last().id.value,
        static_cast<int>(tasks_.size()),
        QStringLiteral("Agent task runtime is ready for deterministic metadata planning and "
                       "refuses execution."),
        AgentTaskSafetyPolicy{},
    };
}

QList<AgentTask> StaticAgentTaskRuntime::tasks() const {
    return tasks_;
}

AgentTask StaticAgentTaskRuntime::createTask(AgentTaskType type, AgentTaskSource source,
                                             AgentTaskPriority priority, const QString& summary) {
    auto task = makeTask(type, source, priority,
                         summary.trimmed().isEmpty() ? defaultTaskSummary(type) : summary);
    tasks_.append(task);
    return task;
}

AgentTaskPlan StaticAgentTaskRuntime::planTask(const AgentTask& task) const {
    return AgentTaskPlan{
        AgentTaskStatus::Planned,
        QStringLiteral("Metadata-only plan prepared for %1. No execution is available.")
            .arg(agentTaskTypeName(task.type).toLower()),
        QList<AgentTaskStep>{
            AgentTaskStep{1, QStringLiteral("%1-classify").arg(task.id.value),
                          QStringLiteral("Classify task source as %1 metadata.")
                              .arg(agentTaskSourceName(task.source)),
                          AgentTaskStatus::Planned},
            AgentTaskStep{2, QStringLiteral("%1-plan").arg(task.id.value),
                          plannedStepSummary(task.type), AgentTaskStatus::Planned},
            AgentTaskStep{3, QStringLiteral("%1-boundary").arg(task.id.value),
                          QStringLiteral("Stop at the no-execution runtime boundary."),
                          AgentTaskStatus::Refused},
        },
    };
}

AgentTaskResult StaticAgentTaskRuntime::refuseExecution(const AgentTask& task) const {
    return AgentTaskResult{
        AgentTaskStatus::Refused,
        QStringLiteral("%1 refused execution by design; metadata was planned only.")
            .arg(agentTaskTypeName(task.type)),
        false,
    };
}

AgentTask StaticAgentTaskRuntime::makeTask(AgentTaskType type, AgentTaskSource source,
                                           AgentTaskPriority priority, const QString& summary) {
    const auto id = AgentTaskId{QStringLiteral("agent-task-%1").arg(nextTaskSequence_++)};
    AgentTask task;
    task.id = id;
    task.type = type;
    task.status = AgentTaskStatus::Planned;
    task.priority = priority;
    task.source = source;
    task.summary = summary;
    task.safetyPolicy = AgentTaskSafetyPolicy{};
    task.plan = planTask(task);
    task.result = refuseExecution(task);
    task.traces = {
        traceFor(1, QStringLiteral("Task Created"), AgentTaskStatus::Planned,
                 QStringLiteral("%1 task metadata created from %2.")
                     .arg(agentTaskTypeName(type), agentTaskSourceName(source))),
        traceFor(2, QStringLiteral("Plan Prepared"), AgentTaskStatus::Planned, task.plan.summary),
        traceFor(3, QStringLiteral("Execution Boundary"), AgentTaskStatus::Refused,
                 task.result.summary),
    };
    return task;
}

} // namespace sentinel::core
