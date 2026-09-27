// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/model/HardwareCapabilityService.h"

#include <QDir>
#include <QFile>
#include <QSysInfo>
#include <QThread>
#include <climits>

#if defined(Q_OS_MACOS)
#include <sys/sysctl.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace sentinel::core {
namespace {

class NativeHardwareAdapter final : public IHardwarePlatformAdapter {
public:
    HardwareFacts collect() const override {
        HardwareFacts facts;
        facts.operatingSystem = QSysInfo::productType();
        facts.cpuArchitecture = QSysInfo::currentCpuArchitecture();
        facts.logicalCpuCount = QThread::idealThreadCount();
#if defined(Q_OS_MACOS)
        uint64_t ram = 0;
        size_t ramSize = sizeof(ram);
        if (sysctlbyname("hw.memsize", &ram, &ramSize, nullptr, 0) == 0)
            facts.systemRamBytes = static_cast<qint64>(ram);
        int physical = 0;
        size_t cpuSize = sizeof(physical);
        if (sysctlbyname("hw.physicalcpu", &physical, &cpuSize, nullptr, 0) == 0)
            facts.physicalCpuCount = physical;
#elif defined(Q_OS_LINUX)
        const auto pages = sysconf(_SC_PHYS_PAGES);
        const auto pageSize = sysconf(_SC_PAGESIZE);
        if (pages > 0 && pageSize > 0 && pages <= LLONG_MAX / pageSize)
            facts.systemRamBytes = static_cast<qint64>(pages) * pageSize;
        const QDir drm(QStringLiteral("/sys/class/drm"));
        for (const auto& card : drm.entryList({QStringLiteral("card[0-9]*")}, QDir::Dirs)) {
            QFile vendor(drm.filePath(card + QStringLiteral("/device/vendor")));
            if (!vendor.open(QIODevice::ReadOnly)) continue;
            GpuDeviceFacts gpu;
            gpu.name = QStringLiteral("PCI GPU %1").arg(QString::fromLatin1(vendor.readAll()).trimmed());
            QFile vram(drm.filePath(card + QStringLiteral("/device/mem_info_vram_total")));
            if (vram.open(QIODevice::ReadOnly)) {
                bool ok = false;
                const auto bytes = QString::fromLatin1(vram.readAll()).trimmed().toLongLong(&ok);
                if (ok) gpu.reportedVramBytes = bytes;
            }
            facts.gpus << gpu;
        }
#elif defined(Q_OS_WIN)
        MEMORYSTATUSEX memory{};
        memory.dwLength = sizeof(memory);
        if (GlobalMemoryStatusEx(&memory))
            facts.systemRamBytes = static_cast<qint64>(memory.ullTotalPhys);
#endif
        return facts;
    }
};

} // namespace

HardwareCapabilityService::HardwareCapabilityService(std::unique_ptr<IHardwarePlatformAdapter> adapter)
    : adapter_(adapter ? std::move(adapter) : std::make_unique<NativeHardwareAdapter>()) {
    refresh();
}

const HardwareFacts& HardwareCapabilityService::facts() const { return facts_; }
void HardwareCapabilityService::refresh() { facts_ = adapter_->collect(); }

} // namespace sentinel::core
