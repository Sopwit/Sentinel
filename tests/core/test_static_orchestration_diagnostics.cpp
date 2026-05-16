#include "sentinel/core/OrchestrationDiagnostics.h"
#include "sentinel/core/StaticProviderCatalog.h"

#include <QtTest>

using sentinel::core::OrchestrationDiagnosticLevel;
using sentinel::core::orchestrationDiagnosticLevelName;
using sentinel::core::OrchestrationDiagnosticsInput;
using sentinel::core::orchestrationDiagnosticSummaries;
using sentinel::core::OrchestrationHealthStatus;
using sentinel::core::OrchestrationReadinessReport;
using sentinel::core::OrchestrationSnapshot;
using sentinel::core::safeOrchestrationReadinessSummary;
using sentinel::core::StaticOrchestrationDiagnostics;
using sentinel::core::StaticProviderCatalog;
using sentinel::core::WorkspaceStateSummary;

class StaticOrchestrationDiagnosticsTest final : public QObject {
    Q_OBJECT

private slots:
    void namesDiagnosticLevels();
    void producesDeterministicDiagnosticOrdering();
    void reportsLocalOnlyHealthyMetadataReady();
    void reportsCloudProvidersNotConfiguredAsInfo();
    void reportsExecutionCapabilityAsError();
};

static OrchestrationSnapshot readySnapshot() {
    OrchestrationSnapshot snapshot;
    snapshot.healthStatus = OrchestrationHealthStatus::Ready;
    snapshot.workspace = WorkspaceStateSummary{
        QStringLiteral("Local Only"),
        QStringLiteral("Routed"),
        QStringLiteral("Local Only -> Local Metadata Provider / Sentinel Local Placeholder"),
        QStringLiteral("Fallback Planned"),
        QStringLiteral("Unknown task uses safe local metadata fallback."),
        QStringLiteral("Atlas (Coordinator, Available, Local)"),
        QStringLiteral("Ambient (Available, Public Metadata, Session)"),
        QStringLiteral("Empty"),
        QStringLiteral("No runtime context yet."),
        QStringLiteral("No agent activity yet."),
        4,
        6,
        5,
        0,
    };
    snapshot.summary = QStringLiteral("Ready orchestration snapshot.");
    snapshot.executionEnabled = false;
    return snapshot;
}

static OrchestrationDiagnosticsInput readyInput() {
    return OrchestrationDiagnosticsInput{
        readySnapshot(), StaticProviderCatalog{}.entries(), true, true, true, true,
    };
}

void StaticOrchestrationDiagnosticsTest::namesDiagnosticLevels() {
    QCOMPARE(orchestrationDiagnosticLevelName(OrchestrationDiagnosticLevel::Info),
             QStringLiteral("Info"));
    QCOMPARE(orchestrationDiagnosticLevelName(OrchestrationDiagnosticLevel::Warning),
             QStringLiteral("Warning"));
    QCOMPARE(orchestrationDiagnosticLevelName(OrchestrationDiagnosticLevel::Error),
             QStringLiteral("Error"));
}

void StaticOrchestrationDiagnosticsTest::producesDeterministicDiagnosticOrdering() {
    const auto report = StaticOrchestrationDiagnostics{}.generate(readyInput());

    QCOMPARE(report.checks.size(), 10);
    QCOMPARE(report.diagnostics.size(), 10);
    QCOMPARE(report.checks.at(0).id, QStringLiteral("routing-mode"));
    QCOMPARE(report.checks.at(1).id, QStringLiteral("selected-route"));
    QCOMPARE(report.checks.at(2).id, QStringLiteral("provider-catalog"));
    QCOMPARE(report.checks.at(3).id, QStringLiteral("agent-registry"));
    QCOMPARE(report.checks.at(4).id, QStringLiteral("memory-taxonomy"));
    QCOMPARE(report.checks.at(5).id, QStringLiteral("task-planner"));
    QCOMPARE(report.checks.at(6).id, QStringLiteral("snapshot-health"));
    QCOMPARE(report.checks.at(7).id, QStringLiteral("privacy-posture"));
    QCOMPARE(report.checks.at(8).id, QStringLiteral("cloud-providers"));
    QCOMPARE(report.checks.at(9).id, QStringLiteral("execution-capability"));
}

void StaticOrchestrationDiagnosticsTest::reportsLocalOnlyHealthyMetadataReady() {
    const auto report = StaticOrchestrationDiagnostics{}.generate(readyInput());

    QCOMPARE(report.status, QStringLiteral("Ready"));
    QCOMPARE(safeOrchestrationReadinessSummary(report),
             QStringLiteral("Ready orchestration readiness: 10 deterministic metadata checks, 10 "
                            "diagnostic entries."));
    for (const auto& diagnostic : report.diagnostics) {
        QCOMPARE(diagnostic.level, OrchestrationDiagnosticLevel::Info);
    }
    QVERIFY(orchestrationDiagnosticSummaries(report).contains(
        QStringLiteral("Info: Privacy Posture - Local-only routing posture is active.")));
    QVERIFY(orchestrationDiagnosticSummaries(report).contains(
        QStringLiteral("Info: Execution Capability - Execution capability remains disabled.")));
}

void StaticOrchestrationDiagnosticsTest::reportsCloudProvidersNotConfiguredAsInfo() {
    const auto report = StaticOrchestrationDiagnostics{}.generate(readyInput());

    const auto summaries = orchestrationDiagnosticSummaries(report);
    QVERIFY(summaries.contains(
        QStringLiteral("Info: Cloud Providers - Cloud provider metadata remains not configured.")));
    QCOMPARE(report.checks.at(8).passed, true);
    QCOMPARE(report.checks.at(8).diagnostic.level, OrchestrationDiagnosticLevel::Info);
}

void StaticOrchestrationDiagnosticsTest::reportsExecutionCapabilityAsError() {
    auto input = readyInput();
    input.snapshot.executionEnabled = true;

    const auto report = StaticOrchestrationDiagnostics{}.generate(input);

    QCOMPARE(report.status, QStringLiteral("Blocked"));
    QCOMPARE(report.checks.at(9).passed, false);
    QCOMPARE(report.checks.at(9).diagnostic.level, OrchestrationDiagnosticLevel::Error);
    QVERIFY(orchestrationDiagnosticSummaries(report).contains(
        QStringLiteral("Error: Execution Capability - Execution capability is enabled in "
                       "metadata.")));
}

QTEST_MAIN(StaticOrchestrationDiagnosticsTest)

#include "test_static_orchestration_diagnostics.moc"
