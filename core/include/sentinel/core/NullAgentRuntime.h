#pragma once

#include "sentinel/core/IAgentRuntime.h"

namespace sentinel::core {

class NullAgentRuntime final : public IAgentRuntime {
public:
    QString name() const override;
    AgentStatus status() const override;
    QList<AgentCapabilityDescriptor> capabilities() const override;
    AgentResponse execute(const AgentRequest& request) override;
};

} // namespace sentinel::core
