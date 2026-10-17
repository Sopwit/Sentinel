// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// P4-04 deterministic fixtures for the read-only telemetry → policy adapter.
//
// Every value here is a FIXTURE value used to make a translation property
// observable; none is a product threshold (PD-D03/PD-D04 remain pending). No
// test reads host telemetry, starts a thread, samples a resource, loads a model
// or touches a process: the adapter is fed explicit `HostResourceSnapshot`s with
// an explicit monotonic clock.

#include "sentinel/core/policy/ResourcePolicy.h"
#include "sentinel/core/policy/ResourcePolicyTelemetryAdapter.h"
#include "sentinel/core/resource/ResourceTelemetry.h"

#include <QDateTime>
#include <QTimeZone>
#include <QtTest>

#include <cmath>
#include <optional>

using sentinel::core::HostResourceSnapshot;
using sentinel::core::ResourceCapabilityReport;
using sentinel::core::ResourceMeasurementOrigin;
using sentinel::core::ResourceMetric;
using sentinel::core::ResourceMetricAvailability;
using sentinel::core::ResourcePolicyConfig;
using sentinel::core::ResourcePolicyEngine;
using sentinel::core::ResourcePolicyInputProvenance;
using sentinel::core::ResourcePolicyInputState;
using sentinel::core::ResourcePolicyPowerSource;
using sentinel::core::ResourcePolicyReason;
using sentinel::core::ResourcePolicyTelemetryAdapter;
using sentinel::core::ResourcePolicyTelemetryAdapterConfig;
using sentinel::core::ResourcePolicyTelemetryMapping;
using sentinel::core::ResourcePowerSource;
using sentinel::core::ResourcePressureState;
using sentinel::core::ResourceReliability;
using sentinel::core::ResourceSampleTimestamp;
using sentinel::core::ResourceScope;
using sentinel::core::ResourceSignal;
using sentinel::core::ResourceTelemetryState;

namespace {

constexpr qint64 kGiB = 1024LL * 1024 * 1024;
constexpr qint64 kMaxAge = 5000;

ResourceSampleTimestamp at(qint64 monotonicMs) {
    ResourceSampleTimestamp stamp;
    stamp.monotonicMs = monotonicMs;
    return stamp;
}

ResourceMetric<qint64> measuredBytes(qint64 value, qint64 capturedMs,
                                     ResourceMeasurementOrigin origin,
                                     ResourceReliability reliability, const QString& sourceId) {
    return sentinel::core::measuredMetric<qint64>(value, at(capturedMs), origin, reliability,
                                                  sourceId);
}

ResourceMetric<double> measuredDouble(double value, qint64 capturedMs,
                                      ResourceMeasurementOrigin origin,
                                      ResourceReliability reliability, const QString& sourceId) {
    return sentinel::core::measuredMetric<double>(value, at(capturedMs), origin, reliability,
                                                  sourceId);
}

// A stale metric preserves its value but is past its freshness bound.
ResourceMetric<double> staleDouble(double value, qint64 capturedMs) {
    ResourceMetric<double> metric = sentinel::core::measuredMetric<double>(
        value, at(capturedMs), ResourceMeasurementOrigin::HostKernel,
        ResourceReliability::Approximate, QStringLiteral("test.pressure.stale"));
    metric.availability = ResourceMetricAvailability::Stale;
    return metric;
}

// Asserts that an optional policy-input value is present and equal to `expected`
// without an unguarded optional access.
template <typename T>
void expectValue(const std::optional<T>& actual, const T& expected, const char* what) {
    QVERIFY2(actual.has_value(), what);
    if (actual.has_value()) {
        QCOMPARE(*actual, expected);
    }
}

QString describe(const ResourcePolicyInputProvenance& provenance) {
    return QStringLiteral(
               "state=%1 availability=%2 present=%3 fresh=%4 applicable=%5 age=%6 origin=%7 "
               "reliability=%8 scope=%9 source=%10 detail=%11 captured=%12")
        .arg(sentinel::core::resourcePolicyInputStateName(provenance.mappedState),
             sentinel::core::resourceMetricAvailabilityName(provenance.availability))
        .arg(provenance.valuePresent)
        .arg(provenance.fresh)
        .arg(provenance.applicable)
        .arg(provenance.ageMs)
        .arg(sentinel::core::resourceMeasurementOriginName(provenance.origin),
             sentinel::core::resourceReliabilityName(provenance.reliability),
             sentinel::core::resourceScopeName(provenance.scope), provenance.sourceId,
             provenance.detail)
        .arg(provenance.capturedAt.monotonicMs);
}

// A stable, non-localized fingerprint of the whole mapping, used to prove
// idempotence.
QString fingerprint(const ResourcePolicyTelemetryMapping& mapping) {
    const auto& d = mapping.diagnostics;
    QStringList parts;
    parts
        << d.adapterId << QString::number(d.mappedAtMonotonicMs)
        << QString::number(d.metricMaxAgeMs) << QString::number(d.hostLogicalCpuCount)
        << QString::number(d.gpuDeviceCount)
        << sentinel::core::resourcePolicyInputStateName(mapping.inputs.cpuBusyFraction.state)
        << sentinel::core::resourcePolicyInputStateName(mapping.inputs.hostAvailableBytes.state)
        << sentinel::core::resourcePolicyInputStateName(mapping.inputs.memoryPressureLevel.state)
        << sentinel::core::resourcePolicyInputStateName(mapping.inputs.thermalLevel.state)
        << sentinel::core::resourcePolicyInputStateName(mapping.inputs.powerSource.state)
        << sentinel::core::resourcePolicyInputStateName(mapping.inputs.batteryPresent.state)
        << sentinel::core::resourcePolicyInputStateName(mapping.inputs.batteryChargeFraction.state)
        << sentinel::core::resourcePolicyInputStateName(mapping.inputs.gpuAvailable.state)
        << sentinel::core::resourcePolicyInputStateName(mapping.inputs.gpuUtilizationFraction.state)
        << sentinel::core::resourcePolicyInputStateName(mapping.inputs.processResidentBytes.state)
        << sentinel::core::resourcePolicyInputStateName(
               mapping.inputs.processCpuFractionOfCore.state);
    for (const auto& provenance :
         {d.cpuBusyFraction, d.hostAvailableBytes, d.memoryPressureLevel, d.thermalLevel,
          d.powerSource, d.batteryPresent, d.batteryChargeFraction, d.gpuAvailable,
          d.gpuUtilizationFraction, d.processResidentBytes, d.processCpuFractionOfCore}) {
        parts << describe(provenance);
    }
    parts << d.notes.join(QLatin1Char(';'));
    return parts.join(QLatin1Char('|'));
}

} // namespace

