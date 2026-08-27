// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMap>
#include <QString>

namespace sentinel::core {

struct ProxyConfig {
    QString httpProxy;
    QString httpsProxy;
    QString noProxy;
    QString ftpProxy;
    bool useSystemProxy{true};
};

class ProxyResolver {
public:
    ProxyResolver();

    ProxyConfig resolveFromEnvironment() const;
    ProxyConfig resolveFromSystem() const;
    QString proxyForUrl(const QString& url, const ProxyConfig& config) const;
    bool shouldBypassProxy(const QString& host, const ProxyConfig& config) const;
    void setConfig(const ProxyConfig& config);
    ProxyConfig config() const;

private:
    ProxyConfig m_config;
};

} // namespace sentinel::core
