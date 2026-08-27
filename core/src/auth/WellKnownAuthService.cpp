// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/auth/WellKnownAuthService.h"

namespace sentinel::core {

WellKnownConfig WellKnownAuthService::discover(const QString& issuerUrl) const {
    WellKnownConfig config;
    config.issuer = issuerUrl;
    config.authorizationEndpoint = issuerUrl + "/authorize";
    config.tokenEndpoint = issuerUrl + "/token";
    config.jwksUri = issuerUrl + "/.well-known/jwks.json";
    return config;
}

bool WellKnownAuthService::validateIssuer(const QString& issuerUrl,
                                          const WellKnownConfig& config) const {
    return config.issuer == issuerUrl;
}

QString WellKnownAuthService::buildAuthUrl(const WellKnownConfig& config, const QString& clientId,
                                           const QString& redirectUri, const QString& state) const {
    return QStringLiteral("%1?client_id=%2&redirect_uri=%3&state=%4&response_type=code")
        .arg(config.authorizationEndpoint, clientId, redirectUri, state);
}

} // namespace sentinel::core