class ResourcePolicyTelemetryAdapterTest final : public QObject {
    Q_OBJECT
private slots:
    void freshMeasurementsMapToFreshInputs();
    void staleMeasurementsMapToStaleAndAreNotCleared();
    void trustedLabelsAreUsedWhenFreshnessDisabled();
    void unsupportedSignalsAreNotZeroConsumption();
    void missingDataNeverFabricatesHealthyValues();
    void invalidValuesArePreservedForThePolicyToReject();
    void batteryTelemetryKeepsUnitsAndPresence();
    void thermalTelemetryMapsNativeLevelString();
    void memoryPressureKeepsOriginAndReliability();
    void processRssIsBytesAndNeverHostOrGpuMemory();
    void gpuUtilizationIsPerDeviceMaximumNeverASum();
    void gpuAvailabilityUsesCapabilityDeclaration();
    void mixedAvailabilityIsPreservedPerSignal();
    void timestampBoundariesFollowFreshnessRule();
    void provenanceIsPreservedInDiagnostics();
    void repeatedIdenticalInputsAreDeterministic();
    void mappingDoesNotActivateAnUncalibratedPolicy();
    void unavailableSignalsAreNotADegradation();
};

// 1. Fresh measurements map to `Measured` inputs with their real values.
void ResourcePolicyTelemetryAdapterTest::freshMeasurementsMapToFreshInputs() {
    HostResourceSnapshot snapshot;
    snapshot.adapterId = QStringLiteral("test.fresh");
    snapshot.capturedAt = at(1000);
    snapshot.process.residentBytes =
        measuredBytes(256LL * 1024 * 1024, 1000, ResourceMeasurementOrigin::HostKernel,
                      ResourceReliability::Authoritative, QStringLiteral("test.rss"));
    snapshot.hostMemory.availableBytes =
        measuredBytes(4 * kGiB, 1000, ResourceMeasurementOrigin::HostKernel,
                      ResourceReliability::Authoritative, QStringLiteral("test.avail"));
    snapshot.hostMemory.pressureLevel =
        measuredDouble(0.25, 1000, ResourceMeasurementOrigin::HostKernel,
                       ResourceReliability::Approximate, QStringLiteral("test.pressure"));
    snapshot.thermal.level = sentinel::core::measuredMetric<QString>(
        QStringLiteral("nominal"), at(1000), ResourceMeasurementOrigin::PlatformService,
        ResourceReliability::Authoritative, QStringLiteral("test.thermal"));

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 1000);

    QCOMPARE(mapping.inputs.processResidentBytes.state, ResourcePolicyInputState::Measured);
    expectValue(mapping.inputs.processResidentBytes.value, 256LL * 1024 * 1024, "rss bytes");
    QCOMPARE(mapping.inputs.hostAvailableBytes.state, ResourcePolicyInputState::Measured);
    expectValue(mapping.inputs.hostAvailableBytes.value, 4 * kGiB, "available bytes");
    QCOMPARE(mapping.inputs.memoryPressureLevel.state, ResourcePolicyInputState::Measured);
    QCOMPARE(mapping.inputs.thermalLevel.state, ResourcePolicyInputState::Measured);
    expectValue(mapping.inputs.thermalLevel.value, QStringLiteral("nominal"), "thermal level");
    QVERIFY(mapping.diagnostics.hostAvailableBytes.fresh);
    QCOMPARE(mapping.diagnostics.hostAvailableBytes.ageMs, 0);
}

