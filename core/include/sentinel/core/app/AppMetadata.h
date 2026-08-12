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
