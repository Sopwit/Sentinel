// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/mcp/McpToolCatalog.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include <QDebug>
#include <QJsonDocument>

namespace sentinel::core {

namespace {
// UTF-8 byte escaping is injective: underscores and punctuation are encoded too,
// so distinct remote names cannot normalize to the same registry ID.
QString idComponent(const QString& value) {
    QString result;
    for (const auto byte : value.toUtf8()) {
        const auto c = static_cast<unsigned char>(byte);
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            result += QLatin1Char(c);
        else
            result += QStringLiteral("_%1_").arg(c, 2, 16, QLatin1Char('0'));
    }
    return result;
}
} // namespace

ToolDescriptor McpToolCatalog::mcpToolToDescriptor(const McpToolDefinition& mcpTool) {
    ToolDescriptor descriptor;
    descriptor.id =
        QStringLiteral("mcp.%1.%2").arg(idComponent(mcpTool.serverName), idComponent(mcpTool.name));
    descriptor.name = mcpTool.name;
    descriptor.description = mcpTool.description;
    descriptor.riskLevel = ToolRiskLevel::Medium; // MCP tools are medium risk by default
    descriptor.executionMode = ToolExecutionMode::Local;
    descriptor.source = ToolSource::MCP;
    descriptor.providerId = QStringLiteral("mcp:%1").arg(mcpTool.serverName);
    descriptor.requiredPermissionDomain = QStringLiteral("tool-execution");
    // The gateway enforces this remote contract locally before tools/call.
    // A missing schema permits only an empty argument object.
    descriptor.inputSchema = mcpTool.inputSchema;
    descriptor.evidenceProduced = {{ObservationDomain::ExternalService, EvidenceFreshness::Live,
                                    EvidenceScope::Provider, {}}};

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

QList<ToolDescriptor>
McpToolCatalog::mcpToolsToDescriptors(const QList<McpToolDefinition>& mcpTools) {
    QList<ToolDescriptor> descriptors;
    for (const auto& mcpTool : mcpTools) {
        descriptors.append(mcpToolToDescriptor(mcpTool));
    }
    return descriptors;
}

void McpToolCatalog::registerMcpTools(const QList<McpToolDefinition>& mcpTools,
                                      IToolRegistry* registry) {
    Q_UNUSED(mcpTools)
    Q_UNUSED(registry)
    qWarning() << "McpToolCatalog: metadata-only registration is unsupported; use executable "
                  "registrations";
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
