// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/IAgentRuntime.h"
#include "sentinel/core/agent/IAgentStepPlanner.h"
#include "sentinel/core/agent/ContextEngine.h"
#include "sentinel/core/interfaces/IChatProvider.h"
#include "sentinel/core/model/ModelRouting.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {
class IToolRegistry;

// IAgentRuntime + IAgentStepPlanner implementation that asks the configured
// chat provider (local or cloud) for the next agent action in JSON form and
// retries one malformed response, then reports planning failure.
class LlmAgentRuntime final : public IAgentRuntime, public IAgentStepPlanner {
public:
    LlmAgentRuntime(QList<ToolDescriptor> tools, IChatProvider* provider);

    QString name() const override;
    AgentStatus status() const override;
    QList<AgentCapabilityDescriptor> capabilities() const override;
    QList<ToolDescriptor> availableTools() const override;
    ToolInvocationPlan plan(const AgentRequest& request) const override;
    AgentResponse execute(const AgentRequest& request) override;

    AgentStepDecision nextStep(const QString& goal,
                               const QList<AgentStepRecord>& history) const override;

    bool lastDecisionUsedLlm() const;
    bool hasModelProvider() const {
        return provider_ != nullptr;
    }
    IChatProvider* modelProvider() const { return provider_; }
    void setObservationIntent(const ObservationIntent& intent) override { activeIntent_ = intent; }
    void setPlannerFeedback(const QString& feedback) override { plannerFeedback_ = feedback; }
    void setStructuredFacts(const QList<StructuredFact>& facts) override { structuredFacts_ = facts; }
    void setPlanningContext(const AgentPlanningContext& context) override { planningContext_ = context; }
    // Called before an accepted run; the owned provider and binding stay fixed
    // for every planner iteration in that run.
    void bindModel(ModelBinding binding, std::shared_ptr<IChatProvider> provider);
    ModelBinding modelBinding() const { return modelBinding_; }
    void setStreamObserver(std::function<void(const QString&)> onDelta,
                           std::shared_ptr<std::atomic_bool> cancellationToken = {}) const;
    void setToolRegistry(const IToolRegistry* registry) {
        registry_ = registry;
    }
    void setAllowedToolIds(const QStringList& ids) { allowedToolIds_ = ids; }

private:
    QString buildPlannerPrompt(const QString& goal, const QList<AgentStepRecord>& history) const;
    AgentStepDecision decisionFromLlmOutput(const QString& output) const;

    QList<ToolDescriptor> tools_;
    const IToolRegistry* registry_ = nullptr;
    QStringList allowedToolIds_;
    IChatProvider* provider_ = nullptr;
    std::shared_ptr<IChatProvider> boundProvider_;
    ModelBinding modelBinding_;
    mutable ObservationIntent activeIntent_;
    mutable QList<StructuredFact> structuredFacts_;
    mutable AgentPlanningContext planningContext_;
    mutable QString plannerFeedback_;
    mutable bool lastDecisionUsedLlm_ = false;
    mutable std::function<void(const QString&)> streamObserver_;
    mutable std::shared_ptr<std::atomic_bool> streamCancellationToken_;
};

} // namespace sentinel::core
