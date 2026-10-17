// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// P4-01 baseline: the only existing telemetry injection seam is
// IHardwarePlatformAdapter. These cases pin the current truth contract
// (unknown stays unknown, GPU VRAM is not system RAM, no fabricated zero)
// before any Adaptive Resource Management sampler exists.

#include "sentinel/core/model/HardwareCapabilityService.h"

#include <QtTest>

using sentinel::core::GpuDeviceFacts;
using sentinel::core::HardwareCapabilityService;
using sentinel::core::HardwareFacts;
using sentinel::core::IHardwarePlatformAdapter;

namespace {

class ScriptedHardwareAdapter final : public IHardwarePlatformAdapter {
public:
    HardwareFacts facts;
    mutable int collectCount = 0;

    HardwareFacts collect() const override {
        ++collectCount;
        return facts;
    }
};

} // namespace

class ResourceTelemetryContractTest final : public QObject {
    Q_OBJECT
private slots:
    void unavailableTelemetryRemainsUnknown();
    void gpuVramIsNeverSubstitutedForSystemRam();
    void refreshIsTheOnlyRereadPath();
    void nativeAdapterNeverReportsZeroCapacity();
};

void ResourceTelemetryContractTest::unavailableTelemetryRemainsUnknown() {
    auto adapter = std::make_unique<ScriptedHardwareAdapter>();
    const auto* scripted = adapter.get();
    HardwareCapabilityService service(std::move(adapter));

    // A platform that reports nothing yields optionals without a value; zero
    // is never fabricated for RAM, GPU list or physical CPU count.
    QVERIFY(!service.facts().systemRamBytes.has_value());
    QCOMPARE(service.facts().gpus.size(), 0);
    QVERIFY(!service.facts().physicalCpuCount.has_value());
    QCOMPARE(service.facts().operatingSystem, QString{});
    // Documented gap (P4-02): logicalCpuCount has no single unknown encoding.
    // The struct default is 0 while QThread::idealThreadCount() reports -1
    // when it cannot detect a core count.
    QCOMPARE(service.facts().logicalCpuCount, 0);
    QCOMPARE(scripted->collectCount, 1);

    service.refresh();
    service.refresh();
    QVERIFY(!service.facts().systemRamBytes.has_value());
    QCOMPARE(scripted->collectCount, 3);
}

void ResourceTelemetryContractTest::gpuVramIsNeverSubstitutedForSystemRam() {
    auto adapter = std::make_unique<ScriptedHardwareAdapter>();
    GpuDeviceFacts unified;
    unified.name = QStringLiteral("Unified fixture GPU");
    unified.backend = QStringLiteral("fixture");
    unified.reportedVramBytes = 8LL * 1024 * 1024 * 1024;
    adapter->facts.gpus.append(unified);
    adapter->facts.logicalCpuCount = 4;
    const auto* scripted = adapter.get();
    HardwareCapabilityService service(std::move(adapter));

    QCOMPARE(service.facts().gpus.size(), 1);
    QVERIFY(service.facts().gpus.first().reportedVramBytes.has_value());
    // A reported GPU allocation is not a system-RAM reading (AC-P4-01 must not
    // conflate Apple unified-memory GPU estimates with resident footprint).
    QVERIFY(!service.facts().systemRamBytes.has_value());
    QCOMPARE(service.facts().logicalCpuCount, 4);
    QCOMPARE(scripted->collectCount, 1);
}

void ResourceTelemetryContractTest::refreshIsTheOnlyRereadPath() {
    auto adapter = std::make_unique<ScriptedHardwareAdapter>();
    adapter->facts.systemRamBytes = 8LL * 1024 * 1024 * 1024;
    auto* scripted = adapter.get();
    HardwareCapabilityService service(std::move(adapter));

    const auto first = service.facts().systemRamBytes;
    QCOMPARE(scripted->collectCount, 1);
    for (int i = 0; i < 5; ++i) {
        // Repeated reads never re-sample: there is no background sampler and no
        // freshness/timestamp on HardwareFacts, so a stale reading is currently
        // indistinguishable from a fresh one (documented P4-01 contract gap).
        QCOMPARE(service.facts().systemRamBytes, first);
    }
    QCOMPARE(scripted->collectCount, 1);

    scripted->facts.systemRamBytes = 16LL * 1024 * 1024 * 1024;
    service.refresh();
    QCOMPARE(scripted->collectCount, 2);
    const auto& refreshedFacts = service.facts();
    QVERIFY(refreshedFacts.systemRamBytes != first);
    QVERIFY(refreshedFacts.systemRamBytes.has_value());
    if (refreshedFacts.systemRamBytes.has_value()) {
        QCOMPARE(refreshedFacts.systemRamBytes.value(), 16LL * 1024 * 1024 * 1024);
    }
}

void ResourceTelemetryContractTest::nativeAdapterNeverReportsZeroCapacity() {
    HardwareCapabilityService service;
    const auto& facts = service.facts();

    QVERIFY(!facts.operatingSystem.isEmpty());
    QVERIFY(!facts.cpuArchitecture.isEmpty());
    if (facts.systemRamBytes) {
        QVERIFY(*facts.systemRamBytes > 0);
    }
    // QThread::idealThreadCount() reports -1 when undetectable, never 0.
    QVERIFY(facts.logicalCpuCount != 0);
    for (const auto& gpu : facts.gpus) {
        QVERIFY(!gpu.name.isEmpty());
        if (gpu.reportedVramBytes) {
            QVERIFY(*gpu.reportedVramBytes > 0);
        }
    }
}

QTEST_MAIN(ResourceTelemetryContractTest)

#include "test_resource_telemetry_contract.moc"
