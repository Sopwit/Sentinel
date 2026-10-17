// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/resource/ResourceTelemetry.h"

#include <QElapsedTimer>

namespace sentinel::core {
namespace {

// A process-local monotonic epoch. QElapsedTimer is documented as monotonic and
// is not affected by wall-clock changes.
const QElapsedTimer& monotonicEpoch() {
    static const QElapsedTimer timer = [] {
        QElapsedTimer value;
        value.start();
        return value;
    }();
    return timer;
}

} // namespace

QString resourceMetricAvailabilityName(ResourceMetricAvailability availability) {
    switch (availability) {
    case ResourceMetricAvailability::Measured:
        return QStringLiteral("measured");
    case ResourceMetricAvailability::Stale:
        return QStringLiteral("stale");
    case ResourceMetricAvailability::Unavailable:
        return QStringLiteral("unavailable");
    case ResourceMetricAvailability::Unsupported:
        return QStringLiteral("unsupported");
    case ResourceMetricAvailability::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString resourceMeasurementOriginName(ResourceMeasurementOrigin origin) {
    switch (origin) {
    case ResourceMeasurementOrigin::SentinelProcess:
        return QStringLiteral("sentinel.process");
    case ResourceMeasurementOrigin::HostKernel:
        return QStringLiteral("host.kernel");
    case ResourceMeasurementOrigin::PlatformService:
        return QStringLiteral("platform.service");
    case ResourceMeasurementOrigin::ExternalProvider:
        return QStringLiteral("external.provider");
    case ResourceMeasurementOrigin::Synthetic:
        return QStringLiteral("synthetic");
    case ResourceMeasurementOrigin::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString resourceReliabilityName(ResourceReliability reliability) {
    switch (reliability) {
    case ResourceReliability::Authoritative:
        return QStringLiteral("authoritative");
    case ResourceReliability::Approximate:
        return QStringLiteral("approximate");
    case ResourceReliability::Unreliable:
        return QStringLiteral("unreliable");
    case ResourceReliability::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString resourceScopeName(ResourceScope scope) {
    switch (scope) {
    case ResourceScope::SentinelProcess:
        return QStringLiteral("sentinel.process");
    case ResourceScope::Host:
        return QStringLiteral("host");
    case ResourceScope::ExternalRuntime:
        return QStringLiteral("external.runtime");
    case ResourceScope::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString resourcePowerSourceName(ResourcePowerSource source) {
    switch (source) {
    case ResourcePowerSource::Ac:
        return QStringLiteral("ac");
    case ResourcePowerSource::Battery:
        return QStringLiteral("battery");
    case ResourcePowerSource::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString resourceSignalName(ResourceSignal signal) {
    switch (signal) {
    case ResourceSignal::ProcessCpu:
        return QStringLiteral("process.cpu");
    case ResourceSignal::ProcessResidentMemory:
        return QStringLiteral("process.resident_memory");
    case ResourceSignal::ProcessVirtualMemory:
        return QStringLiteral("process.virtual_memory");
    case ResourceSignal::ProcessThreadCount:
        return QStringLiteral("process.thread_count");
    case ResourceSignal::HostMemoryTotal:
        return QStringLiteral("host.memory_total");
    case ResourceSignal::HostMemoryAvailable:
        return QStringLiteral("host.memory_available");
    case ResourceSignal::HostMemoryPressure:
        return QStringLiteral("host.memory_pressure");
    case ResourceSignal::BatteryPresence:
        return QStringLiteral("power.battery_presence");
    case ResourceSignal::PowerSource:
        return QStringLiteral("power.source");
    case ResourceSignal::BatteryCharge:
        return QStringLiteral("power.battery_charge");
    case ResourceSignal::ThermalLevel:
        return QStringLiteral("thermal.level");
    case ResourceSignal::ThermalTemperature:
        return QStringLiteral("thermal.temperature");
    case ResourceSignal::GpuMemory:
        return QStringLiteral("gpu.memory");
    case ResourceSignal::GpuUtilization:
        return QStringLiteral("gpu.utilization");
    }
    return QStringLiteral("unknown");
}

ResourceMetricAvailability ResourceCapabilityReport::supportFor(ResourceSignal signal) const {
    for (const auto& entry : entries) {
        if (entry.signal == signal) {
            return entry.baseline;
        }
    }
    return ResourceMetricAvailability::Unknown;
}

qint64 SteadyMonotonicClock::nowMonotonicMs() const {
    return monotonicEpoch().elapsed();
}

CpuFractionResult deriveCpuFractions(std::optional<double> cpuSecondsNow,
                                     std::optional<double> cpuSecondsBefore, qint64 elapsedMs,
                                     int hostLogicalCpuCount) {
    CpuFractionResult result;
    if (!cpuSecondsNow.has_value() || !cpuSecondsBefore.has_value()) {
        return result;
    }
    if (elapsedMs <= 0) {
        return result;
    }
    if (*cpuSecondsNow < 0.0 || *cpuSecondsBefore < 0.0) {
        return result;
    }
    const double deltaSeconds = *cpuSecondsNow - *cpuSecondsBefore;
    // A shrinking cumulative counter means the process restarted or the counter
    // was reset; the delta is not a truthful rate.
    if (deltaSeconds < 0.0) {
        return result;
    }
    const double elapsedSeconds = static_cast<double>(elapsedMs) / 1000.0;
    if (elapsedSeconds <= 0.0) {
        return result;
    }
    result.fractionOfCore = deltaSeconds / elapsedSeconds;
    if (hostLogicalCpuCount > 0) {
        result.fractionOfHost = *result.fractionOfCore / static_cast<double>(hostLogicalCpuCount);
    }
    return result;
}

void applySnapshotFreshness(HostResourceSnapshot& snapshot, qint64 nowMonotonicMs,
                            qint64 maxAgeMs) {
    applyMetricFreshness(snapshot.process.cpuSecondsTotal, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.process.cpuFractionOfCore, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.process.cpuFractionOfHost, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.process.residentBytes, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.process.virtualBytes, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.process.threadCount, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.hostMemory.totalBytes, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.hostMemory.availableBytes, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.hostMemory.pressureLevel, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.power.batteryPresent, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.power.powerSource, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.power.batteryChargeFraction, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.thermal.level, nowMonotonicMs, maxAgeMs);
    applyMetricFreshness(snapshot.thermal.temperatureCelsius, nowMonotonicMs, maxAgeMs);
    for (auto& gpu : snapshot.gpus) {
        applyMetricFreshness(gpu.vramTotalBytes, nowMonotonicMs, maxAgeMs);
        applyMetricFreshness(gpu.vramUsedBytes, nowMonotonicMs, maxAgeMs);
        applyMetricFreshness(gpu.utilizationFraction, nowMonotonicMs, maxAgeMs);
    }
}

} // namespace sentinel::core
