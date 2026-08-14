// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>
#include <functional>

namespace sentinel::core {

enum class AuthMethod : std::uint8_t {
    ApiKey,
    OAuthDeviceCode,
    WellKnownToken
};

struct AuthCredentials {
    QString provider;
    AuthMethod method;
    QString apiKey;
    QString token;
    QString refreshToken;
    QDateTime expiresAt;
};

struct OAuthConfig {
    QString clientId;
    QString deviceCodeUrl;
    QString tokenUrl;
    int pollingIntervalMs{5000};
    int maxAttempts{60};
};

class IAuthService {
public:
    virtual ~IAuthService() = default;

    virtual bool authenticate(const QString& provider) = 0;
    virtual void setApiKey(const QString& provider, const QString& apiKey) = 0;
    virtual std::optional<AuthCredentials> credentials(const QString& provider) const = 0;
    virtual bool hasCredentials(const QString& provider) const = 0;
    virtual void removeCredentials(const QString& provider) = 0;
    virtual QList<QString> providers() const = 0;
    virtual void configureOAuth(const QString& provider, const OAuthConfig& config) = 0;
    virtual bool refreshToken(const QString& provider) = 0;
};

} // namespace sentinel::core
