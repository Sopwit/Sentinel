// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/ObservationEvidence.h"
#include "sentinel/core/runtime/IFileSystemService.h"
#include "sentinel/core/runtime/ToolApproval.h"
#include "sentinel/core/runtime/ToolInvocationPlan.h"
#include "sentinel/core/runtime/ToolSandbox.h"

#include <QObject>
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

enum class ToolFailureCategory {
    None,
    InvalidArguments,
    NotFound,
    PermissionDenied,
    SecurityDenied,
    Unsupported,
    Cancelled,
    Timeout,
    RuntimeUnavailable,
    NetworkFailure,
    PartialResult,
    InternalFailure,
    ProtocolError,
    InvalidToolSchema,
    RemoteExecutionFailure,
    StartupFailure,
    AuthenticationFailure,
    ConfigurationFailure,
    UnsupportedProtocolVersion,
    InteractionRequired,
    InteractionRejected,
    UnsupportedLegacyTransport,
};

inline QString toolFailureCategoryName(ToolFailureCategory category) {
    switch (category) {
    case ToolFailureCategory::None:
        return QStringLiteral("None");
    case ToolFailureCategory::InvalidArguments:
        return QStringLiteral("InvalidArguments");
    case ToolFailureCategory::NotFound:
        return QStringLiteral("NotFound");
    case ToolFailureCategory::PermissionDenied:
        return QStringLiteral("PermissionDenied");
    case ToolFailureCategory::SecurityDenied:
        return QStringLiteral("SecurityDenied");
    case ToolFailureCategory::Unsupported:
        return QStringLiteral("Unsupported");
    case ToolFailureCategory::Cancelled:
        return QStringLiteral("Cancelled");
    case ToolFailureCategory::Timeout:
        return QStringLiteral("Timeout");
    case ToolFailureCategory::RuntimeUnavailable:
        return QStringLiteral("RuntimeUnavailable");
    case ToolFailureCategory::NetworkFailure:
        return QStringLiteral("NetworkFailure");
    case ToolFailureCategory::PartialResult:
        return QStringLiteral("PartialResult");
    case ToolFailureCategory::InternalFailure:
        return QStringLiteral("InternalFailure");
    case ToolFailureCategory::ProtocolError:
        return QStringLiteral("ProtocolError");
    case ToolFailureCategory::InvalidToolSchema:
        return QStringLiteral("InvalidToolSchema");
    case ToolFailureCategory::RemoteExecutionFailure:
        return QStringLiteral("RemoteExecutionFailure");
    case ToolFailureCategory::StartupFailure:
        return QStringLiteral("StartupFailure");
    case ToolFailureCategory::AuthenticationFailure:
        return QStringLiteral("AuthenticationFailure");
    case ToolFailureCategory::ConfigurationFailure:
        return QStringLiteral("ConfigurationFailure");
    case ToolFailureCategory::UnsupportedProtocolVersion:
        return QStringLiteral("UnsupportedProtocolVersion");
    case ToolFailureCategory::InteractionRequired:
        return QStringLiteral("InteractionRequired");
    case ToolFailureCategory::InteractionRejected:
        return QStringLiteral("InteractionRejected");
    case ToolFailureCategory::UnsupportedLegacyTransport:
        return QStringLiteral("UnsupportedLegacyTransport");
    }
    return QStringLiteral("None");
}

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
    QObject* callbackContext = nullptr;
    // Daemon read-only inspection may run bounded file handlers inline after gateway checks.
    bool synchronousReadOnly = false;
};

struct ToolExecutionResult {
    ToolExecutionStatus status = ToolExecutionStatus::NotRequested;
    QString summary;
    StructuredObservationPtr structuredObservation;
    QList<FileMutation> mutations;
    SandboxExecutionResult sandbox;
    ToolFailureCategory failureCategory = ToolFailureCategory::None;
};

} // namespace sentinel::core