// 2. A measurement past the freshness bound becomes `Stale`, never cleared and
// never promoted to fresh.
void ResourcePolicyTelemetryAdapterTest::staleMeasurementsMapToStaleAndAreNotCleared() {
    HostResourceSnapshot snapshot;
    snapshot.hostMemory.availableBytes =
        measuredBytes(3 * kGiB, 1000, ResourceMeasurementOrigin::HostKernel,
                      ResourceReliability::Authoritative, QStringLiteral("test.avail"));
    snapshot.hostMemory.pressureLevel =
        measuredDouble(0.9, 1000, ResourceMeasurementOrigin::HostKernel,
                       ResourceReliability::Approximate, QStringLiteral("test.pressure"));

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 1000 + kMaxAge + 1);

    QCOMPARE(mapping.inputs.hostAvailableBytes.state, ResourcePolicyInputState::Stale);
    expectValue(mapping.inputs.hostAvailableBytes.value, 3 * kGiB, "stale value preserved");
    QVERIFY(mapping.inputs.hostAvailableBytes.usable());
    QVERIFY(!mapping.inputs.hostAvailableBytes.fresh());
    QCOMPARE(mapping.diagnostics.hostAvailableBytes.availability,
             ResourceMetricAvailability::Stale);
    QVERIFY(!mapping.diagnostics.hostAvailableBytes.fresh);
    QCOMPARE(mapping.diagnostics.hostAvailableBytes.ageMs, kMaxAge + 1);
}

// 3. With freshness disabled the adapter trusts the snapshot's labels verbatim.
void ResourcePolicyTelemetryAdapterTest::trustedLabelsAreUsedWhenFreshnessDisabled() {
    HostResourceSnapshot snapshot;
    snapshot.hostMemory.availableBytes =
        measuredBytes(2 * kGiB, 1000, ResourceMeasurementOrigin::HostKernel,
                      ResourceReliability::Authoritative, QStringLiteral("test.avail"));

    ResourcePolicyTelemetryAdapterConfig config;
    config.applyFreshness = false;
    const ResourcePolicyTelemetryAdapter adapter(config);
    const auto mapping = adapter.adapt(snapshot, 1000 + kMaxAge + 10'000);

    QCOMPARE(mapping.inputs.hostAvailableBytes.state, ResourcePolicyInputState::Measured);
    QVERIFY(mapping.inputs.hostAvailableBytes.fresh());
}

// 4. Unsupported is explicit: no value, inapplicable, never zero consumption.
void ResourcePolicyTelemetryAdapterTest::unsupportedSignalsAreNotZeroConsumption() {
    HostResourceSnapshot snapshot;
    snapshot.hostMemory.availableBytes = sentinel::core::unsupportedMetric<qint64>(
        QStringLiteral("test.avail.unsupported"), QStringLiteral("No memory API."));
    snapshot.process.residentBytes = sentinel::core::unsupportedMetric<qint64>(
        QStringLiteral("test.rss.unsupported"), QStringLiteral("No RSS API."));
    sentinel::core::GpuResourceMetrics gpu;
    gpu.deviceId = QStringLiteral("gpu0");
    gpu.utilizationFraction = sentinel::core::unsupportedMetric<double>(
        QStringLiteral("test.gpu.unsupported"), QStringLiteral("No GPU API."));
    snapshot.gpus.append(gpu);

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 1000);

    QCOMPARE(mapping.inputs.hostAvailableBytes.state, ResourcePolicyInputState::Unsupported);
    QVERIFY(!mapping.inputs.hostAvailableBytes.value.has_value());
    QVERIFY(!mapping.inputs.hostAvailableBytes.applicable());
    QCOMPARE(mapping.inputs.processResidentBytes.state, ResourcePolicyInputState::Unsupported);
    QVERIFY(!mapping.inputs.processResidentBytes.value.has_value());
    QCOMPARE(mapping.inputs.gpuUtilizationFraction.state, ResourcePolicyInputState::Unsupported);
    QVERIFY(!mapping.inputs.gpuUtilizationFraction.value.has_value());
    // Never coerced to a zero reading.
    QVERIFY(!(mapping.inputs.hostAvailableBytes.value.value_or(-1) == 0));
    QVERIFY(!(mapping.inputs.processResidentBytes.value.value_or(-1) == 0));
}

