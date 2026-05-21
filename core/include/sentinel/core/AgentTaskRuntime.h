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
    Planned,
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

QString agentTaskTypeName(AgentTaskType type);
QString agentTaskStatusName(AgentTaskStatus status);
QString agentTaskPriorityName(AgentTaskPriority priority);
QString agentTaskSourceName(AgentTaskSource source);
QString agentTaskRuntimeStateName(AgentTaskRuntimeState state);

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

struct AgentTaskTrace {
    int order = 0;
    QString stage;
    AgentTaskStatus status = AgentTaskStatus::Planned;
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
    AgentTaskStatus status = AgentTaskStatus::Planned;
    AgentTaskPriority priority = AgentTaskPriority::Normal;
    AgentTaskSource source = AgentTaskSource::DesktopReadiness;
    QString summary;
    AgentTaskPlan plan;
    AgentTaskResult result;
    QList<AgentTaskTrace> traces;
    AgentTaskSafetyPolicy safetyPolicy;
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

class IAgentTaskRuntime {
public:
    virtual ~IAgentTaskRuntime() = default;

    virtual QString name() const = 0;
    virtual AgentTaskRuntimeStatus runtimeStatus() const = 0;
    virtual QList<AgentTask> tasks() const = 0;
    virtual AgentTask createTask(AgentTaskType type, AgentTaskSource source,
                                 AgentTaskPriority priority, const QString& summary) = 0;
    virtual AgentTaskPlan planTask(const AgentTask& task) const = 0;
    virtual AgentTaskResult refuseExecution(const AgentTask& task) const = 0;
};

class StaticAgentTaskRuntime final : public IAgentTaskRuntime {
public:
    StaticAgentTaskRuntime();

    QString name() const override;
    AgentTaskRuntimeStatus runtimeStatus() const override;
    QList<AgentTask> tasks() const override;
    AgentTask createTask(AgentTaskType type, AgentTaskSource source, AgentTaskPriority priority,
                         const QString& summary) override;
    AgentTaskPlan planTask(const AgentTask& task) const override;
    AgentTaskResult refuseExecution(const AgentTask& task) const override;

private:
    AgentTask makeTask(AgentTaskType type, AgentTaskSource source, AgentTaskPriority priority,
                       const QString& summary);

    QList<AgentTask> tasks_;
    int nextTaskSequence_ = 1;
};

} // namespace sentinel::core
