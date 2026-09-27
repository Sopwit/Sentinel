// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/model/ModelOperationService.h"

#include "sentinel/core/runtime/LocalInference.h"
#include "sentinel/core/runtime/OllamaRuntime.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStorageInfo>
#include <QTimer>
#include <QUuid>

#include <algorithm>

namespace sentinel::core {
namespace {

bool terminal(ModelOperationState state) {
    return state == ModelOperationState::Succeeded || state == ModelOperationState::Failed ||
           state == ModelOperationState::Cancelled;
}

ModelOperationError networkError(QNetworkReply* reply) {
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 401 || status == 403)
        return ModelOperationError::AuthenticationOrConfiguration;
    if (status == 404) return ModelOperationError::ModelNotFound;
    if (status == 507) return ModelOperationError::DiskOrStorageFailure;
    if (status >= 400 && status < 500) return ModelOperationError::OperationRejected;
    if (reply->error() == QNetworkReply::ConnectionRefusedError ||
        reply->error() == QNetworkReply::HostNotFoundError)
        return ModelOperationError::RuntimeUnavailable;
    return ModelOperationError::NetworkFailure;
}

QString safeErrorText(ModelOperationError error) {
    switch (error) {
    case ModelOperationError::None: return QStringLiteral("Completed.");
    case ModelOperationError::RuntimeUnavailable: return QStringLiteral("Runtime unavailable.");
    case ModelOperationError::AuthenticationOrConfiguration:
        return QStringLiteral("Runtime authentication or configuration failed.");
    case ModelOperationError::ModelNotFound: return QStringLiteral("Model not found.");
    case ModelOperationError::NetworkFailure: return QStringLiteral("Network request failed.");
    case ModelOperationError::DiskOrStorageFailure:
        return QStringLiteral("Insufficient or unavailable storage.");
    case ModelOperationError::OperationRejected: return QStringLiteral("Operation rejected.");
    case ModelOperationError::Cancelled: return QStringLiteral("Cancelled.");
    case ModelOperationError::UnsupportedAction: return QStringLiteral("Action unsupported.");
    case ModelOperationError::MalformedRuntimeResponse:
        return QStringLiteral("Runtime returned an invalid response.");
    case ModelOperationError::RateLimited: return QStringLiteral("Source rate limit reached.");
    case ModelOperationError::IntegrityFailure:
        return QStringLiteral("Downloaded artifact failed integrity validation.");
    case ModelOperationError::AuthenticationRequired:
        return QStringLiteral("Hugging Face authentication required.");
    case ModelOperationError::AccessDenied:
        return QStringLiteral("Access to this artifact was denied.");
    case ModelOperationError::GatedModel:
        return QStringLiteral("Access to this gated model has not been granted.");
    }
    return QStringLiteral("Operation failed.");
}

ChatProviderErrorCategory discoveryCategory(ModelOperationError error) {
    switch (error) {
    case ModelOperationError::AuthenticationOrConfiguration:
        return ChatProviderErrorCategory::AuthenticationRequired;
    case ModelOperationError::MalformedRuntimeResponse:
        return ChatProviderErrorCategory::MalformedResponse;
    case ModelOperationError::Cancelled: return ChatProviderErrorCategory::Cancelled;
    case ModelOperationError::OperationRejected:
    case ModelOperationError::ModelNotFound:
        return ChatProviderErrorCategory::RequestRejected;
    default: return ChatProviderErrorCategory::ConnectionFailed;
    }
}

} // namespace

ModelOperationService::ModelOperationService(ModelService& models, ModelLibraryService& library,
                                             QObject* parent)
    : QObject(parent), models_(models), library_(library),
      network_(new QNetworkAccessManager(this)), timeout_(new QTimer(this)) {
    library_.addSourceAdapter(&lmStudioCatalog_);
    library_.addSourceAdapter(&huggingFaceSource_);
    library_.setStorageManager(&storageManager_);
    huggingFaceSource_.setStorageRoot(storageManager_.activeRoot());
    huggingFaceSource_.setKnownStorageRoots(storageManager_.knownRoots());
    connect(&huggingFaceSource_, &HuggingFaceModelSource::catalogChanged, this,
            [this]() { emit catalogChanged(QStringLiteral("hugging-face")); });
    timeout_->setSingleShot(true);
    connect(timeout_, &QTimer::timeout, this, [this]() {
        if (activeReply_) activeReply_->abort();
    });
}

bool ModelOperationService::setHuggingFaceStorageRoot(const QString& path) {
    if (!storageManager_.setActiveRoot(path)) return false;
    huggingFaceSource_.setStorageRoot(storageManager_.activeRoot());
    huggingFaceSource_.setKnownStorageRoots(storageManager_.knownRoots());
    return true;
}

ModelStorageSnapshot ModelOperationService::storageSnapshot() const {
    return storageManager_.snapshot(huggingFaceSource_.cachePath(),
        downloadFile_ ? downloadedBytes_ : 0);
}

void ModelOperationService::setHuggingFaceTokenProvider(std::function<QString()> provider) {
    huggingFaceSource_.setTokenProvider(std::move(provider));
}

