// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/ToolDescriptor.h"
#include "sentinel/core/runtime/ToolInvocationPlan.h"

#include <QList>
#include <QString>

namespace sentinel::core {

struct AgentStepRecord {
    int index = 0;
    QString thought;
    QString toolId;
    QString toolName;
    QList<ToolInvocationArgument> arguments;
    QString observation;
    QString statusText;
    bool succeeded = false;
};

struct AgentStepDecision {
    enum class Kind {
        ToolCall,
        FinalAnswer,
        GiveUp,
    };

    Kind kind = Kind::GiveUp;
    QString thought;
    QString toolId;
    QString toolName;
    ToolRiskLevel riskLevel = ToolRiskLevel::Low;
    ToolExecutionMode executionMode = ToolExecutionMode::Local;
    QList<ToolInvocationArgument> arguments;
    QString answer;
    QString reason;
};

class IAgentStepPlanner {
public:
    virtual ~IAgentStepPlanner() = default;

    virtual AgentStepDecision nextStep(const QString& goal,
                                       const QList<AgentStepRecord>& history) const = 0;
};

inline QString agentStepRecordSummary(const AgentStepRecord& record) {
    QStringList argumentParts;
    for (const auto& argument : record.arguments) {
        argumentParts.append(QStringLiteral("%1=%2").arg(argument.id, argument.value));
    }
    return QStringLiteral("Step %1: %2 (%3) -> %4")
        .arg(QString::number(record.index), record.toolName,
             argumentParts.join(QStringLiteral(", ")), record.statusText);
}

} // namespace sentinel::core
