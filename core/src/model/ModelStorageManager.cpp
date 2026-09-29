// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/model/ModelStorageManager.h"

#include <QDesktopServices>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QUrl>

namespace sentinel::core {
namespace {

QString normalizedDirectory(const QString& path) {
    const auto trimmed = path.trimmed();
    if (trimmed.isEmpty()) return {};
    QDir dir(trimmed);
    if (!dir.isAbsolute() || dir.isRoot()) return {};
    return dir.exists() ? dir.canonicalPath() : QDir::cleanPath(dir.absolutePath());
}

bool inside(const QString& root, const QString& path) {
    return !root.isEmpty() && path.startsWith(root + QDir::separator());
}

} // namespace

ModelStorageManager::ModelStorageManager(QString settingsPath)
    : settingsPath_(std::move(settingsPath)) {
    if (settingsPath_.isEmpty())
        settingsPath_ = QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
                            .filePath(QStringLiteral("model-storage.json"));
    const auto defaultRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                                 .filePath(QStringLiteral("models/huggingface"));
    QDir().mkpath(defaultRoot);
    activeRoot_ = normalizedDirectory(defaultRoot);
    knownRoots_ << activeRoot_;
    QFile file(settingsPath_);
    if (!file.open(QIODevice::ReadOnly)) return;
    const auto object = QJsonDocument::fromJson(file.readAll()).object();
    for (const auto& value : object.value(QStringLiteral("knownRoots")).toArray()) {
        const auto root = normalizedDirectory(value.toString());
        if (!root.isEmpty() && !knownRoots_.contains(root)) knownRoots_ << root;
    }
    const auto devices = object.value(QStringLiteral("rootDevices")).toObject();
    for (auto it = devices.begin(); it != devices.end(); ++it)
        rootDevices_.insert(it.key(), QByteArray::fromBase64(it.value().toString().toLatin1()));
    const auto configured = normalizedDirectory(object.value(QStringLiteral("activeRoot")).toString());
    if (!configured.isEmpty()) activeRoot_ = configured;
    if (!knownRoots_.contains(activeRoot_)) knownRoots_ << activeRoot_;
}

QString ModelStorageManager::activeRoot() const { return activeRoot_; }
QStringList ModelStorageManager::knownRoots() const { return knownRoots_; }

bool ModelStorageManager::setActiveRoot(const QString& path) {
    const auto root = normalizedDirectory(path);
    if (root.isEmpty() || !QDir(root).exists()) return false;
    const QFileInfo rootInfo(root);
    if (!rootInfo.isWritable()) return false;
    const QStorageInfo disk(root);
    if (!disk.isValid() || !disk.isReady()) return false;
    const auto previous = activeRoot_;
    const auto roots = knownRoots_;
    const auto devices = rootDevices_;
    activeRoot_ = root;
    if (!knownRoots_.contains(root)) knownRoots_ << root;
    rootDevices_.insert(root, disk.device());
    if (save()) return true;
    activeRoot_ = previous;
    knownRoots_ = roots;
    rootDevices_ = devices;
    return false;
}

bool ModelStorageManager::save() const {
    QDir().mkpath(QFileInfo(settingsPath_).absolutePath());
    QSaveFile file(settingsPath_);
    if (!file.open(QIODevice::WriteOnly)) return false;
    QJsonArray roots;
    for (const auto& root : knownRoots_) roots.append(root);
    QJsonObject devices;
    for (auto it = rootDevices_.cbegin(); it != rootDevices_.cend(); ++it)
        devices.insert(it.key(), QString::fromLatin1(it.value().toBase64()));
    const auto data = QJsonDocument(QJsonObject{{QStringLiteral("activeRoot"), activeRoot_},
                                             {QStringLiteral("knownRoots"), roots},
                                             {QStringLiteral("rootDevices"), devices}}).toJson();
    return file.write(data) == data.size() && file.commit();
}

bool ModelStorageManager::canStore(qint64 bytes, qint64 reserveBytes) const {
    if (bytes < 0 || reserveBytes < 0 || activeRoot_.isEmpty()) return false;
    const QStorageInfo disk(activeRoot_);
    const auto expectedDevice = rootDevices_.value(activeRoot_);
    return disk.isValid() && disk.isReady() && disk.bytesAvailable() >= bytes &&
           (expectedDevice.isEmpty() || expectedDevice == disk.device()) &&
           disk.bytesAvailable() - bytes >= reserveBytes;
}

bool ModelStorageManager::insideKnownRoot(const QString& path) const {
    const auto canonical = QFileInfo(path).canonicalFilePath();
    for (const auto& root : knownRoots_)
        if (inside(root, canonical)) return true;
    return false;
}

std::optional<ManagedModelArtifact> ModelStorageManager::ownedArtifact(const QString& path) const {
    const auto sidecarPath = path + QStringLiteral(".source.json");
    if (!QFileInfo(path).isFile() || !validSidecar(sidecarPath)) return std::nullopt;
    QFile sidecar(sidecarPath);
    if (!sidecar.open(QIODevice::ReadOnly)) return std::nullopt;
    const auto object = QJsonDocument::fromJson(sidecar.readAll()).object();
    return ManagedModelArtifact{path, object.value(QStringLiteral("repositoryId")).toString(),
                                object.value(QStringLiteral("filename")).toString(),
                                object.value(QStringLiteral("revision")).toString(),
                                QFileInfo(path).size()};
}

bool ModelStorageManager::validSidecar(const QString& sidecarPath) const {
    if (!insideKnownRoot(sidecarPath) ||
        !sidecarPath.endsWith(QStringLiteral(".source.json"))) return false;
    const auto path = sidecarPath.left(sidecarPath.size() - 12);
    QFile sidecar(sidecarPath);
    if (!sidecar.open(QIODevice::ReadOnly) || sidecar.size() > 4096) return false;
    const auto object = QJsonDocument::fromJson(sidecar.readAll()).object();
    const auto repository = object.value(QStringLiteral("repositoryId")).toString();
    const auto filename = object.value(QStringLiteral("filename")).toString();
    const auto revision = object.value(QStringLiteral("revision")).toString();
    if (repository.isEmpty() || QFileInfo(filename).fileName() != QFileInfo(path).fileName() ||
        revision.size() != 40) return false;
    const auto identity = QStringLiteral("%1@%2:%3").arg(repository, revision, filename);
    const auto expectedDirectory = QString::fromLatin1(
        QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha256).toHex());
    return QFileInfo(path).dir().dirName() == expectedDirectory;
}