void ModelOperationService::setOllamaEndpoint(const QString& endpoint) {
    const auto parsed = OllamaConfig::fromEndpoint(endpoint).endpoint;
    if (parsed.isLoopbackHttp() && ollamaEndpoint_ != parsed.toString()) {
        ollamaEndpoint_ = parsed.toString();
        ollamaRuntimeStatus_ = {};
        ollamaRuntimeStatus_.runtimeId = QStringLiteral("ollama");
        library_.setRuntimeStatus(ollamaRuntimeStatus_);
        emit catalogChanged(QStringLiteral("ollama"));
        if (!activeId_.isEmpty() && records_.value(activeId_).providerId == QLatin1String("ollama") &&
            activeReply_ && records_.value(activeId_).cancellable) {
            cancelRequested_ = true;
            activeReply_->abort();
        }
    }
}

void ModelOperationService::setLmStudioEndpoint(const QString& endpoint) {
    const LMStudioConfig config{QUrl(endpoint)};
    if (config.isLoopbackHttp() && lmStudioEndpoint_ != config.endpoint.toString()) {
        lmStudioEndpoint_ = config.endpoint.toString();
        lmStudioCatalog_.clear();
        emit catalogChanged(QStringLiteral("lm-studio"));
        if (!activeId_.isEmpty() && records_.value(activeId_).providerId == QLatin1String("lm-studio") &&
            activeReply_ && records_.value(activeId_).cancellable) {
            cancelRequested_ = true;
            activeReply_->abort();
        }
    }
}

QUrl ModelOperationService::endpointFor(const QString& providerId) const {
    return QUrl(providerId == QLatin1String("ollama") ? ollamaEndpoint_ : lmStudioEndpoint_);
}

QString ModelOperationService::start(ModelLibraryAction action,
                                     const ModelLibraryEntry& entry) {
    ModelOperationRecord record;
    record.libraryId = entry.id;
    record.providerId = entry.provider.id;
    record.runtimeId = entry.runtime.id;
    record.sourceId = entry.source.id;
    record.nativeModelId = entry.nativeModelId;
    record.localFile = entry.localFile;
    record.repositoryId = entry.repositoryId;
    record.artifactFilename = entry.artifactFilename;
    record.revision = entry.revision;
    record.artifactHash = entry.artifactHash;
    record.artifactEtag = entry.artifactEtag;
    record.expectedBytes = entry.sizeBytes;
    if (action == ModelLibraryAction::Pull) record.kind = ModelOperationKind::Pull;
    else if (action == ModelLibraryAction::Remove)
        record.kind = entry.source.id == QLatin1String("hugging-face")
            ? ModelOperationKind::RemoveManagedArtifact : ModelOperationKind::Remove;
    else if (action == ModelLibraryAction::Import) record.kind = ModelOperationKind::Import;
    else if (action == ModelLibraryAction::RemoveRegistration)
        record.kind = ModelOperationKind::RemoveRegistration;
    else if (action == ModelLibraryAction::Download)
        record.kind = ModelOperationKind::Download;
    else if (action == ModelLibraryAction::DownloadAndRegister)
        record.kind = ModelOperationKind::DownloadAndRegister;
    else if (action == ModelLibraryAction::RevealLocalFile)
        record.kind = ModelOperationKind::RevealLocalFile;
    else if (action == ModelLibraryAction::OpenExternalManager) {
        record.kind = ModelOperationKind::OpenExternalManager;
        record.targetUrl = entry.externalManagerUrl;
    } else {
        record.kind = ModelOperationKind::Refresh;
    }
    const bool supported = ModelLibraryService::availableActions(entry).contains(action) &&
        (action == ModelLibraryAction::Pull || action == ModelLibraryAction::Remove ||
         action == ModelLibraryAction::Import || action == ModelLibraryAction::RemoveRegistration ||
         action == ModelLibraryAction::OpenExternalManager ||
         action == ModelLibraryAction::RevealLocalFile ||
         action == ModelLibraryAction::Download ||
         action == ModelLibraryAction::DownloadAndRegister);
    if (!supported) {
        record.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        record.state = ModelOperationState::Failed;
        record.error = ModelOperationError::UnsupportedAction;
        record.statusText = safeErrorText(record.error);
        record.startedAt = record.endedAt = QDateTime::currentDateTimeUtc();
        update(record);
        return record.id;
    }
    return enqueue(record);
}

QString ModelOperationService::pullOllama(const QString& modelId) {
    ModelOperationRecord record;
    record.kind = ModelOperationKind::Pull;
    record.providerId = record.runtimeId = QStringLiteral("ollama");
    record.sourceId = QStringLiteral("ollama-library");
    record.nativeModelId = modelId.trimmed();
    record.libraryId = ModelLibraryService::entryId(record.providerId, record.runtimeId,
                                                    record.sourceId, record.nativeModelId);
    return enqueue(record);
}

