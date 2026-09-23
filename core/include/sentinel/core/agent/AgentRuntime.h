// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/IAgentRuntime.h"

#include <QHash>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace sentinel::core {

class IAgentStepPlanner;
class IToolExecutor;
class IApprovalPolicy;
class ISandboxPolicy;
class IMemoryStore;
class IChatHistoryStore;

// Owns the lifecycle of foreground agent sessions. Dependencies are borrowed
// from the composition root and must outlive this runtime.
class AgentRuntime final : public IAgentRuntime {
public:
    AgentRuntime(std::unique_ptr<IAgentRuntime> metadata, IAgentStepPlanner* planner,
                 IToolExecutor& executor, const IApprovalPolicy& approval,
                 const ISandboxPolicy& sandbox, const IMemoryStore* memoryStore = nullptr,
                 const IChatHistoryStore* chatHistoryStore = nullptr);
    AgentRuntime(std::unique_ptr<IAgentRuntime> metadata, IAgentStepPlanner& planner,
                 IToolExecutor& executor, const IApprovalPolicy& approval,
                 const ISandboxPolicy& sandbox)
        : AgentRuntime(std::move(metadata), &planner, executor, approval, sandbox) {}
    ~AgentRuntime() override;

    QString name() const override;
    AgentStatus status() const override;
    QList<AgentCapabilityDescriptor> capabilities() const override;
    QList<ToolDescriptor> availableTools() const override;
    ToolInvocationPlan plan(const AgentRequest& request) const override;
    AgentResponse execute(const AgentRequest& request) override;

    QString createSession() override;
    AgentLoopState submit(const QString& sessionId, const QString& goal) override;
    AgentLoopState resume(const QString& sessionId, bool approved) override;
    bool approve(const QString& sessionId, bool alwaysAllow) override;
    bool cancel(const QString& sessionId) override;
    AgentLoopState sessionState(const QString& sessionId) const override;
    AgentRuntimeError error(const QString& sessionId) const override;

    void configureSession(const QString& sessionId, AgentSessionOptions options) override;
    bool start(const QString& sessionId, const QString& goal) override;
    bool continueSession(const QString& sessionId, bool approved) override;
    void shutdown() override;
    AgentPipelineResult executePipeline(const AgentRequest& request, bool autonomous) override;
    AgentPipelineResult executeApprovedGoal(const QString& goal) override;
    AgentPipelineResult executeApprovedPlan(const ToolInvocationPlan& plan,
                                            const QString& approvalSummary) override;
    bool supportsSessions() const override;
    QString subscribe(AgentEventCallback callback) override;
    void unsubscribe(const QString& id) override;
    QList<AgentEvent> eventHistory(const QString& sessionId) const override;

private:
    AgentLoopState advance(const QString& sessionId, bool isResume, bool approved,
                           const QString& goal, bool prepared = false);
    bool launch(const QString& sessionId, bool isResume, bool approved, const QString& goal);
    void prepareExecution(const QStringList& toolIds);
    QStringList toolIds() const;
    AgentPipelineResult executeApprovedPlanLocked(const ToolInvocationPlan& plan,
                                                  const QString& approvalSummary);
    void publish(const QString& sessionId, AgentEventType type, AgentEventPayload payload = {},
                 int stepIndex = 0, bool toolCall = false);
    void beginTurn(const QString& sessionId);
    void finishTurn(const QString& sessionId, const AgentLoopState& state);
    struct TurnContext {
        QString id;
        QHash<int, QString> stepIds;
        QHash<int, QString> toolCallIds;
        bool terminal = false;
    };

    std::unique_ptr<IAgentRuntime> metadata_;
    IAgentStepPlanner* planner_;
    IToolExecutor& executor_;
    const IApprovalPolicy& approval_;
    const ISandboxPolicy& sandbox_;
    const IMemoryStore* memoryStore_;
    const IChatHistoryStore* chatHistoryStore_;
    mutable std::mutex mutex_;
    std::mutex workerMutex_;
    std::mutex executionMutex_;
    QHash<QString, AgentLoopState> sessions_;
    QHash<QString, AgentSessionOptions> options_;
    QHash<QString, AgentRuntimeError> errors_;
    QStringList approvedToolIds_;
    QString activeSessionId_;
    std::atomic<bool> cancelRequested_{false};
    std::shared_ptr<std::atomic_bool> modelCancellationToken_ =
        std::make_shared<std::atomic_bool>(false);
    std::condition_variable cancellationPublished_;
    bool cancellationEventPending_ = false;
    std::thread worker_;
    bool shuttingDown_ = false;
    mutable std::mutex eventMutex_;
    struct Subscription {
        AgentEventCallback callback;
        std::mutex mutex;
        std::condition_variable idle;
        bool active = true;
        bool inFlight = false;
        std::thread::id callbackThread;
    };
    QHash<QString, std::shared_ptr<Subscription>> subscribers_;
    QHash<QString, QList<AgentEvent>> history_;
    QHash<QString, TurnContext> turns_;
    QList<AgentEvent> pendingEvents_;
    bool dispatching_ = false;
};

} // namespace sentinel::core
