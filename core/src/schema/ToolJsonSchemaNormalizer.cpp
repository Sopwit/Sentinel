// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/schema/ToolJsonSchemaNormalizer.h"

namespace sentinel::core {

QJsonObject ToolJsonSchemaNormalizer::normalize(const QJsonObject& schema, const QString& provider) const {
    QJsonObject normalized = schema;

    if (!normalized.contains("type")) {
        normalized["type"] = "object";
    }

    if (!normalized.contains("properties")) {
        normalized["properties"] = QJsonObject();
    }

    QJsonObject props = normalized["properties"].toObject();
    for (auto it = props.begin(); it != props.end(); ++it) {
        QJsonObject prop = it.value().toObject();
        if (!prop.contains("type")) {
            prop["type"] = "string";
        }
        it.value() = prop;
    }
    normalized["properties"] = props;

    return normalized;
}

QJsonArray ToolJsonSchemaNormalizer::normalizeToolDefinitions(const QJsonArray& tools, const QString& provider) const {
    QJsonArray normalized;
    for (const auto& tool : tools) {
        QJsonObject t = tool.toObject();
        if (t.contains("inputSchema")) {
            t["inputSchema"] = normalize(t["inputSchema"].toObject(), provider);
        }
        normalized.append(t);
    }
    return normalized;
}

QJsonObject ToolJsonSchemaNormalizer::ensureProperties(const QJsonObject& schema) const {
    QJsonObject s = schema;
    if (!s.contains("properties")) {
        s["properties"] = QJsonObject();
    }
    return s;
}

QJsonObject ToolJsonSchemaNormalizer::addDefaultTypes(const QJsonObject& schema) const {
    return normalize(schema, {});
}

QJsonObject ToolJsonSchemaNormalizer::normalizeParameterNames(const QJsonObject& schema) const {
    QJsonObject s = schema;
    QJsonObject props = s["properties"].toObject();
    QJsonObject normalizedProps;

    for (auto it = props.begin(); it != props.end(); ++it) {
        QString key = it.key();
        key.replace(" ", "_");
        key.replace("-", "_");
        normalizedProps[key] = it.value();
    }

    s["properties"] = normalizedProps;
    return s;
}

ToolSchema ToolJsonSchemaNormalizer::fromMcpTool(const QJsonObject& mcpTool, const QString& serverName) const {
    ToolSchema schema;
    schema.name = mcpTool["name"].toString();
    schema.description = mcpTool["description"].toString();
    schema.inputSchema = mcpTool["inputSchema"].toObject();
    schema.provider = serverName;
    return schema;
}

} // namespace sentinel::core
