#include "sentinel/core/LocalRuntime.h"

#include <QtTest>

using sentinel::core::localRuntimeCapabilitySummaries;
using sentinel::core::LocalRuntimeHealth;
using sentinel::core::localRuntimeHealthName;
using sentinel::core::LocalRuntimeRequest;
using sentinel::core::LocalRuntimeStatus;
using sentinel::core::localRuntimeStatusName;
using sentinel::core::NullLocalRuntime;
using sentinel::core::safeLocalRuntimeResponseSummary;
using sentinel::core::safeLocalRuntimeSummary;

class LocalRuntimeTest final : public QObject {
    Q_OBJECT

private slots:
    void namesStatusAndHealth();
    void exposesDeterministicNullRuntimeMetadata();
    void refusesExecutionSafely();
};

void LocalRuntimeTest::namesStatusAndHealth() {
    QCOMPARE(localRuntimeStatusName(LocalRuntimeStatus::MetadataOnly),
             QStringLiteral("Metadata Only"));
    QCOMPARE(localRuntimeStatusName(LocalRuntimeStatus::Disabled), QStringLiteral("Disabled"));
    QCOMPARE(localRuntimeStatusName(LocalRuntimeStatus::Unavailable),
             QStringLiteral("Unavailable"));
    QCOMPARE(localRuntimeHealthName(LocalRuntimeHealth::Ready), QStringLiteral("Ready"));
    QCOMPARE(localRuntimeHealthName(LocalRuntimeHealth::NotExecutable),
             QStringLiteral("Not Executable"));
    QCOMPARE(localRuntimeHealthName(LocalRuntimeHealth::Unavailable),
             QStringLiteral("Unavailable"));
}

void LocalRuntimeTest::exposesDeterministicNullRuntimeMetadata() {
    const NullLocalRuntime runtime;
    const auto descriptor = runtime.descriptor();

    QCOMPARE(descriptor.id, QStringLiteral("null-local-runtime"));
    QCOMPARE(descriptor.name, QStringLiteral("Null Local Runtime"));
    QCOMPARE(descriptor.status, LocalRuntimeStatus::MetadataOnly);
    QCOMPARE(descriptor.health, LocalRuntimeHealth::NotExecutable);
    QCOMPARE(safeLocalRuntimeSummary(descriptor),
             QStringLiteral("Null Local Runtime is metadata-only; local inference execution is "
                            "disabled."));
    QCOMPARE(descriptor.capabilities.size(), 3);
    QCOMPARE(localRuntimeCapabilitySummaries(descriptor.capabilities).first(),
             QStringLiteral("Metadata Reporting (Enabled): Reports deterministic local runtime "
                            "status and health."));
}

void LocalRuntimeTest::refusesExecutionSafely() {
    const NullLocalRuntime runtime;
    const auto response = runtime.evaluate(LocalRuntimeRequest{QStringLiteral("run model")});

    QVERIFY(!response.accepted);
    QCOMPARE(response.status, QStringLiteral("Refused"));
    QCOMPARE(safeLocalRuntimeResponseSummary(response),
             QStringLiteral("Local runtime boundary is metadata-only; execution is disabled."));
}

QTEST_MAIN(LocalRuntimeTest)

#include "test_local_runtime.moc"
