// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

namespace sentinel::core {

enum class ToolRiskLevel {
    Low,
    Medium,
    High,
};

enum class ToolExecutionMode {
    MetadataOnly,
    Local,
};

enum class ToolSource { BuiltIn, MCP, Plugin, Internal };
enum class ToolScope { Local, Cloud, LocalAndCloud };

struct ToolParameterDescriptor {
    QString id;
    QString description;
    bool required = false;
};

struct ToolDescriptor {
    QString id;
    QString name;
    QString description;
    ToolRiskLevel riskLevel = ToolRiskLevel::Low;
    ToolExecutionMode executionMode = ToolExecutionMode::MetadataOnly;
    QList<ToolParameterDescriptor> parameters;
    QString category;
    ToolSource source = ToolSource::BuiltIn;
    bool enabled = true;
    bool exposedToModel = true;
    // Runtime-enforced JSON Schema subset; also rendered for planner tool discovery.
    QJsonObject inputSchema;
    QString providerId;
    QString version;
    QString requiredPermissionDomain;
    ToolScope scope = ToolScope::Local;
};

} // namespace sentinel::core
