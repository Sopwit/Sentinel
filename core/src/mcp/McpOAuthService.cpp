// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/mcp/McpOAuthService.h"
#include <QUuid>

namespace sentinel::core {

McpOAuthService::McpOAuthService() = default;

bool McpOAuthService::startDeviceFlow(const McpOAuthConfig& config, QString& deviceCode,
                                      QString& userCode) {
    Q_UNUSED(config)
    deviceCode = QUuid::createUuid().toString(QUuid::WithoutBraces).left(16);
    userCode = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8).toUpper();
    return true;
}

bool McpOAuthService::pollForToken(const QString& deviceCode, McpOAuthToken& token) {
    Q_UNUSED(deviceCode)
    token.accessToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
    token.tokenType = "Bearer";
    token.expiresIn = 3600;
    return true;
}

bool McpOAuthService::refreshToken(const QString& refreshToken, McpOAuthToken& token) {
    Q_UNUSED(refreshToken)
    token.accessToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
    token.tokenType = "Bearer";
    token.expiresIn = 3600;
    return true;
}

void McpOAuthService::storeToken(const QString& serverUrl, const McpOAuthToken& token) {
    m_storedTokens[serverUrl] = token;
}

std::optional<McpOAuthToken> McpOAuthService::getStoredToken(const QString& serverUrl) const {
    auto it = m_storedTokens.find(serverUrl);
    if (it == m_storedTokens.end())
        return std::nullopt;
    return it.value();
}

void McpOAuthService::clearToken(const QString& serverUrl) {
    m_storedTokens.remove(serverUrl);
}

} // namespace sentinel::core
