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

int priorityRank(AgentTaskPriority priority) {
    switch (priority) {
    case AgentTaskPriority::High:
        return 0;
    case AgentTaskPriority::Normal:
        return 1;
    case AgentTaskPriority::Low:
        return 2;
    }

    return 1;
}

QString defaultTransitionSummary(AgentTaskStatus status) {
    switch (status) {
    case AgentTaskStatus::Queued:
        return QStringLiteral("Task queued as deterministic metadata.");
    case AgentTaskStatus::Planned:
        return QStringLiteral("Task marked planned as metadata.");
    case AgentTaskStatus::Active:
        return QStringLiteral("Task marked active as metadata only.");
    case AgentTaskStatus::Blocked:
        return QStringLiteral("Task blocked before execution.");
    case AgentTaskStatus::CompletedMetadata:
        return QStringLiteral("Task completed as metadata without execution.");
    case AgentTaskStatus::Refused:
        return QStringLiteral("Task refused at the no-execution boundary.");
    }

    return QStringLiteral("Task lifecycle updated.");
}

bool containsUnsafePlanningIntent(const QString& text) {
    const auto lowered = text.toLower();
    const QStringList blockedTerms{
        QStringLiteral("execute"),   QStringLiteral("tool"),       QStringLiteral("plugin"),
        QStringLiteral("filesystem"), QStringLiteral("file system"), QStringLiteral("shell"),
        QStringLiteral("subprocess"), QStringLiteral("background"), QStringLiteral("cloud"),
        QStringLiteral("api call"),   QStringLiteral("network"),
    };

    for (const auto& term : blockedTerms) {
        if (lowered.contains(term)) {
            return true;
        }
    }
    return false;
}

