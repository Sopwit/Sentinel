// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

namespace sentinel::core {

struct UpdateInfo {
    bool available{false};
    QString currentVersion;
    QString latestVersion;
    QString downloadUrl;
    QString changelog;
    QDateTime releaseDate;
    QString sha256;
};

class IAutoUpdateService {
public:
    virtual ~IAutoUpdateService() = default;

    virtual UpdateInfo checkForUpdates() const = 0;
    virtual bool downloadUpdate(const QString& version) = 0;
    virtual bool applyUpdate() = 0;
    virtual void setAutoCheck(bool enabled) = 0;
    virtual bool autoCheckEnabled() const = 0;
    virtual QString currentVersion() const = 0;
};

} // namespace sentinel::core
