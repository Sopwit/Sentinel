// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// P4-02 sampling lifecycle.
//
// `ResourceMonitor` owns collection for exactly one injected adapter. It is
// deliberately opt-in: constructing it starts nothing, and `start()` is the only
// path that creates the sampler thread. It never starts a model server, never
// touches provider routing and never modifies an execution budget.
//
// Performance contract:
//   * no polling faster than the clamped sampling interval,
//   * no busy loop (condition-variable wait with a stop predicate),
//   * no work on model-execution threads (dedicated sampler thread only),
//   * bounded sample retention,
//   * thread-safe latest-snapshot and history reads,
//   * deterministic shutdown that joins the sampler thread.

#include "sentinel/core/resource/IResourceMonitorAdapter.h"
#include "sentinel/core/resource/ResourceTelemetry.h"

#include <QList>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace sentinel::core {

// Sampling interval bounds. Calibration (PD-D04) may tighten these later; the
// defaults only prevent pathological values, they are not final policy.
inline constexpr int kResourceMonitorMinimumIntervalMs = 250;
inline constexpr int kResourceMonitorMaximumIntervalMs = 60000;
inline constexpr int kResourceMonitorDefaultIntervalMs = 2000;
inline constexpr int kResourceMonitorDefaultHistoryCapacity = 32;
inline constexpr qint64 kResourceMonitorDefaultMetricMaxAgeMs = 5000;

// Clamps a requested sampling interval into the supported bounds. A
// non-positive request falls back to the default interval.
int clampResourceSamplingIntervalMs(int requestedMs);

struct ResourceMonitorConfig {
    // Requested interval between samples. Clamped on construction.
    int samplingIntervalMs = kResourceMonitorDefaultIntervalMs;
    // A measured metric older than this is reclassified as `Stale`.
    qint64 metricMaxAgeMs = kResourceMonitorDefaultMetricMaxAgeMs;
    // Maximum retained snapshots. Values below one are raised to one.
    int historyCapacity = kResourceMonitorDefaultHistoryCapacity;

    int effectiveSamplingIntervalMs() const {
        return clampResourceSamplingIntervalMs(samplingIntervalMs);
    }
};

class ResourceMonitor final {
public:
    // The adapter supplies samples; the clock supplies monotonic timestamps and
    // is injectable so freshness and interval behaviour are deterministic.
    ResourceMonitor(std::unique_ptr<IResourceMonitorAdapter> adapter,
                    std::shared_ptr<IMonotonicClock> clock, ResourceMonitorConfig config = {});
    ~ResourceMonitor();

    ResourceMonitor(const ResourceMonitor&) = delete;
    ResourceMonitor& operator=(const ResourceMonitor&) = delete;
    ResourceMonitor(ResourceMonitor&&) = delete;
    ResourceMonitor& operator=(ResourceMonitor&&) = delete;

    // Starts the sampler thread. Returns false when already running. Idempotent
    // repeated start/stop cycles are supported.
    bool start();
    // Signals cancellation and joins the sampler thread. Safe when not running
    // and safe to call from the destructor path.
    void stop();
    bool isRunning() const;

    ResourceMonitorConfig config() const;
    const ResourceCapabilityReport& capabilities() const;

    // Takes one sample synchronously on the calling thread. Safe to call while
    // the background sampler is running; the two serialize on the internal
    // mutex. Never spawns, blocks or terminates anything else.
    HostResourceSnapshot sampleOnce();

    // Thread-safe copies. `latestSnapshot` returns an empty-adapter snapshot
    // until the first sample is taken.
    HostResourceSnapshot latestSnapshot() const;
    QList<HostResourceSnapshot> history() const;
    quint64 sampleCount() const;

private:
    void runLoop();

    std::unique_ptr<IResourceMonitorAdapter> adapter_;
    std::shared_ptr<IMonotonicClock> clock_;
    ResourceMonitorConfig config_;
    ResourceCapabilityReport capabilities_;

    mutable std::mutex mutex_;
    std::condition_variable wake_;
    std::thread worker_;
    bool stopRequested_ = false;

    std::optional<double> previousCpuSeconds_;
    qint64 previousSampleMonotonicMs_ = 0;
    HostResourceSnapshot latest_;
    std::deque<HostResourceSnapshot> history_;

    std::atomic<bool> running_{false};
    std::atomic<quint64> sampleCount_{0};
};

} // namespace sentinel::core
