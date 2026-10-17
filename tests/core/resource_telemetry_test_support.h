// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Shared deterministic fixtures for the P4-02 resource telemetry tests.
// They intentionally avoid any dependency on the host's battery, thermal
// sensors or GPU so the tests are reproducible everywhere.

#include "sentinel/core/resource/IResourceMonitorAdapter.h"
#include "sentinel/core/resource/ResourceMonitor.h"

#include <utility>

namespace sentinel::core::test {

// A scripted adapter whose returned snapshot is set by the test. The sample
// counter is mutable so it can be observed after the fact without making the
// interface non-const.
class ScriptedResourceMonitorAdapter final : public IResourceMonitorAdapter {
public:
    explicit ScriptedResourceMonitorAdapter(QString id = QStringLiteral("test.scripted"))
        : id_(std::move(id)) {}

    QString adapterId() const override {
        return id_;
    }

    ResourceCapabilityReport capabilities() const override {
        return capabilities_;
    }

    HostResourceSnapshot sample(const ResourceSampleTimestamp& capturedAt) const override {
        ++sampleCount;
        HostResourceSnapshot snapshot = scripted;
        snapshot.adapterId = id_;
        snapshot.capturedAt = capturedAt;
        return snapshot;
    }

    void setCapability(ResourceSignal signal, ResourceMetricAvailability availability) {
        for (auto& entry : capabilities_.entries) {
            if (entry.signal == signal) {
                entry.baseline = availability;
                return;
            }
        }
        capabilities_.entries.append({signal, availability});
    }

    void setCapabilities(ResourceCapabilityReport report) {
        capabilities_ = std::move(report);
    }

    QString id_;
    ResourceCapabilityReport capabilities_;
    HostResourceSnapshot scripted;
    mutable int sampleCount = 0;
};

// An injectable monotonic clock for deterministic freshness and interval tests.
class ScriptedMonotonicClock final : public IMonotonicClock {
public:
    qint64 nowMonotonicMs() const override {
        return now;
    }

    void advance(qint64 milliseconds) {
        now += milliseconds;
    }

    qint64 now = 0;
};

} // namespace sentinel::core::test
