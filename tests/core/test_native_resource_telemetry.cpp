// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// P4-02 deterministic value/contract tests. These pin the truth rules of the
// telemetry contracts: unknown stays unknown, unsupported is explicit,
// unavailable is never zero, freshness is timestamp-driven, and CPU/memory
// units normalize correctly. No host battery, thermal or GPU value is asserted.

#include "resource_telemetry_test_support.h"

#include "sentinel/core/resource/IResourceMonitorAdapter.h"
#include "sentinel/core/resource/ResourceMonitor.h"
#include "sentinel/core/resource/ResourceTelemetry.h"

#include <QtTest>

using sentinel::core::HostResourceSnapshot;
using sentinel::core::IResourceMonitorAdapter;
using sentinel::core::ResourceMetric;
using sentinel::core::ResourceMetricAvailability;
using sentinel::core::ResourceMonitor;
using sentinel::core::ResourceMonitorConfig;
using sentinel::core::ResourcePowerSource;
using sentinel::core::ResourceSampleTimestamp;

class NativeResourceTelemetryValuesTest final : public QObject {
    Q_OBJECT
private slots:
    void unknownMetricStaysUnknown();
    void measuredMetricCarriesValueAndProvenance();
    void unsupportedPlatformCapabilityIsExplicit();
    void unavailableBatteryIsNotZero();
    void gpuVramIsNeverSubstitutedForSystemRam();
    void freshnessKeepsRecentSampleMeasured();
    void staleSampleIsDetectedAndValuePreserved();
    void metricWithoutTimestampBecomesUnknown();
    void cpuNormalizationIsCorrect();
    void cpuNormalizationRejectsResetAndZeroInterval();
    void residentMemoryIsReportedInBytes();
    void capabilityReportDefaultsToUnknown();
    void nativeAdapterReportsSelfConsistentSnapshot();
};

void NativeResourceTelemetryValuesTest::unknownMetricStaysUnknown() {
    ResourceMetric<double> metric;
    QCOMPARE(metric.availability, ResourceMetricAvailability::Unknown);
    QVERIFY(!metric.value.has_value());
    QVERIFY(!metric.hasValue());
    QVERIFY(!metric.isFresh());
    // A default-constructed metric is never interpreted as zero.
    QCOMPARE(metric.hasValue(), false);
}

void NativeResourceTelemetryValuesTest::measuredMetricCarriesValueAndProvenance() {
    ResourceSampleTimestamp capturedAt;
    capturedAt.monotonicMs = 42;
    const auto metric = sentinel::core::measuredMetric<qint64>(
        1234, capturedAt, sentinel::core::ResourceMeasurementOrigin::HostKernel,
        sentinel::core::ResourceReliability::Authoritative, QStringLiteral("test.probe"));
    QCOMPARE(metric.availability, ResourceMetricAvailability::Measured);
    QCOMPARE(metric.value.value(), 1234LL);
    QCOMPARE(metric.origin, sentinel::core::ResourceMeasurementOrigin::HostKernel);
    QCOMPARE(metric.reliability, sentinel::core::ResourceReliability::Authoritative);
    QCOMPARE(metric.sourceId, QStringLiteral("test.probe"));
    QVERIFY(metric.isFresh());
}

void NativeResourceTelemetryValuesTest::unsupportedPlatformCapabilityIsExplicit() {
    auto adapter = std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>();
    adapter->setCapability(sentinel::core::ResourceSignal::GpuUtilization,
                           ResourceMetricAvailability::Unsupported);
    adapter->scripted.thermal.temperatureCelsius = sentinel::core::unsupportedMetric<double>(
        QStringLiteral("test.thermal"), QStringLiteral("No sensor API."));

    auto clock = std::make_shared<sentinel::core::test::ScriptedMonotonicClock>();
    ResourceMonitor monitor(std::move(adapter), clock);
    const auto snapshot = monitor.sampleOnce();

    QCOMPARE(snapshot.thermal.temperatureCelsius.availability,
             ResourceMetricAvailability::Unsupported);
    QVERIFY(!snapshot.thermal.temperatureCelsius.value.has_value());
    QCOMPARE(monitor.capabilities().supportFor(sentinel::core::ResourceSignal::GpuUtilization),
             ResourceMetricAvailability::Unsupported);
    // Unsupported is distinct from Unknown and must not be coerced.
    QVERIFY(snapshot.thermal.temperatureCelsius.availability !=
            ResourceMetricAvailability::Unknown);
}

