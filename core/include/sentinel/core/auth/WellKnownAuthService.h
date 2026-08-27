// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

struct WellKnownConfig {
    QString issuer;
    QString authorizationEndpoint;
    QString tokenEndpoint;
    QString jwksUri;
    QJsonObject metadata;
};

class WellKnownAuthService {
public:
    WellKnownConfig discover(const QString& issuerUrl) const;
    bool validateIssuer(const QString& issuerUrl, const WellKnownConfig& config) const;
    QString buildAuthUrl(const WellKnownConfig& config, const QString& clientId,
                         const QString& redirectUri, const QString& state) const;
};

} // namespace sentinel::core
