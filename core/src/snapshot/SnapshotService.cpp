// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/snapshot/SnapshotService.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>

namespace sentinel::core {

SnapshotService::SnapshotService(QObject* parent) : QObject(parent) {
    m_checkTimer.setInterval(5000); // Check every 5 seconds
    connect(&m_checkTimer, &QTimer::timeout, this, &SnapshotService::checkForChanges);
}

SnapshotService::~SnapshotService() = default;

QString SnapshotService::createSnapshot(const QString& description) {
    Snapshot snapshot;
    snapshot.id = generateSnapshotId();
    snapshot.description = description;
    snapshot.timestamp = QDateTime::currentDateTime();
    snapshot.parentSnapshotId = m_currentSnapshotId;

    // Capture current state of all tracked files
    for (const QString& filePath : m_trackedFiles) {
        FileSnapshot fileSnap;
        fileSnap.filePath = filePath;
        fileSnap.checksum = calculateChecksum(filePath);
        fileSnap.content = readFileContent(filePath);
        fileSnap.timestamp = QDateTime::currentDateTime();

        snapshot.files[filePath] = fileSnap;
    }

    // Save snapshot
    if (!saveSnapshotToFile(snapshot)) {
        qWarning()
            << QStringLiteral("SnapshotService: Failed to save snapshot '%1'").arg(snapshot.id);
        return {};
    }

    m_snapshots[snapshot.id] = snapshot;
    m_currentSnapshotId = snapshot.id;

    emit snapshotCreated(snapshot.id);
    qDebug() << QStringLiteral("SnapshotService: Created snapshot '%1' with %2 files")
                    .arg(snapshot.id)
                    .arg(snapshot.files.size());

    return snapshot.id;
}

bool SnapshotService::restoreSnapshot(const QString& snapshotId) {
    auto snapshot = findSnapshot(snapshotId);
    if (!snapshot) {
        return false;
    }

    // Restore all files from snapshot
    for (auto it = snapshot->files.begin(); it != snapshot->files.end(); ++it) {
        const FileSnapshot& fileSnap = it.value();
        if (!writeFileContent(fileSnap.filePath, fileSnap.content)) {
            qWarning() << QStringLiteral("SnapshotService: Failed to restore file '%1'")
                              .arg(fileSnap.filePath);
            return false;
        }
    }

    m_currentSnapshotId = snapshotId;
    emit snapshotRestored(snapshotId);
    qDebug() << QStringLiteral("SnapshotService: Restored snapshot '%1'").arg(snapshotId);

    return true;
}

bool SnapshotService::deleteSnapshot(const QString& snapshotId) {
    QString path = snapshotPath(snapshotId);
    if (QFile::exists(path)) {
        if (!QFile::remove(path)) {
            return false;
        }
    }

    m_snapshots.remove(snapshotId);

    if (m_currentSnapshotId == snapshotId) {
        m_currentSnapshotId.clear();
    }

    emit snapshotDeleted(snapshotId);
    return true;
}

void SnapshotService::trackFile(const QString& filePath) {
    if (!m_trackedFiles.contains(filePath)) {
        m_trackedFiles.append(filePath);
    }
}

void SnapshotService::untrackFile(const QString& filePath) {
    m_trackedFiles.removeAll(filePath);
}

QStringList SnapshotService::trackedFiles() const {
    return m_trackedFiles;
}

SnapshotDiff SnapshotService::diff(const QString& snapshotId) const {
    QString fromId = snapshotId.isEmpty() ? m_currentSnapshotId : snapshotId;
    if (fromId.isEmpty()) {
        return {};
    }

    auto fromSnapshot = findSnapshot(fromId);
    if (!fromSnapshot) {
        return {};
    }

    SnapshotDiff diff;

    // Check for modified and removed files
    for (auto it = fromSnapshot->files.begin(); it != fromSnapshot->files.end(); ++it) {
        const QString& filePath = it.key();
        const FileSnapshot& oldSnap = it.value();

        if (!QFile::exists(filePath)) {
            diff.removedFiles.append(filePath);
        } else {
            QByteArray currentChecksum = calculateChecksum(filePath);
            if (currentChecksum != oldSnap.checksum) {
                diff.modifiedFiles.append(filePath);
            }
        }
    }

    // Check for added files
    for (const QString& filePath : m_trackedFiles) {
        if (!fromSnapshot->files.contains(filePath)) {
            diff.addedFiles.append(filePath);
        }
    }

    return diff;
}

SnapshotDiff SnapshotService::diffBetween(const QString& fromId, const QString& toId) const {
    auto fromSnapshot = findSnapshot(fromId);
    auto toSnapshot = findSnapshot(toId);

    if (!fromSnapshot || !toSnapshot) {
        return {};
    }

    SnapshotDiff diff;

    // Check for modified and removed files
    for (auto it = fromSnapshot->files.begin(); it != fromSnapshot->files.end(); ++it) {
        const QString& filePath = it.key();
        const FileSnapshot& oldSnap = it.value();

        if (!toSnapshot->files.contains(filePath)) {
            diff.removedFiles.append(filePath);
        } else {
            const FileSnapshot& newSnap = toSnapshot->files[filePath];
            if (newSnap.checksum != oldSnap.checksum) {
                diff.modifiedFiles.append(filePath);
            }
        }
    }

    // Check for added files
    for (auto it = toSnapshot->files.begin(); it != toSnapshot->files.end(); ++it) {
        if (!fromSnapshot->files.contains(it.key())) {
            diff.addedFiles.append(it.key());
        }
    }

    return diff;
}

QList<Snapshot> SnapshotService::snapshots() const {
    return m_snapshots.values();
}

std::optional<Snapshot> SnapshotService::findSnapshot(const QString& snapshotId) const {
    auto it = m_snapshots.find(snapshotId);
    if (it == m_snapshots.end()) {
        // Try loading from file
        auto loaded = loadSnapshotFromFile(snapshotId);
        if (loaded) {
            return loaded;
        }
        return std::nullopt;
    }
    return it.value();
}

QString SnapshotService::currentSnapshotId() const {
    return m_currentSnapshotId;
}

void SnapshotService::enableAutoSave(bool enabled) {
    m_autoSaveEnabled = enabled;
    if (enabled) {
        m_checkTimer.start();
    } else {
        m_checkTimer.stop();
    }
}

bool SnapshotService::isAutoSaveEnabled() const {
    return m_autoSaveEnabled;
}

void SnapshotService::setWorkingDirectory(const QString& dir) {
    m_workingDirectory = dir;
}

QString SnapshotService::workingDirectory() const {
    return m_workingDirectory;
}

void SnapshotService::checkForChanges() {
    for (const QString& filePath : m_trackedFiles) {
        if (!m_currentSnapshotId.isEmpty()) {
            auto currentSnap = findSnapshot(m_currentSnapshotId);
            if (currentSnap && currentSnap->files.contains(filePath)) {
                QByteArray oldChecksum = currentSnap->files[filePath].checksum;
                QByteArray newChecksum = calculateChecksum(filePath);
                if (oldChecksum != newChecksum) {
                    emit fileModified(filePath);
                }
            }
        }
    }
}

QByteArray SnapshotService::calculateChecksum(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    if (!hash.addData(&file)) {
        return {};
    }

    return hash.result();
}

QByteArray SnapshotService::readFileContent(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

bool SnapshotService::writeFileContent(const QString& filePath, const QByteArray& content) const {
    QFileInfo info(filePath);
    QDir dir = info.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    return file.write(content) == content.size();
}

QString SnapshotService::generateSnapshotId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

QString SnapshotService::snapshotPath(const QString& snapshotId) const {
    QString baseDir = m_workingDirectory.isEmpty()
                          ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                          : m_workingDirectory;

    return QStringLiteral("%1/snapshots/%2.json").arg(baseDir, snapshotId);
}

bool SnapshotService::saveSnapshotToFile(const Snapshot& snapshot) const {
    QJsonObject root;
    root["id"] = snapshot.id;
    root["description"] = snapshot.description;
    root["timestamp"] = snapshot.timestamp.toString(Qt::ISODate);
    root["parentSnapshotId"] = snapshot.parentSnapshotId;

    QJsonObject filesObj;
    for (auto it = snapshot.files.begin(); it != snapshot.files.end(); ++it) {
        QJsonObject fileObj;
        fileObj["filePath"] = it.value().filePath;
        fileObj["checksum"] = QString::fromLatin1(it.value().checksum.toHex());
        fileObj["content"] = QString::fromUtf8(it.value().content);
        fileObj["timestamp"] = it.value().timestamp.toString(Qt::ISODate);
        filesObj[it.key()] = fileObj;
    }
    root["files"] = filesObj;

    QString path = snapshotPath(snapshot.id);
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument doc(root);
    return file.write(doc.toJson()) > 0;
}

std::optional<Snapshot> SnapshotService::loadSnapshotFromFile(const QString& snapshotId) const {
    QString path = snapshotPath(snapshotId);
    if (!QFile::exists(path)) {
        return std::nullopt;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return std::nullopt;
    }

    QJsonObject root = doc.object();
    Snapshot snapshot;
    snapshot.id = root["id"].toString();
    snapshot.description = root["description"].toString();
    snapshot.timestamp = QDateTime::fromString(root["timestamp"].toString(), Qt::ISODate);
    snapshot.parentSnapshotId = root["parentSnapshotId"].toString();

    QJsonObject filesObj = root["files"].toObject();
    for (auto it = filesObj.begin(); it != filesObj.end(); ++it) {
        QJsonObject fileObj = it.value().toObject();
        FileSnapshot fileSnap;
        fileSnap.filePath = fileObj["filePath"].toString();
        fileSnap.checksum = QByteArray::fromHex(fileObj["checksum"].toString().toLatin1());
        fileSnap.content = fileObj["content"].toString().toUtf8();
        fileSnap.timestamp = QDateTime::fromString(fileObj["timestamp"].toString(), Qt::ISODate);
        snapshot.files[it.key()] = fileSnap;
    }

    return snapshot;
}

} // namespace sentinel::core
