#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class ExecutionIntent : std::uint8_t {
    ConversationResponse,
    AgentCoordination,
    RuntimePlaceholder,
};

inline QString executionIntentName(ExecutionIntent intent) {
    switch (intent) {
    case ExecutionIntent::ConversationResponse:
        return QStringLiteral("Conversation Response");
    case ExecutionIntent::AgentCoordination:
        return QStringLiteral("Agent Coordination");
    case ExecutionIntent::RuntimePlaceholder:
        return QStringLiteral("Runtime Placeholder");
    }

    return QStringLiteral("Runtime Placeholder");
}

enum class ExecutionPriority : std::uint8_t {
    Low,
    Normal,
    High,
};

inline QString executionPriorityName(ExecutionPriority priority) {
    switch (priority) {
    case ExecutionPriority::Low:
        return QStringLiteral("Low");
    case ExecutionPriority::Normal:
        return QStringLiteral("Normal");
    case ExecutionPriority::High:
        return QStringLiteral("High");
    }

    return QStringLiteral("Normal");
}

enum class ExecutionLifecycleState : std::uint8_t {
    NotRequested,
    Requested,
    Validating,
    PermissionCheck,
    SafetyCheck,
    Coordination,
    ReadyPlaceholder,
    Blocked,
};

inline QString executionLifecycleStateName(ExecutionLifecycleState state) {
    switch (state) {
    case ExecutionLifecycleState::NotRequested:
        return QStringLiteral("Not Requested");
    case ExecutionLifecycleState::Requested:
        return QStringLiteral("Requested");
    case ExecutionLifecycleState::Validating:
        return QStringLiteral("Validating");
    case ExecutionLifecycleState::PermissionCheck:
        return QStringLiteral("Permission Check");
    case ExecutionLifecycleState::SafetyCheck:
        return QStringLiteral("Safety Check");
    case ExecutionLifecycleState::Coordination:
        return QStringLiteral("Coordination");
    case ExecutionLifecycleState::ReadyPlaceholder:
        return QStringLiteral("Ready Placeholder");
    case ExecutionLifecycleState::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Not Requested");
}

enum class ExecutionLifecycleStatus : std::uint8_t {
    NotRequested,
    MetadataOnly,
    ReadyPlaceholder,
    Blocked,
    Rejected,
};

inline QString executionLifecycleStatusName(ExecutionLifecycleStatus status) {
    switch (status) {
    case ExecutionLifecycleStatus::NotRequested:
        return QStringLiteral("Not Requested");
    case ExecutionLifecycleStatus::MetadataOnly:
        return QStringLiteral("Metadata Only");
    case ExecutionLifecycleStatus::ReadyPlaceholder:
        return QStringLiteral("Ready Placeholder");
    case ExecutionLifecycleStatus::Blocked:
        return QStringLiteral("Blocked");
    case ExecutionLifecycleStatus::Rejected:
        return QStringLiteral("Rejected");
    }

    return QStringLiteral("Not Requested");
}

enum class ExecutionTraceLevel : std::uint8_t {
    Info,
    Warning,
    Blocked,
};

inline QString executionTraceLevelName(ExecutionTraceLevel level) {
    switch (level) {
    case ExecutionTraceLevel::Info:
        return QStringLiteral("Info");
    case ExecutionTraceLevel::Warning:
        return QStringLiteral("Warning");
    case ExecutionTraceLevel::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Info");
}

struct ExecutionRequest {
    QString id = QStringLiteral("execution-request-1");
    ExecutionIntent intent = ExecutionIntent::RuntimePlaceholder;
    ExecutionPriority priority = ExecutionPriority::Normal;
    QString summary = QStringLiteral("Metadata-only execution lifecycle request.");
};

struct ExecutionLifecycleTrace {
    int sequence = 0;
    ExecutionLifecycleState state = ExecutionLifecycleState::NotRequested;
    ExecutionTraceLevel level = ExecutionTraceLevel::Info;
    QString summary;
};

struct ExecutionLifecycleResult {
    ExecutionRequest request;
    ExecutionLifecycleState state = ExecutionLifecycleState::NotRequested;
    ExecutionLifecycleStatus status = ExecutionLifecycleStatus::NotRequested;
    QString summary;
    QList<ExecutionLifecycleTrace> traces;
    bool transitionAccepted = false;
    bool executable = false;
};

struct ExecutionSessionId {
    QString value = QStringLiteral("execution-session-1");
};

enum class ExecutionSessionStatus : std::uint8_t {
    Reserved,
    Coordinating,
    Blocked,
};

inline QString executionSessionStatusName(ExecutionSessionStatus status) {
    switch (status) {
    case ExecutionSessionStatus::Reserved:
        return QStringLiteral("Reserved");
    case ExecutionSessionStatus::Coordinating:
        return QStringLiteral("Coordinating");
    case ExecutionSessionStatus::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Reserved");
}

enum class ExecutionOwnership : std::uint8_t {
    ApplicationController,
    MetadataCoordinator,
};

inline QString executionOwnershipName(ExecutionOwnership ownership) {
    switch (ownership) {
    case ExecutionOwnership::ApplicationController:
        return QStringLiteral("Application Controller");
    case ExecutionOwnership::MetadataCoordinator:
        return QStringLiteral("Metadata Coordinator");
    }

    return QStringLiteral("Application Controller");
}

enum class ExecutionCoordinationMode : std::uint8_t {
    MetadataOnly,
    ReadOnlySnapshot,
};

inline QString executionCoordinationModeName(ExecutionCoordinationMode mode) {
    switch (mode) {
    case ExecutionCoordinationMode::MetadataOnly:
        return QStringLiteral("Metadata Only");
    case ExecutionCoordinationMode::ReadOnlySnapshot:
        return QStringLiteral("Read-Only Snapshot");
    }

    return QStringLiteral("Metadata Only");
}

struct ExecutionSession {
    ExecutionSessionId id;
    ExecutionSessionStatus status = ExecutionSessionStatus::Reserved;
    ExecutionOwnership ownership = ExecutionOwnership::ApplicationController;
    ExecutionCoordinationMode coordinationMode = ExecutionCoordinationMode::MetadataOnly;
    QString summary = QStringLiteral("Execution session is reserved for metadata only.");
};

struct ExecutionCoordinationSnapshot {
    ExecutionSession session;
    ExecutionLifecycleResult lifecycle;
    QString summary;
    bool readOnly = true;
    bool executable = false;
};

QString executionLifecycleTraceSummary(const ExecutionLifecycleTrace& trace);
QStringList executionLifecycleTraceSummaries(const QList<ExecutionLifecycleTrace>& traces);
QString safeExecutionLifecycleSummary(const ExecutionLifecycleResult& result);
QString safeExecutionSessionSummary(const ExecutionSession& session);
QString safeExecutionCoordinationSnapshotSummary(const ExecutionCoordinationSnapshot& snapshot);

class IExecutionLifecycle {
public:
    virtual ~IExecutionLifecycle() = default;

    virtual ExecutionLifecycleResult evaluate(const ExecutionRequest& request) const = 0;
    virtual ExecutionLifecycleResult transition(ExecutionLifecycleState current,
                                                ExecutionLifecycleState next,
                                                const ExecutionRequest& request) const = 0;
};

class StaticExecutionLifecycle final : public IExecutionLifecycle {
public:
    ExecutionLifecycleResult evaluate(const ExecutionRequest& request) const override;
    ExecutionLifecycleResult transition(ExecutionLifecycleState current,
                                        ExecutionLifecycleState next,
                                        const ExecutionRequest& request) const override;

private:
    static bool canTransition(ExecutionLifecycleState current, ExecutionLifecycleState next);
};

class ExecutionCoordinator final {
public:
    explicit ExecutionCoordinator(ExecutionSession session = {});

    ExecutionCoordinationSnapshot coordinate(const ExecutionLifecycleResult& lifecycle) const;
    const ExecutionSession& session() const;

private:
    ExecutionSession session_;
};

} // namespace sentinel::core
