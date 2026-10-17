// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// P4-02 resource monitor adapter seam.
//
// An adapter is a stateless-or-lightly-stateful probe that produces one
// `HostResourceSnapshot` on demand. Adapters never own a timer, never run
// background work, never start or stop a model server and never terminate a
// process. Collection lifecycle and freshness live in `ResourceMonitor`.
//
// This is intentionally beside, not inside, `IHardwarePlatformAdapter`
// (static hardware facts). Static facts and live samples are different
// contracts: the former is read once and refreshed explicitly, the latter is
// sampled on an interval with per-metric freshness.

#include "sentinel/core/resource/ResourceTelemetry.h"

#include <QString>
#include <memory>

namespace sentinel::core {

class IResourceMonitorAdapter {
public:
    virtual ~IResourceMonitorAdapter() = default;

    // Stable identity used in `HostResourceSnapshot::adapterId`, for example
    // "macos.native", "linux.proc", "windows.native" or "test.scripted".
    virtual QString adapterId() const = 0;

    // Static declaration of which signals this adapter can provide and how.
    // Must report `Unsupported` for signals the platform cannot provide rather
    // than omitting them.
    virtual ResourceCapabilityReport capabilities() const = 0;

    // Collects one snapshot. `capturedAt` is the batch timestamp supplied by the
    // monitor; adopters may stamp individual metrics with it or with an older
    // per-metric timestamp when a sub-signal is cached. Implementations must
    // return `Unknown`/`Unavailable`/`Unsupported` rather than a fabricated
    // value, and must never block for a network round trip.
    virtual HostResourceSnapshot sample(const ResourceSampleTimestamp& capturedAt) const = 0;
};

// Factory for the platform-native adapter. Returns a valid adapter on every
// supported platform; signals that cannot be sampled are reported as
// `Unavailable` or `Unsupported` in the capability report and snapshot.
std::unique_ptr<IResourceMonitorAdapter> makeNativeResourceMonitorAdapter();

} // namespace sentinel::core