ModelStorageSnapshot ModelStorageManager::snapshot(const QString& cachePath,
                                                    qint64 partialBytes) const {
    ModelStorageSnapshot result;
    result.activeRoot = activeRoot_;
    result.knownRoots = knownRoots_;
    const QStorageInfo disk(activeRoot_);
    const auto expectedDevice = rootDevices_.value(activeRoot_);
    if (disk.isValid() && disk.isReady() &&
        (expectedDevice.isEmpty() || expectedDevice == disk.device()))
        result.availableBytes = disk.bytesAvailable();
    result.partialBytes = partialBytes;
    if (!cachePath.isEmpty()) result.cacheBytes = QFileInfo(cachePath).size();
    for (const auto& root : knownRoots_) {
        const QDir directory(root);
        for (const auto& subdir : directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (subdir.fileName().size() != 64) continue;
            const QDir artifactDir(subdir.absoluteFilePath());
            for (const auto& file : artifactDir.entryInfoList(QDir::Files)) {
                if (file.fileName().endsWith(QStringLiteral(".source.json"))) {
                    const auto artifactPath = file.absoluteFilePath().left(
                        file.absoluteFilePath().size() - 12);
                    if (!QFileInfo::exists(artifactPath) && validSidecar(file.absoluteFilePath()))
                        result.orphanedFiles << file.absoluteFilePath();
                    continue;
                }
                if (const auto artifact = ownedArtifact(file.absoluteFilePath())) {
                    result.managedBytes += artifact->bytes;
                    result.artifacts << *artifact;
                }
            }
        }
    }
    return result;
}

bool ModelStorageManager::removeArtifact(const QString& path) const {
    if (!ownedArtifact(path)) return false;
    if (!QFile::remove(path)) return false;
    return QFile::remove(path + QStringLiteral(".source.json"));
}

bool ModelStorageManager::revealArtifact(const QString& path) const {
    return ownedArtifact(path) && QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
}

bool ModelStorageManager::removeOwnedPartial(const QString& path) const {
    Q_UNUSED(path);
    // QSaveFile owns its temporary file; no persistent partial is created by this service.
    return false;
}

bool ModelStorageManager::removeOrphanedSidecar(const QString& path) const {
    if (!validSidecar(path) || QFileInfo::exists(path.left(path.size() - 12))) return false;
    return QFile::remove(path);
}

} // namespace sentinel::core