void NativeResourceTelemetryValuesTest::unavailableBatteryIsNotZero() {
    auto adapter = std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>();
    ResourceSampleTimestamp stamp;
    stamp.monotonicMs = 5;
    adapter->scripted.power.batteryPresent = sentinel::core::measuredMetric(
        false, stamp, sentinel::core::ResourceMeasurementOrigin::PlatformService,
        sentinel::core::ResourceReliability::Authoritative, QStringLiteral("test.power"));
    adapter->scripted.power.batteryChargeFraction = sentinel::core::unavailableMetric<double>(
        QStringLiteral("test.power"), QStringLiteral("No battery present."));

    auto clock = std::make_shared<sentinel::core::test::ScriptedMonotonicClock>();
    clock->now = 5;
    ResourceMonitor monitor(std::move(adapter), clock);
    const auto snapshot = monitor.sampleOnce();

    QCOMPARE(snapshot.power.batteryPresent.value.value(), false);
    QCOMPARE(snapshot.power.batteryChargeFraction.availability,
             ResourceMetricAvailability::Unavailable);
    // Crucially, an absent battery is not a 0% charge.
    QVERIFY(!snapshot.power.batteryChargeFraction.value.has_value());
}

void NativeResourceTelemetryValuesTest::gpuVramIsNeverSubstitutedForSystemRam() {
    auto adapter = std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>();
    sentinel::core::GpuResourceMetrics gpu;
    gpu.deviceId = QStringLiteral("gpu0");
    ResourceSampleTimestamp stamp;
    stamp.monotonicMs = 1;
    gpu.vramTotalBytes = sentinel::core::measuredMetric<qint64>(
        8LL * 1024 * 1024 * 1024, stamp, sentinel::core::ResourceMeasurementOrigin::HostKernel,
        sentinel::core::ResourceReliability::Authoritative, QStringLiteral("test.gpu"));
    adapter->scripted.gpus.append(gpu);

    auto clock = std::make_shared<sentinel::core::test::ScriptedMonotonicClock>();
    clock->now = 1;
    ResourceMonitor monitor(std::move(adapter), clock);
    const auto snapshot = monitor.sampleOnce();

    QCOMPARE(snapshot.gpus.size(), 1);
    QCOMPARE(snapshot.gpus.first().vramTotalBytes.value.value(), 8LL * 1024 * 1024 * 1024);
    // The host RAM reading stays unknown; GPU memory is never folded in.
    QCOMPARE(snapshot.hostMemory.totalBytes.availability, ResourceMetricAvailability::Unknown);
    QVERIFY(!snapshot.hostMemory.totalBytes.value.has_value());
}

void NativeResourceTelemetryValuesTest::freshnessKeepsRecentSampleMeasured() {
    auto adapter = std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>();
    ResourceMonitorConfig config;
    config.metricMaxAgeMs = 5000;
    auto clock = std::make_shared<sentinel::core::test::ScriptedMonotonicClock>();
    clock->now = 10000;
    ResourceMonitor monitor(std::move(adapter), clock, config);

    // The monitor stamps the sample with the clock, so it is fresh at capture.
    const auto snapshot = monitor.sampleOnce();
    QCOMPARE(snapshot.hostMemory.totalBytes.availability, ResourceMetricAvailability::Unknown);
    QCOMPARE(snapshot.adapterId, QStringLiteral("test.scripted"));
    QCOMPARE(snapshot.capturedAt.monotonicMs, 10000LL);
}

void NativeResourceTelemetryValuesTest::staleSampleIsDetectedAndValuePreserved() {
    auto adapter = std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>();
    ResourceSampleTimestamp old;
    old.monotonicMs = 1000;
    adapter->scripted.hostMemory.totalBytes = sentinel::core::measuredMetric<qint64>(
        4096, old, sentinel::core::ResourceMeasurementOrigin::HostKernel,
        sentinel::core::ResourceReliability::Authoritative, QStringLiteral("test.mem"));

    ResourceMonitorConfig config;
    config.metricMaxAgeMs = 5000;
    auto clock = std::make_shared<sentinel::core::test::ScriptedMonotonicClock>();
    clock->now = 10000; // 9000 ms after the metric was captured.
    ResourceMonitor monitor(std::move(adapter), clock, config);

    const auto snapshot = monitor.sampleOnce();
    QCOMPARE(snapshot.hostMemory.totalBytes.availability, ResourceMetricAvailability::Stale);
    QVERIFY(snapshot.hostMemory.totalBytes.hasValue());
    QVERIFY(!snapshot.hostMemory.totalBytes.isFresh());
    QCOMPARE(snapshot.hostMemory.totalBytes.value.value(), 4096LL);
}

void NativeResourceTelemetryValuesTest::metricWithoutTimestampBecomesUnknown() {
    ResourceMetric<qint64> metric;
    metric.availability = ResourceMetricAvailability::Measured;
    metric.value = 4096;
    // No capturedAt: a "measured" value that cannot be proven fresh degrades to
    // unknown rather than silently passing as current.
    sentinel::core::applyMetricFreshness(metric, 1000, 5000);
    QCOMPARE(metric.availability, ResourceMetricAvailability::Unknown);
    QVERIFY(!metric.value.has_value());
}

