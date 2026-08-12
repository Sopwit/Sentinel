// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/platform/DefaultPlatformService.h"

#include <QSysInfo>

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

} // namespace sentinel::core
