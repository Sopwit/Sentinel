// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/policy/ResourcePolicyTelemetryAdapter.h"

#include <algorithm>
#include <optional>

namespace sentinel::core {

namespace {

// --- Availability mapping -------------------------------------------------

// Provenance for one metric, independent of the value type. `mappedState` is the
// state the metric mapped to (not always `mapState(availability)`: a `Measured`
// label with no value degrades to `Unknown`).
template <typename T>
ResourcePolicyInputProvenance provenanceFrom(const ResourceMetric<T>& metric,
                                             ResourcePolicyInputState mappedState,
                                             ResourceScope scope, qint64 nowMonotonicMs) {
    ResourcePolicyInputProvenance provenance;
    provenance.mappedState = mappedState;
    provenance.availability = metric.availability;
    provenance.fresh = mappedState == ResourcePolicyInputState::Measured;
    provenance.applicable = mappedState != ResourcePolicyInputState::Unavailable &&
                            mappedState != ResourcePolicyInputState::Unsupported;
    provenance.origin = metric.origin;
    provenance.reliability = metric.reliability;
    provenance.scope = scope;
    provenance.sourceId = metric.sourceId;
    provenance.detail = metric.detail;
    provenance.capturedAt = metric.capturedAt;
    if (metric.capturedAt.monotonicMs > 0 && nowMonotonicMs >= metric.capturedAt.monotonicMs) {
        provenance.ageMs = nowMonotonicMs - metric.capturedAt.monotonicMs;
    }
    return provenance;
}

template <typename T> struct MappedPolicyInput {
    ResourcePolicyInput<T> input;
    ResourcePolicyInputProvenance provenance;
};

// Maps one optional-with-availability metric. A `Measured`/`Stale` label without
// a value is a contract violation on the producer side and degrades to
// `Unknown` rather than fabricating a value.
template <typename T>
MappedPolicyInput<T> mapMetric(const ResourceMetric<T>& metric, ResourceScope scope,
                               qint64 nowMonotonicMs) {
    MappedPolicyInput<T> mapped;
    switch (metric.availability) {
    case ResourceMetricAvailability::Measured:
        mapped.input = metric.value.has_value() ? measuredPolicyInput<T>(*metric.value)
                                                : unknownPolicyInput<T>();
        break;
    case ResourceMetricAvailability::Stale:
        mapped.input =
            metric.value.has_value() ? stalePolicyInput<T>(*metric.value) : unknownPolicyInput<T>();
        break;
    case ResourceMetricAvailability::Unavailable:
        mapped.input = inapplicablePolicyInput<T>(ResourcePolicyInputState::Unavailable);
        break;
    case ResourceMetricAvailability::Unsupported:
        mapped.input = inapplicablePolicyInput<T>(ResourcePolicyInputState::Unsupported);
        break;
    case ResourceMetricAvailability::Unknown:
        mapped.input = unknownPolicyInput<T>();
        break;
    }
    mapped.provenance = provenanceFrom(metric, mapped.input.state, scope, nowMonotonicMs);
    mapped.provenance.valuePresent = mapped.input.value.has_value();
    return mapped;
}

// --- Power source ---------------------------------------------------------

ResourcePolicyPowerSource mapPowerSource(ResourcePowerSource source) {
    switch (source) {
    case ResourcePowerSource::Ac:
        return ResourcePolicyPowerSource::Ac;
    case ResourcePowerSource::Battery:
        return ResourcePolicyPowerSource::Battery;
    case ResourcePowerSource::Unknown:
        break;
    }
    return ResourcePolicyPowerSource::Unknown;
}

MappedPolicyInput<ResourcePolicyPowerSource>
mapPowerSourceMetric(const ResourceMetric<ResourcePowerSource>& metric, qint64 nowMonotonicMs) {
    MappedPolicyInput<ResourcePolicyPowerSource> mapped;
    switch (metric.availability) {
    case ResourceMetricAvailability::Measured:
        mapped.input = metric.value.has_value() ? measuredPolicyInput(mapPowerSource(*metric.value))
                                                : unknownPolicyInput<ResourcePolicyPowerSource>();
        break;
    case ResourceMetricAvailability::Stale:
        mapped.input = metric.value.has_value() ? stalePolicyInput(mapPowerSource(*metric.value))
                                                : unknownPolicyInput<ResourcePolicyPowerSource>();
        break;
    case ResourceMetricAvailability::Unavailable:
        mapped.input = inapplicablePolicyInput<ResourcePolicyPowerSource>(
            ResourcePolicyInputState::Unavailable);
        break;
    case ResourceMetricAvailability::Unsupported:
        mapped.input = inapplicablePolicyInput<ResourcePolicyPowerSource>(
            ResourcePolicyInputState::Unsupported);
        break;
    case ResourceMetricAvailability::Unknown:
        mapped.input = unknownPolicyInput<ResourcePolicyPowerSource>();
        break;
    }
    mapped.provenance =
        provenanceFrom(metric, mapped.input.state, ResourceScope::Host, nowMonotonicMs);
    mapped.provenance.valuePresent = mapped.input.value.has_value();
    return mapped;
}

// --- GPU ------------------------------------------------------------------

struct GpuMapping {
    ResourcePolicyInput<bool> available;
    ResourcePolicyInput<double> utilization;
    ResourcePolicyInputProvenance availableProvenance;
    ResourcePolicyInputProvenance utilizationProvenance;
    int deviceCount = 0;
    QStringList notes;
};

// Utilization aggregate for a device whose metric carries no usable value.
ResourcePolicyInput<double> utilizationFromAvailability(ResourceMetricAvailability availability) {
    switch (availability) {
    case ResourceMetricAvailability::Unavailable:
        return inapplicablePolicyInput<double>(ResourcePolicyInputState::Unavailable);
    case ResourceMetricAvailability::Unsupported:
        return inapplicablePolicyInput<double>(ResourcePolicyInputState::Unsupported);
    case ResourceMetricAvailability::Measured:
    case ResourceMetricAvailability::Stale:
        // Supported but no device is present: the device-local value is absent.
        return inapplicablePolicyInput<double>(ResourcePolicyInputState::Unavailable);
    case ResourceMetricAvailability::Unknown:
        break;
    }
    return unknownPolicyInput<double>();
}

// The most conservative single availability across a device's utilization
// metric: Unknown outranks Unavailable outranks Unsupported.
ResourceMetricAvailability aggregateUtilizationAvailability(const QList<GpuResourceMetrics>& gpus) {
    bool anyUnknown = false;
    bool anyUnavailable = false;
    bool anyUnsupported = false;
    for (const auto& device : gpus) {
        switch (device.utilizationFraction.availability) {
        case ResourceMetricAvailability::Unknown:
            anyUnknown = true;
            break;
        case ResourceMetricAvailability::Unavailable:
            anyUnavailable = true;
            break;
        case ResourceMetricAvailability::Unsupported:
            anyUnsupported = true;
            break;
        case ResourceMetricAvailability::Measured:
        case ResourceMetricAvailability::Stale:
            break;
        }
    }
    if (anyUnknown) {
        return ResourceMetricAvailability::Unknown;
    }
    if (anyUnavailable) {
        return ResourceMetricAvailability::Unavailable;
    }
    if (anyUnsupported) {
        return ResourceMetricAvailability::Unsupported;
    }
    return ResourceMetricAvailability::Unknown;
}

// Picks the more definite of two capability declarations: a `Measured`
// declaration outranks `Unavailable`, which outranks `Unsupported`, which
// outranks `Unknown`.
ResourceMetricAvailability combineSupport(ResourceMetricAvailability first,
                                          ResourceMetricAvailability second) {
    const auto rank = [](ResourceMetricAvailability availability) {
        switch (availability) {
        case ResourceMetricAvailability::Measured:
            return 4;
        case ResourceMetricAvailability::Stale:
            return 3;
        case ResourceMetricAvailability::Unavailable:
            return 2;
        case ResourceMetricAvailability::Unsupported:
            return 1;
        case ResourceMetricAvailability::Unknown:
            break;
        }
        return 0;
    };
    return rank(first) >= rank(second) ? first : second;
}

GpuMapping deriveGpu(const HostResourceSnapshot& snapshot,
                     const ResourceCapabilityReport* capabilities, qint64 nowMonotonicMs) {
    GpuMapping gpu;
    gpu.deviceCount = static_cast<int>(snapshot.gpus.size());

    if (gpu.deviceCount > 0) {
        // A device entry exists: the host has a GPU. Utilization is aggregated as
        // the per-device MAXIMUM (never a sum) across usable values.
        gpu.available = measuredPolicyInput<bool>(true);
        const auto& first = snapshot.gpus.first();
        gpu.availableProvenance = provenanceFrom(first.utilizationFraction, gpu.available.state,
                                                 ResourceScope::Host, nowMonotonicMs);
        gpu.availableProvenance.valuePresent = true;
        gpu.availableProvenance.detail = QStringLiteral("GPU device present in snapshot.");

        std::optional<double> best;
        const ResourceMetric<double>* bestMetric = nullptr;
        bool anyFresh = false;
        for (const auto& device : snapshot.gpus) {
            const auto& metric = device.utilizationFraction;
            if (!metric.value.has_value() || !resourceMetricIsUsable(metric.availability)) {
                continue;
            }
            if (!best.has_value() || *metric.value > *best) {
                best = metric.value;
                bestMetric = &metric;
            }
            if (metric.availability == ResourceMetricAvailability::Measured) {
                anyFresh = true;
            }
        }

        if (best.has_value() && bestMetric != nullptr) {
            gpu.utilization =
                anyFresh ? measuredPolicyInput<double>(*best) : stalePolicyInput<double>(*best);
            gpu.utilizationProvenance = provenanceFrom(*bestMetric, gpu.utilization.state,
                                                       ResourceScope::Host, nowMonotonicMs);
            gpu.utilizationProvenance.valuePresent = true;
        } else {
            gpu.utilization =
                utilizationFromAvailability(aggregateUtilizationAvailability(snapshot.gpus));
            gpu.utilizationProvenance =
                provenanceFrom(first.utilizationFraction, gpu.utilization.state,
                               ResourceScope::Host, nowMonotonicMs);
        }
        return gpu;
    }

    // No device entry. The static capability declaration is the only truthful
    // way to distinguish "platform cannot provide a GPU signal" from "GPU
    // supported but currently absent".
    ResourceMetricAvailability support = ResourceMetricAvailability::Unsupported;
    if (capabilities != nullptr) {
        const auto deviceMemory = capabilities->supportFor(ResourceSignal::GpuMemory);
        const auto deviceUtilization = capabilities->supportFor(ResourceSignal::GpuUtilization);
        support = combineSupport(deviceUtilization, deviceMemory);
    }

    const bool supported = support == ResourceMetricAvailability::Measured ||
                           support == ResourceMetricAvailability::Stale;
    if (supported) {
        // The platform can sample a GPU; none is present right now.
        gpu.available = measuredPolicyInput<bool>(false);
    } else if (support == ResourceMetricAvailability::Unavailable) {
        gpu.available = inapplicablePolicyInput<bool>(ResourcePolicyInputState::Unavailable);
    } else if (support == ResourceMetricAvailability::Unknown) {
        gpu.available = unknownPolicyInput<bool>();
    } else {
        gpu.available = inapplicablePolicyInput<bool>(ResourcePolicyInputState::Unsupported);
    }
    gpu.availableProvenance.mappedState = gpu.available.state;
    gpu.availableProvenance.availability = support;
    gpu.availableProvenance.scope = ResourceScope::Host;
    gpu.availableProvenance.sourceId = QStringLiteral("policy.adapter.gpu");
    gpu.availableProvenance.applicable = gpu.available.applicable();
    gpu.availableProvenance.valuePresent = gpu.available.value.has_value();
    gpu.availableProvenance.detail =
        QStringLiteral("No GPU device entry; classified from capabilities.");

    gpu.utilization = utilizationFromAvailability(support);
    gpu.utilizationProvenance.mappedState = gpu.utilization.state;
    gpu.utilizationProvenance.availability = support;
    gpu.utilizationProvenance.scope = ResourceScope::Host;
    gpu.utilizationProvenance.sourceId = QStringLiteral("policy.adapter.gpu");
    gpu.utilizationProvenance.applicable = gpu.utilization.applicable();
    gpu.utilizationProvenance.valuePresent = gpu.utilization.value.has_value();

    gpu.notes.append(QStringLiteral("gpu: no device entry; availability=%1")
                         .arg(resourceMetricAvailabilityName(support)));
    return gpu;
}

ResourcePolicyTelemetryMapping buildMapping(const HostResourceSnapshot& snapshot,
                                            const ResourceCapabilityReport* capabilities,
                                            const ResourcePolicyTelemetryAdapterConfig& config,
                                            qint64 nowMonotonicMs) {
    ResourcePolicyTelemetryMapping mapping;

    // Work on a private copy: freshness reclassification must never mutate the
    // caller's snapshot.
    HostResourceSnapshot local = snapshot;
    if (config.applyFreshness) {
        applySnapshotFreshness(local, nowMonotonicMs, config.metricMaxAgeMs);
    }

    ResourcePolicyInputs& inputs = mapping.inputs;
    ResourcePolicyTelemetryDiagnostics& diagnostics = mapping.diagnostics;
    diagnostics.adapterId = local.adapterId;
    diagnostics.capturedAt = local.capturedAt;
    diagnostics.mappedAtMonotonicMs = nowMonotonicMs;
    diagnostics.metricMaxAgeMs = config.metricMaxAgeMs;
    diagnostics.hostLogicalCpuCount = local.hostLogicalCpuCount;

    // --- Host-wide CPU ----------------------------------------------------
    // P4-02 exposes only Sentinel-process CPU. `process.cpuFractionOfHost` is
    // that process's share of host capacity, NOT host-wide busy fraction, so it
    // must never be mapped here. No approved host-wide sampler exists yet.
    inputs.cpuBusyFraction = unknownPolicyInput<double>();
    diagnostics.cpuBusyFraction.mappedState = ResourcePolicyInputState::Unknown;
    diagnostics.cpuBusyFraction.availability = ResourceMetricAvailability::Unknown;
    diagnostics.cpuBusyFraction.applicable = true;
    diagnostics.cpuBusyFraction.scope = ResourceScope::Host;
    diagnostics.cpuBusyFraction.sourceId = QStringLiteral("policy.adapter.host_cpu.unavailable");
    diagnostics.cpuBusyFraction.detail = QStringLiteral(
        "No host-wide CPU source in P4-02; Sentinel-process CPU is not host CPU (PD-D03).");
    diagnostics.notes.append(
        QStringLiteral("cpuBusyFraction: Unknown — P4-02 has no host-wide CPU sampler (PD-D03)."));

    // --- Host memory ------------------------------------------------------
    {
        auto available =
            mapMetric(local.hostMemory.availableBytes, ResourceScope::Host, nowMonotonicMs);
        inputs.hostAvailableBytes = available.input;
        diagnostics.hostAvailableBytes = available.provenance;
        if (!local.hostMemory.totalBytes.value.has_value()) {
            diagnostics.notes.append(QStringLiteral(
                "hostMemory.totalBytes: not a policy input; only reclaimable bytes are mapped."));
        }
    }
    {
        auto pressure =
            mapMetric(local.hostMemory.pressureLevel, ResourceScope::Host, nowMonotonicMs);
        inputs.memoryPressureLevel = pressure.input;
        diagnostics.memoryPressureLevel = pressure.provenance;
    }

    // --- Thermal ----------------------------------------------------------
    {
        auto level = mapMetric(local.thermal.level, ResourceScope::Host, nowMonotonicMs);
        inputs.thermalLevel = level.input;
        diagnostics.thermalLevel = level.provenance;
    }

    // --- Power ------------------------------------------------------------
    {
        auto source = mapPowerSourceMetric(local.power.powerSource, nowMonotonicMs);
        inputs.powerSource = source.input;
        diagnostics.powerSource = source.provenance;
    }
    {
        auto present = mapMetric(local.power.batteryPresent, ResourceScope::Host, nowMonotonicMs);
        inputs.batteryPresent = present.input;
        diagnostics.batteryPresent = present.provenance;
    }
    {
        auto charge =
            mapMetric(local.power.batteryChargeFraction, ResourceScope::Host, nowMonotonicMs);
        inputs.batteryChargeFraction = charge.input;
        diagnostics.batteryChargeFraction = charge.provenance;
    }

    // --- Sentinel process -------------------------------------------------
    {
        auto resident =
            mapMetric(local.process.residentBytes, ResourceScope::SentinelProcess, nowMonotonicMs);
        inputs.processResidentBytes = resident.input;
        diagnostics.processResidentBytes = resident.provenance;
    }
    {
        auto core = mapMetric(local.process.cpuFractionOfCore, ResourceScope::SentinelProcess,
                              nowMonotonicMs);
        inputs.processCpuFractionOfCore = core.input;
        diagnostics.processCpuFractionOfCore = core.provenance;
    }

    // --- GPU --------------------------------------------------------------
    {
        GpuMapping gpu = deriveGpu(local, capabilities, nowMonotonicMs);
        inputs.gpuAvailable = gpu.available;
        inputs.gpuUtilizationFraction = gpu.utilization;
        diagnostics.gpuAvailable = gpu.availableProvenance;
        diagnostics.gpuUtilizationFraction = gpu.utilizationProvenance;
        diagnostics.gpuDeviceCount = gpu.deviceCount;
        diagnostics.notes.append(gpu.notes);
    }

    // Activity and ownership are runtime facts, not measurements. The adapter
    // never invents them: it leaves them at their neutral defaults.
    return mapping;
}

} // namespace

ResourcePolicyTelemetryAdapter::ResourcePolicyTelemetryAdapter(
    ResourcePolicyTelemetryAdapterConfig config)
    : config_(config) {}

ResourcePolicyTelemetryMapping
ResourcePolicyTelemetryAdapter::adapt(const HostResourceSnapshot& snapshot,
                                      qint64 nowMonotonicMs) const {
    return buildMapping(snapshot, nullptr, config_, nowMonotonicMs);
}

ResourcePolicyTelemetryMapping
ResourcePolicyTelemetryAdapter::adapt(const HostResourceSnapshot& snapshot,
                                      const ResourceCapabilityReport& capabilities,
                                      qint64 nowMonotonicMs) const {
    return buildMapping(snapshot, &capabilities, config_, nowMonotonicMs);
}

} // namespace sentinel::core
