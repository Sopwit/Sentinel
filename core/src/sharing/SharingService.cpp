// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/sharing/SharingService.h"
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

namespace sentinel::core {

SharingService::SharingService(QObject* parent)
    : QObject(parent)
{
    connect(&m_syncTimer, &QTimer::timeout, this, &SharingService::performAutoSync);
}

SharingService::~SharingService() = default;

ShareRecord SharingService::createShare(const QString& sessionId) {
    if (!m_config.enabled || m_config.apiUrl.trimmed().isEmpty()) {
        emit shareError({}, QStringLiteral("Sharing is disabled or no sharing API is configured."));
        return {};
    }
    ShareRecord record = createShareRecord(sessionId);

    QNetworkRequest request(QUrl(m_config.apiUrl + QStringLiteral("/shares")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_config.authToken.isEmpty()) {
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_config.authToken).toUtf8());
    }
    const QJsonObject body{{QStringLiteral("sessionId"), sessionId},
                           {QStringLiteral("secret"), record.secret}};
    QNetworkReply* reply = m_networkManager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000);
    loop.exec();
    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        emit shareError({}, QStringLiteral("Sharing API request timed out."));
        return {};
    }
    const auto payload = reply->readAll();
    const auto error = reply->error();
    reply->deleteLater();
    if (error != QNetworkReply::NoError) {
        emit shareError({}, QStringLiteral("Sharing API request failed."));
        return {};
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit shareError({}, QStringLiteral("Sharing API returned invalid JSON."));
        return {};
    }
    const auto response = document.object();
    record.id = response.value(QStringLiteral("id")).toString();
    record.url = response.value(QStringLiteral("url")).toString();
    if (record.id.isEmpty() || record.url.isEmpty()) {
        emit shareError({}, QStringLiteral("Sharing API response did not contain id and url."));
        return {};
    }

    m_shares[record.id] = record;
    emit shareCreated(record.id, record.url);

    qDebug() << QStringLiteral("SharingService: Created share '%1' for session '%2'")
                    .arg(record.id, sessionId);

    return record;
}

bool SharingService::deleteShare(const QString& shareId) {
    auto it = m_shares.find(shareId);
    if (it == m_shares.end()) {
        return false;
    }

    if (m_config.enabled && !m_config.apiUrl.trimmed().isEmpty()) {
        QNetworkRequest request(QUrl(m_config.apiUrl + QStringLiteral("/shares/%1").arg(shareId)));
        if (!m_config.authToken.isEmpty()) {
            request.setRawHeader("Authorization",
                                 QStringLiteral("Bearer %1").arg(m_config.authToken).toUtf8());
        }
        QNetworkReply* reply = m_networkManager.deleteResource(request);
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(15000);
        loop.exec();
        const bool success = reply->isFinished() && reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        if (!success) {
            emit shareError(shareId, QStringLiteral("Sharing API deletion failed."));
            return false;
        }
    }
    m_shares.erase(it);
    emit shareDeleted(shareId);
    return true;
}

std::optional<ShareRecord> SharingService::findShare(const QString& shareId) const {
    auto it = m_shares.find(shareId);
    if (it == m_shares.end()) {
        return std::nullopt;
    }
    return it.value();
}

QList<ShareRecord> SharingService::shares() const {
    return m_shares.values();
}

std::optional<ShareRecord> SharingService::shareBySession(const QString& sessionId) const {
    for (const auto& share : m_shares) {
        if (share.sessionId == sessionId) {
            return share;
        }
    }
    return std::nullopt;
}

bool SharingService::syncShare(const QString& shareId) {
    auto it = m_shares.find(shareId);
    if (it == m_shares.end()) {
        return false;
    }

    if (!m_config.enabled || m_config.apiUrl.trimmed().isEmpty()) {
        emit shareError(shareId, QStringLiteral("Sharing is disabled or no sharing API is configured."));
        return false;
    }
    QNetworkRequest request(QUrl(m_config.apiUrl + QStringLiteral("/shares/%1").arg(shareId)));
    if (!m_config.authToken.isEmpty()) {
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_config.authToken).toUtf8());
    }
    QNetworkReply* reply = m_networkManager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000);
    loop.exec();
    const bool success = reply->isFinished() && reply->error() == QNetworkReply::NoError;
    reply->deleteLater();
    if (!success) {
        emit shareError(shareId, QStringLiteral("Sharing API synchronization failed."));
        return false;
    }
    it->lastSyncedAt = QDateTime::currentDateTime();

    emit shareSynced(shareId);
    return true;
}

void SharingService::enableAutoSync(bool enabled) {
    if (enabled) {
        m_syncTimer.start(m_config.syncIntervalMs);
    } else {
        m_syncTimer.stop();
    }
}

bool SharingService::isAutoSyncEnabled() const {
    return m_syncTimer.isActive();
}

void SharingService::configure(const ShareConfig& config) {
    m_config = config;

    if (m_config.enabled && m_config.autoSync) {
        m_syncTimer.start(m_config.syncIntervalMs);
    } else {
        m_syncTimer.stop();
    }
}

ShareConfig SharingService::config() const {
    return m_config;
}

void SharingService::performAutoSync() {
    if (!m_config.enabled) {
        return;
    }

    for (auto it = m_shares.begin(); it != m_shares.end(); ++it) {
        if (it->isActive) {
            syncShare(it.key());
        }
    }
}

QString SharingService::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
}

QString SharingService::generateSecret() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

ShareRecord SharingService::createShareRecord(const QString& sessionId) {
    ShareRecord record;
    record.id = generateId();
    record.sessionId = sessionId;
    record.secret = generateSecret();
    record.createdAt = QDateTime::currentDateTime();
    record.isActive = true;
    return record;
}

} // namespace sentinel::core
