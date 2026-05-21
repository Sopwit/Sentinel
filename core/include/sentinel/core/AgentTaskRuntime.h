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

struct AgentPlanningSessionId {
    QString value;
};

enum class AgentPlanningSessionStatus : std::uint8_t {
    Ready,
    Bounded,
    Refused,
};

struct AgentCapabilityId {
    QString value;
};

struct ToolContractId {
    QString value;
};

enum class AgentCapabilityType : std::uint8_t {
    ConversationSummarization,
    MemoryInspection,
    RetrievalPreparation,
    SemanticSupplementPreparation,
    ExportPreparation,
    VoiceResponsePreparation,
    FilesystemAccess,
    ShellExecution,
    PluginRuntime,
};

enum class AgentCapabilityStatus : std::uint8_t {
    EnabledMetadata,
    Disabled,
    Refused,
};

enum class AgentCapabilityScope : std::uint8_t {
    LocalMetadata,
    FutureRuntime,
};

enum class AgentCapabilityRegistryStatus : std::uint8_t {
    Ready,
    Restricted,
    RefusingUnsafeCapabilities,
};

enum class ToolContractType : std::uint8_t {
    ConversationSummary,
    MemoryInspection,
    RetrievalPreparation,
    SemanticSupplementPreparation,
    VoiceResponsePreparation,
    ExportPreparation,
    FutureFilesystemAccess,
    FutureSubprocessExecution,
    FuturePluginRuntime,
    FutureExportAction,
};

enum class ToolContractStatus : std::uint8_t {
    EnabledMetadata,
    Disabled,
    Refused,
};

enum class ToolContractScope : std::uint8_t {
    LocalMetadata,
    FutureRuntime,
    UnsafeRuntime,
};

enum class ToolContractPermission : std::uint8_t {
    LocalOnly,
    ApprovalRequired,
    SandboxRequired,
    ReadOnly,
    Disabled,
    Refused,
    FutureFilesystemAccess,
    FutureSubprocessExecution,
    FuturePluginRuntime,
    FutureExportAction,
};

enum class ToolContractSandbox : std::uint8_t {
    NotRequired,
    RequiredMetadata,
    Denied,
};

enum class ToolContractRegistryStatus : std::uint8_t {
    Ready,
    Restricted,
    RefusingUnsafeContracts,
};

QString agentTaskTypeName(AgentTaskType type);
QString agentTaskStatusName(AgentTaskStatus status);
QString agentTaskPriorityName(AgentTaskPriority priority);
QString agentTaskSourceName(AgentTaskSource source);
QString agentTaskRuntimeStateName(AgentTaskRuntimeState state);
QString agentTaskQueueStatusName(AgentTaskQueueStatus status);
QString agentPlanningSessionStatusName(AgentPlanningSessionStatus status);
QString agentCapabilityTypeName(AgentCapabilityType type);
QString agentCapabilityStatusName(AgentCapabilityStatus status);
QString agentCapabilityScopeName(AgentCapabilityScope scope);
QString agentCapabilityRegistryStatusName(AgentCapabilityRegistryStatus status);
QString toolContractTypeName(ToolContractType type);
QString toolContractStatusName(ToolContractStatus status);
QString toolContractScopeName(ToolContractScope scope);
QString toolContractPermissionName(ToolContractPermission permission);
QString toolContractSandboxName(ToolContractSandbox sandbox);
QString toolContractRegistryStatusName(ToolContractRegistryStatus status);

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

struct AgentPlanningBudget {
    int maxCandidates = 6;
    int maxStepsPerCandidate = 3;
    int maxSummaryCharacters = 240;
    QString summary = QStringLiteral(
        "Planning budget is bounded to 6 candidates, 3 metadata steps per candidate, and 240 "
        "summary characters.");
};

