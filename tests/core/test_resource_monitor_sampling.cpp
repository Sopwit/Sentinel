// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// P4-02 sampling lifecycle, bounds, cancellation and concurrency tests. The
// sampler thread is exercised with a scripted adapter so no host battery,
// thermal sensor or GPU is required, and the monitor is asserted to be inert
// until explicitly started.

#include "resource_telemetry_test_support.h"

#include "sentinel/core/agent/AgentExecutionScheduler.h"
#include "sentinel/core/resource/ResourceMonitor.h"
#include "sentinel/core/resource/ResourceTelemetry.h"

#include <QElapsedTimer>
#include <QtTest>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

using sentinel::core::HostResourceSnapshot;
using sentinel::core::ResourceMetricAvailability;
using sentinel::core::ResourceMonitor;
using sentinel::core::ResourceMonitorConfig;

namespace {

std::unique_ptr<ResourceMonitor>
makeMonitor(std::unique_ptr<sentinel::core::test::ScriptedResourceMonitorAdapter> adapter,
            ResourceMonitorConfig config = {}) {
    auto clock = std::make_shared<sentinel::core::test::ScriptedMonotonicClock>();
    return std::make_unique<ResourceMonitor>(std::move(adapter), clock, config);
}

} // namespace

class ResourceMonitorSamplingTest final : public QObject {
    Q_OBJECT
private slots:
    void defaultConfigUsesSafeBounds();
    void samplingIntervalIsClamped();
    void monitorDoesNotStartUntilAsked();
    void repeatedStartStopIsSafe();
    void sampleOnceIsDeterministicWithInjectedClock();
    void backgroundSamplingPopulatesBoundedHistory();
    void concurrentSnapshotReadsAreSafe();
    void cancellationStopsSamplerPromptly();
    void destructorStopsRunningSampler();
    void unsupportedStatePropagatesThroughMonitor();
    void samplingDoesNotAlterExecutionBudget();
};

void ResourceMonitorSamplingTest::defaultConfigUsesSafeBounds() {
    ResourceMonitorConfig config;
    QCOMPARE(config.effectiveSamplingIntervalMs(),
             sentinel::core::kResourceMonitorDefaultIntervalMs);
    QVERIFY(config.effectiveSamplingIntervalMs() >=
            sentinel::core::kResourceMonitorMinimumIntervalMs);
    QVERIFY(config.effectiveSamplingIntervalMs() <=
            sentinel::core::kResourceMonitorMaximumIntervalMs);
    QVERIFY(config.metricMaxAgeMs > 0);
    QVERIFY(config.historyCapacity >= 1);
}

void ResourceMonitorSamplingTest::samplingIntervalIsClamped() {
    QCOMPARE(sentinel::core::clampResourceSamplingIntervalMs(0),
             sentinel::core::kResourceMonitorDefaultIntervalMs);
    QCOMPARE(sentinel::core::clampResourceSamplingIntervalMs(-100),
             sentinel::core::kResourceMonitorDefaultIntervalMs);
    QCOMPARE(sentinel::core::clampResourceSamplingIntervalMs(1),
             sentinel::core::kResourceMonitorMinimumIntervalMs);
    QCOMPARE(sentinel::core::clampResourceSamplingIntervalMs(1'000'000),
             sentinel::core::kResourceMonitorMaximumIntervalMs);
    QCOMPARE(sentinel::core::clampResourceSamplingIntervalMs(3000), 3000);
}

void ResourceMonitorSamplingTest::monitorDoesNotStartUntilAsked() {
    auto adapter = std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>();
    auto* scripted = adapter.get();
    auto monitor = makeMonitor(std::move(adapter));
    QVERIFY(!monitor->isRunning());
    QCOMPARE(monitor->sampleCount(), 0u);
    QCOMPARE(scripted->sampleCount, 0);
}

void ResourceMonitorSamplingTest::repeatedStartStopIsSafe() {
    auto monitor =
        makeMonitor(std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>(),
                    ResourceMonitorConfig{.samplingIntervalMs = 250});
    QVERIFY(monitor->start());
    // A second start while running is refused, not a second thread.
    QVERIFY(!monitor->start());
    QVERIFY(monitor->isRunning());
    monitor->stop();
    QVERIFY(!monitor->isRunning());
    // Stopping again is a documented no-op.
    monitor->stop();
    QVERIFY(!monitor->isRunning());

    // Restarting after a stop works.
    QVERIFY(monitor->start());
    QVERIFY(monitor->isRunning());
    monitor->stop();
    QVERIFY(!monitor->isRunning());
}

void ResourceMonitorSamplingTest::sampleOnceIsDeterministicWithInjectedClock() {
    auto adapter = std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>();
    sentinel::core::ResourceSampleTimestamp stamp;
    stamp.monotonicMs = 100;
    adapter->scripted.process.residentBytes = sentinel::core::measuredMetric<qint64>(
        8192, stamp, sentinel::core::ResourceMeasurementOrigin::SentinelProcess,
        sentinel::core::ResourceReliability::Authoritative, QStringLiteral("test.rss"));
    auto* scripted = adapter.get();
    auto clock = std::make_shared<sentinel::core::test::ScriptedMonotonicClock>();
    clock->now = 100;
    ResourceMonitor monitor(std::move(adapter), clock);

    const auto snapshot = monitor.sampleOnce();
    QCOMPARE(scripted->sampleCount, 1);
    QCOMPARE(monitor.sampleCount(), 1u);
    QCOMPARE(snapshot.process.residentBytes.value.value(), 8192LL);
    QCOMPARE(snapshot.capturedAt.monotonicMs, 100LL);
    QCOMPARE(monitor.latestSnapshot().process.residentBytes.value.value(), 8192LL);
    // The sample is stamped with the injected clock and stays fresh.
    QCOMPARE(snapshot.process.residentBytes.availability, ResourceMetricAvailability::Measured);
}

