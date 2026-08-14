// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/mcp/McpToolCatalog.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include <QJsonDocument>
#include <QDebug>

namespace sentinel::core {

ToolDescriptor McpToolCatalog::mcpToolToDescriptor(const McpToolDefinition& mcpTool) {
    ToolDescriptor descriptor;
    descriptor.id = QStringLiteral("mcp_%1_%2").arg(mcpTool.serverName, mcpTool.name);
    descriptor.name = mcpTool.name;
    descriptor.description = mcpTool.description;
    descriptor.riskLevel = ToolRiskLevel::Medium; // MCP tools are medium risk by default
    descriptor.executionMode = ToolExecutionMode::Local;

    // Convert input schema to parameter descriptors
    QJsonObject schema = mcpTool.inputSchema;
    QJsonObject properties = schema["properties"].toObject();
    QJsonArray required = schema["required"].toArray();

    for (auto it = properties.begin(); it != properties.end(); ++it) {
        ToolParameterDescriptor param;
        param.id = it.key();
        param.description = it.value().toObject()["description"].toString();
        param.required = required.contains(it.key());
        descriptor.parameters.append(param);
    }

    return descriptor;
}

QList<ToolDescriptor> McpToolCatalog::mcpToolsToDescriptors(const QList<McpToolDefinition>& mcpTools) {
    QList<ToolDescriptor> descriptors;
    for (const auto& mcpTool : mcpTools) {
        descriptors.append(mcpToolToDescriptor(mcpTool));
    }
    return descriptors;
}

void McpToolCatalog::registerMcpTools(const QList<McpToolDefinition>& mcpTools, IToolRegistry* registry) {
    if (!registry) {
        return;
    }

    QList<ToolDescriptor> descriptors = mcpToolsToDescriptors(mcpTools);
    for (const auto& descriptor : descriptors) {
        if (!registry->registerTool(descriptor)) {
            qWarning() << QStringLiteral("McpToolCatalog: Failed to register MCP tool '%1'").arg(descriptor.id);
        }
    }

    qDebug() << QStringLiteral("McpToolCatalog: Registered %1 MCP tools").arg(descriptors.size());
}

QJsonObject McpToolCatalog::parseToolResult(const QJsonObject& result) {
    // MCP tool results follow the MCP protocol format
    // Convert to a simpler format for Sentinel
    QJsonObject parsed;

    if (result.contains("content")) {
        QJsonArray content = result["content"].toArray();
        for (const auto& item : content) {
            QJsonObject contentObj = item.toObject();
            if (contentObj["type"].toString() == "text") {
                parsed["text"] = contentObj["text"];
                break;
            }
        }
    }

    if (result.contains("isError")) {
        parsed["isError"] = result["isError"];
    }

    return parsed;
}

} // namespace sentinel::core