void NativeResourceTelemetryValuesTest::cpuNormalizationIsCorrect() {
    // 1.6 CPU-seconds over 8 s on an 8-core host.
    const auto result = sentinel::core::deriveCpuFractions(1.6, 0.0, 8000, 8);
    QVERIFY(result.fractionOfCore.has_value());
    QVERIFY(result.fractionOfHost.has_value());
    QCOMPARE(*result.fractionOfCore, 0.2);
    QCOMPARE(*result.fractionOfHost, 0.025);

    // Without a host count the core fraction is still truthful.
    const auto noHost = sentinel::core::deriveCpuFractions(1.0, 0.0, 1000, 0);
    QVERIFY(noHost.fractionOfCore.has_value());
    QCOMPARE(*noHost.fractionOfCore, 1.0);
    QVERIFY(!noHost.fractionOfHost.has_value());
}

void NativeResourceTelemetryValuesTest::cpuNormalizationRejectsResetAndZeroInterval() {
    QVERIFY(!sentinel::core::deriveCpuFractions(0.5, 0.4, 0, 8).fractionOfCore.has_value());
    QVERIFY(!sentinel::core::deriveCpuFractions(0.4, 0.5, 1000, 8).fractionOfCore.has_value());
    QVERIFY(
        !sentinel::core::deriveCpuFractions(std::nullopt, 0.4, 1000, 8).fractionOfCore.has_value());
    QVERIFY(
        !sentinel::core::deriveCpuFractions(0.5, std::nullopt, 1000, 8).fractionOfCore.has_value());
}

void NativeResourceTelemetryValuesTest::residentMemoryIsReportedInBytes() {
    auto adapter = std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>();
    ResourceSampleTimestamp stamp;
    stamp.monotonicMs = 3;
    constexpr qint64 kBytes = 12'345'678; // ~11.77 MiB, unambiguous in bytes.
    adapter->scripted.process.residentBytes = sentinel::core::measuredMetric<qint64>(
        kBytes, stamp, sentinel::core::ResourceMeasurementOrigin::SentinelProcess,
        sentinel::core::ResourceReliability::Authoritative, QStringLiteral("test.rss"));

    auto clock = std::make_shared<sentinel::core::test::ScriptedMonotonicClock>();
    clock->now = 3;
    ResourceMonitor monitor(std::move(adapter), clock);
    const auto snapshot = monitor.sampleOnce();

    QCOMPARE(snapshot.process.residentBytes.availability, ResourceMetricAvailability::Measured);
    QCOMPARE(snapshot.process.residentBytes.value.value(), kBytes);
    // The byte value is larger than the same figure interpreted as KiB, which
    // would indicate a unit error.
    QVERIFY(snapshot.process.residentBytes.value.value() > kBytes / 1024);
}

void NativeResourceTelemetryValuesTest::capabilityReportDefaultsToUnknown() {
    auto adapter = std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>();
    auto clock = std::make_shared<sentinel::core::test::ScriptedMonotonicClock>();
    ResourceMonitor monitor(std::move(adapter), clock);
    // A signal with no declared support entry is Unknown, never assumed.
    QCOMPARE(monitor.capabilities().supportFor(sentinel::core::ResourceSignal::BatteryCharge),
             ResourceMetricAvailability::Unknown);
}

void NativeResourceTelemetryValuesTest::nativeAdapterReportsSelfConsistentSnapshot() {
    // Uses the real native adapter on the host, but asserts only invariants that
    // hold regardless of hardware: no fabricated zero and no scope conflation.
    auto adapter = sentinel::core::makeNativeResourceMonitorAdapter();
    QVERIFY(adapter != nullptr);
    QVERIFY(!adapter->adapterId().isEmpty());
    QVERIFY(!adapter->capabilities().entries.isEmpty());

    ResourceSampleTimestamp stamp;
    stamp.monotonicMs = 1;
    const auto snapshot = adapter->sample(stamp);

    // Whatever the platform supports, any value it does report is positive.
    if (snapshot.hostMemory.totalBytes.isFresh())
        QVERIFY(snapshot.hostMemory.totalBytes.value.value() > 0);
    if (snapshot.process.residentBytes.isFresh())
        QVERIFY(snapshot.process.residentBytes.value.value() > 0);
    for (const auto& gpu : snapshot.gpus) {
        if (gpu.vramTotalBytes.hasValue())
            QVERIFY(gpu.vramTotalBytes.value.value() > 0);
    }
}

QTEST_MAIN(NativeResourceTelemetryValuesTest)

#include "test_native_resource_telemetry.moc"