// 5. Missing data is never turned into a healthy value or a zero.
void ResourcePolicyTelemetryAdapterTest::missingDataNeverFabricatesHealthyValues() {
    HostResourceSnapshot snapshot; // every metric default-constructed: Unknown
    snapshot.adapterId = QStringLiteral("test.empty");

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 1000);

    QCOMPARE(mapping.inputs.cpuBusyFraction.state, ResourcePolicyInputState::Unknown);
    QVERIFY(!mapping.inputs.cpuBusyFraction.value.has_value());
    QCOMPARE(mapping.inputs.hostAvailableBytes.state, ResourcePolicyInputState::Unknown);
    QCOMPARE(mapping.inputs.memoryPressureLevel.state, ResourcePolicyInputState::Unknown);
    QCOMPARE(mapping.inputs.processResidentBytes.state, ResourcePolicyInputState::Unknown);
    QCOMPARE(mapping.inputs.batteryPresent.state, ResourcePolicyInputState::Unknown);
    // No policy input carries a fabricated value.
    QVERIFY(!mapping.inputs.hostAvailableBytes.value.has_value());
    QVERIFY(!mapping.inputs.memoryPressureLevel.value.has_value());
    QVERIFY(!mapping.inputs.processResidentBytes.value.has_value());
}

// 6. Invalid values (NaN) are preserved for the policy to reject; the adapter
// neither sanitizes them into a healthy value nor into zero.
void ResourcePolicyTelemetryAdapterTest::invalidValuesArePreservedForThePolicyToReject() {
    HostResourceSnapshot snapshot;
    snapshot.hostMemory.pressureLevel =
        measuredDouble(std::nan(""), 1000, ResourceMeasurementOrigin::HostKernel,
                       ResourceReliability::Approximate, QStringLiteral("test.pressure.nan"));

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 1000);

    QCOMPARE(mapping.inputs.memoryPressureLevel.state, ResourcePolicyInputState::Measured);
    QVERIFY(mapping.inputs.memoryPressureLevel.value.has_value());
    if (mapping.inputs.memoryPressureLevel.value.has_value()) {
        QVERIFY(std::isnan(*mapping.inputs.memoryPressureLevel.value));
    }

    // The policy rejects the impossible fraction as unusable evidence: it cannot
    // escalate (no `Elevated`) and degrades the telemetry verdict.
    ResourcePolicyConfig config;
    config.thresholds.memoryPressureEscalateAt = 0.80;
    config.thresholds.memoryPressureDeescalateAt = 0.50;
    ResourcePolicyEngine engine(config);
    const auto decision = engine.evaluate(mapping.inputs, 1000);
    QCOMPARE(decision.telemetry, ResourceTelemetryState::Degraded);
    QVERIFY(decision.degraded);
    QCOMPARE(decision.state, ResourcePressureState::Normal);
}

// 7. Battery telemetry keeps its units, presence and source conversion.
void ResourcePolicyTelemetryAdapterTest::batteryTelemetryKeepsUnitsAndPresence() {
    HostResourceSnapshot snapshot;
    snapshot.power.powerSource = sentinel::core::measuredMetric<ResourcePowerSource>(
        ResourcePowerSource::Battery, at(1000), ResourceMeasurementOrigin::PlatformService,
        ResourceReliability::Authoritative, QStringLiteral("test.power"));
    snapshot.power.batteryPresent = sentinel::core::measuredMetric<bool>(
        true, at(1000), ResourceMeasurementOrigin::PlatformService,
        ResourceReliability::Authoritative, QStringLiteral("test.battery.present"));
    snapshot.power.batteryChargeFraction =
        measuredDouble(0.42, 1000, ResourceMeasurementOrigin::PlatformService,
                       ResourceReliability::Authoritative, QStringLiteral("test.battery.charge"));

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 1000);

    QCOMPARE(mapping.inputs.powerSource.state, ResourcePolicyInputState::Measured);
    expectValue(mapping.inputs.powerSource.value, ResourcePolicyPowerSource::Battery,
                "power source");
    expectValue(mapping.inputs.batteryPresent.value, true, "battery present");
    expectValue(mapping.inputs.batteryChargeFraction.value, 0.42, "battery charge");
    QCOMPARE(mapping.diagnostics.powerSource.origin, ResourceMeasurementOrigin::PlatformService);
    QCOMPARE(mapping.diagnostics.batteryChargeFraction.sourceId,
             QStringLiteral("test.battery.charge"));
}

