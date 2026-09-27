// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QHash>
#include <QString>
#include <optional>

namespace sentinel::core {

struct ManagedModelArtifact {
    QString path;
    QString repositoryId;
    QString filename;
    QString revision;
    qint64 bytes = 0;
};

struct ModelStorageSnapshot {
    QString activeRoot;
    QStringList knownRoots;
    std::optional<qint64> availableBytes;
    qint64 managedBytes = 0;
    qint64 cacheBytes = 0;
    qint64 partialBytes = 0;
    QList<ManagedModelArtifact> artifacts;
    QStringList orphanedFiles;
};

class ModelStorageManager final {
public:
    explicit ModelStorageManager(QString settingsPath = {});
    QString activeRoot() const;
    QStringList knownRoots() const;
    bool setActiveRoot(const QString& path);
    bool canStore(qint64 bytes, qint64 reserveBytes = 64 * 1024 * 1024) const;
    ModelStorageSnapshot snapshot(const QString& cachePath = {}, qint64 partialBytes = 0) const;
    bool removeArtifact(const QString& path) const;
    bool revealArtifact(const QString& path) const;
    bool clearCache(const QString& cachePath) const;
    bool removeOwnedPartial(const QString& path) const;
    bool removeOrphanedSidecar(const QString& path) const;

private:
    std::optional<ManagedModelArtifact> ownedArtifact(const QString& path) const;
    bool validSidecar(const QString& sidecarPath) const;
    bool insideKnownRoot(const QString& path) const;
    bool save() const;
    QString settingsPath_;
    QString activeRoot_;
    QStringList knownRoots_;
    QHash<QString, QByteArray> rootDevices_;
};

} // namespace sentinel::core
