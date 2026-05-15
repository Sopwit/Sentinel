#pragma once

#include "sentinel/core/ToolSandbox.h"

#include <QList>
#include <QString>

namespace sentinel::core {

enum class ToolInvocationPlanStatus {
    NotRequested,
    Planned,
    EmptyRequest,
    NoToolsAvailable,
    UnknownTool,
};

inline QString toolInvocationPlanStatusName(ToolInvocationPlanStatus status) {
    switch (status) {
    case ToolInvocationPlanStatus::NotRequested:
        return QStringLiteral("Not Requested");
    case ToolInvocationPlanStatus::Planned:
        return QStringLiteral("Planned");
    case ToolInvocationPlanStatus::EmptyRequest:
        return QStringLiteral("Empty Request");
    case ToolInvocationPlanStatus::NoToolsAvailable:
        return QStringLiteral("No Tools Available");
    case ToolInvocationPlanStatus::UnknownTool:
        return QStringLiteral("Unknown Tool");
    }

    return QStringLiteral("Not Requested");
}

struct ToolInvocationArgument {
    QString id;
    QString value;
};

struct PlannedToolInvocation {
    QString toolId;
    QString toolName;
    QString summary;
    QString rationale;
    ToolRiskLevel riskLevel = ToolRiskLevel::Low;
    ToolExecutionMode executionMode = ToolExecutionMode::MetadataOnly;
    QList<ToolInvocationArgument> arguments;
    QList<CapabilityDescriptor> requiredCapabilities;
};

struct ToolInvocationPlan {
    ToolInvocationPlanStatus status = ToolInvocationPlanStatus::NotRequested;
    QString summary;
    QList<PlannedToolInvocation> invocations;
};

} // namespace sentinel::core