// 8. Thermal telemetry is a native level string, never a fabricated score.
void ResourcePolicyTelemetryAdapterTest::thermalTelemetryMapsNativeLevelString() {
    HostResourceSnapshot snapshot;
    snapshot.thermal.level = sentinel::core::measuredMetric<QString>(
        QStringLiteral("serious"), at(2000), ResourceMeasurementOrigin::PlatformService,
        ResourceReliability::Authoritative, QStringLiteral("test.thermal.level"));

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 2000);

    QCOMPARE(mapping.inputs.thermalLevel.state, ResourcePolicyInputState::Measured);
    expectValue(mapping.inputs.thermalLevel.value, QStringLiteral("serious"), "thermal level");
    QCOMPARE(mapping.diagnostics.thermalLevel.sourceId, QStringLiteral("test.thermal.level"));
    QCOMPARE(mapping.diagnostics.thermalLevel.scope, ResourceScope::Host);
}

// 9. Memory pressure keeps its origin and reliability so an Approximate proxy is
// never presented as authoritative.
void ResourcePolicyTelemetryAdapterTest::memoryPressureKeepsOriginAndReliability() {
    HostResourceSnapshot snapshot;
    snapshot.hostMemory.pressureLevel =
        measuredDouble(0.6, 3000, ResourceMeasurementOrigin::HostKernel,
                       ResourceReliability::Approximate, QStringLiteral("test.pressure.proxy"));

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 3000);

    expectValue(mapping.inputs.memoryPressureLevel.value, 0.6, "memory pressure");
    QCOMPARE(mapping.diagnostics.memoryPressureLevel.origin, ResourceMeasurementOrigin::HostKernel);
    QCOMPARE(mapping.diagnostics.memoryPressureLevel.reliability, ResourceReliability::Approximate);
    QCOMPARE(mapping.diagnostics.memoryPressureLevel.sourceId,
             QStringLiteral("test.pressure.proxy"));
}

// 10. Process RSS is bytes, scoped to Sentinel, and never confused with host
// memory or GPU VRAM.
void ResourcePolicyTelemetryAdapterTest::processRssIsBytesAndNeverHostOrGpuMemory() {
    HostResourceSnapshot snapshot;
    snapshot.process.residentBytes =
        measuredBytes(512LL * 1024 * 1024, 1000, ResourceMeasurementOrigin::SentinelProcess,
                      ResourceReliability::Authoritative, QStringLiteral("test.rss"));
    snapshot.hostMemory.availableBytes = sentinel::core::unsupportedMetric<qint64>(
        QStringLiteral("test.avail.unsupported"), QStringLiteral("No memory API."));
    sentinel::core::GpuResourceMetrics gpu;
    gpu.deviceId = QStringLiteral("gpu0");
    gpu.vramUsedBytes =
        measuredBytes(8 * kGiB, 1000, ResourceMeasurementOrigin::PlatformService,
                      ResourceReliability::Authoritative, QStringLiteral("test.vram"));
    snapshot.gpus.append(gpu);

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 1000);

    expectValue(mapping.inputs.processResidentBytes.value, 512LL * 1024 * 1024, "rss bytes");
    QCOMPARE(mapping.diagnostics.processResidentBytes.scope, ResourceScope::SentinelProcess);
    // Host memory stayed Unsupported: the GPU VRAM was not folded into it.
    QCOMPARE(mapping.inputs.hostAvailableBytes.state, ResourcePolicyInputState::Unsupported);
    QVERIFY(!mapping.inputs.hostAvailableBytes.value.has_value());
    // The RSS value is neither host memory nor the 8 GiB VRAM figure.
    QVERIFY(mapping.inputs.processResidentBytes.value.has_value());
    if (mapping.inputs.processResidentBytes.value.has_value()) {
        QVERIFY(*mapping.inputs.processResidentBytes.value != 8 * kGiB);
    }
}

