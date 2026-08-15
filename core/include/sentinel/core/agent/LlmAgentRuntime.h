// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/IAgentRuntime.h"
#include "sentinel/core/agent/IAgentStepPlanner.h"
#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/interfaces/IChatProvider.h"

#include <QList>
#include <QString>

namespace sentinel::core {

// IAgentRuntime + IAgentStepPlanner implementation that asks the configured
// chat provider (local or cloud) for the next agent action in JSON form and
// falls back to the deterministic NullAgentRuntime heuristics whenever the
// provider is unavailable or returns unparseable output.
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

private:
    QString buildPlannerPrompt(const QString& goal, const QList<AgentStepRecord>& history) const;
    AgentStepDecision decisionFromLlmOutput(const QString& output) const;
    AgentStepDecision heuristicDecision(const QString& goal,
                                        const QList<AgentStepRecord>& history) const;

    NullAgentRuntime heuristic_;
    QList<ToolDescriptor> tools_;
    IChatProvider* provider_ = nullptr;
    mutable bool lastDecisionUsedLlm_ = false;
};

} // namespace sentinel::core