QString ModelOperationService::importGguf(const QString& filePath,
                                           const QString& runtimeModelId) {
    ModelOperationRecord record;
    record.kind = ModelOperationKind::Import;
    record.providerId = record.runtimeId = QStringLiteral("llama-cpp-server");
    record.sourceId = QStringLiteral("local-file");
    record.localFile = filePath;
    record.nativeModelId = runtimeModelId;
    record.libraryId = ModelLibraryService::entryId(record.providerId, record.runtimeId,
                                                    record.sourceId,
                                                    QFileInfo(filePath).canonicalFilePath());
    return enqueue(record);
}

QString ModelOperationService::refresh(const QString& providerId) {
    const auto normalized = providerId.trimmed().toLower();
    for (const auto& record : records_) {
        if (record.kind == ModelOperationKind::Refresh && record.providerId == normalized &&
            !terminal(record.state)) return record.id;
    }
    ModelOperationRecord record;
    record.kind = ModelOperationKind::Refresh;
    record.providerId = record.runtimeId = normalized;
    record.libraryId = QStringLiteral("catalog/%1").arg(record.providerId);
    return enqueue(record);
}

QString ModelOperationService::enqueue(ModelOperationRecord record) {
    record.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    record.statusText = QStringLiteral("Queued.");
    update(record);
    queue_.enqueue(record.id);
    startNext();
    return record.id;
}

void ModelOperationService::update(const ModelOperationRecord& record) {
    records_.insert(record.id, record);
    emit operationChanged(record);
}

ModelOperationRecord ModelOperationService::operation(const QString& operationId) const {
    return records_.value(operationId);
}

QList<ModelOperationRecord> ModelOperationService::operations() const {
    auto result = records_.values();
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        return a.startedAt < b.startedAt;
    });
    return result;
}

bool ModelOperationService::cancel(const QString& operationId) {
    auto record = records_.value(operationId);
    if (record.id.isEmpty() || terminal(record.state)) return false;
    if (record.state == ModelOperationState::Queued) {
        queue_.removeAll(operationId);
        record.state = ModelOperationState::Cancelled;
        record.error = ModelOperationError::Cancelled;
        record.statusText = safeErrorText(record.error);
        record.endedAt = QDateTime::currentDateTimeUtc();
        update(record);
        return true;
    }
    if (!record.cancellable || operationId != activeId_ || !activeReply_ || pullSucceeded_)
        return false;
    cancelRequested_ = true;
    activeReply_->abort();
    return true;
}

void ModelOperationService::startNext() {
    if (!activeId_.isEmpty() || queue_.isEmpty()) return;
    activeId_ = queue_.dequeue();
    auto record = records_.value(activeId_);
    record.state = ModelOperationState::Running;
    record.startedAt = QDateTime::currentDateTimeUtc();
    record.cancellable = record.kind == ModelOperationKind::Pull ||
                         record.kind == ModelOperationKind::Refresh ||
                         record.kind == ModelOperationKind::Download ||
                         record.kind == ModelOperationKind::DownloadAndRegister;
    record.statusText = QStringLiteral("Running.");
    update(record);
    if (record.kind == ModelOperationKind::Import) {
        bool storageFailure = false;
        const bool imported = library_.registerLocalGguf(record.localFile, record.nativeModelId,
                                                         &storageFailure);
        finish(imported ? ModelOperationState::Succeeded : ModelOperationState::Failed,
               imported ? ModelOperationError::None
                        : storageFailure ? ModelOperationError::DiskOrStorageFailure
                                         : ModelOperationError::OperationRejected,
               imported ? QStringLiteral("Local GGUF registered.")
                        : QStringLiteral("GGUF file could not be registered."));
        if (imported) emit catalogChanged(record.providerId);
    } else if (record.kind == ModelOperationKind::RemoveRegistration) {
        bool storageFailure = false;
        const bool removed = library_.removeLocalGgufRegistration(record.libraryId,
                                                                  &storageFailure);
        finish(removed ? ModelOperationState::Succeeded : ModelOperationState::Failed,
               removed ? ModelOperationError::None
                       : storageFailure ? ModelOperationError::DiskOrStorageFailure
                                        : ModelOperationError::ModelNotFound,
               removed ? QStringLiteral("Local registration removed.")
                       : QStringLiteral("Local registration not found."));
        if (removed) emit catalogChanged(record.providerId);
    } else if (record.kind == ModelOperationKind::OpenExternalManager) {
        const bool opened = QDesktopServices::openUrl(QUrl(record.targetUrl));
        finish(opened ? ModelOperationState::Succeeded : ModelOperationState::Failed,
               opened ? ModelOperationError::None : ModelOperationError::RuntimeUnavailable,
               opened ? QStringLiteral("External manager opened.")
                      : QStringLiteral("External manager could not be opened."));
    } else if (record.kind == ModelOperationKind::RemoveManagedArtifact) {
        const bool removed = storageManager_.removeArtifact(record.localFile);
        finish(removed ? ModelOperationState::Succeeded : ModelOperationState::Failed,
               removed ? ModelOperationError::None : ModelOperationError::OperationRejected,
               removed ? QStringLiteral("Managed artifact removed.")
                       : safeErrorText(ModelOperationError::OperationRejected));
        if (removed) emit catalogChanged(QStringLiteral("hugging-face"));
    } else if (record.kind == ModelOperationKind::RevealLocalFile) {
        const auto path = QFileInfo(record.localFile).absolutePath();
        const bool opened = QFileInfo(record.localFile).isFile() &&
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        finish(opened ? ModelOperationState::Succeeded : ModelOperationState::Failed,
               opened ? ModelOperationError::None : ModelOperationError::OperationRejected,
               opened ? QStringLiteral("Local file location opened.")
                      : safeErrorText(ModelOperationError::OperationRejected));
    } else if (record.kind == ModelOperationKind::Download ||
               record.kind == ModelOperationKind::DownloadAndRegister) beginDownload(record);
    else beginNetwork(record);
}

