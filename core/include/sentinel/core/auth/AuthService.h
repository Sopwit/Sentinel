// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/auth/IAuthService.h"
#include <QObject>
#include <QMap>

namespace sentinel::core {

class AuthService : public QObject, public IAuthService {
    Q_OBJECT
public:
    explicit AuthService(QObject* parent = nullptr);
    ~AuthService() override;

    bool authenticate(const QString& provider) override;
    void setApiKey(const QString& provider, const QString& apiKey) override;
    std::optional<AuthCredentials> credentials(const QString& provider) const override;
    bool hasCredentials(const QString& provider) const override;
    void removeCredentials(const QString& provider) override;
    QList<QString> providers() const override;
    void configureOAuth(const QString& provider, const OAuthConfig& config) override;
    bool refreshToken(const QString& provider) override;

signals:
    void authenticated(const QString& provider);
    void authenticationFailed(const QString& provider, const QString& error);

private:
    QMap<QString, AuthCredentials> m_credentials;
    QMap<QString, OAuthConfig> m_oauthConfigs;
};

} // namespace sentinel::core
