// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <optional>
#include <memory>

namespace sentinel::core {

struct GpuDeviceFacts {
    QString name;
    QString backend;
    std::optional<qint64> reportedVramBytes;
};

struct HardwareFacts {
    QString operatingSystem;
    QString cpuArchitecture;
    int logicalCpuCount = 0;
    std::optional<int> physicalCpuCount;
    std::optional<qint64> systemRamBytes;
    QList<GpuDeviceFacts> gpus;
    QStringList availableBackends;
};

class IHardwarePlatformAdapter {
public:
    virtual ~IHardwarePlatformAdapter() = default;
    virtual HardwareFacts collect() const = 0;
};

class HardwareCapabilityService final {
public:
    explicit HardwareCapabilityService(std::unique_ptr<IHardwarePlatformAdapter> adapter = {});
    const HardwareFacts& facts() const;
    void refresh();

private:
    std::unique_ptr<IHardwarePlatformAdapter> adapter_;
    HardwareFacts facts_;
};

} // namespace sentinel::core