void ModelOperationService::consumeDownloadData(const QByteArray& data) {
    if (data.isEmpty() || !downloadFile_ || downloadStorageFailed_) return;
    if (downloadPrefix_.size() < 24)
        downloadPrefix_.append(data.left(24 - downloadPrefix_.size()));
    if (downloadFile_->write(data) != data.size()) {
        downloadStorageFailed_ = true;
        if (activeReply_) activeReply_->abort();
        return;
    }
    downloadedBytes_ += data.size();
    downloadHash_->addData(data);
    auto record = records_.value(activeId_);
    record.bytesTransferred = downloadedBytes_;
    if (record.bytesTotal && *record.bytesTotal > 0)
        record.progress = qBound(0.0, double(downloadedBytes_) / *record.bytesTotal, 1.0);
    record.statusText = QStringLiteral("Downloading artifact.");
    update(record);
}

void ModelOperationService::beginDownload(const ModelOperationRecord& record) {
    ModelLibraryEntry artifact;
    artifact.source.id = record.sourceId;
    artifact.repositoryId = record.repositoryId;
    artifact.artifactFilename = record.artifactFilename;
    artifact.revision = record.revision;
    artifact.artifactHash = record.artifactHash;
    artifact.artifactEtag = record.artifactEtag;
    artifact.sizeBytes = record.expectedBytes;
    const auto destination = huggingFaceSource_.destinationPath(artifact);
    const auto url = huggingFaceSource_.downloadUrl(artifact);
    if (record.sourceId != QLatin1String("hugging-face") || destination.isEmpty() ||
        !url.isValid() || record.artifactFilename.endsWith(QStringLiteral(".gguf"), Qt::CaseInsensitive) == false) {
        finish(ModelOperationState::Failed, ModelOperationError::UnsupportedAction,
               safeErrorText(ModelOperationError::UnsupportedAction));
        return;
    }
    if (record.kind == ModelOperationKind::DownloadAndRegister &&
        huggingFaceSource_.isDownloaded(artifact)) {
        bool storageFailure = false;
        const auto existing = huggingFaceSource_.existingPath(artifact);
        if (!library_.registerLocalGguf(existing, {}, &storageFailure, &artifact)) {
            const auto error = storageFailure ? ModelOperationError::DiskOrStorageFailure
                                              : ModelOperationError::OperationRejected;
            finish(ModelOperationState::Failed, error, safeErrorText(error));
            return;
        }
        auto completed = records_.value(record.id);
        completed.localFile = existing;
        update(completed);
        finish(ModelOperationState::Succeeded, ModelOperationError::None,
               QStringLiteral("Downloaded GGUF registered."));
        emit catalogChanged(QStringLiteral("llama-cpp-server"));
        emit catalogChanged(QStringLiteral("hugging-face"));
        return;
    }
    if (!QDir(storageManager_.activeRoot()).exists() ||
        !storageManager_.canStore(record.expectedBytes.value_or(0))) {
        finish(ModelOperationState::Failed, ModelOperationError::DiskOrStorageFailure,
               safeErrorText(ModelOperationError::DiskOrStorageFailure));
        return;
    }
    const auto parent = QFileInfo(destination).absolutePath();
    if (!QDir().mkpath(parent)) {
        finish(ModelOperationState::Failed, ModelOperationError::DiskOrStorageFailure,
               safeErrorText(ModelOperationError::DiskOrStorageFailure));
        return;
    }
    downloadFile_ = std::make_unique<QSaveFile>(destination);
    if (!downloadFile_->open(QIODevice::WriteOnly)) {
        downloadFile_.reset();
        finish(ModelOperationState::Failed, ModelOperationError::DiskOrStorageFailure,
               safeErrorText(ModelOperationError::DiskOrStorageFailure));
        return;
    }
    downloadHash_ = std::make_unique<QCryptographicHash>(QCryptographicHash::Sha256);
    downloadPrefix_.clear();
    downloadedBytes_ = 0;
    downloadStorageFailed_ = false;
    downloadRedirects_ = 0;
    cancelRequested_ = pullSucceeded_ = false;
    auto initial = records_.value(record.id);
    initial.bytesTotal = record.expectedBytes;
    initial.statusText = QStringLiteral("Connecting to Hugging Face.");
    update(initial);
    issueDownloadRequest(url, record.id, destination, artifact);
}