// 11. GPU utilization is the per-device maximum, never a sum, and a GPU with no
// utilization signal is not zero.
void ResourcePolicyTelemetryAdapterTest::gpuUtilizationIsPerDeviceMaximumNeverASum() {
    HostResourceSnapshot snapshot;
    sentinel::core::GpuResourceMetrics first;
    first.deviceId = QStringLiteral("gpu0");
    first.utilizationFraction =
        measuredDouble(0.30, 1000, ResourceMeasurementOrigin::PlatformService,
                       ResourceReliability::Authoritative, QStringLiteral("test.gpu0"));
    sentinel::core::GpuResourceMetrics second;
    second.deviceId = QStringLiteral("gpu1");
    second.utilizationFraction =
        measuredDouble(0.75, 1000, ResourceMeasurementOrigin::PlatformService,
                       ResourceReliability::Authoritative, QStringLiteral("test.gpu1"));
    snapshot.gpus.append(first);
    snapshot.gpus.append(second);

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 1000);

    QCOMPARE(mapping.inputs.gpuUtilizationFraction.state, ResourcePolicyInputState::Measured);
    expectValue(mapping.inputs.gpuUtilizationFraction.value, 0.75, "gpu max, not sum");
    expectValue(mapping.inputs.gpuAvailable.value, true, "gpu available");
    QCOMPARE(mapping.diagnostics.gpuDeviceCount, 2);
    QCOMPARE(mapping.diagnostics.gpuUtilizationFraction.sourceId, QStringLiteral("test.gpu1"));
}

// 12. With no device, GPU availability is classified from the static capability
// declaration rather than guessed.
void ResourcePolicyTelemetryAdapterTest::gpuAvailabilityUsesCapabilityDeclaration() {
    const ResourcePolicyTelemetryAdapter adapter;

    struct Case {
        ResourceMetricAvailability baseline;
        ResourcePolicyInputState expectedAvailable;
        ResourcePolicyInputState expectedUtilization;
    };
    const Case cases[] = {
        {ResourceMetricAvailability::Measured, ResourcePolicyInputState::Measured,
         ResourcePolicyInputState::Unavailable},
        {ResourceMetricAvailability::Unavailable, ResourcePolicyInputState::Unavailable,
         ResourcePolicyInputState::Unavailable},
        {ResourceMetricAvailability::Unsupported, ResourcePolicyInputState::Unsupported,
         ResourcePolicyInputState::Unsupported},
    };

    for (const auto& testCase : cases) {
        ResourceCapabilityReport capabilities;
        capabilities.entries.append({ResourceSignal::GpuUtilization, testCase.baseline});
        HostResourceSnapshot snapshot; // no GPU entries
        const auto mapping = adapter.adapt(snapshot, 1000);
        QCOMPARE(mapping.inputs.gpuAvailable.state, ResourcePolicyInputState::Unsupported);
        const auto withCapabilities = adapter.adapt(snapshot, capabilities, 1000);
        QCOMPARE(withCapabilities.inputs.gpuAvailable.state, testCase.expectedAvailable);
        QCOMPARE(withCapabilities.inputs.gpuUtilizationFraction.state,
                 testCase.expectedUtilization);
    }
}

// 13. Mixed availability maps each signal independently.
void ResourcePolicyTelemetryAdapterTest::mixedAvailabilityIsPreservedPerSignal() {
    HostResourceSnapshot snapshot;
    snapshot.hostMemory.availableBytes =
        measuredBytes(5 * kGiB, 1000, ResourceMeasurementOrigin::HostKernel,
                      ResourceReliability::Authoritative, QStringLiteral("test.avail"));
    snapshot.hostMemory.pressureLevel = staleDouble(0.7, 1000);
    snapshot.thermal.level = sentinel::core::unsupportedMetric<QString>(
        QStringLiteral("test.thermal"), QStringLiteral("No thermal API."));
    snapshot.power.powerSource = sentinel::core::unavailableMetric<ResourcePowerSource>(
        QStringLiteral("test.power"), QStringLiteral("No battery."));
    snapshot.power.batteryChargeFraction = ResourceMetric<double>{}; // Unknown
    snapshot.process.residentBytes =
        measuredBytes(128LL * 1024 * 1024, 1000, ResourceMeasurementOrigin::SentinelProcess,
                      ResourceReliability::Authoritative, QStringLiteral("test.rss"));

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 1000);

    QCOMPARE(mapping.inputs.hostAvailableBytes.state, ResourcePolicyInputState::Measured);
    QCOMPARE(mapping.inputs.memoryPressureLevel.state, ResourcePolicyInputState::Stale);
    QCOMPARE(mapping.inputs.thermalLevel.state, ResourcePolicyInputState::Unsupported);
    QCOMPARE(mapping.inputs.powerSource.state, ResourcePolicyInputState::Unavailable);
    QCOMPARE(mapping.inputs.batteryChargeFraction.state, ResourcePolicyInputState::Unknown);
    QCOMPARE(mapping.inputs.processResidentBytes.state, ResourcePolicyInputState::Measured);
    QVERIFY(!mapping.inputs.thermalLevel.applicable());
    QVERIFY(!mapping.inputs.powerSource.applicable());
}

