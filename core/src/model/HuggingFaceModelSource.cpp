// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/model/HuggingFaceModelSource.h"
#include "sentinel/core/network/NetworkPolicyService.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QUrlQuery>
#include <QTimer>
#include <memory>

namespace sentinel::core {
namespace {

constexpr int pageSize = 100;
constexpr int cacheMinutes = 15;

bool validRepoId(const QString& id) {
    static const QRegularExpression pattern(
        QStringLiteral("^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$"));
    return pattern.match(id).hasMatch() && !id.contains(QStringLiteral(".."));
}

bool validFilename(const QString& filename) {
    if (filename.isEmpty() || filename.startsWith(QLatin1Char('/')) ||
        filename.contains(QLatin1Char('\\'))) return false;
    const auto parts = filename.split(QLatin1Char('/'));
    for (const auto& part : parts)
        if (part.isEmpty() || part == QLatin1String(".") || part == QLatin1String("..")) return false;
    return true;
}

bool pinnedRevision(const QString& revision) {
    static const QRegularExpression pattern(QStringLiteral("^[a-fA-F0-9]{40}$"));
    return pattern.match(revision).hasMatch();
}

QString quantization(const QString& filename) {
    static const QRegularExpression pattern(
        QStringLiteral("(?:^|[._-])(Q(?:[2-8]_[A-Z0-9_]+|[48]_0)|F16|F32)\\.gguf$"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match = pattern.match(filename);
    return match.hasMatch() ? match.captured(1).toUpper() : QString{};
}

QString shardGroup(const QString& filename) {
    static const QRegularExpression pattern(
        QStringLiteral("^(.*)-([0-9]{5})-of-([0-9]{5})\\.gguf$"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match = pattern.match(filename);
    return match.hasMatch() ? match.captured(1) + QStringLiteral("/") + match.captured(3)
                            : QString{};
}

QString artifactIdentity(const QString& repositoryId, const QString& revision,
                         const QString& filename) {
    return QStringLiteral("%1@%2:%3").arg(repositoryId, revision, filename);
}

} // namespace

HuggingFaceModelSource::HuggingFaceModelSource(QString cachePath, QObject* parent)
    : QObject(parent), cachePath_(std::move(cachePath)),
      network_(new QNetworkAccessManager(this)) {
    if (cachePath_.isEmpty())
        cachePath_ = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
            .filePath(QStringLiteral("model-library-huggingface.json"));
    storageRoot_ = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("models/huggingface"));
    knownStorageRoots_ << storageRoot_;
    loadCache();
}

HuggingFaceModelSource::Snapshot HuggingFaceModelSource::snapshot() const {
    return {cachedModels_, storageRoot_, knownStorageRoots_, catalogState(), catalogDetail()};
}
HuggingFaceModelSource::Snapshot HuggingFaceModelSource::pageSnapshot(
    const Snapshot& snapshot, int page, int pageSize, int* pageCount) {
    // Page model families, keeping size/quantization variants together.
    static const QRegularExpression encoding("(?:[-_\\s])(?:q\\d(?:_[a-z0-9]+)*|iq\\d(?:_[a-z0-9]+)*|f16|f32|bf16|fp16|fp32)(?=$|[-_\\s])", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression size("(?:[-_\\s])\\d+(?:\\.\\d+)?[bm](?=$|[-_\\s])", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression suffix("[-_\\s]+gguf$", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression trailing("[-_\\s]+$");
    QHash<QString, int> indices;
    QList<int> families;
    for (const auto& value : snapshot.models) {
        const auto object = value.toObject();
        auto key = object.value("id").toString(object.value("modelId").toString()).toLower();
        key.remove(encoding).remove(size).remove(suffix).remove(trailing);
        if (!indices.contains(key)) indices.insert(key, indices.size());
        families.append(indices.value(key));
    }
    pageSize = qMax(1, pageSize);
    const int pages = qMax(1, (indices.size() + pageSize - 1) / pageSize);
    if (pageCount) *pageCount = pages;
    const int first = qBound(0, page, pages - 1) * pageSize;
    Snapshot result = snapshot;
    result.models = {};
    for (qsizetype i = 0; i < families.size(); ++i)
        if (families[i] >= first && families[i] < first + pageSize)
            result.models.append(snapshot.models.at(i));
    return result;
}

HuggingFaceModelSource::HuggingFaceModelSource(const Snapshot& snapshot)
    : storageRoot_(snapshot.storageRoot), knownStorageRoots_(snapshot.knownStorageRoots),
      cachedModels_(snapshot.models), state_(snapshot.state), detail_(snapshot.detail) {}
void HuggingFaceModelSource::visitSnapshot(const Snapshot& snapshot,
    const std::function<void(const HuggingFaceRepository&, const QList<ModelLibraryEntry>&)>& visitor,
    const std::function<bool()>& cancelled) {
    // This instance owns only immutable metadata and filesystem lookup configuration.
    // It creates no network objects and never touches the live source on another thread.
    HuggingFaceModelSource source(snapshot);
    for (const auto& value : snapshot.models) {
        if (cancelled && cancelled()) break;
        const auto repository = source.parseRepository(value.toObject());
        if (!repository.id.isEmpty()) visitor(repository, source.entriesFor(repository));
    }
}

QString HuggingFaceModelSource::sourceId() const { return QStringLiteral("hugging-face"); }
QString HuggingFaceModelSource::providerId() const { return {}; }
HuggingFaceCatalogState HuggingFaceModelSource::catalogState() const {
    if ((state_ == HuggingFaceCatalogState::Current || state_ == HuggingFaceCatalogState::Empty) &&
        fetchedAt_.isValid() &&
        fetchedAt_.secsTo(QDateTime::currentDateTimeUtc()) >= cacheMinutes * 60)
        return cachedModels_.isEmpty() ? HuggingFaceCatalogState::Unavailable
                                        : HuggingFaceCatalogState::Stale;
    return state_;
}
QString HuggingFaceModelSource::catalogDetail() const {
    if (detail_.isEmpty() && fetchedAt_.isValid())
        return QStringLiteral("Cached Hugging Face metadata.");
    return catalogState() == HuggingFaceCatalogState::Stale &&
           state_ == HuggingFaceCatalogState::Current
        ? QStringLiteral("Cached Hugging Face metadata; refresh required.") : detail_;
}
QString HuggingFaceModelSource::cachedQuery() const { return cachedQuery_; }
QDateTime HuggingFaceModelSource::fetchedAt() const { return fetchedAt_; }
QString HuggingFaceModelSource::cachePath() const { return cachePath_; }

bool HuggingFaceModelSource::clearMetadataCache(int olderThanDays) {
    if (activeReply_) return false;
    const QFileInfo file(cachePath_);
    if (!file.exists()) {
        cachedModels_ = {};
        cachedQuery_.clear();
        nextPage_ = QUrl{};
        fetchedAt_ = {};
        state_ = HuggingFaceCatalogState::Unavailable;
        emit catalogChanged();
        return true;
    }
    const auto root = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).canonicalPath();
    if (root.isEmpty() || file.isSymLink() ||
        !file.canonicalFilePath().startsWith(root + QDir::separator())) return false;
    if (olderThanDays > 0 && file.lastModified().toUTC() >=
            QDateTime::currentDateTimeUtc().addDays(-olderThanDays)) return true;
    if (!QFile::remove(cachePath_)) return false;
    cachedModels_ = {};
    cachedQuery_.clear();
    nextPage_ = QUrl{};
    fetchedAt_ = {};
    state_ = HuggingFaceCatalogState::Unavailable;
    emit catalogChanged();
    return true;
}

void HuggingFaceModelSource::setTokenProvider(std::function<QString()> provider) {
    tokenProvider_ = std::move(provider);
}

QString HuggingFaceModelSource::token() const {
    return tokenProvider_ ? tokenProvider_().trimmed() : QString{};
}

void HuggingFaceModelSource::setStorageRoot(const QString& path) {
    if (path.trimmed().isEmpty()) return;
    const auto resolved = QDir(path).absolutePath();
    if (resolved == storageRoot_) return;
    storageRoot_ = resolved;
    if (!knownStorageRoots_.contains(resolved)) knownStorageRoots_ << resolved;
    emit catalogChanged();
}

void HuggingFaceModelSource::setKnownStorageRoots(const QStringList& roots) {
    knownStorageRoots_ = roots;
    if (!knownStorageRoots_.contains(storageRoot_)) knownStorageRoots_ << storageRoot_;
    emit catalogChanged();
}

QString HuggingFaceModelSource::storageRoot() const { return storageRoot_; }

QString HuggingFaceModelSource::destinationPath(const ModelLibraryEntry& entry) const {
    return destinationPathAtRoot(entry, storageRoot_);
}

QString HuggingFaceModelSource::destinationPathAtRoot(const ModelLibraryEntry& entry,
                                                      const QString& root) const {
    if (entry.source.id != sourceId() || !validRepoId(entry.repositoryId) ||
        !validFilename(entry.artifactFilename) || !pinnedRevision(entry.revision) ||
        !knownStorageRoots_.contains(root)) return {};
    const auto digest = QCryptographicHash::hash(
        artifactIdentity(entry.repositoryId, entry.revision, entry.artifactFilename).toUtf8(),
        QCryptographicHash::Sha256).toHex();
    return QDir(root).filePath(QString::fromLatin1(digest) + QLatin1Char('/') +
                                       QFileInfo(entry.artifactFilename).fileName());
}

QUrl HuggingFaceModelSource::downloadUrl(const ModelLibraryEntry& entry) const {
    if (destinationPath(entry).isEmpty()) return {};
    QUrl url(QStringLiteral("https://huggingface.co"));
    url.setPath(QStringLiteral("/%1/resolve/%2/%3")
                    .arg(entry.repositoryId, entry.revision, entry.artifactFilename));
    return url;
}

bool HuggingFaceModelSource::isDownloaded(const ModelLibraryEntry& entry) const {
    return !existingPath(entry).isEmpty();
}

QString HuggingFaceModelSource::existingPath(const ModelLibraryEntry& entry) const {
    const auto destination = destinationPath(entry);
    if (destination.isEmpty()) return {};
    const auto relative = QDir(storageRoot_).relativeFilePath(destination);
    for (const auto& root : knownStorageRoots_) {
    const auto path = QDir(root).filePath(relative);
    QFile file(path + QStringLiteral(".source.json"));
    if (!QFileInfo::exists(path) || !file.open(QIODevice::ReadOnly)) continue;
    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) continue;
    const auto info = document.object();
    if (info.value(QStringLiteral("repositoryId")).toString() == entry.repositoryId &&
           info.value(QStringLiteral("filename")).toString() == entry.artifactFilename &&
           info.value(QStringLiteral("revision")).toString() == entry.revision &&
           info.value(QStringLiteral("sha256")).toString() == entry.artifactHash &&
           (!entry.sizeBytes || QFileInfo(path).size() == *entry.sizeBytes)) return path;
    }
    return {};
}

HuggingFaceRepository
HuggingFaceModelSource::parseRepository(const QJsonObject& object) const {
    HuggingFaceRepository repository;
    repository.id = object.value(QStringLiteral("id")).toString();
    if (repository.id.isEmpty())
        repository.id = object.value(QStringLiteral("modelId")).toString();
    if (!validRepoId(repository.id)) return {};
    repository.metadata = object;
    repository.publisher = repository.id.section(QLatin1Char('/'), 0, 0);
    repository.displayName = repository.id.section(QLatin1Char('/'), 1);
    repository.revision = object.value(QStringLiteral("sha")).toString();
    if (!pinnedRevision(repository.revision)) repository.revision.clear();
    repository.lastUpdated = object.value(QStringLiteral("lastModified")).toString();
    repository.pipelineTask = object.value("pipeline_tag").toString();
    if (object.value("downloads").isDouble())
        repository.downloads = object.value("downloads").toInteger();
    if (object.value("likes").isDouble())
        repository.likes = object.value("likes").toInteger();
    repository.gated =
        object.value("gated").toBool() ||
        (object.value("gated").isString() && object.value("gated").toString() != "false");
    const auto card = object.value(QStringLiteral("cardData")).toObject();
    repository.license = card.value(QStringLiteral("license")).toString();
    if (repository.license.isEmpty())
        repository.license = object.value(QStringLiteral("license")).toString();
    repository.architecture = object.value(QStringLiteral("config")).toObject()
                                  .value(QStringLiteral("model_type")).toString();
    if (repository.architecture.isEmpty()) repository.architecture = object.value("gguf").toObject().value("architecture").toString();
    if (repository.architecture.isEmpty()) repository.architecture = object.value("config").toObject().value("architectures").toArray().isEmpty() ? QString{} : object.value("config").toObject().value("architectures").toArray().first().toString();
    for (const auto& tag : object.value(QStringLiteral("tags")).toArray())
        if (tag.isString() && repository.tags.size() < 80) repository.tags.append(tag.toString());
    const auto task = object.value(QStringLiteral("pipeline_tag")).toString();
    if (!task.isEmpty() && !repository.tags.contains(task))
        repository.tags.append(task);
    if (repository.license.isEmpty()) {
        for (const auto& tag : repository.tags) {
            if (tag.startsWith(QStringLiteral("license:"))) {
                repository.license = tag.mid(8);
                break;
            }
        }
    }
    for (const auto& sibling : object.value(QStringLiteral("siblings")).toArray()) {
        if (!sibling.isObject()) continue;
        const auto data = sibling.toObject();
        HuggingFaceArtifact artifact;
        artifact.repositoryId = repository.id;
        artifact.filename = data.value(QStringLiteral("rfilename")).toString();
        if (!validFilename(artifact.filename)) continue;
        artifact.revision = repository.revision;
        artifact.blobId = data.value(QStringLiteral("blobId")).toString();
        artifact.etag = data.value(QStringLiteral("etag")).toString();
        const auto lfs = data.value(QStringLiteral("lfs")).toObject();
        artifact.sha256 = lfs.value(QStringLiteral("sha256")).toString();
        if (artifact.sha256.isEmpty()) artifact.sha256 = lfs.value(QStringLiteral("oid")).toString();
        if (!QRegularExpression(QStringLiteral("^[a-fA-F0-9]{64}$"))
                 .match(artifact.sha256).hasMatch()) artifact.sha256.clear();
        const auto size = data.value(QStringLiteral("size")).toVariant().toLongLong();
        const auto lfsSize = lfs.value(QStringLiteral("size")).toVariant().toLongLong();
        if (size > 0 || lfsSize > 0) artifact.sizeBytes = lfsSize > 0 ? lfsSize : size;
        if (artifact.filename.endsWith(QStringLiteral(".gguf"), Qt::CaseInsensitive)) {
            artifact.format = QStringLiteral("GGUF");
            artifact.quantization = quantization(artifact.filename);
            artifact.shardGroup = shardGroup(artifact.filename);
        } else if (artifact.filename.endsWith(QStringLiteral(".safetensors"), Qt::CaseInsensitive)) {
            artifact.format = QStringLiteral("safetensors");
        } else if (artifact.filename.endsWith(".bin")) artifact.format = "PyTorch";
        else if (artifact.filename.endsWith(".onnx")) artifact.format = "ONNX";
        repository.artifacts.append(artifact);
    }
    return repository;
}

QList<HuggingFaceRepository> HuggingFaceModelSource::repositories() const {
    QList<HuggingFaceRepository> result;
    for (const auto& value : cachedModels_) {
        if (!value.isObject()) continue;
        auto repository = parseRepository(value.toObject());
        if (!repository.id.isEmpty()) result.append(repository);
    }
    return result;
}

QList<ModelLibraryEntry>
HuggingFaceModelSource::entriesFor(const HuggingFaceRepository& repository) const {
    QList<ModelLibraryEntry> result;
    const auto baseUrl = QStringLiteral("https://huggingface.co/%1").arg(repository.id);
    auto add = [&](const HuggingFaceArtifact* artifact) {
        ModelLibraryEntry entry;
        entry.source = {sourceId(), QStringLiteral("Hugging Face"),
                        QStringLiteral("Hugging Face Hub API"), baseUrl};
        entry.repositoryId = repository.id;
        entry.publisher = repository.publisher;
        entry.displayName = artifact ? QFileInfo(artifact->filename).fileName()
                                     : repository.displayName;
        entry.license = repository.license;
        entry.architecture = repository.architecture;
        entry.revision = repository.revision;
        entry.lastUpdated = repository.lastUpdated;
        entry.tags = repository.tags;
        entry.local = false;
        entry.availability = catalogState() == HuggingFaceCatalogState::Current ||
                                     catalogState() == HuggingFaceCatalogState::Empty
            ? ModelLibraryAvailability::Available : ModelLibraryAvailability::Unknown;
        switch (catalogState()) {
        case HuggingFaceCatalogState::Current:
            entry.catalog = ModelLibraryCatalogState::Current; break;
        case HuggingFaceCatalogState::AuthenticationRequired:
            entry.catalog = ModelLibraryCatalogState::AuthenticationRequired; break;
        case HuggingFaceCatalogState::AccessDenied:
            entry.catalog = ModelLibraryCatalogState::AccessDenied; break;
        case HuggingFaceCatalogState::GatedModel:
            entry.catalog = ModelLibraryCatalogState::GatedModel; break;
        case HuggingFaceCatalogState::RateLimited:
            entry.catalog = ModelLibraryCatalogState::RateLimited; break;
        case HuggingFaceCatalogState::Empty:
            entry.catalog = ModelLibraryCatalogState::Empty; break;
        case HuggingFaceCatalogState::Unavailable:
            entry.catalog = ModelLibraryCatalogState::Unavailable; break;
        case HuggingFaceCatalogState::Offline:
            entry.catalog = ModelLibraryCatalogState::Unavailable; break;
        case HuggingFaceCatalogState::Stale:
            entry.catalog = ModelLibraryCatalogState::StaleCached; break;
        }
        entry.catalogDetail = catalogDetail();
        entry.installation = ModelInstallationStrategy::SentinelManaged;
        entry.sourceProvenance = QStringLiteral("Hugging Face API; revision pinned when reported");
        if (artifact) {
            entry.artifactId = artifactIdentity(repository.id, repository.revision,
                                                artifact->filename);
            entry.artifactFilename = artifact->filename;
        entry.artifactHash = artifact->sha256;
            entry.artifactBlobId = artifact->blobId;
            entry.artifactEtag = artifact->etag;
            entry.format = artifact->format;
            entry.quantization = artifact->quantization;
            entry.shardGroup = artifact->shardGroup;
            entry.sizeBytes = artifact->sizeBytes;
            entry.compatibilityHint = artifact->format == QLatin1String("GGUF") &&
                                      artifact->shardGroup.isEmpty()
                ? QStringLiteral("GGUF artifact; llama.cpp support depends on the runtime.")
                : QString{};
            QUrl artifactUrl(QStringLiteral("https://huggingface.co"));
            artifactUrl.setPath(QStringLiteral("/%1/blob/%2/%3")
                .arg(repository.id,
                     repository.revision.isEmpty() ? QStringLiteral("main")
                                                    : repository.revision,
                     artifact->filename));
            entry.source.url = artifactUrl.toString(QUrl::FullyEncoded);
            if (isDownloaded(entry)) {
                entry.localFile = existingPath(entry);
                entry.installed = ModelLibraryInstalledState::Installed;
            } else entry.installed = ModelLibraryInstalledState::NotInstalled;
        } else {
            entry.artifactId = artifactIdentity(repository.id, repository.revision, QString{});
            entry.installed = ModelLibraryInstalledState::Unknown;
        }
        result.append(entry);
    };
    if (repository.artifacts.isEmpty()) add(nullptr);
    else for (const auto& artifact : repository.artifacts) add(&artifact);
    return result;
}

QList<ModelLibraryEntry> HuggingFaceModelSource::entries() const {
    QList<ModelLibraryEntry> result;
    for (const auto& repository : repositories()) result.append(entriesFor(repository));
    return result;
}

QList<ModelLibraryEntry> HuggingFaceModelSource::query(const HuggingFaceSearchQuery& filter) const {
    QList<ModelLibraryEntry> result;
    for (const auto& entry : entries()) {
        if (!filter.text.isEmpty() && !entry.repositoryId.contains(filter.text, Qt::CaseInsensitive) &&
            !entry.displayName.contains(filter.text, Qt::CaseInsensitive)) continue;
        if (filter.ggufOnly && entry.format != QLatin1String("GGUF")) continue;
        if (!filter.publisher.isEmpty() &&
            entry.publisher.compare(filter.publisher, Qt::CaseInsensitive)) continue;
        if (!filter.family.isEmpty() &&
            entry.family.compare(filter.family, Qt::CaseInsensitive)) continue;
        if (!filter.architecture.isEmpty() &&
            entry.architecture.compare(filter.architecture, Qt::CaseInsensitive)) continue;
        if (!filter.quantization.isEmpty() &&
            entry.quantization.compare(filter.quantization, Qt::CaseInsensitive)) continue;
        const bool candidate = entry.format == QLatin1String("GGUF") &&
                               entry.shardGroup.isEmpty();
        if (filter.llamaCppCandidate && candidate != *filter.llamaCppCandidate) continue;
        result.append(entry);
    }
    return result;
}

void HuggingFaceModelSource::search(const QString& text, bool forceRefresh) {
    searchCatalog(text, {}, "downloads", forceRefresh);
}

void HuggingFaceModelSource::searchCatalog(const QString& text, const QString& task,
                                           const QString& sort, bool forceRefresh, bool ggufOnly) {
    const auto query = !ggufOnly && task.isEmpty() && sort == "downloads"
                           ? text.trimmed()
                           : QString::fromUtf8(QJsonDocument(QJsonObject{{"text", text.trimmed()},
                                                                         {"task", task},
                                                                         {"sort", sort}, {"ggufOnly", ggufOnly}})
                                                   .toJson(QJsonDocument::Compact));
    if (!forceRefresh && query == cachedQuery_ && fetchedAt_.isValid() &&
        fetchedAt_.secsTo(QDateTime::currentDateTimeUtc()) < cacheMinutes * 60 &&
        (state_ == HuggingFaceCatalogState::Current || state_ == HuggingFaceCatalogState::Empty)) {
        emit requestFinished(true);
        if (autoFetch_ && hasMore()) QTimer::singleShot(0, this, &HuggingFaceModelSource::fetchMore);
        return;
    }
    if (activeReply_) { auto* previous = activeReply_; activeReply_ = nullptr; previous->abort(); }
    if (query != cachedQuery_) {
        cachedModels_ = {};
        fetchedAt_ = {};
        cachedQuery_ = query;
        nextPage_ = QUrl{};
        emit catalogChanged();
    }
    visitedPages_.clear();
    QUrl url(QStringLiteral("https://huggingface.co/api/models"));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("search"), text.trimmed());
    if (!task.isEmpty())
        parameters.addQueryItem(QStringLiteral("pipeline_tag"), task);
    if (ggufOnly)
        parameters.addQueryItem(QStringLiteral("filter"), QStringLiteral("gguf"));
    parameters.addQueryItem(QStringLiteral("sort"), sort == "lastModified" ? sort : "downloads");
    parameters.addQueryItem(QStringLiteral("direction"), QStringLiteral("-1"));
    parameters.addQueryItem(QStringLiteral("limit"), QString::number(pageSize));
    parameters.addQueryItem(QStringLiteral("full"), QStringLiteral("true"));
    parameters.addQueryItem(QStringLiteral("blobs"), QStringLiteral("true"));
    parameters.addQueryItem(QStringLiteral("cardData"), QStringLiteral("true"));
    url.setQuery(parameters);
    request(url, query, {});
}

QUrl HuggingFaceModelSource::continuationUrl(const QString& links) {
    static const QRegularExpression nextLink(QStringLiteral("<([^>]+)>;\\s*rel=\"next\""));
    const auto match = nextLink.match(links);
    const QUrl candidate(match.captured(1));
    // A server-supplied continuation must never redirect a token to another origin.
    return match.hasMatch() && candidate.scheme() == "https" &&
                   candidate.host() == "huggingface.co" && candidate.port(-1) == -1 &&
                   candidate.userInfo().isEmpty() && candidate.path() == "/api/models"
               ? candidate
               : QUrl{};
}

void HuggingFaceModelSource::fetchMore() {
    if (!fetching() && hasMore())
        request(nextPage_, cachedQuery_, {}, true);
}

void HuggingFaceModelSource::fetchRepository(const QString& repositoryId) {
    if (!validRepoId(repositoryId)) {
        detail_ = QStringLiteral("Invalid repository identity.");
        emit requestFinished(false);
        return;
    }
    QUrl url(QStringLiteral("https://huggingface.co"));
    url.setPath(QStringLiteral("/api/models/%1").arg(repositoryId));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("blobs"), QStringLiteral("true"));
    url.setQuery(parameters);
    request(url, cachedQuery_, repositoryId);
}

void HuggingFaceModelSource::request(const QUrl& url, const QString& query,
                                     const QString& repositoryId, bool append) {
    if (activeReply_) { auto* previous = activeReply_; activeReply_ = nullptr; previous->abort(); }
    if (repositoryId.isEmpty()) visitedPages_.insert(url.toString());
    responseBuffer_.clear();
    responseTooLarge_ = false;
    const auto decision = NetworkPolicyService::instance().check(url);
    if (decision != NetworkDecision::Allowed) {
        state_ = HuggingFaceCatalogState::Offline;
        detail_ = NetworkPolicyService::code(decision);
        emit catalogChanged();
        emit requestFinished(false);
        return;
    }
    QNetworkRequest request(url);
    request.setTransferTimeout(12000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::SameOriginRedirectPolicy);
    const auto credential = token();
    if (!credential.isEmpty())
        request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + credential.toUtf8());
    activeReply_ = network_->get(request);
    auto* reply = activeReply_;
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        if (reply != activeReply_) return;
        responseBuffer_.append(reply->readAll());
        if (responseBuffer_.size() > 8 * 1024 * 1024) {
            responseTooLarge_ = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, query, repositoryId, append]() {
        if (reply != activeReply_) { reply->deleteLater(); return; }
        activeReply_ = nullptr;
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto errorCode = reply->rawHeader("X-Error-Code");
        const auto links = QString::fromUtf8(reply->rawHeader("Link"));
        responseBuffer_.append(reply->readAll());
        const auto body = responseBuffer_;
        const bool okay = reply->error() == QNetworkReply::NoError &&
                          status >= 200 && status < 300 && !responseTooLarge_ &&
                          body.size() <= 8 * 1024 * 1024;
        reply->deleteLater();
        if (!okay) {
            state_ = status == 401 ? HuggingFaceCatalogState::AuthenticationRequired
                : status == 403 && errorCode == "GatedRepo"
                    ? HuggingFaceCatalogState::GatedModel
                : status == 403 ? HuggingFaceCatalogState::AccessDenied
                : status == 429 ? HuggingFaceCatalogState::RateLimited
                : cachedModels_.isEmpty() ? HuggingFaceCatalogState::Unavailable
                                           : HuggingFaceCatalogState::Stale;
            detail_ = status == 401
                ? QStringLiteral("Hugging Face authentication required.")
                : status == 403 && errorCode == "GatedRepo"
                    ? QStringLiteral("Access to this gated model has not been granted.")
                : status == 403 ? QStringLiteral("Hugging Face access denied.")
                : status == 429 ? QStringLiteral("Hugging Face rate limit reached.")
                                : QStringLiteral("Hugging Face catalog unavailable; cached metadata retained.");
            emit catalogChanged();
            emit requestFinished(false);
            return;
        }
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(body, &error);
        if (error.error != QJsonParseError::NoError ||
            (repositoryId.isEmpty() && !document.isArray()) ||
            (!repositoryId.isEmpty() && !document.isObject())) {
            state_ = cachedModels_.isEmpty() ? HuggingFaceCatalogState::Unavailable
                                             : HuggingFaceCatalogState::Stale;
            detail_ = QStringLiteral("Hugging Face returned invalid catalog metadata.");
            emit catalogChanged();
            emit requestFinished(false);
            return;
        }
        if (repositoryId.isEmpty()) {
            QJsonArray next = append ? cachedModels_ : QJsonArray{};
            QSet<QString> seen;
            for (const auto& old : next) seen.insert(old.toObject().value("id").toString());
            for (const auto& value : document.array()) {
                const auto id = value.isObject() ? parseRepository(value.toObject()).id : QString{};
                if (!id.isEmpty() && !seen.contains(id)) {
                    next.append(value);
                    seen.insert(id);
                }
            }
            cachedModels_ = next;
            nextPage_ = continuationUrl(links);
            if (visitedPages_.contains(nextPage_.toString())) nextPage_ = QUrl{};
            cachedQuery_ = query;
            fetchedAt_ = QDateTime::currentDateTimeUtc();
        } else {
            const auto object = document.object();
            if (parseRepository(object).id != repositoryId) {
                state_ = cachedModels_.isEmpty() ? HuggingFaceCatalogState::Unavailable
                                                 : HuggingFaceCatalogState::Stale;
                detail_ = QStringLiteral("Repository metadata identity did not match.");
                emit catalogChanged();
                emit requestFinished(false);
                return;
            }
            bool replaced = false;
            for (int i = 0; i < cachedModels_.size(); ++i) {
                if (parseRepository(cachedModels_.at(i).toObject()).id == repositoryId) {
                    cachedModels_.replace(i, object);
                    replaced = true;
                    break;
                }
            }
            if (!replaced) cachedModels_.append(object);
            const auto revision = object.value("sha").toString();
            if (pinnedRevision(revision)) {
                for (const auto& sibling : object.value("siblings").toArray()) {
                    const auto name = sibling.toObject().value("rfilename").toString();
                    if (name == "config.json" || name == "README.md") fetchSupplement(repositoryId, revision, name);
                }
            }
            if (!fetchedAt_.isValid()) fetchedAt_ = QDateTime::currentDateTimeUtc();
        }
        state_ = cachedModels_.isEmpty() ? HuggingFaceCatalogState::Empty
                                         : HuggingFaceCatalogState::Current;
        detail_ = cachedModels_.isEmpty() ? QStringLiteral("No matching repositories.")
                                           : QStringLiteral("Hugging Face catalog available.");
        saveCache();
        emit catalogChanged();
        emit requestFinished(true);
        if (autoFetch_ && hasMore()) QTimer::singleShot(750, this, [this, query] {
            if (autoFetch_ && query == cachedQuery_ && !fetching() && hasMore()) fetchMore();
        });
    });
}

void HuggingFaceModelSource::fetchSupplement(const QString& repositoryId, const QString& revision, const QString& filename) {
    QUrl url("https://huggingface.co");
    url.setPath(QString("/%1/resolve/%2/%3").arg(repositoryId, revision, filename));
    if (NetworkPolicyService::instance().check(url) != NetworkDecision::Allowed) return;
    QNetworkRequest request(url);
    request.setTransferTimeout(12000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
    if (!token().isEmpty()) request.setRawHeader("Authorization", "Bearer " + token().toUtf8());
    auto* reply = network_->get(request);
    reply->setReadBufferSize(512 * 1024);
    auto bytes = std::make_shared<QByteArray>();
    connect(reply, &QNetworkReply::readyRead, this, [reply, bytes] {
        bytes->append(reply->readAll());
        if (bytes->size() > 256 * 1024) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, bytes, repositoryId, revision, filename] {
        bytes->append(reply->readAll());
        const bool success = reply->error() == QNetworkReply::NoError && bytes->size() <= 256 * 1024;
        reply->deleteLater();
        if (!success) return;
        for (int i = 0; i < cachedModels_.size(); ++i) {
            auto object = cachedModels_.at(i).toObject();
            if (object.value("id").toString() != repositoryId || object.value("sha").toString() != revision) continue;
            if (filename == "config.json") {
                const auto config = QJsonDocument::fromJson(*bytes);
                if (!config.isObject()) return;
                auto merged = object.value("config").toObject();
                const auto fields = config.object();
                for (auto it = fields.begin(); it != fields.end(); ++it) merged[it.key()] = it.value();
                object["config"] = merged;
            } else object["modelCard"] = QString::fromUtf8(*bytes);
            cachedModels_.replace(i, object);
            saveCache(); emit catalogChanged(); return;
        }
    });
}

void HuggingFaceModelSource::loadCache() {
    QFile file(cachePath_);
    if (QFileInfo(file).size() > 64 * 1024 * 1024 || !file.open(QIODevice::ReadOnly)) return;
    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) return;
    const auto data = document.object();
    cachedQuery_ = data.value(QStringLiteral("query")).toString();
    nextPage_ = continuationUrl(data.value("nextPage").toString());
    fetchedAt_ = QDateTime::fromString(data.value(QStringLiteral("fetchedAt")).toString(),
                                       Qt::ISODateWithMs);
    for (const auto& item : data.value(QStringLiteral("models")).toArray()) {
        if (item.isObject() && !parseRepository(item.toObject()).id.isEmpty())
            cachedModels_.append(item);
    }
    if (fetchedAt_.isValid() &&
        fetchedAt_.secsTo(QDateTime::currentDateTimeUtc()) < cacheMinutes * 60)
        state_ = cachedModels_.isEmpty() ? HuggingFaceCatalogState::Empty
                                         : HuggingFaceCatalogState::Current;
    else if (!cachedModels_.isEmpty()) state_ = HuggingFaceCatalogState::Stale;
    detail_ = state_ == HuggingFaceCatalogState::Stale
        ? QStringLiteral("Cached Hugging Face metadata; refresh required.") : QString{};
}

bool HuggingFaceModelSource::saveCache() const {
    if (!QDir().mkpath(QFileInfo(cachePath_).absolutePath())) return false;
    QSaveFile file(cachePath_);
    if (!file.open(QIODevice::WriteOnly)) return false;
    const auto payload =
        QJsonDocument(
            QJsonObject{{QStringLiteral("nextPage"),
                         nextPage_.isEmpty()
                             ? QString{}
                             : QString("<%1>; rel=\"next\"").arg(nextPage_.toString())},
                        {QStringLiteral("query"), cachedQuery_},
                        {QStringLiteral("fetchedAt"), fetchedAt_.toString(Qt::ISODateWithMs)},
                        {QStringLiteral("models"), cachedModels_}})
            .toJson(QJsonDocument::Compact);
    if (payload.size() > 64 * 1024 * 1024 || file.write(payload) != payload.size()) return false;
    return file.commit();
}

} // namespace sentinel::core
