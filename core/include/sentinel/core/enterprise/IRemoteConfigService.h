// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core {

struct RemoteConfig {
    QString version;
    QJsonObject settings;
    QJsonObject features;
    QDateTime fetchedAt;
};

class IRemoteConfigService {
public:
    virtual ~IRemoteConfigService() = default;

    virtual bool fetchConfig(const QString& url) = 0;
    virtual RemoteConfig config() const = 0;
    virtual bool isStale() const = 0;
    virtual void setAutoRefresh(bool enabled) = 0;
    virtual void setRefreshInterval(int seconds) = 0;
    virtual bool featureEnabled(const QString& feature) const = 0;
};

} // namespace sentinel::core
