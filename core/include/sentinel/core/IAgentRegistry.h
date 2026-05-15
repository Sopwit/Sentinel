#pragma once

#include "sentinel/core/AgentMetadata.h"

namespace sentinel::core {

class IAgentRegistry {
public:
    virtual ~IAgentRegistry() = default;

    virtual QList<AgentDescriptor> agents() const = 0;
    virtual AgentDescriptor agentById(const QString& id) const = 0;
};

} // namespace sentinel::core
