// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/model/ModelLibrary.h"

#include <QDateTime>
#include <QJsonArray>
#include <QObject>
#include <QStringList>

#include <functional>
#include <optional>

class QNetworkAccessManager;
class QNetworkReply;

namespace sentinel::core {

enum class HuggingFaceCatalogState {
    Current, Stale, Unavailable, AuthenticationRequired, AccessDenied,
    GatedModel, RateLimited, Empty
};

struct HuggingFaceSearchQuery {
    QString text;
    bool ggufOnly = false;
    QString publisher;
    QString family;
    QString architecture;
    QString quantization;
    std::optional<bool> llamaCppCandidate;
};

struct HuggingFaceArtifact {
    QString repositoryId;
    QString filename;
    QString revision;
    QString blobId;
    QString etag;
    QString sha256;
    QString format;
    QString quantization;
    QString shardGroup;
    std::optional<qint64> sizeBytes;
};

struct HuggingFaceRepository {
    QString id;
    QString publisher;
    QString displayName;
    QString license;
    QString architecture;
    QString revision;
    QString lastUpdated;
    QStringList tags;
    QList<HuggingFaceArtifact> artifacts;
};

class HuggingFaceModelSource final : public QObject, public IModelLibrarySourceAdapter {
    Q_OBJECT
public:
    explicit HuggingFaceModelSource(QString cachePath = {}, QObject* parent = nullptr);
    QString sourceId() const override;
    QString providerId() const override;
    QList<ModelLibraryEntry> entries() const override;
    QList<ModelLibraryEntry> query(const HuggingFaceSearchQuery& filter) const;
    QList<HuggingFaceRepository> repositories() const;
    HuggingFaceCatalogState catalogState() const;
    QString catalogDetail() const;
    QString cachedQuery() const;
    QDateTime fetchedAt() const;
    QString cachePath() const;
    void search(const QString& text);
    void fetchRepository(const QString& repositoryId);
    void setTokenProvider(std::function<QString()> provider);
    QString token() const;
    void setStorageRoot(const QString& path);
    void setKnownStorageRoots(const QStringList& roots);
    QString storageRoot() const;
    QString destinationPath(const ModelLibraryEntry& entry) const;
    QString existingPath(const ModelLibraryEntry& entry) const;
    QUrl downloadUrl(const ModelLibraryEntry& entry) const;
    bool isDownloaded(const ModelLibraryEntry& entry) const;

signals:
    void catalogChanged();
    void requestFinished(bool success);

private:
    void request(const QUrl& url, const QString& query, const QString& repositoryId);
    void loadCache();
    bool saveCache() const;
    HuggingFaceRepository parseRepository(const QJsonObject& object) const;
    QList<ModelLibraryEntry> entriesFor(const HuggingFaceRepository& repository) const;

    QString cachePath_;
    QString storageRoot_;
    QStringList knownStorageRoots_;
    QJsonArray cachedModels_;
    QString cachedQuery_;
    QDateTime fetchedAt_;
    HuggingFaceCatalogState state_ = HuggingFaceCatalogState::Unavailable;
    QString detail_;
    std::function<QString()> tokenProvider_;
    QNetworkAccessManager* network_ = nullptr;
    QNetworkReply* activeReply_ = nullptr;
    QByteArray responseBuffer_;
    bool responseTooLarge_ = false;
};

} // namespace sentinel::core
