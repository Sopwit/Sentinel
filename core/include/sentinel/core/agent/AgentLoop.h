// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/AgentLoopState.h"
#include "sentinel/core/agent/IAgentStepPlanner.h"
#include "sentinel/core/doomloop/DoomLoopDetector.h"
#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/runtime/ToolExecutionGateway.h"
#include "sentinel/core/runtime/ToolInvocationPlan.h"
#include "sentinel/core/runtime/ToolOutputTruncator.h"

#include <atomic>
#include <functional>

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

namespace sentinel::core {

class IApprovalPolicy;
class ISandboxPolicy;
class IToolExecutor;

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
    enum class ToolTransition { Requested, ApprovalRequired, ExecutionStarted, ExecutionFinished };
    using ToolCallback =
        std::function<void(ToolTransition, int, const ToolInvocationPlan&, const AgentStepRecord*)>;
    using PlanningCallback = std::function<void(int, bool)>;
    using OutputCallback =
        std::function<void(int, const QString&, ProcessStream, const QByteArray&)>;
    using CompletionCallback = std::function<void(const AgentLoopState&)>;
    using ToolCallIdProvider = std::function<QString(int)>;

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
    void setToolCallback(ToolCallback callback);
    void setPlanningCallback(PlanningCallback callback);
    void setOutputCallback(OutputCallback callback);
    void setToolCallIdProvider(ToolCallIdProvider provider);

    AgentLoopState run(const QString& goal, const QString& sessionId = QString());
    AgentLoopState resume(AgentLoopState state, bool approved);
    void runAsync(const QString& goal, const QString& sessionId, QObject* context,
                  CompletionCallback completion);
    void resumeAsync(AgentLoopState state, bool approved, QObject* context,
                     CompletionCallback completion);
    void cancelAsync();

private:
    AgentLoopState advance(AgentLoopState state);
    void executeStep(AgentLoopState& state, const ToolInvocationPlan& plan, const QString& thought,
                     ApprovalDecision approval);
    void appendBlockedStep(AgentLoopState& state, const ToolInvocationPlan& plan,
                           const QString& thought, const QString& statusText,
                           const QString& observation);
    ToolInvocationPlan planFromDecision(const AgentStepDecision& decision) const;
    bool cancellationRequested() const;
    void advanceAsync();
    void executeStepAsync(const ToolInvocationPlan& plan, const QString& thought,
                          ApprovalDecision approval);
    void completeAsync();
    void scheduleAsyncAdvance();

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
    ToolCallback toolCallback_;
    PlanningCallback planningCallback_;
    OutputCallback outputCallback_;
    ToolCallIdProvider toolCallIdProvider_;
    CompletionCallback completionCallback_;
    AgentLoopState asyncState_;
    QObject* asyncContext_ = nullptr;
    IToolExecutor::Cancel cancelTool_;
    bool asyncFinished_ = false;
    bool waitingForTool_ = false;
};

} // namespace sentinel::core