struct AgentPlanningSessionPolicy {
    bool enabled = true;
    bool localOnly = true;
    bool deterministicOrdering = true;
    bool executionAllowed = false;
    bool toolExecutionAllowed = false;
    bool filesystemActionsAllowed = false;
    bool shellExecutionAllowed = false;
    bool pluginExecutionAllowed = false;
    bool cloudCallsAllowed = false;
    bool backgroundWorkersAllowed = false;
    AgentPlanningBudget budget;
    QString summary = QStringLiteral(
        "Agent planning sessions are local metadata only; execution, tools, plugins, filesystem "
        "actions, shell, cloud calls, and background workers are blocked.");
};

struct AgentPlanningSafetyReport {
    bool safe = true;
    bool localOnly = true;
    bool executionAttempted = false;
    bool toolExecutionBlocked = true;
    bool filesystemActionsBlocked = true;
    bool shellExecutionBlocked = true;
    bool pluginExecutionBlocked = true;
    bool cloudCallsBlocked = true;
    bool backgroundWorkersBlocked = true;
    QString summary = QStringLiteral("Planning candidate passed metadata-only safety checks.");
};

struct AgentPlanningRefusal {
    QString candidateId;
    QString reason;
    QString safeSummary;
};

struct AgentPlanningCandidate {
    QString id;
    QString taskId;
    AgentTaskType taskType = AgentTaskType::PlanResponse;
    AgentTaskSource source = AgentTaskSource::DesktopReadiness;
    AgentTaskPriority priority = AgentTaskPriority::Normal;
    int order = 0;
    QString summary;
    QStringList stepSummaries;
    AgentPlanningSafetyReport safetyReport;
    bool refused = false;
    AgentPlanningRefusal refusal;
};

struct AgentPlanningArbitration {
    int selectedCount = 0;
    int refusedCount = 0;
    int omittedCount = 0;
    QString summary;
    QStringList summaries;
};

struct AgentPlanningFallback {
    bool used = false;
    QString reason;
    QString summary;
};

struct AgentPlanningResult {
    AgentPlanningSessionStatus status = AgentPlanningSessionStatus::Ready;
    QList<AgentPlanningCandidate> candidates;
    AgentPlanningArbitration arbitration;
    QList<AgentPlanningRefusal> refusals;
    AgentPlanningSafetyReport safetyReport;
    AgentPlanningFallback fallback;
    bool executionAttempted = false;
    QString summary;
};

struct AgentPlanningSessionSummary {
    AgentPlanningSessionStatus status = AgentPlanningSessionStatus::Ready;
    int candidateCount = 0;
    int refusedCount = 0;
    int fallbackCount = 0;
    QString summary;
};

struct AgentPlanningSession {
    AgentPlanningSessionId id;
    AgentPlanningSessionStatus status = AgentPlanningSessionStatus::Ready;
    AgentPlanningSessionPolicy policy;
    AgentPlanningResult result;
    AgentPlanningSessionSummary summary;
};

struct AgentCapabilityRequirement {
    QString summary;
    bool satisfied = true;
};

struct AgentCapabilityRestriction {
    QString summary;
    bool restricted = false;
};

struct AgentCapabilityPolicy {
    bool localOnly = true;
    bool metadataOnly = true;
    bool executionAllowed = false;
    bool toolExecutionAllowed = false;
    bool filesystemActionsAllowed = false;
    bool shellExecutionAllowed = false;
    bool pluginExecutionAllowed = false;
    bool cloudCallsAllowed = false;
    QString summary = QStringLiteral(
        "Agent capability is metadata-only; execution, tools, filesystem actions, shell, "
        "plugins, and cloud calls are blocked.");
};

struct AgentCapabilityReadiness {
    bool ready = true;
    bool refused = false;
    QString summary;
};

struct AgentCapabilitySafetyReport {
    bool safe = true;
    bool executionAttempted = false;
    bool restricted = false;
    QString refusalSummary;
    QString summary;
};