// 14. Timestamp boundaries: age == max age is fresh; one millisecond beyond is
// stale; a measured value with no timestamp becomes Unknown.
void ResourcePolicyTelemetryAdapterTest::timestampBoundariesFollowFreshnessRule() {
    const ResourcePolicyTelemetryAdapter adapter;
    HostResourceSnapshot snapshot;
    snapshot.hostMemory.availableBytes =
        measuredBytes(1 * kGiB, 1000, ResourceMeasurementOrigin::HostKernel,
                      ResourceReliability::Authoritative, QStringLiteral("test.avail"));

    QCOMPARE(adapter.adapt(snapshot, 1000).inputs.hostAvailableBytes.state,
             ResourcePolicyInputState::Measured); // age 0
    QCOMPARE(adapter.adapt(snapshot, 1000 + kMaxAge).inputs.hostAvailableBytes.state,
             ResourcePolicyInputState::Measured); // age == max age, still fresh
    QCOMPARE(adapter.adapt(snapshot, 1000 + kMaxAge + 1).inputs.hostAvailableBytes.state,
             ResourcePolicyInputState::Stale); // age one past

    HostResourceSnapshot untimed;
    untimed.hostMemory.availableBytes = sentinel::core::measuredMetric<qint64>(
        1 * kGiB, ResourceSampleTimestamp{}, ResourceMeasurementOrigin::HostKernel,
        ResourceReliability::Authoritative, QStringLiteral("test.avail.notime"));
    const auto untimedMapping = adapter.adapt(untimed, 1000);
    QCOMPARE(untimedMapping.inputs.hostAvailableBytes.state, ResourcePolicyInputState::Unknown);
    QVERIFY(!untimedMapping.inputs.hostAvailableBytes.value.has_value());
    QCOMPARE(untimedMapping.diagnostics.hostAvailableBytes.ageMs, -1);
}

// 15. Provenance the policy input cannot carry is preserved on the side channel.
void ResourcePolicyTelemetryAdapterTest::provenanceIsPreservedInDiagnostics() {
    ResourceSampleTimestamp wall;
    wall.monotonicMs = 4000;
    wall.wallClockUtc = QDateTime::fromSecsSinceEpoch(1'700'000'000, QTimeZone::UTC);
    HostResourceSnapshot snapshot;
    snapshot.adapterId = QStringLiteral("test.native");
    snapshot.hostLogicalCpuCount = 12;
    snapshot.hostMemory.availableBytes = sentinel::core::measuredMetric<qint64>(
        6 * kGiB, wall, ResourceMeasurementOrigin::HostKernel, ResourceReliability::Approximate,
        QStringLiteral("test.host.memory"));

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 4000);
    const auto& provenance = mapping.diagnostics.hostAvailableBytes;

    QCOMPARE(mapping.diagnostics.adapterId, QStringLiteral("test.native"));
    QCOMPARE(mapping.diagnostics.hostLogicalCpuCount, 12);
    QCOMPARE(provenance.origin, ResourceMeasurementOrigin::HostKernel);
    QCOMPARE(provenance.reliability, ResourceReliability::Approximate);
    QCOMPARE(provenance.scope, ResourceScope::Host);
    QCOMPARE(provenance.sourceId, QStringLiteral("test.host.memory"));
    QCOMPARE(provenance.capturedAt.monotonicMs, 4000);
    QCOMPARE(provenance.capturedAt.wallClockUtc, wall.wallClockUtc);
    QVERIFY(provenance.valuePresent);
    QVERIFY(provenance.fresh);
    // The policy input itself keeps only state + value.
    QCOMPARE(mapping.inputs.hostAvailableBytes.state, ResourcePolicyInputState::Measured);
    expectValue(mapping.inputs.hostAvailableBytes.value, 6 * kGiB, "available bytes");
}

