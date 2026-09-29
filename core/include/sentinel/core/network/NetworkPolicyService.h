// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QHostAddress>
#include <QString>
#include <QUrl>
#include <atomic>

namespace sentinel::core {

enum class NetworkMode { Online, Offline, LocalOnly };
enum class NetworkDecision { Allowed, Offline, LocalOnly, InvalidEndpoint };

// One process-wide effective policy for product network transports. This is distinct from
// tool authorization and filesystem permissions; callers must still apply those gates.
class NetworkPolicyService final {
public:
    static NetworkPolicyService& instance() {
        static NetworkPolicyService policy;
        return policy;
    }

    NetworkMode mode() const { return mode_.load(); }
    void setMode(NetworkMode mode) { mode_.store(mode); }

    NetworkDecision check(const QUrl& url) const {
        if (!url.isValid() || url.host().isEmpty()) return NetworkDecision::InvalidEndpoint;
        if (mode() == NetworkMode::Online) return NetworkDecision::Allowed;
        QHostAddress address;
        if (url.host().compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0 ||
            (address.setAddress(url.host()) && address.isLoopback()))
            return NetworkDecision::Allowed;
        return mode() == NetworkMode::Offline ? NetworkDecision::Offline
                                               : NetworkDecision::LocalOnly;
    }

    static QString code(NetworkDecision decision) {
        switch (decision) {
        case NetworkDecision::Allowed: return {};
        case NetworkDecision::Offline: return QStringLiteral("Offline");
        case NetworkDecision::LocalOnly: return QStringLiteral("LocalOnly");
        case NetworkDecision::InvalidEndpoint: return QStringLiteral("InvalidEndpoint");
        }
        return QStringLiteral("InvalidEndpoint");
    }

private:
    NetworkPolicyService() = default;
    std::atomic<NetworkMode> mode_{NetworkMode::Online};
};

} // namespace sentinel::core