struct AgentCapabilitySummary {
    QString id;
    AgentCapabilityType type = AgentCapabilityType::ConversationSummarization;
    AgentCapabilityStatus status = AgentCapabilityStatus::Disabled;
    AgentCapabilityScope scope = AgentCapabilityScope::LocalMetadata;
    bool restricted = false;
    bool executionAttempted = false;
    QString summary;
    QString readinessSummary;
    QString safetySummary;
};

struct AgentCapability {
    AgentCapabilityId id;
    AgentCapabilityType type = AgentCapabilityType::ConversationSummarization;
    AgentCapabilityStatus status = AgentCapabilityStatus::Disabled;
    AgentCapabilityScope scope = AgentCapabilityScope::LocalMetadata;
    int order = 0;
    QString summary;
    AgentCapabilityPolicy policy;
    QList<AgentCapabilityRequirement> requirements;
    QList<AgentCapabilityRestriction> restrictions;
    AgentCapabilityReadiness readiness;
    AgentCapabilitySafetyReport safetyReport;
};

struct AgentCapabilityRegistrySummary {
    AgentCapabilityRegistryStatus status = AgentCapabilityRegistryStatus::Ready;
    int totalCount = 0;
    int enabledCount = 0;
    int disabledCount = 0;
    int restrictedCount = 0;
    int refusedCount = 0;
    bool executionAttempted = false;
    QString summary;
};

struct AgentCapabilityRegistry {
    AgentCapabilityRegistryStatus status = AgentCapabilityRegistryStatus::Ready;
    QList<AgentCapability> capabilities;
    AgentCapabilityRegistrySummary summary;
    QString safetySummary;
    QString readinessSummary;
};

struct ToolContractPolicy {
    bool localOnly = true;
    bool metadataOnly = true;
    bool executionAllowed = false;
    bool toolRuntimeAllowed = false;
    bool filesystemActionsAllowed = false;
    bool subprocessExecutionAllowed = false;
    bool pluginRuntimeAllowed = false;
    bool cloudCallsAllowed = false;
    QString summary = QStringLiteral(
        "Tool contract is metadata-only; execution, tool runtime, filesystem actions, "
        "subprocesses, plugins, and cloud calls are blocked.");
};

struct ToolContractRestriction {
    QString summary;
    bool restricted = false;
};

struct ToolContractSafetyReport {
    bool safe = true;
    bool executionAttempted = false;
    bool restricted = false;
    bool unsafeScopeDenied = false;
    QString refusalSummary;
    QString summary;
};

struct ToolContractReadiness {
    bool ready = true;
    bool sandboxRequired = false;
    bool refused = false;
    QString summary;
};

struct ToolContractSummary {
    QString id;
    ToolContractType type = ToolContractType::ConversationSummary;
    ToolContractStatus status = ToolContractStatus::Disabled;
    ToolContractScope scope = ToolContractScope::LocalMetadata;
    bool restricted = false;
    bool executionAttempted = false;
    QString summary;
    QString permissionSummary;
    QString sandboxSummary;
    QString readinessSummary;
    QString safetySummary;
};

struct ToolContract {
    ToolContractId id;
    ToolContractType type = ToolContractType::ConversationSummary;
    ToolContractStatus status = ToolContractStatus::Disabled;
    ToolContractScope scope = ToolContractScope::LocalMetadata;
    int order = 0;
    QString summary;
    ToolContractPolicy policy;
    QList<ToolContractPermission> permissions;
    ToolContractSandbox sandbox = ToolContractSandbox::NotRequired;
    QList<ToolContractRestriction> restrictions;
    ToolContractSafetyReport safetyReport;
    ToolContractReadiness readiness;
};

struct ToolContractRegistrySummary {
    ToolContractRegistryStatus status = ToolContractRegistryStatus::Ready;
    int totalCount = 0;
    int enabledCount = 0;
    int disabledCount = 0;
    int restrictedCount = 0;
    int refusedCount = 0;
    bool executionAttempted = false;
    QString summary;
};

