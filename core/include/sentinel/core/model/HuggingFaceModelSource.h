// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/model/ModelLibrary.h"

#include <QDateTime>
#include <QJsonArray>
#include <QObject>
#include <QStringList>
#include <QSet>

#include <functional>
#include <optional>

class QNetworkAccessManager;
class QNetworkReply;
class OllamaRuntimeTest;

namespace sentinel::core {

enum class HuggingFaceCatalogState {
    Current, Stale, Unavailable, AuthenticationRequired, AccessDenied,
    GatedModel, RateLimited, Empty, Offline
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
    QString pipelineTask;
    std::optional<qint64> downloads;
    std::optional<qint64> likes;
    bool gated = false;
    QStringList tags;
    QJsonObject metadata;
    QList<HuggingFaceArtifact> artifacts;
};

class HuggingFaceModelSource final : public QObject, public IModelLibrarySourceAdapter {
    Q_OBJECT
    friend class ::OllamaRuntimeTest;

public:
    explicit HuggingFaceModelSource(QString cachePath = {}, QObject* parent = nullptr);
    QString sourceId() const override;
    QString providerId() const override;
    QList<ModelLibraryEntry> entries() const override;
    QList<ModelLibraryEntry> query(const HuggingFaceSearchQuery& filter) const;
    QList<HuggingFaceRepository> repositories() const;
    HuggingFaceCatalogState catalogState() const;
    QString catalogDetail() const;
    bool fetching() const {
        return activeReply_ != nullptr;
    }
    QString cachedQuery() const;
    QDateTime fetchedAt() const;
    QString cachePath() const;
    bool clearMetadataCache(int olderThanDays = 0);
    void search(const QString& text, bool forceRefresh = false);
    void searchCatalog(const QString& text, const QString& task, const QString& sort,
                       bool forceRefresh = false, bool ggufOnly = false);
    bool hasMore() const {
        return !nextPage_.isEmpty();
    }
    void fetchMore();
    void setAutoFetch(bool enabled) { autoFetch_ = enabled; }
    void fetchRepository(const QString& repositoryId);
    void setTokenProvider(std::function<QString()> provider);
    QString token() const;
    void setStorageRoot(const QString& path);
    void setKnownStorageRoots(const QStringList& roots);
    QString storageRoot() const;
    QString destinationPath(const ModelLibraryEntry& entry) const;
    QString destinationPathAtRoot(const ModelLibraryEntry& entry, const QString& root) const;
    QString existingPath(const ModelLibraryEntry& entry) const;
    QUrl downloadUrl(const ModelLibraryEntry& entry) const;
    bool isDownloaded(const ModelLibraryEntry& entry) const;

signals:
    void catalogChanged();
    void requestFinished(bool success);

private:
    void request(const QUrl& url, const QString& query, const QString& repositoryId, bool append = false);
    static QUrl continuationUrl(const QString& links);
    void loadCache();
    bool saveCache() const;
    void fetchSupplement(const QString& repositoryId, const QString& revision, const QString& filename);
    HuggingFaceRepository parseRepository(const QJsonObject& object) const;
    QList<ModelLibraryEntry> entriesFor(const HuggingFaceRepository& repository) const;

    QString cachePath_;
    QString storageRoot_;
    QStringList knownStorageRoots_;
    QJsonArray cachedModels_;
    QString cachedQuery_;
    QUrl nextPage_;
    QDateTime fetchedAt_;
    HuggingFaceCatalogState state_ = HuggingFaceCatalogState::Unavailable;
    QString detail_;
    std::function<QString()> tokenProvider_;
    QNetworkAccessManager* network_ = nullptr;
    QNetworkReply* activeReply_ = nullptr;
    QByteArray responseBuffer_;
    bool responseTooLarge_ = false;
    bool autoFetch_ = false;
    QSet<QString> visitedPages_;
};

} // namespace sentinel::core
