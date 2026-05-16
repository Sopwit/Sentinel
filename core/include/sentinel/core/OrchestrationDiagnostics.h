#pragma once

#include "sentinel/core/OrchestrationSnapshot.h"
#include "sentinel/core/ProviderCatalog.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <utility>

namespace sentinel::core {

enum class OrchestrationDiagnosticLevel : std::uint8_t {
    Info,
    Warning,
    Error,
};

inline QString orchestrationDiagnosticLevelName(OrchestrationDiagnosticLevel level) {
    switch (level) {
    case OrchestrationDiagnosticLevel::Info:
        return QStringLiteral("Info");
    case OrchestrationDiagnosticLevel::Warning:
        return QStringLiteral("Warning");
    case OrchestrationDiagnosticLevel::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Info");
}

struct OrchestrationDiagnostic {
    QString id;
    OrchestrationDiagnosticLevel level = OrchestrationDiagnosticLevel::Info;
    QString title;
    QString message;
};

struct OrchestrationReadinessCheck {
    QString id;
    QString title;
    bool passed = false;
    OrchestrationDiagnostic diagnostic;
};

struct OrchestrationReadinessReport {
    QString status;
    QString summary;
    QList<OrchestrationReadinessCheck> checks;
    QList<OrchestrationDiagnostic> diagnostics;
};

struct OrchestrationDiagnosticsInput {
    OrchestrationSnapshot snapshot;
    QList<ProviderCatalogEntry> providerCatalogEntries;
    bool providerCatalogAvailable = false;
    bool agentRegistryAvailable = false;
    bool memoryCatalogAvailable = false;
    bool taskPlannerAvailable = false;
};

QString safeOrchestrationReadinessSummary(const OrchestrationReadinessReport& report);
QStringList orchestrationDiagnosticSummaries(const OrchestrationReadinessReport& report);

class StaticOrchestrationDiagnostics final {
public:
    OrchestrationReadinessReport generate(const OrchestrationDiagnosticsInput& input) const;
};

} // namespace sentinel::core
