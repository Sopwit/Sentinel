// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace sentinel::core {

struct ToolSchema {
    QString name;
    QString description;
    QJsonObject inputSchema;
    QString provider;
};

class ToolJsonSchemaNormalizer {
public:
    QJsonObject normalize(const QJsonObject& schema, const QString& provider) const;
    QJsonArray normalizeToolDefinitions(const QJsonArray& tools, const QString& provider) const;
    QJsonObject ensureProperties(const QJsonObject& schema) const;
    QJsonObject addDefaultTypes(const QJsonObject& schema) const;
    QJsonObject normalizeParameterNames(const QJsonObject& schema) const;
    ToolSchema fromMcpTool(const QJsonObject& mcpTool, const QString& serverName) const;
};

} // namespace sentinel::core
