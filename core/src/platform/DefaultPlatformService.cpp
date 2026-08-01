// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/platform/DefaultPlatformService.h"

#include <QSysInfo>
#include <utility>

namespace sentinel::core {

QString DefaultPlatformService::platformName() const {
#if defined(Q_OS_LINUX)
    return QStringLiteral("Linux Desktop (%1)").arg(QSysInfo::prettyProductName());
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macOS (%1)").arg(QSysInfo::prettyProductName());
#elif defined(Q_OS_WIN)
    return QStringLiteral("Windows (%1)").arg(QSysInfo::prettyProductName());
#else
    return QStringLiteral("Generic Desktop");
#endif
}

bool DefaultNotificationService::isAvailable() const {
    return true;
}

bool DefaultSystemIntegrationService::isAvailable() const {
    return true;
}

DefaultIntegration::DefaultIntegration(QString id, QString displayName, bool available)
    : m_id(std::move(id)), m_displayName(std::move(displayName)), m_available(available) {}

QString DefaultIntegration::id() const {
    return m_id;
}

QString DefaultIntegration::displayName() const {
    return m_displayName;
}

bool DefaultIntegration::isAvailable() const {
    return m_available;
}

DefaultPlugin::DefaultPlugin(QString id, QString displayName)
    : m_id(std::move(id)), m_displayName(std::move(displayName)) {}

QString DefaultPlugin::id() const {
    return m_id;
}

QString DefaultPlugin::displayName() const {
    return m_displayName;
}

void DefaultPlugin::initialize() {
    m_initialized = true;
}

void DefaultPlugin::shutdown() {
    m_initialized = false;
}

bool DefaultPlugin::isInitialized() const {
    return m_initialized;
}

} // namespace sentinel::core
