// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/ObservationEvidence.h"
#include "sentinel/core/runtime/ToolApproval.h"
#include "sentinel/core/runtime/IFileSystemService.h"
#include "sentinel/core/runtime/ToolInvocationPlan.h"
#include "sentinel/core/runtime/ToolSandbox.h"

#include <QString>
#include <QStringList>

namespace sentinel::core {

enum class ToolExecutionStatus {
    NotRequested,
    PlaceholderSucceeded,
    Succeeded,
    Failed,
    InvalidArguments,
    InvalidToolContract,
    Blocked,
    Cancelled,
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
    case ToolExecutionStatus::Failed:
        return QStringLiteral("Failed");
    case ToolExecutionStatus::InvalidArguments:
        return QStringLiteral("Invalid Arguments");
    case ToolExecutionStatus::InvalidToolContract:
        return QStringLiteral("Invalid Tool Contract");
    case ToolExecutionStatus::Blocked:
        return QStringLiteral("Blocked");
    case ToolExecutionStatus::Cancelled:
        return QStringLiteral("Cancelled");
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
    StructuredObservationPtr structuredObservation;
    QList<FileMutation> mutations;
};

} // namespace sentinel::core