void ModelOperationService::issueDownloadRequest(const QUrl& url, const QString& id,
                                                 const QString& destination,
                                                 const ModelLibraryEntry& artifact) {
    QNetworkRequest request(url);
    request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);
    if (url.scheme() == QLatin1String("https") &&
        url.host().compare(QStringLiteral("huggingface.co"), Qt::CaseInsensitive) == 0 &&
        url.port(-1) == -1) {
        const auto token = huggingFaceSource_.token();
        if (!token.isEmpty()) request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    }
    activeReply_ = network_->get(request);
    connect(activeReply_, &QNetworkReply::readyRead, this, [this, id]() {
        if (activeId_ != id || !activeReply_) return;
        const auto status = activeReply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status >= 200 && status < 300) consumeDownloadData(activeReply_->readAll());
    });
    connect(activeReply_, &QNetworkReply::downloadProgress, this,
            [this, id](qint64 received, qint64 total) {
        if (activeId_ != id || total <= 0) return;
        auto current = records_.value(id);
        if (!current.expectedBytes) current.bytesTotal = total;
        current.progress = qBound(0.0, double(received) / total, 1.0);
        update(current);
    });
    connect(activeReply_, &QNetworkReply::finished, this, [this, id, destination, artifact]() {
        if (activeId_ != id || !activeReply_) return;
        auto* reply = activeReply_;
        activeReply_ = nullptr;
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status >= 300 && status < 400) {
            const auto target = reply->url().resolved(
                reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
            reply->deleteLater();
            if (!cancelRequested_ && ++downloadRedirects_ <= 5 && target.isValid() &&
                target.scheme() == QLatin1String("https") && !target.host().isEmpty() &&
                target.userInfo().isEmpty()) {
                issueDownloadRequest(target, id, destination, artifact);
                return;
            }
            downloadFile_->cancelWriting();
            downloadFile_.reset();
            downloadHash_.reset();
            const auto error = cancelRequested_ ? ModelOperationError::Cancelled
                                                : ModelOperationError::NetworkFailure;
            finish(cancelRequested_ ? ModelOperationState::Cancelled : ModelOperationState::Failed,
                   error, safeErrorText(error));
            return;
        }
        if (status >= 200 && status < 300) consumeDownloadData(reply->readAll());
        const auto hubError = reply->rawHeader("X-Error-Code");
        const auto networkFailure = status == 401 ? ModelOperationError::AuthenticationRequired
            : status == 403 && hubError == "GatedRepo" ? ModelOperationError::GatedModel
            : status == 403 ? ModelOperationError::AccessDenied
            : status == 429 ? ModelOperationError::RateLimited
            : status == 404 ? ModelOperationError::ModelNotFound
                            : ModelOperationError::NetworkFailure;
        const bool httpOk = reply->error() == QNetworkReply::NoError &&
                            status >= 200 && status < 300;
        reply->deleteLater();
        if (cancelRequested_ || downloadStorageFailed_ || !httpOk) {
            downloadFile_->cancelWriting();
            downloadFile_.reset();
            downloadHash_.reset();
            const auto error = downloadStorageFailed_ ? ModelOperationError::DiskOrStorageFailure
                : cancelRequested_ ? ModelOperationError::Cancelled : networkFailure;
            finish(error == ModelOperationError::Cancelled ? ModelOperationState::Cancelled
                                                           : ModelOperationState::Failed,
                   error, safeErrorText(error));
            return;
        }
        const bool validHeader = validGgufHeader(downloadPrefix_);
        const bool sizeMatches = !artifact.sizeBytes || downloadedBytes_ == *artifact.sizeBytes;
        const bool hashMatches = artifact.artifactHash.isEmpty() ||
            QString::fromLatin1(downloadHash_->result().toHex())
                .compare(artifact.artifactHash, Qt::CaseInsensitive) == 0;
        if (!validHeader || !sizeMatches || !hashMatches || !downloadFile_->commit()) {
            downloadFile_.reset();
            downloadHash_.reset();
            const auto error = validHeader && sizeMatches && hashMatches
                ? ModelOperationError::DiskOrStorageFailure
                : ModelOperationError::IntegrityFailure;
            finish(ModelOperationState::Failed, error, safeErrorText(error));
            return;
        }
        downloadFile_.reset();
        downloadHash_.reset();
        QSaveFile sidecar(destination + QStringLiteral(".source.json"));
        const auto sidecarPayload = QJsonDocument(QJsonObject{
            {QStringLiteral("repositoryId"), artifact.repositoryId},
            {QStringLiteral("filename"), artifact.artifactFilename},
            {QStringLiteral("revision"), artifact.revision},
            {QStringLiteral("sha256"), artifact.artifactHash},
            {QStringLiteral("etag"), artifact.artifactEtag}}).toJson(QJsonDocument::Compact);
        if (!sidecar.open(QIODevice::WriteOnly) ||
            sidecar.write(sidecarPayload) != sidecarPayload.size() || !sidecar.commit()) {
            QFile::remove(destination);
            finish(ModelOperationState::Failed, ModelOperationError::DiskOrStorageFailure,
                   safeErrorText(ModelOperationError::DiskOrStorageFailure));
            return;
        }
        const auto record = records_.value(id);
        if (record.kind == ModelOperationKind::DownloadAndRegister) {
            bool storageFailure = false;
            if (!library_.registerLocalGguf(destination, {}, &storageFailure, &artifact)) {
                QFile::remove(destination);
                QFile::remove(destination + QStringLiteral(".source.json"));
                const auto error = storageFailure ? ModelOperationError::DiskOrStorageFailure
                                                  : ModelOperationError::OperationRejected;
                finish(ModelOperationState::Failed, error, safeErrorText(error));
                return;
            }
            emit catalogChanged(QStringLiteral("llama-cpp-server"));
        }
        auto completed = records_.value(id);
        completed.localFile = destination;
        completed.bytesTransferred = downloadedBytes_;
        completed.progress = 1.0;
        update(completed);
        finish(ModelOperationState::Succeeded, ModelOperationError::None,
               record.kind == ModelOperationKind::DownloadAndRegister
                   ? QStringLiteral("GGUF downloaded and registered.")
                   : QStringLiteral("GGUF downloaded."));
        emit catalogChanged(QStringLiteral("hugging-face"));
    });
}

