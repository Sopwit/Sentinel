#include "sentinel/core/RuntimeIntegration.h"

#include <QtTest>

using sentinel::core::LocalRuntimeAdapterHealth;
using sentinel::core::localRuntimeAdapterHealthName;
using sentinel::core::LocalRuntimeAdapterStatus;
using sentinel::core::localRuntimeAdapterStatusName;
using sentinel::core::ProviderRuntimeBridgeRequest;
using sentinel::core::ProviderRuntimeBridgeStatus;
using sentinel::core::providerRuntimeBridgeStatusName;
using sentinel::core::runtimeIntegrationCheckSummaries;
using sentinel::core::RuntimeIntegrationReadiness;
using sentinel::core::runtimeIntegrationReadinessName;
using sentinel::core::safeLocalRuntimeAdapterSummary;
using sentinel::core::safeProviderRuntimeBridgeResponseSummary;
using sentinel::core::safeProviderRuntimeBridgeSummary;
using sentinel::core::safeRuntimeIntegrationReportSummary;
using sentinel::core::StaticLocalRuntimeAdapter;
using sentinel::core::StaticProviderRuntimeBridge;
using sentinel::core::StaticRuntimeIntegrationReadiness;

class RuntimeIntegrationTest final : public QObject {
    Q_OBJECT

private slots:
    void namesRuntimeIntegrationMetadata();
    void exposesDeterministicAdapterMetadata();
    void reportsProviderBridgeNotConnected();
    void reportsReadinessInStableOrder();
};

void RuntimeIntegrationTest::namesRuntimeIntegrationMetadata() {
    QCOMPARE(localRuntimeAdapterStatusName(LocalRuntimeAdapterStatus::Placeholder),
             QStringLiteral("Placeholder"));
    QCOMPARE(localRuntimeAdapterHealthName(LocalRuntimeAdapterHealth::MetadataOnly),
             QStringLiteral("Metadata Only"));
    QCOMPARE(providerRuntimeBridgeStatusName(ProviderRuntimeBridgeStatus::NotConnected),
             QStringLiteral("Not Connected"));
    QCOMPARE(runtimeIntegrationReadinessName(RuntimeIntegrationReadiness::Blocked),
             QStringLiteral("Blocked"));
}

void RuntimeIntegrationTest::exposesDeterministicAdapterMetadata() {
    const StaticLocalRuntimeAdapter adapter;
    const auto descriptor = adapter.descriptor();

    QCOMPARE(descriptor.id, QStringLiteral("ollama-local-runtime-adapter-placeholder"));
    QCOMPARE(descriptor.status, LocalRuntimeAdapterStatus::Placeholder);
    QCOMPARE(descriptor.health, LocalRuntimeAdapterHealth::MetadataOnly);
    QCOMPARE(descriptor.capabilities.size(), 3);
    QCOMPARE(safeLocalRuntimeAdapterSummary(descriptor),
             QStringLiteral("Ollama local runtime adapter contract is metadata-only; no runtime "
                            "connection is configured."));
    QVERIFY(!descriptor.capabilities[0].available);
    QVERIFY(!descriptor.capabilities[0].executable);
}

void RuntimeIntegrationTest::reportsProviderBridgeNotConnected() {
    const StaticProviderRuntimeBridge bridge;
    const auto summary = bridge.summary();
    const auto response = bridge.evaluate(ProviderRuntimeBridgeRequest{});

    QCOMPARE(summary.status, ProviderRuntimeBridgeStatus::NotConnected);
    QVERIFY(!summary.connected);
    QVERIFY(!summary.executable);
    QCOMPARE(safeProviderRuntimeBridgeSummary(summary),
             QStringLiteral("Provider runtime bridge is not connected and cannot execute provider "
                            "requests."));
    QCOMPARE(response.status, ProviderRuntimeBridgeStatus::NotConnected);
    QVERIFY(!response.connected);
    QVERIFY(!response.executable);
    QCOMPARE(safeProviderRuntimeBridgeResponseSummary(response),
             QStringLiteral("Provider runtime bridge is metadata-only; no provider or local "
                            "runtime request was executed."));
}

void RuntimeIntegrationTest::reportsReadinessInStableOrder() {
    const StaticLocalRuntimeAdapter adapter;
    const StaticProviderRuntimeBridge bridge;
    const StaticRuntimeIntegrationReadiness readiness;
    const auto report = readiness.evaluate(adapter.descriptor(), bridge.summary());

    QCOMPARE(report.readiness, RuntimeIntegrationReadiness::Blocked);
    QVERIFY(!report.executable);
    QCOMPARE(report.checks.size(), 5);
    QCOMPARE(report.checks[0].id, QStringLiteral("runtime-integration.adapter-contract"));
    QCOMPARE(report.checks[1].id, QStringLiteral("runtime-integration.endpoint"));
    QCOMPARE(report.checks[2].id, QStringLiteral("runtime-integration.model-discovery"));
    QCOMPARE(report.checks[3].id, QStringLiteral("runtime-integration.provider-bridge"));
    QCOMPARE(report.checks[4].id, QStringLiteral("runtime-integration.execution"));
    QCOMPARE(safeRuntimeIntegrationReportSummary(report),
             QStringLiteral("Runtime integration readiness is blocked: adapter is metadata-only, "
                            "bridge is not connected, and execution remains disabled."));
    QVERIFY(runtimeIntegrationCheckSummaries(report.checks)
                .contains(QStringLiteral(
                    "Blocked: Model Discovery - Model discovery is not implemented and no "
                    "filesystem or runtime scan is performed.")));
}

QTEST_MAIN(RuntimeIntegrationTest)

#include "test_runtime_integration.moc"
