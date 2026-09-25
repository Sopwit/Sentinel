// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/runtime/BuiltInToolProvider.h"

#include <utility>

namespace sentinel::core {

NullAgentRuntime::NullAgentRuntime()
    : NullAgentRuntime(QList<ToolDescriptor>{ToolDescriptor{
          QStringLiteral("local-plan-summary"),
          QStringLiteral("Local Plan Summary"),
          QStringLiteral("Local planning summary tool."),
          ToolRiskLevel::Low,
          ToolExecutionMode::MetadataOnly,
          {
              ToolParameterDescriptor{QStringLiteral("topic"),
                                      QStringLiteral("Short user topic for local summary."), true},
          },
      }}) {}

NullAgentRuntime::NullAgentRuntime(QList<ToolDescriptor> tools) {
    for (auto& tool : tools) {
        toolRegistry_.registerTool(std::move(tool));
    }
}

QString NullAgentRuntime::name() const {
    return QStringLiteral("NullAgentRuntime");
}

AgentStatus NullAgentRuntime::status() const {
    return AgentStatus::Ready;
}

QList<AgentCapabilityDescriptor> NullAgentRuntime::capabilities() const {
    return {
        {
            QStringLiteral("local-plan-execution"),
            QStringLiteral("Executes approved local planning tools through the tool gateway."),
            true,
        },
    };
}

QList<ToolDescriptor> NullAgentRuntime::availableTools() const {
    return toolRegistry_.listTools();
}

QList<ToolDescriptor> NullAgentRuntime::standardTools() {
    return BuiltInToolProvider::descriptors();
}

ToolInvocationPlan NullAgentRuntime::plan(const AgentRequest& request) const {
    const auto trimmed = request.prompt.trimmed();
    if (trimmed.isEmpty()) {
        return {
            ToolInvocationPlanStatus::EmptyRequest,
            QStringLiteral("Agent request was empty."),
            {},
        };
    }

    const auto tools = toolRegistry_.listTools();
    if (tools.isEmpty()) {
        return {
            ToolInvocationPlanStatus::NoToolsAvailable,
            QStringLiteral("No tool metadata is available for planning."),
            {},
        };
    }

    QString selectedToolId;
    QList<ToolInvocationArgument> arguments;

    const auto requestedToolId = request.requestedToolId.trimmed();
    if (!requestedToolId.isEmpty()) {
        const auto tool = toolRegistry_.findToolById(requestedToolId);
        if (!tool.has_value()) {
            return {
                ToolInvocationPlanStatus::UnknownTool,
                QStringLiteral("Requested tool metadata was not found: %1").arg(requestedToolId),
                {},
            };
        }
        selectedToolId = requestedToolId;
        for (const auto& param : tool->parameters) {
            arguments.append(
                ToolInvocationArgument{param.id, param.required ? trimmed : QString()});
        }
    } else {
        return {ToolInvocationPlanStatus::NotRequested,
                QStringLiteral("An explicit tool ID is required for metadata planning."), {}};
    }

    const auto toolOpt = toolRegistry_.findToolById(selectedToolId);
    if (!toolOpt.has_value()) {
        return {ToolInvocationPlanStatus::UnknownTool,
                QStringLiteral("Planned tool was not found in registry: %1").arg(selectedToolId),
                {}};
    }

    const auto& tool = *toolOpt;
    QList<PlannedToolInvocation> invocations;
    invocations.append(PlannedToolInvocation{
        tool.id,
        tool.name,
        QStringLiteral("Plan metadata for %1").arg(tool.name),
        QStringLiteral("Dynamic tool plan for: %1").arg(trimmed),
        tool.riskLevel,
        tool.executionMode,
        arguments,
    });

    return {
        ToolInvocationPlanStatus::Planned,
        QStringLiteral("Tool plan prepared: %1").arg(tool.name),
        invocations,
    };
}

AgentResponse NullAgentRuntime::execute(const AgentRequest& request) {
    Q_UNUSED(request)
    return {false, QStringLiteral("Agent execution requires AgentRuntime."), status()};
}

} // namespace sentinel::core