void ModelOperationService::beginNetwork(const ModelOperationRecord& record) {
    if (record.kind == ModelOperationKind::Refresh &&
        record.providerId != QLatin1String("ollama") &&
        record.providerId != QLatin1String("lm-studio")) {
        finish(ModelOperationState::Failed, ModelOperationError::UnsupportedAction,
               safeErrorText(ModelOperationError::UnsupportedAction));
        return;
    }
    if (record.kind != ModelOperationKind::Refresh &&
        (record.providerId != QLatin1String("ollama") || record.nativeModelId.isEmpty())) {
        finish(ModelOperationState::Failed, ModelOperationError::UnsupportedAction,
               safeErrorText(ModelOperationError::UnsupportedAction));
        return;
    }
    const auto endpoint = endpointFor(record.providerId);
    if (endpoint.scheme() != QLatin1String("http") ||
        (endpoint.host() != QLatin1String("127.0.0.1") &&
         endpoint.host() != QLatin1String("localhost"))) {
        finish(ModelOperationState::Failed, ModelOperationError::AuthenticationOrConfiguration,
               safeErrorText(ModelOperationError::AuthenticationOrConfiguration));
        return;
    }
    QUrl url = endpoint;
    activeEndpoint_ = endpoint;
    url.setPath(record.providerId == QLatin1String("lm-studio")
                    ? QStringLiteral("/api/v1/models")
                    : record.kind == ModelOperationKind::Pull ? QStringLiteral("/api/pull")
                    : record.kind == ModelOperationKind::Remove ? QStringLiteral("/api/delete")
                                                                 : QStringLiteral("/api/tags"));
    url.setQuery(QString());
    url.setFragment(QString());
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    if (record.kind == ModelOperationKind::Refresh) activeReply_ = network_->get(request);
    else {
        request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/json"));
        const auto payload = QJsonDocument(QJsonObject{{QStringLiteral("name"),
                                                        record.nativeModelId}})
                                 .toJson(QJsonDocument::Compact);
        activeReply_ = record.kind == ModelOperationKind::Pull
            ? network_->post(request, payload)
            : network_->sendCustomRequest(request, QByteArrayLiteral("DELETE"), payload);
    }
    pullBuffer_.clear();
    pullSucceeded_ = pullFailed_ = cancelRequested_ = false;
    pullError_ = ModelOperationError::None;
    if (record.kind == ModelOperationKind::Pull) timeout_->stop();
    else timeout_->start(15000);
    const auto id = record.id;
    connect(activeReply_, &QNetworkReply::readyRead, this, [this, id]() {
        if (activeId_ != id || !activeReply_ ||
            records_.value(id).kind != ModelOperationKind::Pull) return;
        pullBuffer_.append(activeReply_->readAll());
        processPullLines(false);
    });
    connect(activeReply_, &QNetworkReply::finished, this, [this, id]() {
        if (activeId_ != id || !activeReply_) return;
        timeout_->stop();
        auto* reply = activeReply_;
        activeReply_ = nullptr;
        const auto record = records_.value(id);
        const bool sameEndpoint = activeEndpoint_ == endpointFor(record.providerId);
        const auto body = reply->readAll();
        const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool httpOk = reply->error() == QNetworkReply::NoError &&
                            statusCode >= 200 && statusCode < 300;
        const auto failure = networkError(reply);
        reply->deleteLater();
        if (record.kind == ModelOperationKind::Pull) {
            pullBuffer_.append(body);
            processPullLines(true);
            if (pullSucceeded_ && !pullFailed_ && (httpOk || cancelRequested_)) {
                finish(ModelOperationState::Succeeded, ModelOperationError::None,
                       QStringLiteral("Model installed."));
                if (sameEndpoint) {
                    models_.applyOllamaModelMutation(record.nativeModelId, true);
                    emit catalogChanged(record.providerId);
                    refreshAfterMutation(record.providerId);
                }
            } else if (cancelRequested_ && !pullSucceeded_) {
                finish(ModelOperationState::Cancelled, ModelOperationError::Cancelled,
                       safeErrorText(ModelOperationError::Cancelled));
            } else {
                const auto error = pullFailed_ ? pullError_
                    : httpOk ? ModelOperationError::MalformedRuntimeResponse : failure;
                finish(ModelOperationState::Failed, error, safeErrorText(error));
            }
            return;
        }
        if (cancelRequested_ && record.kind == ModelOperationKind::Refresh) {
            finish(ModelOperationState::Cancelled, ModelOperationError::Cancelled,
                   safeErrorText(ModelOperationError::Cancelled));
            return;
        }
        if (!httpOk) {
            finish(ModelOperationState::Failed, failure, safeErrorText(failure));
            return;
        }
        if (record.kind == ModelOperationKind::Remove) {
            if (!body.trimmed().isEmpty()) {
                const auto response = QJsonDocument::fromJson(body);
                if (response.isObject() &&
                    response.object().value(QStringLiteral("error")).isString()) {
                    finish(ModelOperationState::Failed, ModelOperationError::OperationRejected,
                           safeErrorText(ModelOperationError::OperationRejected));
                    return;
                }
            }
            finish(ModelOperationState::Succeeded, ModelOperationError::None,
                   QStringLiteral("Model removed."));
            if (sameEndpoint) {
                models_.applyOllamaModelMutation(record.nativeModelId, false);
                emit catalogChanged(record.providerId);
                refreshAfterMutation(record.providerId);
            }
            return;
        }
        if (record.providerId == QLatin1String("lm-studio")) {
            if (!lmStudioCatalog_.acceptResponse(body)) {
                finish(ModelOperationState::Failed,
                       ModelOperationError::MalformedRuntimeResponse,
                       safeErrorText(ModelOperationError::MalformedRuntimeResponse));
                return;
            }
            ProviderDiscoveryOutcome outcome;
            outcome.completed = true;
            models_.acceptProviderDiscovery(record.providerId, lmStudioCatalog_.discoveredModels(),
                                            outcome, models_.beginProviderHealthObservation(), true);
        } else {
            QJsonParseError parseError;
            const auto document = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject() ||
                !document.object().value(QStringLiteral("models")).isArray()) {
                finish(ModelOperationState::Failed,
                       ModelOperationError::MalformedRuntimeResponse,
                       safeErrorText(ModelOperationError::MalformedRuntimeResponse));
                return;
            }
            OllamaModelDiscoveryResult discovery;
            for (const auto& value : document.object().value(QStringLiteral("models")).toArray()) {
                if (!value.isObject()) continue;
                const auto item = value.toObject();
                const auto name = item.value(QStringLiteral("name")).toString().trimmed();
                if (name.isEmpty()) continue;
                OllamaModelSummary model;
                model.name = name;
                model.modifiedAt = item.value(QStringLiteral("modified_at")).toString();
                model.sizeBytes = item.value(QStringLiteral("size")).toVariant().toLongLong();
                model.family = item.value(QStringLiteral("details")).toObject()
                                   .value(QStringLiteral("family")).toString();
                discovery.models.append(model);
            }
            discovery.lifecycle = ChatRequestLifecycle::Completed;
            discovery.errorCategory = ChatProviderErrorCategory::None;
            models_.acceptOllamaDiscovery(discovery, models_.beginProviderHealthObservation());
        }
        finish(ModelOperationState::Succeeded, ModelOperationError::None,
               QStringLiteral("Catalog refreshed."));
        emit catalogChanged(record.providerId);
        if (record.providerId == QLatin1String("ollama") && sameEndpoint)
            refreshOllamaTelemetry();
    });
}

