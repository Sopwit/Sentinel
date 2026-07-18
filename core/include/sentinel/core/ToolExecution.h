#pragma once

#include "sentinel/core/ToolApproval.h"
#include "sentinel/core/ToolInvocationPlan.h"
#include "sentinel/core/ToolSandbox.h"

#include <QString>
#include <QStringList>

namespace sentinel::core {

enum class ToolExecutionStatus {
    NotRequested,
    PlaceholderSucceeded,
    Succeeded,
    Blocked,
    EmptyPlan,
    UnknownTool,
};

inline QString toolExecutionStatusName(ToolExecutionStatus status) {
    switch (status) {
    case ToolExecutionStatus::NotRequested:
        return QStringLiteral("Not Requested");
    case ToolExecutionStatus::PlaceholderSucceeded:
        return QStringLiteral("Placeholder Succeeded");
    case ToolExecutionStatus::Succeeded:
        return QStringLiteral("Succeeded");
    case ToolExecutionStatus::Blocked:
        return QStringLiteral("Blocked");
    case ToolExecutionStatus::EmptyPlan:
        return QStringLiteral("Empty Plan");
    case ToolExecutionStatus::UnknownTool:
        return QStringLiteral("Unknown Tool");
    }

    return QStringLiteral("Not Requested");
}

struct ToolExecutionRequest {
    ToolInvocationPlan plan;
    ApprovalDecision approval;
    SandboxEvaluationResult sandbox;
    QStringList knownToolIds;
};

struct ToolExecutionResult {
    ToolExecutionStatus status = ToolExecutionStatus::NotRequested;
    QString summary;
};

} // namespace sentinel::core
