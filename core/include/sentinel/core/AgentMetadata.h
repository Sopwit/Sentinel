#pragma once

#include "sentinel/core/ProviderCatalog.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class AgentRole : std::uint8_t {
    Coordinator,
    Planner,
    Researcher,
    Builder,
    Guardian,
    Companion,
};

enum class AgentState : std::uint8_t {
    Available,
    Standby,
    Unavailable,
};

enum class AgentPriority : std::uint8_t {
    Low,
    Normal,
    High,
    Critical,
};

inline QString agentRoleName(AgentRole role) {
    switch (role) {
    case AgentRole::Coordinator:
        return QStringLiteral("Coordinator");
    case AgentRole::Planner:
        return QStringLiteral("Planner");
    case AgentRole::Researcher:
        return QStringLiteral("Researcher");
    case AgentRole::Builder:
        return QStringLiteral("Builder");
    case AgentRole::Guardian:
        return QStringLiteral("Guardian");
    case AgentRole::Companion:
        return QStringLiteral("Companion");
    }

    return QStringLiteral("Coordinator");
}

inline QString agentStateName(AgentState state) {
    switch (state) {
    case AgentState::Available:
        return QStringLiteral("Available");
    case AgentState::Standby:
        return QStringLiteral("Standby");
    case AgentState::Unavailable:
        return QStringLiteral("Unavailable");
    }

    return QStringLiteral("Unavailable");
}

inline QString agentPriorityName(AgentPriority priority) {
    switch (priority) {
    case AgentPriority::Low:
        return QStringLiteral("Low");
    case AgentPriority::Normal:
        return QStringLiteral("Normal");
    case AgentPriority::High:
        return QStringLiteral("High");
    case AgentPriority::Critical:
        return QStringLiteral("Critical");
    }

    return QStringLiteral("Normal");
}

struct AgentTaskAffinity {
    TaskType taskType = TaskType::Unknown;
    int weight = 0;
};

struct AgentCapabilityProfile {
    QString summary;
    QStringList capabilityTags;
    QStringList preferredTaskTypes;
    ProviderKind providerAffinity = ProviderKind::Local;
    CatalogPrivacyLevel privacyAffinity = CatalogPrivacyLevel::LocalOnly;
};

struct AgentDescriptor {
    QString id;
    QString displayName;
    AgentRole role = AgentRole::Coordinator;
    AgentState state = AgentState::Standby;
    AgentPriority priority = AgentPriority::Normal;
    AgentCapabilityProfile capabilityProfile;
    QList<AgentTaskAffinity> taskAffinities;
};

struct AgentRuntimeSnapshot {
    QList<AgentDescriptor> agents;
    QString currentAgentId;
    QString summary;
    bool autonomousExecutionAllowed = false;
};

inline bool isAgentAvailable(AgentState state) {
    return state == AgentState::Available;
}

inline QString agentDescriptorSummary(const AgentDescriptor& agent) {
    return QStringLiteral("%1 (%2, %3, %4)")
        .arg(agent.displayName, agentRoleName(agent.role), agentStateName(agent.state),
             providerKindName(agent.capabilityProfile.providerAffinity));
}

} // namespace sentinel::core
