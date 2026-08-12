// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/IAgentRuntime.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/RealToolExecutor.h"

namespace sentinel::core {

class NullAgentRuntime final : public IAgentRuntime {
public:
    NullAgentRuntime();
    explicit NullAgentRuntime(QList<ToolDescriptor> tools);
    static QList<ToolDescriptor> standardTools();

    QString name() const override;
    AgentStatus status() const override;
    QList<AgentCapabilityDescriptor> capabilities() const override;
    QList<ToolDescriptor> availableTools() const override;
    ToolInvocationPlan plan(const AgentRequest& request) const override;
    AgentResponse execute(const AgentRequest& request) override;

private:
    InMemoryToolRegistry toolRegistry_;
    RealToolExecutor executor_;
};

} // namespace sentinel::core
