#pragma once

#include "sentinel/core/AgentMetadata.h"
#include "sentinel/core/MemoryMetadata.h"
#include "sentinel/core/ProviderCatalog.h"

#include <QList>
#include <QString>

#include <cstdint>

namespace sentinel::core {

enum class TaskPlanStatus : std::uint8_t {
    NotRequested,
    Planned,
    FallbackPlanned,
    Blocked,
};

inline QString taskPlanStatusName(TaskPlanStatus status) {
    switch (status) {
    case TaskPlanStatus::NotRequested:
        return QStringLiteral("Not Requested");
    case TaskPlanStatus::Planned:
        return QStringLiteral("Planned");
    case TaskPlanStatus::FallbackPlanned:
        return QStringLiteral("Fallback Planned");
    case TaskPlanStatus::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Not Requested");
}

struct CapabilityNode {
    QString id;
    QString name;
    ProviderKind providerKind = ProviderKind::Local;
    CatalogAvailability availability = CatalogAvailability::Unavailable;
    CatalogPrivacyLevel privacyLevel = CatalogPrivacyLevel::LocalOnly;
    QStringList supportedTaskTypes;
    int ramHintMb = 0;
    int diskHintMb = 0;
};

struct CapabilityGraph {
    QList<CapabilityNode> nodes;
};

struct PlannedTaskStep {
    int order = 0;
    QString id;
    QString summary;
    QString providerId;
    QString modelId;
    QString agentId;
    QString agentName;
    QString memoryTypeId;
    QString memoryTypeName;
    ProviderKind providerKind = ProviderKind::Local;
    CatalogAvailability availability = CatalogAvailability::Unavailable;
    bool localOnly = true;
};

struct TaskPlanningRequest {
    TaskClassification task;
    RoutingMode routingMode = RoutingMode::LocalOnly;
    QList<ProviderCatalogEntry> catalogEntries;
    QList<AgentDescriptor> agents;
    QList<MemoryShardDescriptor> memoryShards;
};

struct TaskPlan {
    TaskPlanStatus status = TaskPlanStatus::NotRequested;
    RoutingMode routingMode = RoutingMode::LocalOnly;
    TaskClassification task;
    CapabilityGraph capabilityGraph;
    QList<PlannedTaskStep> steps;
    QString summary;
    QString preferredAgentId;
    QString preferredAgentSummary;
    QString preferredMemoryTypeId;
    QString preferredMemorySummary;
    QList<MemoryAffinity> memoryAffinities;
    bool networkRequired = false;
    bool modelExecutionAllowed = false;
};

inline QString safeTaskPlanSummary(const TaskPlan& plan) {
    if (!plan.summary.isEmpty()) {
        return plan.summary;
    }

    return taskPlanStatusName(plan.status);
}

} // namespace sentinel::core
