// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/SessionExportService.h"
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUuid>

namespace sentinel::core {

SessionExportService::SessionExportService(QObject* parent) : QObject(parent) {}
SessionExportService::~SessionExportService() = default;

QString SessionExportService::exportSession(const QString& sessionId,
                                            const ExportConfig& config) const {
    QJsonObject exportData;
    exportData["sessionId"] = sessionId;
    exportData["exportedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    exportData["version"] = "1.0";

    if (conversationStore_) {
        QJsonArray messages;
        for (const auto& message : conversationStore_->loadMessages(sessionId)) {
            messages.append(QJsonObject{
                {"messageId", message.messageId},
                {"role", chatRoleName(message.role)},
                {"content", message.content},
                {"timestamp", message.timestampUtc.toString(Qt::ISODateWithMs)},
                {"status", static_cast<int>(message.status)},
            });
        }
        exportData["messages"] = messages;
        const auto summary = conversationStore_->loadSummaryMetadata(sessionId);
        exportData["summary"] = QJsonObject{
            {"text", summary.summaryText},
            {"timestamp", summary.summaryTimestampUtc.toString(Qt::ISODateWithMs)},
            {"readiness", summary.readinessState},
        };
    }

    if (config.redactSensitive) {
        exportData = redactSensitive(exportData);
    }

    QJsonDocument doc(exportData);
    return doc.toJson(QJsonDocument::Indented);
}

bool SessionExportService::importSession(const QString& jsonContent, QString& sessionId) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonContent.toUtf8());
    if (doc.isNull())
        return false;

    if (!doc.isObject())
        return false;
    QJsonObject data = doc.object();
    if (data.value("version").toString().isEmpty())
        return false;
    sessionId = data["sessionId"].toString();
    return !sessionId.isEmpty();
}

bool SessionExportService::importFromUrl(const QString& url, QString& sessionId) {
    const QUrl target(url);
    if (!target.isValid() ||
        (target.scheme() != QStringLiteral("http") && target.scheme() != QStringLiteral("https"))) {
        return false;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request;
    request.setUrl(target);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000);
    loop.exec();
    if (!timer.isActive()) {
        reply->abort();
        reply->deleteLater();
        return false;
    }
    timer.stop();
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return false;
    }
    const QString payload = QString::fromUtf8(reply->readAll());
    reply->deleteLater();
    return importSession(payload, sessionId);
}

QJsonObject SessionExportService::redactSensitive(const QJsonObject& data) const {
    QJsonObject redacted = data;
    for (const QString& key : {QStringLiteral("apiKey"), QStringLiteral("token"),
                               QStringLiteral("authorization"), QStringLiteral("password")}) {
        redacted.remove(key);
    }
    for (auto it = redacted.begin(); it != redacted.end(); ++it) {
        if (it.value().isObject()) {
            it.value() = redactSensitive(it.value().toObject());
        } else if (it.value().isArray()) {
            QJsonArray array;
            for (const auto& item : it.value().toArray()) {
                array.append(item.isObject() ? QJsonValue(redactSensitive(item.toObject())) : item);
            }
            it.value() = array;
        }
    }
    return redacted;
}

void SessionExportService::setConversationStore(IConversationStore* store) {
    conversationStore_ = store;
}

QString SessionExportService::generateSessionId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
}

} // namespace sentinel::core
