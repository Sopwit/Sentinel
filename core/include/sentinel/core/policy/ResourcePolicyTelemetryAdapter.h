// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// P4-04 read-only telemetry → policy-input adapter.
//
// This unit is the missing link between the P4-02 measurement contract
// (`HostResourceSnapshot`) and the corrected P4-03.1 policy-input contract
// (`ResourcePolicyInputs`). It is a *pure translation*: one snapshot in, one
// immutable bundle of policy inputs plus a diagnostics side-channel out.
//
// Structural guarantees (all pinned by tests):
//   * No thread, no timer, no resource sampler, no IO. `adapt` operates on a
//     private copy of the caller's snapshot and never mutates it.
//   * No model unload, no process termination, no permission mutation, no
//     provider/ownership change, no admission enforcement and no automatic
//     profile switching. The translator holds no `ResourcePolicyEngine`.
//   * It never calls `ResourcePolicyEngine::evaluate`. The engine stays inert
//     (uncalibrated) until PD-D03/PD-D04 land; a caller may feed the produced
//     inputs to an engine explicitly, and does so at its own risk.
//   * Availability is mapped truthfully: `Measured`/`Stale`/`Unknown`/
//     `Unavailable`/`Unsupported` are never collapsed and no zero, healthy or
//     default value is fabricated for a missing measurement.
//
// Unit and identity rules (the reason this adapter must not be a "convenience"
// pass-through):
//   * Host-wide memory (`hostMemory.availableBytes`) never becomes process RSS,
//     and process RSS (`process.residentBytes`) never becomes host memory.
//   * GPU VRAM is never folded into host memory or process RSS. Only the
//     device-local utilization fraction is mapped, as a per-device maximum.
//   * Sentinel-process CPU (`process.cpuFractionOfHost` / `cpuFractionOfCore`)
//     is never treated as host-wide CPU. P4-02 has no approved host-wide CPU
//     source, so `cpuBusyFraction` is always `Unknown` here.
//   * `Unsupported`/`Unavailable` signals are excluded from evidence rather than
//     read as zero consumption.

#include "sentinel/core/policy/ResourcePolicy.h"
#include "sentinel/core/resource/ResourceMonitor.h"
#include "sentinel/core/resource/ResourceTelemetry.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {

// Translation knobs. The default max age is the P4-02 transport bound; it is a
// freshness window, not a policy threshold (PD-D04 calibrates policy).
struct ResourcePolicyTelemetryAdapterConfig {
    // A `Measured` metric older than this is reclassified as `Stale` before
    // mapping, preserving the value (the policy then never lets a stale value
    // clear pressure).
    qint64 metricMaxAgeMs = kResourceMonitorDefaultMetricMaxAgeMs;
    // When true (default) freshness is re-applied to the copied snapshot.
    // When false the snapshot's availability labels are taken as-is.
    bool applyFreshness = true;
};

// Provenance for one mapped policy input: everything the policy input drops.
// This is read-only diagnostic data and must never be fed back into policy
// evaluation.
struct ResourcePolicyInputProvenance {
    // The policy-input state the metric was mapped to.
    ResourcePolicyInputState mappedState = ResourcePolicyInputState::Unknown;
    // P4-02 availability before mapping (after freshness re-application).
    ResourceMetricAvailability availability = ResourceMetricAvailability::Unknown;
    // True when a value exists on the input (fresh or stale).
    bool valuePresent = false;
    // True when the mapped input is fresh (`Measured`).
    bool fresh = false;
    // True when the signal applies to this host at all.
    bool applicable = true;
    // Best-effort age of the underlying sample; negative when it cannot be
    // derived (no captured monotonic timestamp).
    qint64 ageMs = -1;
    ResourceMeasurementOrigin origin = ResourceMeasurementOrigin::Unknown;
    ResourceReliability reliability = ResourceReliability::Unknown;
    ResourceScope scope = ResourceScope::Unknown;
    // Stable source identity, for example "macos.host_statistics64".
    QString sourceId;
    // Human-readable diagnostic detail where safe to surface.
    QString detail;
    ResourceSampleTimestamp capturedAt;
};

// The side-channel the policy input cannot carry. It records source identity,
// per-signal provenance, the clock used for freshness and any translation
// caveat, so a caller can explain a degraded reading without re-deriving it.
struct ResourcePolicyTelemetryDiagnostics {
    // Source snapshot identity, e.g. "macos.native".
    QString adapterId;
    // Batch capture time of the source snapshot.
    ResourceSampleTimestamp capturedAt;
    // Monotonic clock the translation used for freshness and ages.
    qint64 mappedAtMonotonicMs = 0;
    qint64 metricMaxAgeMs = 0;
    // Host logical CPU count as reported by the source snapshot; <= 0 unknown.
    int hostLogicalCpuCount = 0;
    // GPU device count reported by the snapshot (entries, not a capacity sum).
    int gpuDeviceCount = 0;

    // One entry per policy input, in `ResourcePolicyInputs` order.
    ResourcePolicyInputProvenance cpuBusyFraction;
    ResourcePolicyInputProvenance hostAvailableBytes;
    ResourcePolicyInputProvenance memoryPressureLevel;
    ResourcePolicyInputProvenance thermalLevel;
    ResourcePolicyInputProvenance powerSource;
    ResourcePolicyInputProvenance batteryPresent;
    ResourcePolicyInputProvenance batteryChargeFraction;
    ResourcePolicyInputProvenance gpuAvailable;
    ResourcePolicyInputProvenance gpuUtilizationFraction;
    ResourcePolicyInputProvenance processResidentBytes;
    ResourcePolicyInputProvenance processCpuFractionOfCore;

    // Stable, non-localized translation caveats (for example a missing
    // host-wide CPU source). Empty when the mapping needed no caveat.
    QStringList notes;
};

// The complete, immutable result of one translation. `inputs` is what a caller
// may hand to `ResourcePolicyEngine::evaluate`; `diagnostics` is for humans.
struct ResourcePolicyTelemetryMapping {
    ResourcePolicyInputs inputs;
    ResourcePolicyTelemetryDiagnostics diagnostics;
};

// Stateless, deterministic translator. Holding no engine, no clock and no
// sampler, it can be constructed freely and shared read-only.
class ResourcePolicyTelemetryAdapter final {
public:
    explicit ResourcePolicyTelemetryAdapter(ResourcePolicyTelemetryAdapterConfig config = {});

    const ResourcePolicyTelemetryAdapterConfig& config() const {
        return config_;
    }

    // Snapshot-only mapping. Because the snapshot alone cannot promise what the
    // platform could *ever* provide, an absent GPU is mapped to `Unsupported`.
    ResourcePolicyTelemetryMapping adapt(const HostResourceSnapshot& snapshot,
                                         qint64 nowMonotonicMs) const;

    // Mapping that also consults the static capability declarations so a signal
    // the snapshot cannot classify (for example an absent GPU) is reported as
    // `Unsupported` (platform cannot provide it) or `Unavailable` (supported but
    // absent) rather than guessed. `capabilities` must outlive the call.
    ResourcePolicyTelemetryMapping adapt(const HostResourceSnapshot& snapshot,
                                         const ResourceCapabilityReport& capabilities,
                                         qint64 nowMonotonicMs) const;

private:
    ResourcePolicyTelemetryAdapterConfig config_;
};

} // namespace sentinel::core
