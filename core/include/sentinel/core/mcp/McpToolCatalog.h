// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/mcp/IMcpService.h"
#include "sentinel/core/runtime/ToolDescriptor.h"
#include <QMap>
#include <QList>

namespace sentinel::core {

class IToolRegistry;

class McpToolCatalog {
public:
    McpToolCatalog() = default;

    // Convert MCP tools to Sentinel ToolDescriptors
    static ToolDescriptor mcpToolToDescriptor(const McpToolDefinition& mcpTool);
    static QList<ToolDescriptor> mcpToolsToDescriptors(const QList<McpToolDefinition>& mcpTools);

    // Register MCP tools with a tool registry
    static void registerMcpTools(const QList<McpToolDefinition>& mcpTools, IToolRegistry* registry);

    // Parse MCP tool call results
    static QJsonObject parseToolResult(const QJsonObject& result);
};

} // namespace sentinel::core