void ModelOperationService::refreshOllamaTelemetry() {
    const auto endpoint = endpointFor(QStringLiteral("ollama"));
    for (const auto& path : {QStringLiteral("/api/version"), QStringLiteral("/api/ps")}) {
        auto url = endpoint;
        url.setPath(path);
        QNetworkRequest request(url);
        request.setTransferTimeout(5000);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::ManualRedirectPolicy);
        auto* reply = network_->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, endpoint, path]() {
            const auto valid = endpoint == endpointFor(QStringLiteral("ollama")) &&
                reply->error() == QNetworkReply::NoError &&
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 &&
                reply->bytesAvailable() <= 256 * 1024;
            const auto body = valid ? reply->readAll() : QByteArray{};
            reply->deleteLater();
            if (!valid) return;
            const auto document = QJsonDocument::fromJson(body);
            if (!document.isObject()) return;
            const auto object = document.object();
            ollamaRuntimeStatus_.runtimeId = QStringLiteral("ollama");
            if (path == QLatin1String("/api/version")) {
                ollamaRuntimeStatus_.version = object.value(QStringLiteral("version")).toString();
            } else {
                if (!object.value(QStringLiteral("models")).isArray()) return;
                ollamaRuntimeStatus_.activeModelIds.clear();
                qint64 vram = 0;
                bool vramReported = false;
                for (const auto& value : object.value(QStringLiteral("models")).toArray()) {
                    const auto item = value.toObject();
                    const auto name = item.value(QStringLiteral("name")).toString();
                    if (!name.isEmpty()) ollamaRuntimeStatus_.activeModelIds << name;
                    if (item.value(QStringLiteral("size_vram")).isDouble()) {
                        vram += item.value(QStringLiteral("size_vram")).toVariant().toLongLong();
                        vramReported = true;
                    }
                }
                ollamaRuntimeStatus_.reportedActiveVramBytes = vramReported
                    ? std::optional<qint64>(vram) : std::nullopt;
                ollamaRuntimeStatus_.activeModelId = ollamaRuntimeStatus_.activeModelIds.size() == 1
                    ? ollamaRuntimeStatus_.activeModelIds.first() : QString{};
            }
            library_.setRuntimeStatus(ollamaRuntimeStatus_);
            emit catalogChanged(QStringLiteral("ollama"));
        });
    }
}

