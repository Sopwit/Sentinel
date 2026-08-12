// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/platform/INotificationService.h"
#include "sentinel/core/platform/IPlatformService.h"
#include "sentinel/core/platform/ISystemIntegrationService.h"

#include <QString>

namespace sentinel::core {

class DefaultPlatformService final : public IPlatformService {
public:
    DefaultPlatformService() = default;
    ~DefaultPlatformService() override = default;

    QString platformName() const override;
};

class DefaultNotificationService final : public INotificationService {
public:
    DefaultNotificationService() = default;
    ~DefaultNotificationService() override = default;

    bool isAvailable() const override;
};

class DefaultSystemIntegrationService final : public ISystemIntegrationService {
public:
    DefaultSystemIntegrationService() = default;
    ~DefaultSystemIntegrationService() override = default;

    bool isAvailable() const override;
};

} // namespace sentinel::core
