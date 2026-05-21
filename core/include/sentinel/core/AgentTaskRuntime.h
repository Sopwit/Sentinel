#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

struct AgentTaskId {
    QString value;
};

enum class AgentTaskType : std::uint8_t {
    SummarizeConversation,
    InspectMemoryStatus,
    PlanResponse,
    PrepareRetrievalContext,
    PrepareVoiceResponse,
    PrepareExportAction,
};

enum class AgentTaskStatus : std::uint8_t {
    Queued,
    Planned,
    Active,
    Blocked,
    CompletedMetadata,
    Refused,
};

enum class AgentTaskPriority : std::uint8_t {
    Low,
    Normal,
    High,
};

enum class AgentTaskSource : std::uint8_t {
    DesktopReadiness,
    ConversationMetadata,
    MemoryMetadata,
    VoiceMetadata,
    ExportMetadata,
};

enum class AgentTaskRuntimeState : std::uint8_t {
    Ready,
    RefusingExecution,
};

enum class AgentTaskQueueStatus : std::uint8_t {
    Ready,
    Empty,
    RefusingExecution,
};

QString agentTaskTypeName(AgentTaskType type);
QString agentTaskStatusName(AgentTaskStatus status);
QString agentTaskPriorityName(AgentTaskPriority priority);
QString agentTaskSourceName(AgentTaskSource source);
QString agentTaskRuntimeStateName(AgentTaskRuntimeState state);
QString agentTaskQueueStatusName(AgentTaskQueueStatus status);

struct AgentTaskStep {
    int order = 0;
    QString id;
    QString summary;
    AgentTaskStatus status = AgentTaskStatus::Planned;
};

struct AgentTaskPlan {
    AgentTaskStatus status = AgentTaskStatus::Planned;
    QString summary;
    QList<AgentTaskStep> steps;
};

struct AgentTaskResult {
    AgentTaskStatus status = AgentTaskStatus::Refused;
    QString summary;
    bool executionAttempted = false;
};

struct AgentTaskQueueResult {
    AgentTaskStatus status = AgentTaskStatus::Queued;
    QString taskId;
    QString summary;
    bool accepted = true;
    bool executionAttempted = false;
};

struct AgentTaskTrace {
    int order = 0;
    QString stage;
    AgentTaskStatus status = AgentTaskStatus::Planned;
    QString summary;
};

struct AgentTaskLifecycleEvent {
    int order = 0;
    QString taskId;
    AgentTaskStatus status = AgentTaskStatus::Queued;
    QString summary;
};

struct AgentTaskLifecycle {
    QString taskId;
    QList<AgentTaskLifecycleEvent> events;
    QString summary;
};

struct AgentTaskSafetyPolicy {
    bool localOnly = true;
    bool toolExecutionAllowed = false;
    bool filesystemActionsAllowed = false;
    bool shellExecutionAllowed = false;
    bool pluginExecutionAllowed = false;
    bool cloudCallsAllowed = false;
    QString summary = QStringLiteral(
        "Agent task runtime is metadata-only; execution, tools, shell, plugins, filesystem "
        "actions, and cloud calls are blocked.");
};

struct AgentTask {
    AgentTaskId id;
    AgentTaskType type = AgentTaskType::PlanResponse;
    AgentTaskStatus status = AgentTaskStatus::Queued;
    AgentTaskPriority priority = AgentTaskPriority::Normal;
    AgentTaskSource source = AgentTaskSource::DesktopReadiness;
    int queueOrder = 0;
    QString summary;
    AgentTaskPlan plan;
    AgentTaskResult result;
    QList<AgentTaskTrace> traces;
    AgentTaskLifecycle lifecycle;
    AgentTaskSafetyPolicy safetyPolicy;
};

struct AgentTaskQueuePolicy {
    bool deterministicOrdering = true;
    bool executionAllowed = false;
    bool backgroundWorkersAllowed = false;
    QString summary = QStringLiteral(
        "Agent task queue is deterministic metadata only; execution and background work are "
        "blocked.");
};