void ModelOperationService::processPullLines(bool finalChunk) {
    while (true) {
        const auto newline = pullBuffer_.indexOf('\n');
        if (newline < 0 && !finalChunk) break;
        if (newline < 0 && pullBuffer_.isEmpty()) break;
        const auto line = (newline >= 0 ? pullBuffer_.left(newline) : pullBuffer_).trimmed();
        pullBuffer_.remove(0, newline >= 0 ? newline + 1 : pullBuffer_.size());
        if (line.isEmpty()) continue;
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            pullFailed_ = true;
            pullError_ = ModelOperationError::MalformedRuntimeResponse;
            continue;
        }
        const auto item = document.object();
        if (item.value(QStringLiteral("error")).isString()) {
            const auto message = item.value(QStringLiteral("error")).toString().toLower();
            pullError_ = message.contains(QStringLiteral("space")) ||
                                 message.contains(QStringLiteral("disk"))
                ? ModelOperationError::DiskOrStorageFailure
                : message.contains(QStringLiteral("not found"))
                    ? ModelOperationError::ModelNotFound
                    : ModelOperationError::OperationRejected;
            pullFailed_ = true;
            continue;
        }
        const auto status = item.value(QStringLiteral("status")).toString();
        if (status == QLatin1String("success")) pullSucceeded_ = true;
        auto record = records_.value(activeId_);
        if (item.value(QStringLiteral("completed")).isDouble())
            record.bytesTransferred = item.value(QStringLiteral("completed")).toVariant().toLongLong();
        if (item.value(QStringLiteral("total")).isDouble())
            record.bytesTotal = item.value(QStringLiteral("total")).toVariant().toLongLong();
        if (record.bytesTotal && *record.bytesTotal > 0 && record.bytesTransferred)
            record.progress = qBound(0.0, double(*record.bytesTransferred) / *record.bytesTotal, 1.0);
        if (pullSucceeded_) {
            record.progress = 1.0;
            record.cancellable = false;
        }
        record.statusText = pullSucceeded_ ? QStringLiteral("Installed.")
            : record.bytesTotal ? QStringLiteral("Downloading model.")
                                : QStringLiteral("Pulling model.");
        update(record);
    }
}

void ModelOperationService::finish(ModelOperationState state, ModelOperationError error,
                                    const QString& status) {
    if (activeId_.isEmpty()) return;
    auto record = records_.value(activeId_);
    record.state = state;
    record.error = error;
    record.statusText = status;
    record.cancellable = false;
    record.endedAt = QDateTime::currentDateTimeUtc();
    if (record.kind == ModelOperationKind::Refresh && state == ModelOperationState::Failed &&
        (record.providerId == QLatin1String("ollama") ||
         record.providerId == QLatin1String("lm-studio"))) {
        if (record.providerId == QLatin1String("ollama")) {
            ollamaRuntimeStatus_.activeModelIds.clear();
            ollamaRuntimeStatus_.activeModelId.clear();
            ollamaRuntimeStatus_.reportedActiveVramBytes.reset();
            library_.setRuntimeStatus(ollamaRuntimeStatus_);
        }
        const auto category = discoveryCategory(error);
        if (record.providerId == QLatin1String("ollama")) {
            OllamaModelDiscoveryResult discovery;
            discovery.errorCategory = category;
            discovery.safeDetail = status;
            models_.acceptOllamaDiscovery(discovery, models_.beginProviderHealthObservation());
        } else if (lmStudioCatalog_.hasSnapshot()) {
            lmStudioCatalog_.markStale();
            ProviderDiscoveryOutcome outcome;
            outcome.category = category;
            models_.acceptProviderDiscovery(record.providerId, {}, outcome,
                                            models_.beginProviderHealthObservation(), true);
        }
        emit catalogChanged(record.providerId);
    }
    update(record);
    activeId_.clear();
    QTimer::singleShot(0, this, [this]() { startNext(); });
}

void ModelOperationService::refreshAfterMutation(const QString& providerId) {
    refresh(providerId);
}

} // namespace sentinel::core
