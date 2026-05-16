#include "sentinel/core/StaticAgentRegistry.h"

#include <utility>

namespace sentinel::core {

namespace {

AgentDescriptor agent(QString id, QString name, AgentRole role, AgentPriority priority,
                      QString summary, QStringList tags, QList<AgentTaskAffinity> affinities,
                      ProviderKind providerAffinity = ProviderKind::Local,
                      CatalogPrivacyLevel privacyAffinity = CatalogPrivacyLevel::LocalOnly) {
    AgentCapabilityProfile profile{
        std::move(summary), std::move(tags), {}, {}, providerAffinity, privacyAffinity,
    };
    for (const auto& affinity : affinities) {
        profile.preferredTaskTypes.append(taskTypeName(affinity.taskType));
    }
    profile.memoryAffinities.append(
        MemoryAffinity{role == AgentRole::Guardian ? MemoryType::Reflective : MemoryType::Semantic,
                       TaskType::Unknown, 10});

    return AgentDescriptor{
        std::move(id),      std::move(name),       role, AgentState::Available, priority,
        std::move(profile), std::move(affinities),
    };
}

QList<AgentDescriptor> defaultAgents() {
    return {
        agent(QStringLiteral("atlas"), QStringLiteral("Atlas"), AgentRole::Coordinator,
              AgentPriority::High,
              QStringLiteral("Coordinates local-first task metadata and planner handoffs."),
              {QStringLiteral("coordination"), QStringLiteral("routing-metadata")},
              {{TaskType::Unknown, 90}, {TaskType::Planning, 80}, {TaskType::Chat, 50}}),
        agent(
            QStringLiteral("orin"), QStringLiteral("Orin"), AgentRole::Planner, AgentPriority::High,
            QStringLiteral("Structures multi-step plans from capability metadata."),
            {QStringLiteral("planning"), QStringLiteral("capability-graph")},
            {{TaskType::Planning, 95}, {TaskType::ToolPlanning, 75}, {TaskType::LongContext, 55}}),
        agent(QStringLiteral("vela"), QStringLiteral("Vela"), AgentRole::Researcher,
              AgentPriority::Normal,
              QStringLiteral("Summarizes long-context metadata and research-oriented tasks."),
              {QStringLiteral("summarization"), QStringLiteral("context")},
              {{TaskType::Summarization, 90}, {TaskType::LongContext, 85}, {TaskType::Chat, 45}},
              ProviderKind::Cloud, CatalogPrivacyLevel::CloudMetadataOnly),
        agent(QStringLiteral("kaze"), QStringLiteral("Kaze"), AgentRole::Builder,
              AgentPriority::Normal,
              QStringLiteral("Maps coding and tool-planning requests to static plan metadata."),
              {QStringLiteral("coding"), QStringLiteral("tool-planning")},
              {{TaskType::Coding, 95}, {TaskType::ToolPlanning, 90}, {TaskType::Planning, 45}}),
        agent(QStringLiteral("nyx"), QStringLiteral("Nyx"), AgentRole::Guardian,
              AgentPriority::Critical,
              QStringLiteral("Keeps sensitive task metadata on local-only boundaries."),
              {QStringLiteral("privacy"), QStringLiteral("safety")},
              {{TaskType::SensitiveData, 100}, {TaskType::Planning, 60}, {TaskType::Unknown, 50}}),
        agent(QStringLiteral("sol"), QStringLiteral("Sol"), AgentRole::Companion,
              AgentPriority::Normal,
              QStringLiteral("Handles conversational and synthesis-oriented metadata flows."),
              {QStringLiteral("chat"), QStringLiteral("synthesis")},
              {{TaskType::Chat, 90}, {TaskType::Summarization, 70}, {TaskType::Unknown, 40}}),
    };
}

} // namespace

StaticAgentRegistry::StaticAgentRegistry() : StaticAgentRegistry(defaultAgents()) {}

StaticAgentRegistry::StaticAgentRegistry(QList<AgentDescriptor> agents)
    : agents_(std::move(agents)) {}

QList<AgentDescriptor> StaticAgentRegistry::agents() const {
    return agents_;
}

AgentDescriptor StaticAgentRegistry::agentById(const QString& id) const {
    const auto normalized = id.trimmed();
    for (const auto& agent : agents_) {
        if (agent.id == normalized) {
            return agent;
        }
    }
    return {};
}

} // namespace sentinel::core
