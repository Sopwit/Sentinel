// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/IAgentStepPlanner.h"
#include "sentinel/core/doomloop/DoomLoopDetector.h"
#include "sentinel/core/runtime/ToolExecutionGateway.h"
#include "sentinel/core/runtime/ToolInvocationPlan.h"
#include "sentinel/core/runtime/ToolOutputTruncator.h"

#include <atomic>
#include <functional>

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {

class IApprovalPolicy;
class ISandboxPolicy;
class IToolExecutor;

enum class AgentLoopPhase {
    Idle,
    Running,
    AwaitingApproval,
    Completed,
    Cancelled,
    Failed,
    Stuck,
};

inline QString agentLoopPhaseName(AgentLoopPhase phase) {
    switch (phase) {
    case AgentLoopPhase::Idle:
        return QStringLiteral("Idle");
    case AgentLoopPhase::Running:
        return QStringLiteral("Running");
    case AgentLoopPhase::AwaitingApproval:
        return QStringLiteral("Awaiting Approval");
    case AgentLoopPhase::Completed:
        return QStringLiteral("Completed");
    case AgentLoopPhase::Cancelled:
        return QStringLiteral("Cancelled");
    case AgentLoopPhase::Failed:
        return QStringLiteral("Failed");
    case AgentLoopPhase::Stuck:
        return QStringLiteral("Stuck");
    }

    return QStringLiteral("Idle");
}

struct AgentLoopState {
    QString sessionId;
    QString goal;
    AgentLoopPhase phase = AgentLoopPhase::Idle;
    QList<AgentStepRecord> steps;
    QString finalAnswer;
    QString abortReason;
    ToolInvocationPlan pendingApprovalPlan;
    QString pendingApprovalThought;
};

class AgentLoop {
public:
    struct Config {
        int maxIterations = 12;
        bool autonomousMode = false;
        int observationPreviewLines = 60;
        qint64 observationMaxBytes = 8192;
        QStringList sessionApprovedToolIds;
    };

    using StepCallback = std::function<void(const AgentStepRecord&)>;
    using StatusCallback = std::function<void(const QString&)>;
    using CancelQuery = std::function<bool()>;

    AgentLoop(IAgentStepPlanner& planner, const IToolExecutor& executor,
              const IApprovalPolicy& approvalPolicy, const ISandboxPolicy& sandboxPolicy,
              QStringList knownToolIds)
        : AgentLoop(planner, executor, approvalPolicy, sandboxPolicy, std::move(knownToolIds),
                    Config{}) {}

    AgentLoop(IAgentStepPlanner& planner, const IToolExecutor& executor,
              const IApprovalPolicy& approvalPolicy, const ISandboxPolicy& sandboxPolicy,
              QStringList knownToolIds, Config config);

    void setStepCallback(StepCallback callback);
    void setStatusCallback(StatusCallback callback);
    void setCancelQuery(CancelQuery query);

    AgentLoopState run(const QString& goal, const QString& sessionId = QString());
    AgentLoopState resume(AgentLoopState state, bool approved);

private:
    AgentLoopState advance(AgentLoopState state);
    void executeStep(AgentLoopState& state, const ToolInvocationPlan& plan, const QString& thought,
                     ApprovalDecision approval);
    void appendBlockedStep(AgentLoopState& state, const ToolInvocationPlan& plan,
                           const QString& thought, const QString& statusText,
                           const QString& observation);
    ToolInvocationPlan planFromDecision(const AgentStepDecision& decision) const;
    bool cancellationRequested() const;

    IAgentStepPlanner& planner_;
    const IToolExecutor& executor_;
    const IApprovalPolicy& approvalPolicy_;
    const ISandboxPolicy& sandboxPolicy_;
    ToolExecutionGateway gateway_;
    DoomLoopDetector doomDetector_{DoomLoopDetector::Config{}};
    ToolOutputTruncator truncator_;
    QStringList knownToolIds_;
    Config config_;
    std::atomic<bool> cancelled_{false};
    StepCallback stepCallback_;
    StatusCallback statusCallback_;
    CancelQuery cancelQuery_;
};

} // namespace sentinel::core
