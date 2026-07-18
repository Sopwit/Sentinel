#pragma once

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
};

} // namespace sentinel::core
