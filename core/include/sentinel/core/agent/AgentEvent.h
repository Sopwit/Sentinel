// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/AgentLoopState.h"
#include "sentinel/core/runtime/ProcessExecutor.h"

#include <QDateTime>
#include <QString>
#include <variant>

namespace sentinel::core {

enum class AgentEventType {
    SessionCreated,
    RunStarted,
    ModelRequestStarted,
    ModelOutputDelta,
    ModelRequestCompleted,
    ToolRequested,
    ToolApprovalRequired,
    ToolApprovalResolved,
    ToolExecutionStarted,
    ToolOutput,
    ToolExecutionCompleted,
    ToolExecutionFailed,
    AgentStepCompleted,
    AgentCompleted,
    AgentFailed,
    AgentCancelled,
    RuntimeStateChanged,
};

struct AgentStateEvent {
    AgentLoopPhase phase = AgentLoopPhase::Idle;
};

struct AgentTextEvent {
    QString text;
};

struct AgentToolEvent {
    QString toolId;
    QList<ToolInvocationArgument> arguments;
    ToolRiskLevel risk = ToolRiskLevel::Low;
    QString output;
};

struct AgentStepEvent {
    AgentStepRecord step;
};

struct AgentToolOutputEvent {
    QString processId;
    ProcessStream stream = ProcessStream::Stdout;
    QByteArray chunk;
};

struct AgentRunEvent {
    AgentLoopPhase phase = AgentLoopPhase::Idle;
    QString finalAnswer;
    QString abortReason;
    int completedSteps = 0;
    AgentToolEvent pendingTool;
    QString pendingThought;
};

using AgentEventPayload =
    std::variant<std::monostate, AgentStateEvent, AgentTextEvent, AgentToolEvent,
                 AgentToolOutputEvent, AgentStepEvent, AgentRunEvent>;

struct AgentEvent {
    QString id;
    QString sessionId;
    QString turnId;
    QString stepId;
    QString toolCallId;
    QString providerId;
    QString modelId;
    AgentEventType type = AgentEventType::SessionCreated;
    QDateTime timestamp;
    AgentEventPayload payload;
};

} // namespace sentinel::core
