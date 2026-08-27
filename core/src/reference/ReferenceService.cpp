// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/reference/ReferenceService.h"
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTextStream>
#include <QTimer>

namespace sentinel::core {

ReferenceService::ReferenceService(QObject* parent) : QObject(parent) {}

ReferenceService::~ReferenceService() = default;

bool ReferenceService::addReference(const Reference& ref) {
    if (!ref.isValid()) {
        return false;
    }

    if (m_references.contains(ref.name)) {
        return false;
    }

    Reference newRef = ref;
    newRef.isAvailable = validateReference(ref);

    m_references[ref.name] = newRef;
    emit referenceAdded(ref.name);
    return true;
}

bool ReferenceService::removeReference(const QString& name) {
    auto it = m_references.find(name);
    if (it == m_references.end()) {
        return false;
    }

    m_references.erase(it);
    emit referenceRemoved(name);
    return true;
}

bool ReferenceService::updateReference(const Reference& ref) {
    if (!ref.isValid()) {
        return false;
    }

    auto it = m_references.find(ref.name);
    if (it == m_references.end()) {
        return false;
    }

    Reference updatedRef = ref;
    updatedRef.isAvailable = validateReference(ref);

    m_references[ref.name] = updatedRef;
    emit referenceUpdated(ref.name);
    return true;
}

QList<Reference> ReferenceService::references() const {
    return m_references.values();
}

std::optional<Reference> ReferenceService::findReference(const QString& name) const {
    auto it = m_references.find(name);
    if (it == m_references.end()) {
        return std::nullopt;
    }
    return it.value();
}

QString ReferenceService::getReferenceContent(const QString& name) const {
    auto ref = findReference(name);
    if (!ref || !ref->isAvailable) {
        return {};
    }

    switch (ref->type) {
    case ReferenceType::LocalPath:
        return readLocalContent(ref->path);
    case ReferenceType::Url:
        return readUrlContent(ref->path);
    case ReferenceType::Repository:
        return readRepositoryContent(ref->path);
    }

    return {};
}

QString ReferenceService::getReferenceContent(const QString& name, const QString& query) const {
    const auto content = getReferenceContent(name);
    const auto needle = query.simplified();
    if (needle.isEmpty() || content.isEmpty()) {
        return content;
    }
    const auto lines = content.split('\n');
    QStringList matches;
    for (const auto& line : lines) {
        if (line.contains(needle, Qt::CaseInsensitive)) {
            matches.append(line);
        }
    }
    return matches.join('\n');
}

int ReferenceService::loadReferencesFromConfig(const QJsonArray& refsArray) {
    int loadedCount = 0;

    for (const auto& refValue : refsArray) {
        QJsonObject refObj = refValue.toObject();
        Reference ref;
        ref.name = refObj["name"].toString();
        ref.description = refObj["description"].toString();
        ref.path = refObj["path"].toString();

        QString typeStr = refObj["type"].toString().toLower();
        if (typeStr == "repository" || typeStr == "repo") {
            ref.type = ReferenceType::Repository;
        } else if (typeStr == "url") {
            ref.type = ReferenceType::Url;
        } else {
            ref.type = ReferenceType::LocalPath;
        }

        if (ref.isValid()) {
            ref.isAvailable = validateReference(ref);
            m_references[ref.name] = ref;
            loadedCount++;
        }
    }

    return loadedCount;
}

bool ReferenceService::validateReference(const Reference& ref) const {
    switch (ref.type) {
    case ReferenceType::LocalPath: {
        QFileInfo info(ref.path);
        return info.exists();
    }
    case ReferenceType::Url: {
        QUrl url(ref.path);
        return url.isValid();
    }
    case ReferenceType::Repository: {
        QFileInfo info(ref.path);
        return info.exists() && info.isDir();
    }
    }

    return false;
}

void ReferenceService::refreshAvailability() {
    for (auto it = m_references.begin(); it != m_references.end(); ++it) {
        bool wasAvailable = it->isAvailable;
        it->isAvailable = validateReference(it.value());

        if (wasAvailable != it->isAvailable) {
            emit referenceAvailabilityChanged(it.key(), it->isAvailable);
        }
    }
}

QString ReferenceService::readLocalContent(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QTextStream stream(&file);
    return stream.readAll();
}

QString ReferenceService::readUrlContent(const QString& url) const {
    QNetworkRequest request{QUrl(url)};
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QNetworkReply* reply = m_networkManager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000);
    loop.exec();
    if (!reply->isFinished() || reply->error() != QNetworkReply::NoError) {
        if (!reply->isFinished())
            reply->abort();
        reply->deleteLater();
        return {};
    }
    const auto content = QString::fromUtf8(reply->readAll());
    reply->deleteLater();
    return content;
}

QString ReferenceService::readRepositoryContent(const QString& repoPath) const {
    // Read key files from repository
    QDir dir(repoPath);
    if (!dir.exists()) {
        return {};
    }

    QString content;
    QStringList importantFiles = {"README.md", "README", "CONTRIBUTING.md", "ARCHITECTURE.md"};

    for (const QString& fileName : importantFiles) {
        QString filePath = dir.filePath(fileName);
        if (QFile::exists(filePath)) {
            content += QStringLiteral("## %1\n\n").arg(fileName);
            content += readLocalContent(filePath);
            content += "\n\n";
        }
    }

    return content;
}

} // namespace sentinel::core
