#include "sentinel/core/OrchestrationDiagnostics.h"

#include <utility>

namespace sentinel::core {

namespace {

OrchestrationDiagnostic diagnostic(QString id, OrchestrationDiagnosticLevel level, QString title,
                                   QString message) {
    return OrchestrationDiagnostic{std::move(id), level, std::move(title), std::move(message)};
}

OrchestrationReadinessCheck check(QString id, QString title, bool passed,
                                  OrchestrationDiagnostic diagnostic) {
    return OrchestrationReadinessCheck{std::move(id), std::move(title), passed,
                                       std::move(diagnostic)};
}

bool hasError(const QList<OrchestrationDiagnostic>& diagnostics) {
    for (const auto& item : diagnostics) {
        if (item.level == OrchestrationDiagnosticLevel::Error) {
            return true;
        }
    }
    return false;
}

bool hasWarning(const QList<OrchestrationDiagnostic>& diagnostics) {
    for (const auto& item : diagnostics) {
        if (item.level == OrchestrationDiagnosticLevel::Warning) {
            return true;
        }
    }
    return false;
}

bool hasAvailableCloudProvider(const QList<ProviderCatalogEntry>& entries) {
    for (const auto& entry : entries) {
        if (entry.descriptor.kind == ProviderKind::Cloud &&
            isCatalogEntryAvailable(entry.availability)) {
            return true;
        }
    }
    return false;
}

bool hasCloudProvider(const QList<ProviderCatalogEntry>& entries) {
    for (const auto& entry : entries) {
        if (entry.descriptor.kind == ProviderKind::Cloud) {
            return true;
        }
    }
    return false;
}

} // namespace

QString safeOrchestrationReadinessSummary(const OrchestrationReadinessReport& report) {
    if (!report.summary.isEmpty()) {
        return report.summary;
    }

    return report.status.isEmpty() ? QStringLiteral("Unknown") : report.status;
}

QStringList orchestrationDiagnosticSummaries(const OrchestrationReadinessReport& report) {
    QStringList summaries;
    for (const auto& item : report.diagnostics) {
        summaries.append(
            QStringLiteral("%1: %2 - %3")
                .arg(orchestrationDiagnosticLevelName(item.level), item.title, item.message));
    }
    return summaries;
}

OrchestrationReadinessReport
StaticOrchestrationDiagnostics::generate(const OrchestrationDiagnosticsInput& input) const {
    OrchestrationReadinessReport report;

    const auto& workspace = input.snapshot.workspace;

    auto appendCheck = [&report](OrchestrationReadinessCheck readinessCheck) {
        report.diagnostics.append(readinessCheck.diagnostic);
        report.checks.append(std::move(readinessCheck));
    };

    const auto routingModePresent = !workspace.routingMode.trimmed().isEmpty();
    appendCheck(
        check(QStringLiteral("routing-mode"), QStringLiteral("Routing Mode"), routingModePresent,
              diagnostic(QStringLiteral("routing-mode"),
                         routingModePresent ? OrchestrationDiagnosticLevel::Info
                                            : OrchestrationDiagnosticLevel::Error,
                         QStringLiteral("Routing Mode"),
                         routingModePresent
                             ? QStringLiteral("%1 routing mode is set.").arg(workspace.routingMode)
                             : QStringLiteral("Routing mode metadata is missing."))));

    const auto selectedRoutePresent =
        !workspace.selectedProviderModelSummary.trimmed().isEmpty() &&
        !workspace.selectedProviderModelSummary.startsWith(QStringLiteral("No "));
    appendCheck(check(
        QStringLiteral("selected-route"), QStringLiteral("Selected Route"), selectedRoutePresent,
        diagnostic(QStringLiteral("selected-route"),
                   selectedRoutePresent ? OrchestrationDiagnosticLevel::Info
                                        : OrchestrationDiagnosticLevel::Error,
                   QStringLiteral("Selected Route"),
                   selectedRoutePresent
                       ? QStringLiteral("Selected provider/model metadata is present.")
                       : QStringLiteral("Selected provider/model route metadata is missing."))));

    const auto providerCatalogReady =
        input.providerCatalogAvailable && workspace.providerCatalogCount > 0;
    appendCheck(
        check(QStringLiteral("provider-catalog"), QStringLiteral("Provider Catalog"),
              providerCatalogReady,
              diagnostic(QStringLiteral("provider-catalog"),
                         providerCatalogReady ? OrchestrationDiagnosticLevel::Info
                                              : OrchestrationDiagnosticLevel::Error,
                         QStringLiteral("Provider Catalog"),
                         providerCatalogReady
                             ? QStringLiteral("%1 provider catalog entries are available.")
                                   .arg(workspace.providerCatalogCount)
                             : QStringLiteral("Provider catalog metadata is unavailable."))));

    const auto agentRegistryReady =
        input.agentRegistryAvailable && workspace.registeredAgentCount > 0;
    appendCheck(check(QStringLiteral("agent-registry"), QStringLiteral("Agent Registry"),
                      agentRegistryReady,
                      diagnostic(QStringLiteral("agent-registry"),
                                 agentRegistryReady ? OrchestrationDiagnosticLevel::Info
                                                    : OrchestrationDiagnosticLevel::Error,
                                 QStringLiteral("Agent Registry"),
                                 agentRegistryReady
                                     ? QStringLiteral("%1 agent descriptors are available.")
                                           .arg(workspace.registeredAgentCount)
                                     : QStringLiteral("Agent registry metadata is unavailable."))));

    const auto memoryCatalogReady =
        input.memoryCatalogAvailable && workspace.memoryCatalogCount > 0;
    appendCheck(check(
        QStringLiteral("memory-taxonomy"), QStringLiteral("Memory Taxonomy"), memoryCatalogReady,
        diagnostic(QStringLiteral("memory-taxonomy"),
                   memoryCatalogReady ? OrchestrationDiagnosticLevel::Info
                                      : OrchestrationDiagnosticLevel::Error,
                   QStringLiteral("Memory Taxonomy"),
                   memoryCatalogReady
                       ? QStringLiteral("%1 memory taxonomy categories are available.")
                             .arg(workspace.memoryCatalogCount)
                       : QStringLiteral("Memory taxonomy metadata is unavailable."))));

    const auto taskPlannerReady = input.taskPlannerAvailable &&
                                  workspace.taskPlanStatus != QStringLiteral("Blocked") &&
                                  !workspace.taskPlanStatus.trimmed().isEmpty();
    appendCheck(check(
        QStringLiteral("task-planner"), QStringLiteral("Task Planner"), taskPlannerReady,
        diagnostic(QStringLiteral("task-planner"),
                   taskPlannerReady ? OrchestrationDiagnosticLevel::Info
                                    : OrchestrationDiagnosticLevel::Error,
                   QStringLiteral("Task Planner"),
                   taskPlannerReady ? QStringLiteral("Task planner metadata is available: %1.")
                                          .arg(workspace.taskPlanStatus)
                                    : QStringLiteral("Task planner metadata is blocked or "
                                                     "unavailable."))));

    const auto healthReady = input.snapshot.healthStatus == OrchestrationHealthStatus::Ready;
    appendCheck(
        check(QStringLiteral("snapshot-health"), QStringLiteral("Snapshot Health"), healthReady,
              diagnostic(QStringLiteral("snapshot-health"),
                         healthReady ? OrchestrationDiagnosticLevel::Info
                                     : OrchestrationDiagnosticLevel::Warning,
                         QStringLiteral("Snapshot Health"),
                         QStringLiteral("Snapshot health is %1.")
                             .arg(orchestrationHealthStatusName(input.snapshot.healthStatus)))));

    const auto localOnly = workspace.routingMode == QStringLiteral("Local Only");
    appendCheck(check(
        QStringLiteral("privacy-posture"), QStringLiteral("Privacy Posture"), localOnly,
        diagnostic(QStringLiteral("privacy-posture"),
                   localOnly ? OrchestrationDiagnosticLevel::Info
                             : OrchestrationDiagnosticLevel::Warning,
                   QStringLiteral("Privacy Posture"),
                   localOnly ? QStringLiteral("Local-only routing posture is active.")
                             : QStringLiteral("Routing posture is %1; cloud execution remains "
                                              "unavailable in this phase.")
                                   .arg(workspace.routingMode))));

    const auto cloudProvidersPresent = hasCloudProvider(input.providerCatalogEntries);
    const auto cloudProvidersUnavailable = !hasAvailableCloudProvider(input.providerCatalogEntries);
    appendCheck(check(
        QStringLiteral("cloud-providers"), QStringLiteral("Cloud Providers"),
        cloudProvidersPresent && cloudProvidersUnavailable,
        diagnostic(QStringLiteral("cloud-providers"),
                   cloudProvidersUnavailable ? OrchestrationDiagnosticLevel::Info
                                             : OrchestrationDiagnosticLevel::Warning,
                   QStringLiteral("Cloud Providers"),
                   cloudProvidersPresent
                       ? (cloudProvidersUnavailable
                              ? QStringLiteral("Cloud provider metadata remains not configured.")
                              : QStringLiteral("One or more cloud providers appear available in "
                                               "metadata."))
                       : QStringLiteral("No cloud provider metadata is present."))));

    appendCheck(
        check(QStringLiteral("execution-capability"), QStringLiteral("Execution Capability"),
              !input.snapshot.executionEnabled,
              diagnostic(QStringLiteral("execution-capability"),
                         input.snapshot.executionEnabled ? OrchestrationDiagnosticLevel::Error
                                                         : OrchestrationDiagnosticLevel::Info,
                         QStringLiteral("Execution Capability"),
                         input.snapshot.executionEnabled
                             ? QStringLiteral("Execution capability is enabled in metadata.")
                             : QStringLiteral("Execution capability remains disabled."))));

    if (hasError(report.diagnostics)) {
        report.status = QStringLiteral("Blocked");
    } else if (hasWarning(report.diagnostics)) {
        report.status = QStringLiteral("Review");
    } else {
        report.status = QStringLiteral("Ready");
    }

    report.summary =
        QStringLiteral("%1 orchestration readiness: %2 deterministic metadata checks, %3 "
                       "diagnostic entries.")
            .arg(report.status)
            .arg(report.checks.size())
            .arg(report.diagnostics.size());
    return report;
}

} // namespace sentinel::core
