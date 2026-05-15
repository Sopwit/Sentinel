#pragma once

#include "sentinel/core/ToolDescriptor.h"

#include <QList>
#include <QString>

namespace sentinel::core {

enum class AgentStatus {
    Unavailable,
    Ready,
    Busy,
    Error,
};

inline QString agentStatusName(AgentStatus status) {
    switch (status) {
    case AgentStatus::Unavailable:
        return QStringLiteral("Unavailable");
    case AgentStatus::Ready:
        return QStringLiteral("Ready");
    case AgentStatus::Busy:
        return QStringLiteral("Busy");
    case AgentStatus::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Unavailable");
}

struct AgentCapabilityDescriptor {
    QString id;
    QString description;
    bool enabled = false;
};

struct AgentRequest {
    QString prompt;
};

struct AgentResponse {
    bool success = false;
    QString message;
    AgentStatus status = AgentStatus::Unavailable;
};

class IAgentRuntime {
public:
    virtual ~IAgentRuntime() = default;

    virtual QString name() const = 0;
    virtual AgentStatus status() const = 0;
    virtual QList<AgentCapabilityDescriptor> capabilities() const = 0;
    virtual QList<ToolDescriptor> availableTools() const = 0;
    virtual AgentResponse execute(const AgentRequest& request) = 0;
};

} // namespace sentinel::core
