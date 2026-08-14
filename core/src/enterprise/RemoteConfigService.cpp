// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/enterprise/RemoteConfigService.h"

namespace sentinel::core {

RemoteConfigService::RemoteConfigService(QObject* parent) : QObject(parent) {}
RemoteConfigService::~RemoteConfigService() = default;

bool RemoteConfigService::fetchConfig(const QString& url) {
    Q_UNUSED(url)
    m_config.fetchedAt = QDateTime::currentDateTime();
    return true;
}

RemoteConfig RemoteConfigService::config() const { return m_config; }
bool RemoteConfigService::isStale() const {
    return m_config.fetchedAt.secsTo(QDateTime::currentDateTime()) > m_refreshInterval;
}
void RemoteConfigService::setAutoRefresh(bool enabled) { m_autoRefresh = enabled; }
void RemoteConfigService::setRefreshInterval(int seconds) { m_refreshInterval = seconds; }

bool RemoteConfigService::featureEnabled(const QString& feature) const {
    return m_config.features[feature].toBool();
}

} // namespace sentinel::core
