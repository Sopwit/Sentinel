#pragma once

#include "sentinel/core/IAgentRuntime.h"
#include "sentinel/core/InMemoryToolRegistry.h"

namespace sentinel::core {

class NullAgentRuntime final : public IAgentRuntime {
public:
    NullAgentRuntime();
    explicit NullAgentRuntime(QList<ToolDescriptor> tools);

    QString name() const override;
    AgentStatus status() const override;
    QList<AgentCapabilityDescriptor> capabilities() const override;
    QList<ToolDescriptor> availableTools() const override;
    ToolInvocationPlan plan(const AgentRequest& request) const override;
    AgentResponse execute(const AgentRequest& request) override;

private:
    InMemoryToolRegistry toolRegistry_;
};

} // namespace sentinel::core
