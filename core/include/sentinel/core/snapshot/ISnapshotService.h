// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QMap>

namespace sentinel::core {

struct FileSnapshot {
    QString filePath;
    QByteArray checksum;
    QByteArray content;
    QDateTime timestamp;
    bool isModified{false};
};

struct Snapshot {
    QString id;
    QString description;
    QDateTime timestamp;
    QMap<QString, FileSnapshot> files;
    QString parentSnapshotId;
};

struct SnapshotDiff {
    QStringList addedFiles;
    QStringList removedFiles;
    QStringList modifiedFiles;
    int totalChanges() const { return addedFiles.size() + removedFiles.size() + modifiedFiles.size(); }
};

class ISnapshotService {
public:
    virtual ~ISnapshotService() = default;

    // Snapshot operations
    virtual QString createSnapshot(const QString& description = {}) = 0;
    virtual bool restoreSnapshot(const QString& snapshotId) = 0;
    virtual bool deleteSnapshot(const QString& snapshotId) = 0;

    // File tracking
    virtual void trackFile(const QString& filePath) = 0;
    virtual void untrackFile(const QString& filePath) = 0;
    virtual QStringList trackedFiles() const = 0;

    // Diff operations
    virtual SnapshotDiff diff(const QString& snapshotId = {}) const = 0;
    virtual SnapshotDiff diffBetween(const QString& fromId, const QString& toId) const = 0;

    // Query
    virtual QList<Snapshot> snapshots() const = 0;
    virtual std::optional<Snapshot> findSnapshot(const QString& snapshotId) const = 0;
    virtual QString currentSnapshotId() const = 0;

    // Auto-save
    virtual void enableAutoSave(bool enabled) = 0;
    virtual bool isAutoSaveEnabled() const = 0;
};

} // namespace sentinel::core
