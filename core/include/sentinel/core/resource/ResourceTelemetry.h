// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// P4-02 native resource telemetry contracts.
//
// This is a measurement contract, deliberately separate from the *authorization*
// contract `ResourceAuthorizationSnapshot` (runtime/ToolInvocationPlan.h). A
// reading is never a grant and never substitutes for one.
//
// Truth rules enforced by these types:
//   * A metric that cannot be sampled is never encoded as zero.
//   * `Unknown` (cannot sample now), `Unavailable` (supported but currently
//     absent), `Unsupported` (platform cannot ever provide it) and `Stale`
//     (last measured value is past its freshness bound) stay distinct.
//   * Host-wide metrics and Sentinel-process metrics are separate structures.
//   * GPU VRAM is never substituted for system RAM.
//   * Every measured value carries a timestamp, an origin and a reliability.

#include <QDateTime>
#include <QList>
#include <QString>
#include <optional>

namespace sentinel::core {

// Availability of a single metric value. The enum is ordered from "no truthful
// answer at all" to "current measurement" so that callers can rank fallbacks.
enum class ResourceMetricAvailability {
    // The adapter does not know whether it can sample the metric yet.
    Unknown,
    // A value was sampled and is within its freshness bound.
    Measured,
    // A value was sampled but is older than the configured maximum age. The
    // value is preserved so callers can degrade coherently instead of to zero.
    Stale,
    // The platform supports the metric but it is currently absent (for example
    // a desktop host with no battery).
    Unavailable,
    // The platform or adapter cannot ever provide this metric.
    Unsupported,
};

// Where a measurement came from. Origin is descriptive, not a trust level.
enum class ResourceMeasurementOrigin {
    Unknown,
    // Measured from within the Sentinel process about itself (self-scoped).
    SentinelProcess,
    // Read from host-wide kernel counters or syscalls (host-scoped).
    HostKernel,
    // Read from an operating-system platform service (power, thermal).
    PlatformService,
    // Reported by an external runtime we do not own (for example Ollama).
    ExternalProvider,
    // Produced by an injected test fixture.
    Synthetic,
};

// How much confidence the adapter has in the value, never a substitute for
// authorisation. `Authoritative` means directly measured; `Approximate` means
// derived or subject to platform caveats (for example a memory-load proxy).
enum class ResourceReliability {
    Unknown,
    Authoritative,
    Approximate,
    Unreliable,
};

// Distinguishes host-wide capacity from Sentinel-process consumption. Callers
// must never fold one scope into the other.
enum class ResourceScope {
    Unknown,
    SentinelProcess,
    Host,
    ExternalRuntime,
};

// Physical power source. `Unknown` is a real state, not "AC" by default.
enum class ResourcePowerSource {
    Unknown,
    Ac,
    Battery,
};

// A capture timestamp for one metric. Monotonic milliseconds are the freshness
// authority; wall clock is best-effort metadata for presentation only.
struct ResourceSampleTimestamp {
    // Milliseconds on a monotonic clock (never a wall clock). Zero means the
    // adapter did not stamp the sample and the value cannot be trusted as fresh.
    qint64 monotonicMs = 0;
    // Best-effort wall clock time; may be invalid when unavailable.
    QDateTime wallClockUtc;
};

// A single measured-or-not metric. The value is optional and must never be
// fabricated. Availability is authoritative over the value's presence.
template <typename T> struct ResourceMetric {
    ResourceMetricAvailability availability = ResourceMetricAvailability::Unknown;
    std::optional<T> value;
    ResourceSampleTimestamp capturedAt;
    ResourceMeasurementOrigin origin = ResourceMeasurementOrigin::Unknown;
    ResourceReliability reliability = ResourceReliability::Unknown;
    // Stable machine-readable probe identity, e.g. "macos.host_statistics64".
    QString sourceId;
    // Human-readable explanation for unknown/unavailable/unsupported states.
    QString detail;

    // True when a value exists and is either fresh or stale. Stale values are
    // usable but must be treated as degraded by any consumer.
    bool hasValue() const {
        return value.has_value() && (availability == ResourceMetricAvailability::Measured ||
                                     availability == ResourceMetricAvailability::Stale);
    }
    // True only for a value within its freshness bound.
    bool isFresh() const {
        return value.has_value() && availability == ResourceMetricAvailability::Measured;
    }
};

// True when a value exists and may be consumed (fresh or stale).
inline bool resourceMetricIsUsable(ResourceMetricAvailability availability) {
    return availability == ResourceMetricAvailability::Measured ||
           availability == ResourceMetricAvailability::Stale;
}

// Names for diagnostics and test messages. Never localized.
QString resourceMetricAvailabilityName(ResourceMetricAvailability availability);
QString resourceMeasurementOriginName(ResourceMeasurementOrigin origin);
QString resourceReliabilityName(ResourceReliability reliability);
QString resourceScopeName(ResourceScope scope);
QString resourcePowerSourceName(ResourcePowerSource source);

// ---------------------------------------------------------------------------
// Metric factories. These encode the truth rules at construction time so no
// adapter has to remember them.
// ---------------------------------------------------------------------------

template <typename T>
ResourceMetric<T> measuredMetric(T value, const ResourceSampleTimestamp& capturedAt,
                                 ResourceMeasurementOrigin origin, ResourceReliability reliability,
                                 QString sourceId) {
    ResourceMetric<T> metric;
    metric.availability = ResourceMetricAvailability::Measured;
    metric.value = std::move(value);
    metric.capturedAt = capturedAt;
    metric.origin = origin;
    metric.reliability = reliability;
    metric.sourceId = std::move(sourceId);
    return metric;
}

template <typename T>
ResourceMetric<T> unknownMetric(ResourceMeasurementOrigin origin, QString sourceId,
                                QString detail) {
    ResourceMetric<T> metric;
    metric.availability = ResourceMetricAvailability::Unknown;
    metric.origin = origin;
    metric.sourceId = std::move(sourceId);
    metric.detail = std::move(detail);
    return metric;
}

template <typename T> ResourceMetric<T> unavailableMetric(QString sourceId, QString detail) {
    ResourceMetric<T> metric;
    metric.availability = ResourceMetricAvailability::Unavailable;
    metric.sourceId = std::move(sourceId);
    metric.detail = std::move(detail);
    return metric;
}

template <typename T> ResourceMetric<T> unsupportedMetric(QString sourceId, QString detail) {
    ResourceMetric<T> metric;
    metric.availability = ResourceMetricAvailability::Unsupported;
    metric.sourceId = std::move(sourceId);
    metric.detail = std::move(detail);
    return metric;
}

// Reclassifies a `Measured` metric older than `maxAgeMs` as `Stale`, preserving
// the value. A measured metric with no timestamp becomes `Unknown` because it
// cannot be proven fresh. Any other availability is left untouched.
template <typename T>
void applyMetricFreshness(ResourceMetric<T>& metric, qint64 nowMonotonicMs, qint64 maxAgeMs) {
    if (metric.availability != ResourceMetricAvailability::Measured)
        return;
    if (!metric.value.has_value())
        return;
    const qint64 captured = metric.capturedAt.monotonicMs;
    if (captured <= 0) {
        metric.availability = ResourceMetricAvailability::Unknown;
        metric.value.reset();
        return;
    }
    if (nowMonotonicMs - captured > maxAgeMs)
        metric.availability = ResourceMetricAvailability::Stale;
}

// ---------------------------------------------------------------------------
// Snapshot structure. Sentinel-process and host-wide metrics are separate.
// ---------------------------------------------------------------------------

struct ProcessResourceMetrics {
    // Cumulative CPU seconds consumed by the Sentinel process since start.
    ResourceMetric<double> cpuSecondsTotal;
    // Derived average utilization over the sampling interval as a fraction of
    // one logical core (1.0 == one full core).
    ResourceMetric<double> cpuFractionOfCore;
    // Derived average utilization as a fraction of total host capacity (0..1).
    ResourceMetric<double> cpuFractionOfHost;
    // Resident memory in BYTES. Never kilobytes, never a GPU allocation.
    ResourceMetric<qint64> residentBytes;
    // Virtual address space in BYTES.
    ResourceMetric<qint64> virtualBytes;
    ResourceMetric<int> threadCount;
};

struct HostMemoryMetrics {
    ResourceMetric<qint64> totalBytes;
    // Available (reclaimable) memory in BYTES. This is NOT free RAM only and is
    // NOT a GPU value.
    ResourceMetric<qint64> availableBytes;
    // Native memory-pressure indicator normalized to 0..1. The mapping is
    // platform-specific and reported as `Approximate` where derived.
    ResourceMetric<double> pressureLevel;
};

struct PowerResourceMetrics {
    ResourceMetric<bool> batteryPresent;
    ResourceMetric<ResourcePowerSource> powerSource;
    // Charge in 0..1. Unknown when the platform reports an unknown percentage.
    ResourceMetric<double> batteryChargeFraction;
};

struct ThermalResourceMetrics {
    // Native thermal level string (for example "nominal", "fair", "serious",
    // "critical"). Deliberately not a fabricated numeric score.
    ResourceMetric<QString> level;
    // Absolute temperature in degrees Celsius, where a temperature sensor is
    // independently readable.
    ResourceMetric<double> temperatureCelsius;
};

struct GpuResourceMetrics {
    QString deviceId;
    // VRAM is a device-local resource and is never folded into host RAM.
    ResourceMetric<qint64> vramTotalBytes;
    ResourceMetric<qint64> vramUsedBytes;
    ResourceMetric<double> utilizationFraction;
};

// The single read model produced by a monitor adapter and enriched by
// `ResourceMonitor`. Named distinctly from `ResourceAuthorizationSnapshot`.
struct HostResourceSnapshot {
    // Adapter identity, e.g. "macos.native".
    QString adapterId;
    // Batch capture time. Individual metric timestamps may differ.
    ResourceSampleTimestamp capturedAt;
    // Host logical CPU count used for normalization; <= 0 means unknown.
    int hostLogicalCpuCount = 0;
    ProcessResourceMetrics process;
    HostMemoryMetrics hostMemory;
    PowerResourceMetrics power;
    ThermalResourceMetrics thermal;
    QList<GpuResourceMetrics> gpus;
};

// ---------------------------------------------------------------------------
// Capability reporting.
// ---------------------------------------------------------------------------

// Every signal a snapshot can carry. Used for static per-adapter capability
// declarations so platform gaps are explicit rather than silently missing.
enum class ResourceSignal {
    ProcessCpu,
    ProcessResidentMemory,
    ProcessVirtualMemory,
    ProcessThreadCount,
    HostMemoryTotal,
    HostMemoryAvailable,
    HostMemoryPressure,
    BatteryPresence,
    PowerSource,
    BatteryCharge,
    ThermalLevel,
    ThermalTemperature,
    GpuMemory,
    GpuUtilization,
};

QString resourceSignalName(ResourceSignal signal);

struct ResourceSignalSupport {
    ResourceSignal signal = ResourceSignal::ProcessCpu;
    // Baseline capability: `Measured` if the adapter intends to sample it,
    // `Unavailable` if supported but not currently present, `Unsupported` if the
    // platform cannot provide it, `Unknown` if not yet determined.
    ResourceMetricAvailability baseline = ResourceMetricAvailability::Unknown;
};

struct ResourceCapabilityReport {
    QString adapterId;
    QList<ResourceSignalSupport> entries;