// 16. The same snapshot and clock produce a byte-identical mapping.
void ResourcePolicyTelemetryAdapterTest::repeatedIdenticalInputsAreDeterministic() {
    HostResourceSnapshot snapshot;
    snapshot.adapterId = QStringLiteral("test.repeat");
    snapshot.process.residentBytes =
        measuredBytes(300LL * 1024 * 1024, 1000, ResourceMeasurementOrigin::SentinelProcess,
                      ResourceReliability::Authoritative, QStringLiteral("test.rss"));
    snapshot.hostMemory.availableBytes =
        measuredBytes(2 * kGiB, 1000, ResourceMeasurementOrigin::HostKernel,
                      ResourceReliability::Authoritative, QStringLiteral("test.avail"));
    snapshot.thermal.level = sentinel::core::measuredMetric<QString>(
        QStringLiteral("fair"), at(1000), ResourceMeasurementOrigin::PlatformService,
        ResourceReliability::Authoritative, QStringLiteral("test.thermal"));

    const ResourcePolicyTelemetryAdapter adapter;
    const auto first = adapter.adapt(snapshot, 1000);
    const auto second = adapter.adapt(snapshot, 1000);

    QCOMPARE(fingerprint(first), fingerprint(second));
    // The source snapshot is never mutated by the translation.
    QCOMPARE(snapshot.hostMemory.availableBytes.availability, ResourceMetricAvailability::Measured);
}

// 17. Feeding a fresh, pressure-looking mapping to the production-default engine
// does not activate any policy: it stays uncalibrated and inert.
void ResourcePolicyTelemetryAdapterTest::mappingDoesNotActivateAnUncalibratedPolicy() {
    HostResourceSnapshot snapshot;
    snapshot.process.residentBytes =
        measuredBytes(8 * kGiB, 1000, ResourceMeasurementOrigin::SentinelProcess,
                      ResourceReliability::Authoritative, QStringLiteral("test.rss"));
    snapshot.hostMemory.availableBytes =
        measuredBytes(16, 1000, ResourceMeasurementOrigin::HostKernel,
                      ResourceReliability::Authoritative, QStringLiteral("test.avail"));
    snapshot.hostMemory.pressureLevel =
        measuredDouble(1.0, 1000, ResourceMeasurementOrigin::HostKernel,
                       ResourceReliability::Authoritative, QStringLiteral("test.pressure"));

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 1000);

    ResourcePolicyEngine engine; // production default: uncalibrated
    const auto decision = engine.evaluate(mapping.inputs, 1000);

    QVERIFY(!decision.thresholdsCalibrated);
    QCOMPARE(decision.state, ResourcePressureState::Unknown);
    QCOMPARE(decision.primaryReason, ResourcePolicyReason::ThresholdsNotCalibrated);
    QCOMPARE(decision.actions.size(), 1);
    QCOMPARE(decision.actions.first(), sentinel::core::ResourcePolicyAction::None);
    QVERIFY(!decision.refuseNewAdmission);
    QVERIFY(!decision.restrictBackgroundAdmission);
    QVERIFY(decision.foregroundAdmissionAllowed);
    QVERIFY(!decision.toolConcurrencyLimit.has_value());
    QVERIFY(!decision.backgroundConcurrencyLimit.has_value());
    QCOMPARE(decision.ownershipPosture,
             sentinel::core::ResourceOwnershipPosture::PreserveAllResidency);
}

// 18. A host whose only supplied signals are Unavailable/Unsupported contributes
// no evidence and is not treated as a degradation that could hold a restriction.
void ResourcePolicyTelemetryAdapterTest::unavailableSignalsAreNotADegradation() {
    HostResourceSnapshot snapshot;
    snapshot.power.powerSource = sentinel::core::unavailableMetric<ResourcePowerSource>(
        QStringLiteral("test.power"), QStringLiteral("No battery."));
    snapshot.thermal.level = sentinel::core::unsupportedMetric<QString>(
        QStringLiteral("test.thermal"), QStringLiteral("No thermal API."));

    const ResourcePolicyTelemetryAdapter adapter;
    const auto mapping = adapter.adapt(snapshot, 1000);

    ResourcePolicyConfig config;
    config.thresholds.thermalWarningLevels = {QStringLiteral("critical")};
    config.thresholds.escalateOnBattery = true;
    ResourcePolicyEngine engine(config);
    const auto decision = engine.evaluate(mapping.inputs, 1000);

    // Both watched signals are inapplicable: no verdict, no escalation.
    QCOMPARE(decision.primaryReason, ResourcePolicyReason::NoWatchedSignals);
    QCOMPARE(decision.state, ResourcePressureState::Unknown);
    QVERIFY(!decision.refuseNewAdmission);
}

QTEST_MAIN(ResourcePolicyTelemetryAdapterTest)

#include "test_resource_policy_adapter.moc"
