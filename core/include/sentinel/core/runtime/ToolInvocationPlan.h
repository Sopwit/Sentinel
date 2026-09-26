// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/ToolSandbox.h"
#include "sentinel/core/runtime/ProcessSandbox.h"
#include "sentinel/core/runtime/ToolDescriptor.h"
#include "sentinel/core/runtime/IFileSystemService.h"

#include <QJsonValue>
#include <atomic>
#include <memory>
#include <optional>
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
    // Undefined means a legacy string argument. Model and normalized calls retain JSON type.
    QJsonValue jsonValue = QJsonValue(QJsonValue::Undefined);
    bool operator==(const ToolInvocationArgument&) const = default;
};

struct AuthorizedFileSystemResource {
    QString argument;
    QString patchAction;
    AccessMode access = AccessMode::Read;
    AuthorizedPath path;
};

struct ResourceAuthorizationSnapshot {
    QList<ToolInvocationArgument> normalizedArguments;
    QList<AuthorizationRequest> requests;
    QList<AuthorizedFileSystemResource> files;
    QString workingDirectory;
    bool authorized = false;
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
    // Runtime-only cancellation state; never part of the model/tool argument contract.
    std::shared_ptr<std::atomic_bool> cancellation;
    std::shared_ptr<std::atomic_bool> toolCancellation;
    std::shared_ptr<const ToolDescriptor> descriptorSnapshot;
    std::shared_ptr<const ResourceAuthorizationSnapshot> resourceSnapshot;
    std::optional<SandboxExecutionPlan> processSandbox;
    QList<int> dependsOn;
    QString providerCallId;
    QString runtimeToolCallId;
};

struct ToolInvocationPlan {
    ToolInvocationPlanStatus status = ToolInvocationPlanStatus::NotRequested;
    QString summary;
    QList<PlannedToolInvocation> invocations;
    QString batchId;
};

} // namespace sentinel::core
