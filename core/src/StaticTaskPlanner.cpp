#include "sentinel/core/StaticTaskPlanner.h"

#include <algorithm>
#include <utility>

namespace sentinel::core {

namespace {

bool taskRequiresLocalOnly(const TaskPlanningRequest& request) {
    return request.routingMode == RoutingMode::LocalOnly || request.task.sensitive ||
           request.task.type == TaskType::SensitiveData;
}

bool routingAllowsCloud(RoutingMode mode) {
    return mode == RoutingMode::CloudAllowed;
}

bool supportsTask(const QStringList& supportedTaskTypes, TaskType taskType) {
    return taskType == TaskType::Unknown || supportedTaskTypes.contains(taskTypeName(taskType));
}

CapabilityGraph buildCapabilityGraph(QList<ProviderCatalogEntry> entries) {
    std::sort(entries.begin(), entries.end(),
              [](const ProviderCatalogEntry& left, const ProviderCatalogEntry& right) {
                  return left.descriptor.id < right.descriptor.id;
              });

    CapabilityGraph graph;
    for (const auto& entry : entries) {
        graph.nodes.append(CapabilityNode{
            entry.descriptor.id,
            entry.descriptor.name,
            entry.descriptor.kind,
            entry.availability,
            entry.privacyLevel,
            entry.descriptor.capabilityProfile.supportedTaskTypes,
            entry.ramHintMb,
            entry.diskHintMb,
        });
    }
    return graph;
}

struct Candidate {
    ProviderCatalogEntry provider;
    ModelCatalogEntry model;
};

QList<Candidate> candidatesFor(const TaskPlanningRequest& request) {
    QList<Candidate> candidates;
    for (const auto& provider : request.catalogEntries) {
        for (const auto& model : provider.models) {
            if (!supportsTask(model.descriptor.recommendedTaskTypes, request.task.type) &&
                !supportsTask(provider.descriptor.capabilityProfile.supportedTaskTypes,
                              request.task.type)) {
                continue;
            }
            candidates.append(Candidate{provider, model});
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& left, const Candidate& right) {
                  if (left.provider.descriptor.kind != right.provider.descriptor.kind) {
                      return left.provider.descriptor.kind == ProviderKind::Local;
                  }
                  if (left.provider.availability != right.provider.availability) {
                      return isCatalogEntryAvailable(left.provider.availability);
                  }
                  return left.provider.descriptor.id < right.provider.descriptor.id;
              });
    return candidates;
}

bool candidateAvailable(const Candidate& candidate) {
    return isCatalogEntryAvailable(candidate.provider.availability) &&
           isCatalogEntryAvailable(candidate.model.availability) &&
           candidate.model.descriptor.installed;
}

bool candidateIsLocal(const Candidate& candidate) {
    return candidate.provider.descriptor.kind == ProviderKind::Local &&
           candidate.model.descriptor.localOnly;
}

int priorityRank(AgentPriority priority) {
    switch (priority) {
    case AgentPriority::Critical:
        return 4;
    case AgentPriority::High:
        return 3;
    case AgentPriority::Normal:
        return 2;
    case AgentPriority::Low:
        return 1;
    }

    return 0;
}

int affinityWeightFor(const AgentDescriptor& agent, TaskType taskType) {
    int weight = 0;
    for (const auto& affinity : agent.taskAffinities) {
        if (affinity.taskType == taskType) {
            weight = std::max(weight, affinity.weight);
        }
    }
    return weight;
}

AgentDescriptor preferredAgentFor(const TaskPlanningRequest& request) {
    AgentDescriptor preferred;
    int preferredWeight = -1;
    int preferredPriority = -1;

    for (const auto& agent : request.agents) {
        if (!isAgentAvailable(agent.state)) {
            continue;
        }

        const auto weight = affinityWeightFor(agent, request.task.type);
        const auto priority = priorityRank(agent.priority);
        if (weight > preferredWeight ||
            (weight == preferredWeight && priority > preferredPriority) ||
            (weight == preferredWeight && priority == preferredPriority &&
             (preferred.id.isEmpty() || agent.id < preferred.id))) {
            preferred = agent;
            preferredWeight = weight;
            preferredPriority = priority;
        }
    }

    return preferred;
}

PlannedTaskStep stepFor(const Candidate& candidate, const AgentDescriptor& preferredAgent,
                        int order, QString summary) {
    return PlannedTaskStep{
        order,
        QStringLiteral("step-%1-%2").arg(order).arg(candidate.model.descriptor.id),
        std::move(summary),
        candidate.provider.descriptor.id,
        candidate.model.descriptor.id,
        preferredAgent.id,
        preferredAgent.displayName,
        candidate.provider.descriptor.kind,
        candidate.provider.availability,
        candidate.model.descriptor.localOnly,
    };
}

PlannedTaskStep graphStep(int capabilityCount) {
    return PlannedTaskStep{
        1,
        QStringLiteral("step-1-capability-graph"),
        QStringLiteral("Evaluate %1 provider capability metadata node(s).").arg(capabilityCount),
        {},
        {},
        {},
        {},
        ProviderKind::Local,
        CatalogAvailability::Available,
        true,
    };
}

TaskPlan blockedPlan(const TaskPlanningRequest& request, CapabilityGraph graph, QString summary,
                     const AgentDescriptor& preferredAgent) {
    return TaskPlan{
        TaskPlanStatus::Blocked,
        request.routingMode,
        request.task,
        std::move(graph),
        {},
        std::move(summary),
        preferredAgent.id,
        preferredAgent.id.isEmpty() ? QString() : agentDescriptorSummary(preferredAgent),
        false,
        false,
    };
}

} // namespace

TaskPlan StaticTaskPlanner::plan(const TaskPlanningRequest& request) const {
    auto graph = buildCapabilityGraph(request.catalogEntries);
    const auto preferredAgent = preferredAgentFor(request);
    if (request.catalogEntries.isEmpty()) {
        return blockedPlan(request, std::move(graph),
                           QStringLiteral("No provider catalog metadata is available."),
                           preferredAgent);
    }

    const auto localRequired = taskRequiresLocalOnly(request);
    const auto candidates = candidatesFor(request);

    for (const auto& candidate : candidates) {
        if (!candidateAvailable(candidate) || !candidateIsLocal(candidate)) {
            continue;
        }

        auto status = TaskPlanStatus::Planned;
        auto summary =
            QStringLiteral("Task plan uses local metadata route: %1 / %2.")
                .arg(candidate.provider.descriptor.name, candidate.model.descriptor.name);
        if (request.task.type == TaskType::Unknown) {
            status = TaskPlanStatus::FallbackPlanned;
            summary = QStringLiteral("Unknown task uses safe local metadata fallback: %1 / %2.")
                          .arg(candidate.provider.descriptor.name, candidate.model.descriptor.name);
        } else if (!localRequired && routingAllowsCloud(request.routingMode)) {
            status = TaskPlanStatus::FallbackPlanned;
            summary =
                QStringLiteral("Cloud metadata is unavailable; using local fallback: %1 / %2.")
                    .arg(candidate.provider.descriptor.name, candidate.model.descriptor.name);
        }

        const QList<PlannedTaskStep> steps{graphStep(graph.nodes.size()),
                                           stepFor(candidate, preferredAgent, 2, summary)};
        return TaskPlan{
            status,
            request.routingMode,
            request.task,
            std::move(graph),
            steps,
            summary,
            preferredAgent.id,
            preferredAgent.id.isEmpty() ? QString() : agentDescriptorSummary(preferredAgent),
            false,
            false,
        };
    }

    if (localRequired) {
        return blockedPlan(
            request, std::move(graph),
            QStringLiteral("No available local metadata capability can satisfy this task."),
            preferredAgent);
    }

    if (!routingAllowsCloud(request.routingMode)) {
        return blockedPlan(
            request, std::move(graph),
            QStringLiteral("No available local metadata capability exists for this routing mode."),
            preferredAgent);
    }

    return blockedPlan(
        request, std::move(graph),
        QStringLiteral("Cloud metadata candidates are unavailable or not configured."),
        preferredAgent);
}

} // namespace sentinel::core
