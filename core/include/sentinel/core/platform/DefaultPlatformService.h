// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/IPlugin.h"
#include "sentinel/core/app/IIntegration.h"
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

class DefaultIntegration final : public IIntegration {
public:
    explicit DefaultIntegration(QString id = QStringLiteral("default_integration"),
                               QString displayName = QStringLiteral("Default Integration"),
                               bool available = true);
    ~DefaultIntegration() override = default;

    QString id() const override;
    QString displayName() const override;
    bool isAvailable() const override;

private:
    QString m_id;
    QString m_displayName;
    bool m_available{true};
};

class DefaultPlugin final : public IPlugin {
public:
    explicit DefaultPlugin(QString id = QStringLiteral("default_plugin"),
                           QString displayName = QStringLiteral("Default Core Plugin"));
    ~DefaultPlugin() override = default;

    QString id() const override;
    QString displayName() const override;
    void initialize() override;
    void shutdown() override;

    bool isInitialized() const;

private:
    QString m_id;
    QString m_displayName;
    bool m_initialized{false};
};

} // namespace sentinel::core
