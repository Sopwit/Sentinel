// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/enterprise/IRemoteConfigService.h"
#include <QObject>

namespace sentinel::core {

class RemoteConfigService : public QObject, public IRemoteConfigService {
    Q_OBJECT
public:
    explicit RemoteConfigService(QObject* parent = nullptr);
    ~RemoteConfigService() override;

    bool fetchConfig(const QString& url) override;
    RemoteConfig config() const override;
    bool isStale() const override;
    void setAutoRefresh(bool enabled) override;
    void setRefreshInterval(int seconds) override;
    bool featureEnabled(const QString& feature) const override;

private:
    RemoteConfig m_config;
    bool m_autoRefresh{false};
    int m_refreshInterval{300};
};

} // namespace sentinel::core
