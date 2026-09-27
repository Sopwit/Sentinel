// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/model/ModelLibrary.h"
#include "sentinel/core/model/HuggingFaceModelSource.h"
#include "sentinel/core/model/ModelStorageManager.h"

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QQueue>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace sentinel::core {

enum class ModelOperationKind {
    Pull, Remove, Import, RemoveRegistration, Refresh, OpenExternalManager,
    Download, DownloadAndRegister, RemoveManagedArtifact, RevealLocalFile
};
enum class ModelOperationState { Queued, Running, Succeeded, Failed, Cancelled };
enum class ModelOperationError {
    None, RuntimeUnavailable, AuthenticationOrConfiguration, ModelNotFound,
    NetworkFailure, DiskOrStorageFailure, OperationRejected, Cancelled,
    UnsupportedAction, MalformedRuntimeResponse, RateLimited, IntegrityFailure,
    AuthenticationRequired, AccessDenied, GatedModel
};

struct ModelOperationRecord {
    QString id;
    ModelOperationKind kind = ModelOperationKind::Refresh;
    ModelOperationState state = ModelOperationState::Queued;
    ModelOperationError error = ModelOperationError::None;
    QString libraryId;
    QString providerId;
    QString runtimeId;
    QString sourceId;
    QString nativeModelId;
    QString localFile;
    QString targetUrl;
    QString repositoryId;
    QString artifactFilename;
    QString revision;
    QString artifactHash;
    QString artifactEtag;
    std::optional<qint64> expectedBytes;
    std::optional<double> progress;
    std::optional<qint64> bytesTransferred;
    std::optional<qint64> bytesTotal;
    QString statusText;
    QDateTime startedAt;
    QDateTime endedAt;
    bool cancellable = false;
};

class ModelOperationService final : public QObject {
    Q_OBJECT
public:
    ModelOperationService(ModelService& models, ModelLibraryService& library,
                          QObject* parent = nullptr);
    void setOllamaEndpoint(const QString& endpoint);
    void setLmStudioEndpoint(const QString& endpoint);
    HuggingFaceModelSource* huggingFaceSource() { return &huggingFaceSource_; }
    ModelStorageManager& storageManager() { return storageManager_; }
    ModelStorageSnapshot storageSnapshot() const;
    bool setHuggingFaceStorageRoot(const QString& path);
    void setHuggingFaceTokenProvider(std::function<QString()> provider);
    QString start(ModelLibraryAction action, const ModelLibraryEntry& entry);
    QString pullOllama(const QString& modelId);
    QString importGguf(const QString& filePath, const QString& runtimeModelId = {});
    QString refresh(const QString& providerId);
    bool cancel(const QString& operationId);
    ModelOperationRecord operation(const QString& operationId) const;
    QList<ModelOperationRecord> operations() const;

signals:
    void operationChanged(const sentinel::core::ModelOperationRecord& record);
    void catalogChanged(const QString& providerId);

private:
    QString enqueue(ModelOperationRecord record);
    void startNext();
    void beginNetwork(const ModelOperationRecord& record);
    void beginDownload(const ModelOperationRecord& record);
    void issueDownloadRequest(const QUrl& url, const QString& id,
                              const QString& destination, const ModelLibraryEntry& artifact);
    void consumeDownloadData(const QByteArray& data);
    void processPullLines(bool finalChunk);
    void finish(ModelOperationState state, ModelOperationError error, const QString& status);
    void update(const ModelOperationRecord& record);
    void refreshAfterMutation(const QString& providerId);
    void refreshOllamaTelemetry();
    QUrl endpointFor(const QString& providerId) const;

    ModelService& models_;
    ModelLibraryService& library_;
    LMStudioNativeCatalogAdapter lmStudioCatalog_;
    HuggingFaceModelSource huggingFaceSource_;
    ModelStorageManager storageManager_;
    QNetworkAccessManager* network_ = nullptr;
    QNetworkReply* activeReply_ = nullptr;
    std::unique_ptr<QSaveFile> downloadFile_;
    std::unique_ptr<QCryptographicHash> downloadHash_;
    QByteArray downloadPrefix_;
    qint64 downloadedBytes_ = 0;
    bool downloadStorageFailed_ = false;
    int downloadRedirects_ = 0;
    QUrl activeEndpoint_;
    QTimer* timeout_ = nullptr;
    QHash<QString, ModelOperationRecord> records_;
    QQueue<QString> queue_;
    QString activeId_;
    QByteArray pullBuffer_;
    bool pullSucceeded_ = false;
    bool pullFailed_ = false;
    ModelOperationError pullError_ = ModelOperationError::None;
    bool cancelRequested_ = false;
    QString ollamaEndpoint_ = QStringLiteral("http://127.0.0.1:11434");
    QString lmStudioEndpoint_ = QStringLiteral("http://127.0.0.1:1234");
    ModelRuntimeStatus ollamaRuntimeStatus_;
};

} // namespace sentinel::core

Q_DECLARE_METATYPE(sentinel::core::ModelOperationRecord)
