// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/AutoUpdateService.h"

#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

namespace sentinel::core {

AutoUpdateService::AutoUpdateService(QUrl manifestUrl, QObject* parent)
    : QObject(parent), m_manifestUrl(std::move(manifestUrl)) {
    if (!m_manifestUrl.isValid()) {
        m_manifestUrl = QUrl(qEnvironmentVariable("SENTINEL_UPDATE_MANIFEST"));
    }
}
AutoUpdateService::~AutoUpdateService() = default;

UpdateInfo AutoUpdateService::checkForUpdates() const {
    UpdateInfo info;
    info.currentVersion = currentVersion();
    if (!m_manifestUrl.isValid()) return info;
    QNetworkAccessManager manager;
    QNetworkRequest request;
    request.setUrl(m_manifestUrl);
    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(10000);
    loop.exec();
    if (timer.isActive() && reply->error() == QNetworkReply::NoError) {
        const QJsonObject object = QJsonDocument::fromJson(reply->readAll()).object();
        info.latestVersion = object.value(QStringLiteral("version")).toString();
        info.downloadUrl = object.value(QStringLiteral("downloadUrl")).toString();
        info.changelog = object.value(QStringLiteral("changelog")).toString();
        info.releaseDate = QDateTime::fromString(object.value(QStringLiteral("releaseDate")).toString(), Qt::ISODate);
        info.sha256 = object.value(QStringLiteral("sha256")).toString().trimmed().toLower();
        info.available = !info.latestVersion.isEmpty() && info.latestVersion != info.currentVersion;
    }
    reply->deleteLater();
    return info;
}

bool AutoUpdateService::downloadUpdate(const QString& version) {
    const UpdateInfo info = checkForUpdates();
    if (info.latestVersion != version || info.downloadUrl.isEmpty()) return false;
    QNetworkAccessManager manager;
    QNetworkRequest downloadRequest;
    downloadRequest.setUrl(QUrl(info.downloadUrl));
    QNetworkReply* reply = manager.get(downloadRequest);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() != QNetworkReply::NoError) { reply->deleteLater(); return false; }
    const QByteArray artifact = reply->readAll();
    reply->deleteLater();
    if (info.sha256.isEmpty() ||
        QString::fromLatin1(QCryptographicHash::hash(artifact, QCryptographicHash::Sha256).toHex()) != info.sha256) {
        return false;
    }
    const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                             .filePath(QStringLiteral("sentinel-update-%1.bin").arg(version));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(artifact) != artifact.size()) return false;
    file.close();
    m_downloadedArtifact = path;
    return true;
}

bool AutoUpdateService::applyUpdate() {
    const QString updater = qEnvironmentVariable("SENTINEL_UPDATER_COMMAND");
    if (m_downloadedArtifact.isEmpty() || updater.isEmpty() || !QFileInfo::exists(m_downloadedArtifact)) return false;
    return QProcess::startDetached(updater, {m_downloadedArtifact});
}

void AutoUpdateService::setAutoCheck(bool enabled) { m_autoCheck = enabled; }
bool AutoUpdateService::autoCheckEnabled() const { return m_autoCheck; }
QString AutoUpdateService::currentVersion() const { return "1.0.0"; }

void AutoUpdateService::setManifestUrl(const QUrl& url) { m_manifestUrl = url; }

} // namespace sentinel::core
