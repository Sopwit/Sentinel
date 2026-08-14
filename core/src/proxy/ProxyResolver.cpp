// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/proxy/ProxyResolver.h"
#include <QUrl>
#include <QHostInfo>

namespace sentinel::core {

ProxyResolver::ProxyResolver() {
    m_config = resolveFromEnvironment();
}

ProxyConfig ProxyResolver::resolveFromEnvironment() const {
    ProxyConfig config;
    config.httpProxy = QString::fromUtf8(qgetenv("http_proxy"));
    config.httpsProxy = QString::fromUtf8(qgetenv("https_proxy"));
    config.noProxy = QString::fromUtf8(qgetenv("no_proxy"));
    config.ftpProxy = QString::fromUtf8(qgetenv("ftp_proxy"));
    return config;
}

ProxyConfig ProxyResolver::resolveFromSystem() const {
    return m_config;
}

QString ProxyResolver::proxyForUrl(const QString& url, const ProxyConfig& config) const {
    QUrl u(url);
    if (u.scheme() == "https") return config.httpsProxy;
    if (u.scheme() == "http") return config.httpProxy;
    if (u.scheme() == "ftp") return config.ftpProxy;
    return {};
}

bool ProxyResolver::shouldBypassProxy(const QString& host, const ProxyConfig& config) const {
    if (config.noProxy.isEmpty()) return false;
    QStringList bypassList = config.noProxy.split(',', Qt::SkipEmptyParts);
    for (const auto& entry : bypassList) {
        if (host == entry.trimmed()) return true;
        if (entry.trimmed() == "*") return true;
    }
    return false;
}

void ProxyResolver::setConfig(const ProxyConfig& config) { m_config = config; }
ProxyConfig ProxyResolver::config() const { return m_config; }

} // namespace sentinel::core
