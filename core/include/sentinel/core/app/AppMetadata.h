// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

namespace sentinel::core {

struct AppMetadata {
    static QString appId();
    static QString displayName();
    // Application name used before the product was renamed to "Sentinel". Kept
    // solely so existing installs can migrate their per-application storage roots.
    static QString legacyDisplayName();
    static QString version();
    static QString projectVersion();
    static QString buildNumber();
    static QString gitCommit();
    static QString buildType();
    static QString platform();
    static QString architecture();
    static QString organizationName();
    static QString copyright();
    static QStringList safeBuildSummaries();
};

} // namespace sentinel::core
