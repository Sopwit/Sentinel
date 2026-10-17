// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// P4-02 measurement probe (not a policy test). Reports the per-sample cost of
// the native adapter and the observed sampling interval / jitter of the
// background monitor. Assertions are deliberately loose: this records evidence
// for the audit, it does not encode final thresholds.

#include "sentinel/core/resource/IResourceMonitorAdapter.h"
#include "sentinel/core/resource/ResourceMonitor.h"

#include <QElapsedTimer>
#include <QtTest>

#include <algorithm>
#include <vector>

using sentinel::core::HostResourceSnapshot;
using sentinel::core::ResourceMonitor;
using sentinel::core::ResourceMonitorConfig;
using sentinel::core::ResourceSampleTimestamp;

class BenchResourceTelemetryTest final : public QObject {
    Q_OBJECT
private slots:
    void nativeAdapterPerSampleCost();
    void backgroundSamplingIntervalAndJitter();
};

void BenchResourceTelemetryTest::nativeAdapterPerSampleCost() {
    auto adapter = sentinel::core::makeNativeResourceMonitorAdapter();
    ResourceSampleTimestamp stamp;
    stamp.monotonicMs = 1;

    for (int i = 0; i < 10; ++i)
        (void)adapter->sample(stamp);

    constexpr int kIterations = 500;
    QElapsedTimer timer;
    std::vector<double> perSampleUs;
    perSampleUs.reserve(kIterations);
    for (int i = 0; i < kIterations; ++i) {
        timer.restart();
        (void)adapter->sample(stamp);
        perSampleUs.push_back(static_cast<double>(timer.nsecsElapsed()) / 1000.0);
    }
    std::sort(perSampleUs.begin(), perSampleUs.end());
    const double medianUs = perSampleUs.at(perSampleUs.size() / 2);
    const double p95Us = perSampleUs.at(static_cast<size_t>(perSampleUs.size() * 0.95));
    qInfo().noquote() << QStringLiteral("native adapter per-sample median=%1us p95=%2us")
                             .arg(medianUs, 0, 'f', 2)
                             .arg(p95Us, 0, 'f', 2);
    // A single native sample must be far cheaper than the sampling interval.
    QVERIFY2(p95Us < 5000.0, "native sample cost unexpectedly high");
}

void BenchResourceTelemetryTest::backgroundSamplingIntervalAndJitter() {
    ResourceMonitorConfig config;
    config.samplingIntervalMs = 500;
    config.historyCapacity = 64;
    ResourceMonitor monitor(sentinel::core::makeNativeResourceMonitorAdapter(), nullptr, config);
    QVERIFY(monitor.start());
    QTRY_VERIFY_WITH_TIMEOUT(monitor.sampleCount() >= 8, 20000);
    monitor.stop();

    const auto history = monitor.history();
    QVERIFY(history.size() >= 2);
    std::vector<qint64> deltas;
    for (size_t i = 1; i < history.size(); ++i) {
        const qint64 delta =
            history.at(i).capturedAt.monotonicMs - history.at(i - 1).capturedAt.monotonicMs;
        if (delta > 0)
            deltas.push_back(delta);
    }
    QVERIFY(!deltas.empty());
    std::sort(deltas.begin(), deltas.end());
    const qint64 median = deltas.at(deltas.size() / 2);
    const qint64 maxDelta = deltas.back();
    qInfo().noquote() << QStringLiteral("background sampling samples=%1 median_interval=%2ms "
                                        "max_interval=%3ms")
                             .arg(history.size())
                             .arg(median)
                             .arg(maxDelta);
    // No sample may be faster than the clamped floor, and none may stall for
    // more than a few intervals on an idle host.
    QVERIFY(deltas.front() >= sentinel::core::kResourceMonitorMinimumIntervalMs / 2);
    QVERIFY(maxDelta < 5000);
}

QTEST_MAIN(BenchResourceTelemetryTest)

#include "bench_resource_telemetry.moc"