    // Returns `Unknown` when the signal has no declared support entry.
    ResourceMetricAvailability supportFor(ResourceSignal signal) const;
};

// ---------------------------------------------------------------------------
// Injectable monotonic clock.
// ---------------------------------------------------------------------------

class IMonotonicClock {
public:
    virtual ~IMonotonicClock() = default;
    // Milliseconds from an arbitrary monotonic epoch. Only differences are
    // meaningful; never mix with wall-clock time.
    virtual qint64 nowMonotonicMs() const = 0;
};

class SteadyMonotonicClock final : public IMonotonicClock {
public:
    qint64 nowMonotonicMs() const override;
};

// ---------------------------------------------------------------------------
// Derived-value helpers (pure functions, unit-testable without a platform).
// ---------------------------------------------------------------------------

struct CpuFractionResult {
    std::optional<double> fractionOfCore;
    std::optional<double> fractionOfHost;
};

// Derives average CPU utilization from a cumulative CPU-seconds counter.
// Returns empty optionals when the inputs cannot yield a truthful value:
//   * missing or negative counter values,
//   * a non-positive elapsed interval,
//   * a counter that went backwards (reset).
// `hostLogicalCpuCount <= 0` yields no host fraction but keeps the core
// fraction, because the core fraction does not depend on the host core count.
CpuFractionResult deriveCpuFractions(std::optional<double> cpuSecondsNow,
                                     std::optional<double> cpuSecondsBefore, qint64 elapsedMs,
                                     int hostLogicalCpuCount);

// Applies freshness reclassification field-by-field to a whole snapshot.
void applySnapshotFreshness(HostResourceSnapshot& snapshot, qint64 nowMonotonicMs, qint64 maxAgeMs);

} // namespace sentinel::core