QString boundedSummary(QString summary, int maxCharacters) {
    summary = summary.simplified();
    if (maxCharacters <= 0 || summary.size() <= maxCharacters) {
        return summary;
    }
    return summary.left(maxCharacters - 3).trimmed() + QStringLiteral("...");
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
    case AgentTaskStatus::Queued:
        return QStringLiteral("Queued");
    case AgentTaskStatus::Planned:
        return QStringLiteral("Planned");
    case AgentTaskStatus::Active:
        return QStringLiteral("Active");
    case AgentTaskStatus::Blocked:
        return QStringLiteral("Blocked");
    case AgentTaskStatus::CompletedMetadata:
        return QStringLiteral("Completed Metadata");
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

QString agentTaskQueueStatusName(AgentTaskQueueStatus status) {
    switch (status) {
    case AgentTaskQueueStatus::Ready:
        return QStringLiteral("Ready");
    case AgentTaskQueueStatus::Empty:
        return QStringLiteral("Empty");
    case AgentTaskQueueStatus::RefusingExecution:
        return QStringLiteral("Refusing Execution");
    }

    return QStringLiteral("Ready");
}

QString agentPlanningSessionStatusName(AgentPlanningSessionStatus status) {
    switch (status) {
    case AgentPlanningSessionStatus::Ready:
        return QStringLiteral("Ready");
    case AgentPlanningSessionStatus::Bounded:
        return QStringLiteral("Bounded");
    case AgentPlanningSessionStatus::Refused:
        return QStringLiteral("Refused");
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

QString agentTaskLifecycleEventSummary(const AgentTaskLifecycleEvent& event) {
    return QStringLiteral("%1. %2 [%3]: %4")
        .arg(event.order)
        .arg(event.taskId, agentTaskStatusName(event.status),
             event.summary.isEmpty() ? defaultTransitionSummary(event.status) : event.summary);
}

QStringList agentTaskLifecycleSummaries(const AgentTaskLifecycle& lifecycle) {
    auto ordered = lifecycle.events;
    std::sort(ordered.begin(), ordered.end(),
              [](const AgentTaskLifecycleEvent& left, const AgentTaskLifecycleEvent& right) {
                  if (left.order == right.order) {
                      return left.taskId < right.taskId;
                  }
                  return left.order < right.order;
              });

    QStringList summaries;
    for (const auto& event : ordered) {
        summaries.append(agentTaskLifecycleEventSummary(event));
    }
    return summaries;
}

QString agentTaskQueueSummaryText(const AgentTaskQueueSummary& summary) {
    return QStringLiteral(
               "%1 queue: %2 total, %3 active, %4 planned, %5 blocked, %6 completed, %7 refused.")
        .arg(agentTaskQueueStatusName(summary.status))
        .arg(summary.totalCount)
        .arg(summary.activeCount)
        .arg(summary.plannedCount)
        .arg(summary.blockedCount)
        .arg(summary.completedCount)
        .arg(summary.refusedCount);
}

QStringList agentTaskQueueTaskSummaries(const AgentTaskQueue& queue) {
    QStringList summaries;
    for (const auto& task : queue.tasks) {
        summaries.append(agentTaskSummary(task));
    }
    return summaries;
}

QString agentPlanningCandidateSummary(const AgentPlanningCandidate& candidate) {
    const auto status = candidate.refused ? QStringLiteral("Refused") : QStringLiteral("Planned");
    return QStringLiteral("%1. %2 [%3/%4]: %5")
        .arg(candidate.order)
        .arg(agentTaskTypeName(candidate.taskType), status, agentTaskPriorityName(candidate.priority),
             candidate.summary);
}

QStringList agentPlanningCandidateSummaries(const AgentPlanningSession& session) {
    QStringList summaries;
    for (const auto& candidate : session.result.candidates) {
        summaries.append(agentPlanningCandidateSummary(candidate));
    }
    return summaries;
}

QStringList agentPlanningRefusalSummaries(const AgentPlanningSession& session) {
    QStringList summaries;
    for (const auto& refusal : session.result.refusals) {
        summaries.append(refusal.safeSummary);
    }
    return summaries;
}

QStringList agentPlanningArbitrationSummaries(const AgentPlanningSession& session) {
    return session.result.arbitration.summaries;
}

QString agentPlanningFallbackSummary(const AgentPlanningSession& session) {
    return session.result.fallback.summary.isEmpty()
        ? QStringLiteral("No planning fallback required.")
        : session.result.fallback.summary;
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
    return orderedTasks();
}

AgentTaskQueue StaticAgentTaskRuntime::queue() const {
    const auto summary = queueSummary();
    return AgentTaskQueue{
        summary.status,
        orderedTasks(),
        AgentTaskQueuePolicy{},
        summary,
    };
}

AgentTask StaticAgentTaskRuntime::createTask(AgentTaskType type, AgentTaskSource source,
                                             AgentTaskPriority priority, const QString& summary) {
    auto task = makeTask(type, source, priority,
                         summary.trimmed().isEmpty() ? defaultTaskSummary(type) : summary);
    tasks_.append(task);
    return task;
}

AgentTaskQueueResult StaticAgentTaskRuntime::markTaskPlanned(const AgentTaskId& id) {
    return updateTaskStatus(id, AgentTaskStatus::Planned,
                            QStringLiteral("Task marked planned as metadata only."), true);
}

AgentTaskQueueResult StaticAgentTaskRuntime::markTaskBlocked(const AgentTaskId& id,
                                                             const QString& reason) {
    const auto summary = reason.trimmed().isEmpty()
        ? QStringLiteral("Task blocked before execution.")
        : QStringLiteral("Task blocked before execution: %1").arg(reason.trimmed());
    return updateTaskStatus(id, AgentTaskStatus::Blocked, summary, true);
}

AgentTaskQueueResult StaticAgentTaskRuntime::completeTaskAsMetadata(const AgentTaskId& id,
                                                                    const QString& summary) {
    const auto resultSummary = summary.trimmed().isEmpty()
        ? QStringLiteral("Task completed as metadata without execution.")
        : summary.trimmed();
    return updateTaskStatus(id, AgentTaskStatus::CompletedMetadata, resultSummary, true);
}

AgentTaskQueueResult StaticAgentTaskRuntime::refuseTask(const AgentTaskId& id,
                                                        const QString& reason) {
    const auto summary = reason.trimmed().isEmpty()
        ? QStringLiteral("Task refused at the no-execution boundary.")
        : QStringLiteral("Task refused at the no-execution boundary: %1").arg(reason.trimmed());
    return updateTaskStatus(id, AgentTaskStatus::Refused, summary, true);
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

AgentPlanningSession StaticAgentTaskRuntime::planningSession() const {
    const AgentPlanningSessionPolicy policy;
    const auto ordered = orderedTasks();
    const auto candidateLimit = std::max(0, policy.budget.maxCandidates);
    const auto selectedCount = std::min(static_cast<int>(ordered.size()), candidateLimit);
    const auto omittedCount = static_cast<int>(ordered.size()) - selectedCount;

    QList<AgentPlanningCandidate> candidates;
    QList<AgentPlanningRefusal> refusals;
    QStringList arbitrationSummaries;

    for (int index = 0; index < selectedCount; ++index) {
        auto candidate = planningCandidateForTask(ordered.at(index), index + 1);
        if (candidate.refused) {
            refusals.append(candidate.refusal);
        }
        arbitrationSummaries.append(QStringLiteral("%1 selected by priority/queue/id order.")
                                        .arg(candidate.id));
        candidates.append(candidate);
    }

    const auto refusedCount = static_cast<int>(refusals.size());
    AgentPlanningSessionStatus status = AgentPlanningSessionStatus::Ready;
    if (refusedCount > 0) {
        status = AgentPlanningSessionStatus::Refused;
    } else if (omittedCount > 0) {
        status = AgentPlanningSessionStatus::Bounded;
    }

    AgentPlanningFallback fallback;
    if (omittedCount > 0) {
        fallback.used = true;
        fallback.reason = QStringLiteral("candidate budget exhausted");
        fallback.summary =
            QStringLiteral("%1 planning candidate(s) omitted after deterministic budget limit.")
                .arg(omittedCount);
    } else if (refusedCount > 0) {
        fallback.used = true;
        fallback.reason = QStringLiteral("unsafe candidate refused");
        fallback.summary =
            QStringLiteral("%1 unsafe planning candidate(s) refused; metadata-only fallback "
                           "preserved.")
                .arg(refusedCount);
    } else {
        fallback.summary = QStringLiteral("No planning fallback required.");
    }

    const AgentPlanningArbitration arbitration{
        selectedCount,
        refusedCount,
        omittedCount,
        QStringLiteral("Planning arbitration selected %1 candidate(s), refused %2, omitted %3 by "
                       "deterministic priority/queue/id order.")
            .arg(selectedCount)
            .arg(refusedCount)
            .arg(omittedCount),
        arbitrationSummaries,
    };

    const AgentPlanningSafetyReport safetyReport{
        refusedCount == 0,
        true,
        false,
        true,
        true,
        true,
        true,
        true,
        true,
        refusedCount == 0
            ? QStringLiteral("Planning session passed metadata-only safety checks.")
            : QStringLiteral("Planning session refused unsafe metadata before execution."),
    };

    const AgentPlanningResult result{
        status,
        candidates,
        arbitration,
        refusals,
        safetyReport,
        fallback,
        false,
        QStringLiteral("%1 planning session: %2 candidate(s), %3 refused, %4 omitted; execution "
                       "attempted: no.")
            .arg(agentPlanningSessionStatusName(status))
            .arg(selectedCount)
            .arg(refusedCount)
            .arg(omittedCount),
    };

    const AgentPlanningSessionSummary summary{
        status,
        selectedCount,
        refusedCount,
        fallback.used ? 1 : 0,
        result.summary,
    };

    return AgentPlanningSession{
        AgentPlanningSessionId{QStringLiteral("agent-planning-session-local")},
        status,
        policy,
        result,
        summary,
    };
}

AgentTask StaticAgentTaskRuntime::makeTask(AgentTaskType type, AgentTaskSource source,
                                           AgentTaskPriority priority, const QString& summary) {
    const auto id = AgentTaskId{QStringLiteral("agent-task-%1").arg(nextTaskSequence_++)};
    AgentTask task;
    task.id = id;
    task.type = type;
    task.status = AgentTaskStatus::Queued;
    task.priority = priority;
    task.source = source;
    task.queueOrder = nextTaskSequence_ - 1;
    task.summary = summary;
    task.safetyPolicy = AgentTaskSafetyPolicy{};
    task.plan = planTask(task);
    task.result = refuseExecution(task);
    task.lifecycle = AgentTaskLifecycle{
        task.id.value,
        QList<AgentTaskLifecycleEvent>{
            AgentTaskLifecycleEvent{nextLifecycleSequence_++, task.id.value,
                                    AgentTaskStatus::Queued,
                                    QStringLiteral("%1 task queued as metadata from %2.")
                                        .arg(agentTaskTypeName(type), agentTaskSourceName(source))},
        },
        QStringLiteral("%1 task queued as metadata.").arg(agentTaskTypeName(type)),
    };
    task.traces = {
        traceFor(1, QStringLiteral("Task Queued"), AgentTaskStatus::Queued,
                 QStringLiteral("%1 task metadata created from %2.")
                     .arg(agentTaskTypeName(type), agentTaskSourceName(source))),
        traceFor(2, QStringLiteral("Plan Prepared"), AgentTaskStatus::Planned, task.plan.summary),
        traceFor(3, QStringLiteral("Execution Boundary"), AgentTaskStatus::Refused,
                 task.result.summary),
    };
    return task;
}

AgentTaskQueueResult StaticAgentTaskRuntime::updateTaskStatus(const AgentTaskId& id,
                                                              AgentTaskStatus status,
                                                              const QString& summary,
                                                              bool accepted) {
    for (auto& task : tasks_) {
        if (task.id.value != id.value) {
            continue;
        }

        task.status = status;
        task.lifecycle.events.append(AgentTaskLifecycleEvent{
            nextLifecycleSequence_++, task.id.value, status,
            summary.trimmed().isEmpty() ? defaultTransitionSummary(status) : summary.trimmed()});
        task.lifecycle.summary = QStringLiteral("%1 [%2]: %3")
                                     .arg(task.id.value, agentTaskStatusName(status),
                                          task.lifecycle.events.last().summary);
        task.traces.append(traceFor(task.traces.size() + 1, QStringLiteral("Lifecycle Update"),
                                    status, task.lifecycle.events.last().summary));
        if (status == AgentTaskStatus::Refused) {
            task.result = refuseExecution(task);
        }
        return AgentTaskQueueResult{status, task.id.value, task.lifecycle.events.last().summary,
                                    accepted, false};
    }

    return AgentTaskQueueResult{
        AgentTaskStatus::Refused,
        id.value,
        QStringLiteral("Task id not found; queue request refused without execution."),
        false,
        false,
    };
}

AgentPlanningCandidate StaticAgentTaskRuntime::planningCandidateForTask(const AgentTask& task,
                                                                        int order) const {
    const AgentPlanningSessionPolicy policy;
    const auto summary =
        boundedSummary(task.summary.isEmpty() ? defaultTaskSummary(task.type) : task.summary,
                       policy.budget.maxSummaryCharacters);
    const auto unsafe = containsUnsafePlanningIntent(summary);
    const auto candidateId = QStringLiteral("agent-planning-candidate-%1").arg(order);
    const auto refusalSummary = unsafe
        ? QStringLiteral("%1 refused: planning request mentions blocked execution authority.")
              .arg(candidateId)
        : QString();

    AgentPlanningCandidate candidate;
    candidate.id = candidateId;
    candidate.taskId = task.id.value;
    candidate.taskType = task.type;
    candidate.source = task.source;
    candidate.priority = task.priority;
    candidate.order = order;
    candidate.summary = summary;
    candidate.stepSummaries = {
        QStringLiteral("Classify %1 as %2 metadata.")
            .arg(task.id.value, agentTaskSourceName(task.source)),
        QStringLiteral("Prepare bounded %1 plan metadata.")
            .arg(agentTaskTypeName(task.type).toLower()),
        QStringLiteral("Stop at the no-execution planning boundary."),
    };
    candidate.safetyReport = AgentPlanningSafetyReport{
        !unsafe,
        true,
        false,
        true,
        true,
        true,
        true,
        true,
        true,
        unsafe ? QStringLiteral("Planning candidate refused before execution authority.")
               : QStringLiteral("Planning candidate passed metadata-only safety checks."),
    };
    candidate.refused = unsafe;
    candidate.refusal = AgentPlanningRefusal{
        candidateId,
        unsafe ? QStringLiteral("blocked execution authority") : QString(),
        refusalSummary,
    };
    return candidate;
}

AgentTaskQueueSummary StaticAgentTaskRuntime::queueSummary() const {
    AgentTaskQueueSummary summary;
    summary.status =
        tasks_.isEmpty() ? AgentTaskQueueStatus::Empty : AgentTaskQueueStatus::RefusingExecution;
    summary.totalCount = static_cast<int>(tasks_.size());

    int latestOrder = -1;
    for (const auto& task : tasks_) {
        switch (task.status) {
        case AgentTaskStatus::Queued:
        case AgentTaskStatus::Planned:
            ++summary.plannedCount;
            break;
        case AgentTaskStatus::Active:
            ++summary.activeCount;
            break;
        case AgentTaskStatus::Blocked:
            ++summary.blockedCount;
            break;
        case AgentTaskStatus::CompletedMetadata:
            ++summary.completedCount;
            break;
        case AgentTaskStatus::Refused:
            ++summary.refusedCount;
            break;
        }

        for (const auto& event : task.lifecycle.events) {
            if (event.order > latestOrder) {
                latestOrder = event.order;
                summary.latestLifecycleSummary = agentTaskLifecycleEventSummary(event);
            }
        }
    }

    summary.summary = agentTaskQueueSummaryText(summary);
    return summary;
}

QList<AgentTask> StaticAgentTaskRuntime::orderedTasks() const {
    auto ordered = tasks_;
    std::sort(ordered.begin(), ordered.end(), [](const AgentTask& left, const AgentTask& right) {
        const auto leftPriority = priorityRank(left.priority);
        const auto rightPriority = priorityRank(right.priority);
        if (leftPriority != rightPriority) {
            return leftPriority < rightPriority;
        }
        if (left.queueOrder != right.queueOrder) {
            return left.queueOrder < right.queueOrder;
        }
        return left.id.value < right.id.value;
    });
    return ordered;
}

} // namespace sentinel::core