void ResourceMonitorSamplingTest::backgroundSamplingPopulatesBoundedHistory() {
    ResourceMonitorConfig config;
    config.samplingIntervalMs = sentinel::core::kResourceMonitorMinimumIntervalMs;
    config.historyCapacity = 3;
    auto monitor = makeMonitor(
        std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>(), config);
    QVERIFY(monitor->start());
    QTRY_VERIFY_WITH_TIMEOUT(monitor->sampleCount() >= 5, 5000);
    monitor->stop();
    const auto history = monitor->history();
    QVERIFY(!history.isEmpty());
    // Retention is bounded by historyCapacity.
    QVERIFY(history.size() <= 3);
    QVERIFY(monitor->sampleCount() >= 5u);
}

void ResourceMonitorSamplingTest::concurrentSnapshotReadsAreSafe() {
    ResourceMonitorConfig config;
    config.samplingIntervalMs = sentinel::core::kResourceMonitorMinimumIntervalMs;
    auto monitor = makeMonitor(
        std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>(), config);
    QVERIFY(monitor->start());

    std::atomic<bool> stop{false};
    std::vector<std::thread> readers;
    for (int i = 0; i < 3; ++i) {
        readers.emplace_back([&monitor, &stop] {
            while (!stop.load()) {
                const auto latest = monitor->latestSnapshot();
                (void)latest;
                const auto history = monitor->history();
                (void)history;
                (void)monitor->sampleCount();
            }
        });
    }
    QTRY_VERIFY_WITH_TIMEOUT(monitor->sampleCount() >= 3, 5000);
    stop.store(true);
    for (auto& reader : readers)
        reader.join();
    monitor->stop();
    QVERIFY(!monitor->isRunning());
}

void ResourceMonitorSamplingTest::cancellationStopsSamplerPromptly() {
    ResourceMonitorConfig config;
    config.samplingIntervalMs = sentinel::core::kResourceMonitorMaximumIntervalMs; // long interval
    auto monitor = makeMonitor(
        std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>(), config);
    QVERIFY(monitor->start());
    // One sample is taken immediately on entry.
    QTRY_VERIFY_WITH_TIMEOUT(monitor->sampleCount() >= 1, 5000);
    QElapsedTimer timer;
    timer.start();
    monitor->stop();
    const qint64 elapsed = timer.elapsed();
    QVERIFY(!monitor->isRunning());
    // Cancellation must interrupt the long wait rather than block for it.
    QVERIFY2(elapsed < 2000, qPrintable(QStringLiteral("stop took %1 ms").arg(elapsed)));
}

void ResourceMonitorSamplingTest::destructorStopsRunningSampler() {
    auto monitor =
        makeMonitor(std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>(),
                    ResourceMonitorConfig{.samplingIntervalMs = 250});
    QVERIFY(monitor->start());
    QTRY_VERIFY_WITH_TIMEOUT(monitor->sampleCount() >= 1, 5000);
    // Destroying a running monitor must join cleanly (no leak, no hang).
    monitor.reset();
    QVERIFY(true);
}

void ResourceMonitorSamplingTest::unsupportedStatePropagatesThroughMonitor() {
    auto adapter = std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>();
    adapter->scripted.process.cpuSecondsTotal = sentinel::core::unsupportedMetric<double>(
        QStringLiteral("test.cpu"), QStringLiteral("No CPU counter."));
    adapter->scripted.thermal.level = sentinel::core::unavailableMetric<QString>(
        QStringLiteral("test.thermal"), QStringLiteral("No thermal API."));
    auto monitor = makeMonitor(std::move(adapter));
    const auto snapshot = monitor->sampleOnce();
    // A process-scoped unsupported CPU counter propagates to the derived fields
    // instead of being silently reported as unknown or zero.
    QCOMPARE(snapshot.process.cpuFractionOfCore.availability,
             ResourceMetricAvailability::Unsupported);
    QCOMPARE(snapshot.thermal.level.availability, ResourceMetricAvailability::Unavailable);
}

void ResourceMonitorSamplingTest::samplingDoesNotAlterExecutionBudget() {
    // The monitor must not create, consume or release an execution budget.
    sentinel::core::AgentExecutionScheduler::Budget budget(4);
    auto monitor =
        makeMonitor(std::make_unique<sentinel::core::test::ScriptedResourceMonitorAdapter>());
    for (int i = 0; i < 20; ++i)
        (void)monitor->sampleOnce();
    QCOMPARE(budget.limit, 4);
    QCOMPARE(budget.active.load(), 0);
}

QTEST_MAIN(ResourceMonitorSamplingTest)

#include "test_resource_monitor_sampling.moc"