struct ToolContractRegistry {
    ToolContractRegistryStatus status = ToolContractRegistryStatus::Ready;
    QList<ToolContract> contracts;
    ToolContractRegistrySummary summary;
    QString permissionSummary;
    QString sandboxSummary;
    QString readinessSummary;
    QString safetySummary;
};

QString agentTaskSummary(const AgentTask& task);
QString agentTaskTraceSummary(const AgentTaskTrace& trace);
QStringList agentTaskTraceSummaries(const QList<AgentTaskTrace>& traces);
QString agentTaskLifecycleEventSummary(const AgentTaskLifecycleEvent& event);
QStringList agentTaskLifecycleSummaries(const AgentTaskLifecycle& lifecycle);
QString agentTaskQueueSummaryText(const AgentTaskQueueSummary& summary);
QStringList agentTaskQueueTaskSummaries(const AgentTaskQueue& queue);
QString agentPlanningCandidateSummary(const AgentPlanningCandidate& candidate);
QStringList agentPlanningCandidateSummaries(const AgentPlanningSession& session);
QStringList agentPlanningRefusalSummaries(const AgentPlanningSession& session);
QStringList agentPlanningArbitrationSummaries(const AgentPlanningSession& session);
QString agentPlanningFallbackSummary(const AgentPlanningSession& session);
AgentCapabilitySummary agentCapabilitySummary(const AgentCapability& capability);
QString agentCapabilitySummaryText(const AgentCapability& capability);
QStringList agentCapabilitySummaries(const AgentCapabilityRegistry& registry);
QStringList agentCapabilityReadinessSummaries(const AgentCapabilityRegistry& registry);
QStringList agentCapabilitySafetySummaries(const AgentCapabilityRegistry& registry);
ToolContractSummary toolContractSummary(const ToolContract& contract);
QString toolContractSummaryText(const ToolContract& contract);
QString toolContractPermissionSummary(const ToolContract& contract);
QString toolContractSandboxSummary(const ToolContract& contract);
QStringList toolContractSummaries(const ToolContractRegistry& registry);
QStringList toolContractPermissionSummaries(const ToolContractRegistry& registry);
QStringList toolContractSandboxSummaries(const ToolContractRegistry& registry);
QStringList toolContractReadinessSummaries(const ToolContractRegistry& registry);
QStringList toolContractSafetySummaries(const ToolContractRegistry& registry);

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
    virtual AgentPlanningSession planningSession() const = 0;
    virtual AgentCapabilityRegistry capabilityRegistry() const = 0;
    virtual ToolContractRegistry toolContractRegistry() const = 0;
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
    AgentPlanningSession planningSession() const override;
    AgentCapabilityRegistry capabilityRegistry() const override;
    ToolContractRegistry toolContractRegistry() const override;

private:
    AgentTask makeTask(AgentTaskType type, AgentTaskSource source, AgentTaskPriority priority,
                       const QString& summary);
    AgentTaskQueueResult updateTaskStatus(const AgentTaskId& id, AgentTaskStatus status,
                                          const QString& summary, bool accepted);
    AgentPlanningCandidate planningCandidateForTask(const AgentTask& task, int order) const;
    AgentCapability makeCapability(AgentCapabilityType type, AgentCapabilityStatus status,
                                   AgentCapabilityScope scope, int order,
                                   const QString& summary) const;
    ToolContract makeToolContract(ToolContractType type, ToolContractStatus status,
                                  ToolContractScope scope, int order, const QString& summary,
                                  QList<ToolContractPermission> permissions,
                                  ToolContractSandbox sandbox) const;
    AgentTaskQueueSummary queueSummary() const;
    QList<AgentTask> orderedTasks() const;

    QList<AgentTask> tasks_;
    int nextTaskSequence_ = 1;
    int nextLifecycleSequence_ = 1;
};

} // namespace sentinel::core
