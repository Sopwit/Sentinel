// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/auth/AuthService.h"
#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrlQuery>

namespace sentinel::core {

AuthService::AuthService(QObject* parent) : QObject(parent) {}
AuthService::~AuthService() = default;

bool AuthService::authenticate(const QString& provider) {
    if (!m_credentials.contains(provider)) {
        emit authenticationFailed(provider, "No credentials configured");
        return false;
    }
    emit authenticated(provider);
    return true;
}

void AuthService::setApiKey(const QString& provider, const QString& apiKey) {
    AuthCredentials creds;
    creds.provider = provider;
    creds.method = AuthMethod::ApiKey;
    creds.apiKey = apiKey;
    m_credentials[provider] = creds;
}

std::optional<AuthCredentials> AuthService::credentials(const QString& provider) const {
    auto it = m_credentials.find(provider);
    if (it == m_credentials.end())
        return std::nullopt;
    return it.value();
}

bool AuthService::hasCredentials(const QString& provider) const {
    return m_credentials.contains(provider);
}

void AuthService::removeCredentials(const QString& provider) {
    m_credentials.remove(provider);
}

QList<QString> AuthService::providers() const {
    return m_credentials.keys();
}

void AuthService::configureOAuth(const QString& provider, const OAuthConfig& config) {
    m_oauthConfigs[provider] = config;
}

bool AuthService::refreshToken(const QString& provider) {
    const auto configIt = m_oauthConfigs.constFind(provider);
    const auto credentialsIt = m_credentials.find(provider);
    if (configIt == m_oauthConfigs.constEnd() || credentialsIt == m_credentials.end() ||
        credentialsIt->refreshToken.isEmpty() || configIt->tokenUrl.isEmpty()) {
        emit authenticationFailed(provider,
                                  QStringLiteral("OAuth refresh configuration is incomplete."));
        return false;
    }

    QUrlQuery form;
    form.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    form.addQueryItem(QStringLiteral("refresh_token"), credentialsIt->refreshToken);
    if (!configIt->clientId.isEmpty())
        form.addQueryItem(QStringLiteral("client_id"), configIt->clientId);

    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl(configIt->tokenUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
    QNetworkReply* reply = manager.post(request, form.query(QUrl::FullyEncoded).toUtf8());
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000);
    loop.exec();
    if (!reply->isFinished() || reply->error() != QNetworkReply::NoError) {
        if (!reply->isFinished())
            reply->abort();
        const auto error = reply->errorString();
        reply->deleteLater();
        emit authenticationFailed(
            provider, error.isEmpty() ? QStringLiteral("OAuth refresh failed.") : error);
        return false;
    }
    const auto payload = reply->readAll();
    reply->deleteLater();
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit authenticationFailed(provider,
                                  QStringLiteral("OAuth token endpoint returned invalid JSON."));
        return false;
    }
    const auto object = document.object();
    const auto token = object.value(QStringLiteral("access_token")).toString();
    if (token.isEmpty()) {
        emit authenticationFailed(provider,
                                  QStringLiteral("OAuth response did not contain access_token."));
        return false;
    }
    credentialsIt->token = token;
    if (object.contains(QStringLiteral("refresh_token")))
        credentialsIt->refreshToken = object.value(QStringLiteral("refresh_token")).toString();
    credentialsIt->expiresAt = QDateTime::currentDateTimeUtc().addSecs(
        object.value(QStringLiteral("expires_in")).toInt(3600));
    emit authenticated(provider);
    return true;
}

} // namespace sentinel::core
