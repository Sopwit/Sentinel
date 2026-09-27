// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/model/ModelService.h"
#include "sentinel/core/model/HardwareCapabilityService.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <optional>

namespace sentinel::core {

bool validGgufHeader(const QByteArray& prefix);

enum class ModelLibraryCatalogState {
    Current, StaleCached, Unavailable, AuthenticationRequired, AccessDenied,
    GatedModel, RateLimited,
    Empty, ConfiguredOnly, LocalOnly
};
enum class ModelInstallationStrategy {
    SentinelManaged, RuntimeManaged, ExternalApplicationManaged,
    CloudNoInstall, ImportLocal, Unsupported
};
enum class ModelLibraryAction {
    Select, Install, Pull, Import, Remove, OpenExternalManager,
    RevealLocalFile, ViewSource, RemoveRegistration, Download, DownloadAndRegister
};
enum class ModelLibraryAvailability { Available, Unavailable, Unknown };
enum class ModelLibraryInstalledState { Installed, NotInstalled, Unknown };
enum class ModelCompatibility { Compatible, LikelyCompatible, ResourceConstrained,
                                Incompatible, Unknown };
enum class ModelResourceEvidence { Unknown, Reported, Estimated };

struct ModelRuntimeStatus {
    QString runtimeId;
    std::optional<bool> installed;
    std::optional<bool> configured;
    std::optional<bool> reachable;
    QString version;
    QString activeModelId;
    QStringList activeModelIds;
    QString accelerationBackend;
    std::optional<qint64> reportedActiveVramBytes;
};

struct ModelFitInfo {
    ModelCompatibility state = ModelCompatibility::Unknown;
    QStringList reasons;
    ModelResourceEvidence diskEvidence = ModelResourceEvidence::Unknown;
    ModelResourceEvidence memoryEvidence = ModelResourceEvidence::Unknown;
    std::optional<qint64> requiredDiskBytes;
    std::optional<qint64> estimatedMemoryLowerBoundBytes;
    QString executionSuitability;
};

struct ModelLibraryProvider {
    QString id;
    QString displayName;
    ProviderKind kind = ProviderKind::Local;
};

struct ModelLibraryRuntime {
    QString id;
    QString displayName;
};

struct ModelLibrarySource {
    QString id;
    QString displayName;
    QString provenance;
    QString url;
};

struct ModelLibraryEntry {
    QString id;
    QString displayName;
    ModelLibraryProvider provider;
    ModelLibraryRuntime runtime;
    ModelLibrarySource source;
    QString nativeModelId;
    QString publisher;
    QString family;
    QString architecture;
    ModelCapabilities capabilities;
    bool local = true;
    ModelLibraryInstalledState installed = ModelLibraryInstalledState::Unknown;
    ModelLibraryAvailability availability = ModelLibraryAvailability::Unknown;
    ModelInstallationStrategy installation = ModelInstallationStrategy::Unsupported;
    QString artifactId;
    QString repositoryId;
    QString artifactFilename;
    QString revision;
    QString artifactHash;
    QString artifactBlobId;
    QString artifactEtag;
    QString license;
    QString lastUpdated;
    QString shardGroup;
    QString compatibilityHint;
    QStringList tags;
    QString localFile;
    bool managedStorage = false;
    ModelFitInfo fit;
    QList<ModelRuntimeStatus> runtimeStatuses;
    bool registeredLocalFile = false;
    QString externalManagerUrl;
    bool externalManagerAvailable = false;
    QString format;
    QString quantization;
    std::optional<qint64> sizeBytes;
    std::optional<qint64> diskBytes;
    ModelLibraryCatalogState catalog = ModelLibraryCatalogState::Unavailable;
    QString catalogDetail;
    QString sourceProvenance;
    QList<ModelLibraryAction> actions;
};

struct ModelLibraryProviderState {
    ModelLibraryProvider provider;
    ModelLibraryRuntime runtime;
    ModelLibraryCatalogState catalog = ModelLibraryCatalogState::Unavailable;
    QString detail;
};

struct ModelLibraryQuery {
    QString text;
    QStringList providerIds;
    QStringList runtimeIds;
    std::optional<bool> local;
    std::optional<bool> installed;
    std::optional<bool> available;
    std::optional<bool> toolCalling;
    std::optional<bool> structuredOutput;
    std::optional<bool> vision;
    std::optional<bool> audio;
    QString family;
    QString format;
    QString quantization;
    QString publisher;
    QString architecture;
    std::optional<bool> gguf;
    std::optional<bool> llamaCppCandidate;
};

// Native source metadata can enrich endpoint discovery without becoming a
// second authority for capabilities or provider/model resolution.
class IModelLibrarySourceAdapter {
public:
    virtual ~IModelLibrarySourceAdapter() = default;
    virtual QString sourceId() const = 0;
    virtual QString providerId() const = 0;
    virtual QList<ModelLibraryEntry> entries() const = 0;
};

class LMStudioNativeCatalogAdapter final : public IModelLibrarySourceAdapter {
public:
    QString sourceId() const override;
    QString providerId() const override;
    QList<ModelLibraryEntry> entries() const override;
    QList<OllamaModelSummary> discoveredModels() const;
    bool hasSnapshot() const;
    bool acceptResponse(const QByteArray& response);
    void markStale();
    void clear();

private:
    QList<ModelLibraryEntry> entries_;
    QList<OllamaModelSummary> discovered_;
    bool hasSnapshot_ = false;
};

class ModelLibraryService final {
public:
    explicit ModelLibraryService(const ModelService& models, QString registrationsPath = {});
    void addSourceAdapter(const IModelLibrarySourceAdapter* adapter);
    void setStorageManager(const class ModelStorageManager* storage);
    void setRuntimeStatus(const ModelRuntimeStatus& status);
    bool registerLocalGguf(const QString& filePath, const QString& runtimeModelId = {},
                           bool* storageFailure = nullptr,
                           const ModelLibraryEntry* origin = nullptr);
    bool removeLocalGgufRegistration(const QString& entryId,
                                     bool* storageFailure = nullptr);
    std::optional<ModelLibraryEntry> localGgufCandidate(const QString& filePath) const;
    QList<ModelLibraryProviderState> providerStates() const;
    QList<ModelLibraryEntry> entries() const;
    QList<ModelLibraryEntry> query(const ModelLibraryQuery& filter) const;
    static QList<ModelLibraryAction> availableActions(const ModelLibraryEntry& entry);
    const HardwareFacts& hardwareFacts() const;
    void refreshHardwareFacts();
    static QString entryId(const QString& providerId, const QString& runtimeId,
                           const QString& sourceId, const QString& nativeModelId);

private:
    struct LocalGgufRegistration {
        QString filePath;
        QString runtimeModelId;
        QString repositoryId;
        QString artifactFilename;
        QString revision;
        QString artifactHash;
    };
    bool saveRegistrations() const;
    const ModelService& models_;
    QString registrationsPath_;
    QList<LocalGgufRegistration> registrations_;
    QList<const IModelLibrarySourceAdapter*> adapters_;
    HardwareCapabilityService hardware_;
    const class ModelStorageManager* storage_ = nullptr;
    QHash<QString, ModelRuntimeStatus> runtimeStatuses_;
};

} // namespace sentinel::core