struct AgentTaskQueueSummary {
    AgentTaskQueueStatus status = AgentTaskQueueStatus::Ready;
    int totalCount = 0;
    int activeCount = 0;
    int plannedCount = 0;
    int blockedCount = 0;
    int completedCount = 0;
    int refusedCount = 0;
    QString latestLifecycleSummary;
    QString summary;
};

struct AgentTaskQueue {
    AgentTaskQueueStatus status = AgentTaskQueueStatus::Ready;
    QList<AgentTask> tasks;
    AgentTaskQueuePolicy policy;
    AgentTaskQueueSummary summary;
};

struct AgentTaskRuntimeStatus {
    AgentTaskRuntimeState state = AgentTaskRuntimeState::Ready;
    QString activeTaskId;
    int taskCount = 0;
    QString summary;
    AgentTaskSafetyPolicy safetyPolicy;
};

QString agentTaskSummary(const AgentTask& task);
QString agentTaskTraceSummary(const AgentTaskTrace& trace);
QStringList agentTaskTraceSummaries(const QList<AgentTaskTrace>& traces);
QString agentTaskLifecycleEventSummary(const AgentTaskLifecycleEvent& event);
QStringList agentTaskLifecycleSummaries(const AgentTaskLifecycle& lifecycle);
QString agentTaskQueueSummaryText(const AgentTaskQueueSummary& summary);
QStringList agentTaskQueueTaskSummaries(const AgentTaskQueue& queue);

class IAgentTaskRuntime {
public:
    virtual ~IAgentTaskRuntime() = default;

    virtual QString name() const = 0;
    virtual AgentTaskRuntimeStatus runtimeStatus() const = 0;
    virtual QList<AgentTask> tasks() const = 0;
    virtual AgentTaskQueue queue() const = 0;
    virtual AgentTask createTask(AgentTaskType type, AgentTaskSource source,
                                 AgentTaskPriority priority, const QString& summary) = 0;
    virtual AgentTaskQueueResult markTaskPlanned(const AgentTaskId& id) = 0;
    virtual AgentTaskQueueResult markTaskBlocked(const AgentTaskId& id, const QString& reason) = 0;
    virtual AgentTaskQueueResult completeTaskAsMetadata(const AgentTaskId& id,
                                                        const QString& summary) = 0;
    virtual AgentTaskQueueResult refuseTask(const AgentTaskId& id, const QString& reason) = 0;
    virtual AgentTaskPlan planTask(const AgentTask& task) const = 0;
    virtual AgentTaskResult refuseExecution(const AgentTask& task) const = 0;
};

class StaticAgentTaskRuntime final : public IAgentTaskRuntime {
public:
    StaticAgentTaskRuntime();

    QString name() const override;
    AgentTaskRuntimeStatus runtimeStatus() const override;
    QList<AgentTask> tasks() const override;
    AgentTaskQueue queue() const override;
    AgentTask createTask(AgentTaskType type, AgentTaskSource source, AgentTaskPriority priority,
                         const QString& summary) override;
    AgentTaskQueueResult markTaskPlanned(const AgentTaskId& id) override;
    AgentTaskQueueResult markTaskBlocked(const AgentTaskId& id, const QString& reason) override;
    AgentTaskQueueResult completeTaskAsMetadata(const AgentTaskId& id,
                                                const QString& summary) override;
    AgentTaskQueueResult refuseTask(const AgentTaskId& id, const QString& reason) override;
    AgentTaskPlan planTask(const AgentTask& task) const override;
    AgentTaskResult refuseExecution(const AgentTask& task) const override;

private:
    AgentTask makeTask(AgentTaskType type, AgentTaskSource source, AgentTaskPriority priority,
                       const QString& summary);
    AgentTaskQueueResult updateTaskStatus(const AgentTaskId& id, AgentTaskStatus status,
                                          const QString& summary, bool accepted);
    AgentTaskQueueSummary queueSummary() const;
    QList<AgentTask> orderedTasks() const;

    QList<AgentTask> tasks_;
    int nextTaskSequence_ = 1;
    int nextLifecycleSequence_ = 1;
};

} // namespace sentinel::core
