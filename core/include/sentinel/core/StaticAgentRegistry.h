#pragma once

#include "sentinel/core/IAgentRegistry.h"

namespace sentinel::core {

class StaticAgentRegistry final : public IAgentRegistry {
public:
    StaticAgentRegistry();
    explicit StaticAgentRegistry(QList<AgentDescriptor> agents);

    QList<AgentDescriptor> agents() const override;
    AgentDescriptor agentById(const QString& id) const override;

private:
    QList<AgentDescriptor> agents_;
};

} // namespace sentinel::core
