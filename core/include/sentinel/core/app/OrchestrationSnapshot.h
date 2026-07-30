#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class OrchestrationHealthStatus : std::uint8_t {
    Unknown,
    Ready,
    Degraded,
};

inline QString orchestrationHealthStatusName(OrchestrationHealthStatus status) {
    switch (status) {
    case OrchestrationHealthStatus::Unknown:
        return QStringLiteral("Unknown");
    case OrchestrationHealthStatus::Ready:
        return QStringLiteral("Ready");
    case OrchestrationHealthStatus::Degraded:
        return QStringLiteral("Degraded");
    }

    return QStringLiteral("Unknown");
}

struct OrchestrationSignal {
    QString id;
    QString label;
    QString value;
};

struct WorkspaceStateSummary {
    QString routingMode;
    QString routingStatus;
    QString selectedProviderModelSummary;
    QString taskPlanStatus;
    QString taskPlanSummary;
    QString preferredAgentSummary;
    QString memoryAffinitySummary;
    QString runtimeContextStatus;
    QString runtimeContextSummary;
    QString latestActivitySummary;
    int providerCatalogCount = 0;
    int registeredAgentCount = 0;
    int memoryCatalogCount = 0;
    int activityCount = 0;
};

struct OrchestrationSnapshot {
    OrchestrationHealthStatus healthStatus = OrchestrationHealthStatus::Unknown;
    WorkspaceStateSummary workspace;
    QList<OrchestrationSignal> signalList;
    QString summary;
    bool executionEnabled = false;
};

inline QString safeOrchestrationSnapshotSummary(const OrchestrationSnapshot& snapshot) {
    if (!snapshot.summary.isEmpty()) {
        return snapshot.summary;
    }

    return orchestrationHealthStatusName(snapshot.healthStatus);
}

inline QStringList orchestrationSignalSummaries(const OrchestrationSnapshot& snapshot) {
    QStringList summaries;
    for (const auto& signal : snapshot.signalList) {
        summaries.append(QStringLiteral("%1: %2").arg(signal.label, signal.value));
    }
    return summaries;
}

} // namespace sentinel::core
