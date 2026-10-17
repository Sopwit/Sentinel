// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/resource/ResourceMonitor.h"

#include <QDateTime>

#include <chrono>
#include <utility>

namespace sentinel::core {

int clampResourceSamplingIntervalMs(int requestedMs) {
    if (requestedMs <= 0) {
        return kResourceMonitorDefaultIntervalMs;
    }
    if (requestedMs < kResourceMonitorMinimumIntervalMs) {
        return kResourceMonitorMinimumIntervalMs;
    }
    if (requestedMs > kResourceMonitorMaximumIntervalMs) {
        return kResourceMonitorMaximumIntervalMs;
    }
    return requestedMs;
}

ResourceMonitor::ResourceMonitor(std::unique_ptr<IResourceMonitorAdapter> adapter,
                                 std::shared_ptr<IMonotonicClock> clock,
                                 ResourceMonitorConfig config)
    : adapter_(std::move(adapter)), clock_(std::move(clock)), config_(config) {
    if (!adapter_) {
        adapter_ = makeNativeResourceMonitorAdapter();
    }
    if (!clock_) {
        clock_ = std::make_shared<SteadyMonotonicClock>();
    }
    if (config_.historyCapacity < 1) {
        config_.historyCapacity = 1;
    }
    if (config_.metricMaxAgeMs <= 0) {
        config_.metricMaxAgeMs = kResourceMonitorDefaultMetricMaxAgeMs;
    }
    capabilities_ = adapter_->capabilities();
    latest_.adapterId = adapter_->adapterId();
}

ResourceMonitor::~ResourceMonitor() {
    stop();
}

bool ResourceMonitor::start() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_.load()) {
            return false;
        }
        stopRequested_ = false;
        running_.store(true);
        worker_ = std::thread([this] { runLoop(); });
    }
    wake_.notify_all();
    return true;
}

void ResourceMonitor::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_.load()) {
            return;
        }
        stopRequested_ = true;
    }
    wake_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
    running_.store(false);
}

bool ResourceMonitor::isRunning() const {
    return running_.load();
}

ResourceMonitorConfig ResourceMonitor::config() const {
    return config_;
}

const ResourceCapabilityReport& ResourceMonitor::capabilities() const {
    return capabilities_;
}

void ResourceMonitor::runLoop() {
    // Sample once on entry, then wait for the interval with a stop predicate so
    // cancellation is immediate and there is no busy loop.
    while (true) {
        sampleOnce();
        std::unique_lock<std::mutex> lock(mutex_);
        if (stopRequested_) {
            break;
        }
        wake_.wait_for(lock, std::chrono::milliseconds(config_.effectiveSamplingIntervalMs()),
                       [this] { return stopRequested_; });
        if (stopRequested_) {
            break;
        }
    }
}

HostResourceSnapshot ResourceMonitor::sampleOnce() {
    // Serialize the whole sampling critical section so a caller-driven sample
    // can never race the background sampler (including adapter-internal state).
    std::lock_guard<std::mutex> lock(mutex_);

    const qint64 now = clock_->nowMonotonicMs();
    ResourceSampleTimestamp capturedAt;
    capturedAt.monotonicMs = now;
    capturedAt.wallClockUtc = QDateTime::currentDateTimeUtc();

    HostResourceSnapshot snapshot = adapter_->sample(capturedAt);
    if (snapshot.adapterId.isEmpty()) {
        snapshot.adapterId = adapter_->adapterId();
    }
    if (snapshot.capturedAt.monotonicMs <= 0) {
        snapshot.capturedAt = capturedAt;
    }

    // Derive average CPU utilization from the cumulative counter when the
    // adapter reports one. Adapters that publish pre-derived fractions leave
    // `cpuSecondsTotal` unknown and are passed through unchanged.
    auto& cpuTotal = snapshot.process.cpuSecondsTotal;
    if (cpuTotal.hasValue()) {
        const auto result =
            deriveCpuFractions(cpuTotal.value, previousCpuSeconds_,
                               now - previousSampleMonotonicMs_, snapshot.hostLogicalCpuCount);
        if (result.fractionOfCore) {
            snapshot.process.cpuFractionOfCore = measuredMetric(
                *result.fractionOfCore, capturedAt, ResourceMeasurementOrigin::SentinelProcess,
                ResourceReliability::Approximate, QStringLiteral("derived.counter_delta"));
        } else {
            snapshot.process.cpuFractionOfCore = unknownMetric<double>(
                ResourceMeasurementOrigin::SentinelProcess, QStringLiteral("derived.counter_delta"),
                QStringLiteral("No previous CPU counter baseline."));
        }
        if (result.fractionOfHost) {
            snapshot.process.cpuFractionOfHost = measuredMetric(
                *result.fractionOfHost, capturedAt, ResourceMeasurementOrigin::SentinelProcess,
                ResourceReliability::Approximate, QStringLiteral("derived.counter_delta"));
        } else if (!result.fractionOfCore) {
            snapshot.process.cpuFractionOfHost = unknownMetric<double>(
                ResourceMeasurementOrigin::SentinelProcess, QStringLiteral("derived.counter_delta"),
                QStringLiteral("No previous CPU counter baseline."));
        }
        previousCpuSeconds_ = cpuTotal.value;
    } else {
        // Propagate the adapter's reason instead of inventing a state.
        const auto availability = cpuTotal.availability;
        if (availability == ResourceMetricAvailability::Unsupported) {
            snapshot.process.cpuFractionOfCore =
                unsupportedMetric<double>(cpuTotal.sourceId, cpuTotal.detail);
            snapshot.process.cpuFractionOfHost =
                unsupportedMetric<double>(cpuTotal.sourceId, cpuTotal.detail);
        }
    }
    previousSampleMonotonicMs_ = now;

    applySnapshotFreshness(snapshot, now, config_.metricMaxAgeMs);

    latest_ = snapshot;
    history_.push_back(snapshot);
    while (static_cast<int>(history_.size()) > config_.historyCapacity) {
        history_.pop_front();
    }
    sampleCount_.fetch_add(1);
    return snapshot;
}

HostResourceSnapshot ResourceMonitor::latestSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_;
}

QList<HostResourceSnapshot> ResourceMonitor::history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return QList<HostResourceSnapshot>(history_.begin(), history_.end());
}

quint64 ResourceMonitor::sampleCount() const {
    return sampleCount_.load();
}

} // namespace sentinel::core
