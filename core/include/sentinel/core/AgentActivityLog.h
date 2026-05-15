#pragma once

#include <QList>
#include <QString>

namespace sentinel::core {

enum class AgentActivityType {
    RequestReceived,
    PlanCreated,
    ApprovalEvaluated,
    SandboxEvaluated,
    PlaceholderExecutionEvaluated,
    PipelineCompleted,
};

inline QString agentActivityTypeName(AgentActivityType type) {
    switch (type) {
    case AgentActivityType::RequestReceived:
        return QStringLiteral("Request Received");
    case AgentActivityType::PlanCreated:
        return QStringLiteral("Plan Created");
    case AgentActivityType::ApprovalEvaluated:
        return QStringLiteral("Approval Evaluated");
    case AgentActivityType::SandboxEvaluated:
        return QStringLiteral("Sandbox Evaluated");
    case AgentActivityType::PlaceholderExecutionEvaluated:
        return QStringLiteral("Placeholder Execution Evaluated");
    case AgentActivityType::PipelineCompleted:
        return QStringLiteral("Pipeline Completed");
    }

    return QStringLiteral("Request Received");
}

enum class AgentActivityStatus {
    Recorded,
    Completed,
    Blocked,
    Cleared,
};

inline QString agentActivityStatusName(AgentActivityStatus status) {
    switch (status) {
    case AgentActivityStatus::Recorded:
        return QStringLiteral("Recorded");
    case AgentActivityStatus::Completed:
        return QStringLiteral("Completed");
    case AgentActivityStatus::Blocked:
        return QStringLiteral("Blocked");
    case AgentActivityStatus::Cleared:
        return QStringLiteral("Cleared");
    }

    return QStringLiteral("Recorded");
}

struct AgentActivityEntry {
    int sequenceId = 0;
    AgentActivityType type = AgentActivityType::RequestReceived;
    AgentActivityStatus status = AgentActivityStatus::Recorded;
    QString summary;

    QString typeName() const {
        return agentActivityTypeName(type);
    }

    QString statusName() const {
        return agentActivityStatusName(status);
    }
};

class AgentActivityLog {
public:
    const QList<AgentActivityEntry>& entries() const;
    AgentActivityEntry append(AgentActivityType type, AgentActivityStatus status,
                              const QString& summary);
    void clear();
    int count() const;
    QString latestSummary() const;

private:
    QList<AgentActivityEntry> entries_;
    int nextSequenceId_ = 1;
};

} // namespace sentinel::core
