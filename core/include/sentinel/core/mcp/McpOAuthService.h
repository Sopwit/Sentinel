// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QMap>
#include <QString>

namespace sentinel::core {

struct McpOAuthConfig {
    QString clientId;
    QString authorizationUrl;
    QString tokenUrl;
    QString redirectUri;
    int pollingIntervalMs{5000};
    int maxAttempts{60};
};

struct McpOAuthToken {
    QString accessToken;
    QString refreshToken;
    QString tokenType;
    int expiresIn{0};
    QString scope;
};

class McpOAuthService {
public:
    McpOAuthService();

    bool startDeviceFlow(const McpOAuthConfig& config, QString& deviceCode, QString& userCode);
    bool pollForToken(const QString& deviceCode, McpOAuthToken& token);
    bool refreshToken(const QString& refreshToken, McpOAuthToken& token);
    void storeToken(const QString& serverUrl, const McpOAuthToken& token);
    std::optional<McpOAuthToken> getStoredToken(const QString& serverUrl) const;
    void clearToken(const QString& serverUrl);

private:
    QMap<QString, McpOAuthToken> m_storedTokens;
};

} // namespace sentinel::core
