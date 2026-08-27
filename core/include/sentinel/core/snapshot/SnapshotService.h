// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/snapshot/ISnapshotService.h"
#include <QMap>
#include <QObject>
#include <QTimer>
#include <QUuid>

namespace sentinel::core {

class SnapshotService : public QObject, public ISnapshotService {
    Q_OBJECT
public:
    explicit SnapshotService(QObject* parent = nullptr);
    ~SnapshotService() override;

    // ISnapshotService interface
    QString createSnapshot(const QString& description = {}) override;
    bool restoreSnapshot(const QString& snapshotId) override;
    bool deleteSnapshot(const QString& snapshotId) override;

    void trackFile(const QString& filePath) override;
    void untrackFile(const QString& filePath) override;
    QStringList trackedFiles() const override;

    SnapshotDiff diff(const QString& snapshotId = {}) const override;
    SnapshotDiff diffBetween(const QString& fromId, const QString& toId) const override;

    QList<Snapshot> snapshots() const override;
    std::optional<Snapshot> findSnapshot(const QString& snapshotId) const override;
    QString currentSnapshotId() const override;

    void enableAutoSave(bool enabled) override;
    bool isAutoSaveEnabled() const override;

    // Set working directory
    void setWorkingDirectory(const QString& dir);
    QString workingDirectory() const;

signals:
    void snapshotCreated(const QString& snapshotId);
    void snapshotRestored(const QString& snapshotId);
    void snapshotDeleted(const QString& snapshotId);
    void fileModified(const QString& filePath);

private slots:
    void checkForChanges();

private:
    QByteArray calculateChecksum(const QString& filePath) const;
    QByteArray readFileContent(const QString& filePath) const;
    bool writeFileContent(const QString& filePath, const QByteArray& content) const;
    QString generateSnapshotId() const;
    QString snapshotPath(const QString& snapshotId) const;
    bool saveSnapshotToFile(const Snapshot& snapshot) const;
    std::optional<Snapshot> loadSnapshotFromFile(const QString& snapshotId) const;

    QString m_workingDirectory;
    QStringList m_trackedFiles;
    QMap<QString, Snapshot> m_snapshots;
    QString m_currentSnapshotId;
    bool m_autoSaveEnabled{false};
    QTimer m_checkTimer;
};

} // namespace sentinel::core
