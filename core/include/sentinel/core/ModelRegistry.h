#pragma once

#include "sentinel/core/OllamaRuntime.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

using ModelId = QString;
using ModelProviderId = QString;
using ModelDisplayName = QString;
using ModelFamily = QString;
using ModelFormat = QString;
using ModelSizeClass = QString;

enum class ModelCapability : std::uint8_t {
    Chat,
    Code,
    Reasoning,
    Vision,
    Audio,
    Embeddings,
    Tools,
    Streaming,
    Unknown,
};

enum class ModelReadiness : std::uint8_t {
    Available,
    Missing,
    Disabled,
    Incompatible,
    Unsupported,
    Unknown,
};

enum class ModelStatus : std::uint8_t {
    Available,
    Missing,
    Disabled,
    Placeholder,
    Unknown,
};

enum class ModelSource : std::uint8_t {
    Local,
    Cloud,
    Placeholder,
    Unknown,
};

struct ModelRestriction {
    bool localOnly = true;
    bool managementUnavailable = true;
    bool cloudUnavailable = true;
    QString summary = QStringLiteral("Local metadata only; no model management action is active.");
};

struct ModelSafetyReport {
    bool fileInspectionAttempted = false;
    bool filesystemScanAttempted = false;
    bool cloudCallAttempted = false;
    bool toolExecutionAttempted = false;
    QString summary = QStringLiteral("Model metadata was built without filesystem scans, file "
                                     "inspection, cloud calls, tools, downloads, or deletes.");
};

struct ModelRuntimeBadge {
    QString providerLabel = QStringLiteral("Unknown Provider");
    QString scopeLabel = QStringLiteral("Unknown Scope");
    QString readinessLabel = QStringLiteral("unknown");
};

struct ModelSummary {
    ModelId id;
    ModelProviderId providerId;
    QString rawName;
    ModelDisplayName displayName;
    ModelFamily family = QStringLiteral("Unknown");
    ModelFormat format = QStringLiteral("Unknown");
    ModelSizeClass sizeClass = QStringLiteral("Unknown");
    QList<ModelCapability> capabilities{ModelCapability::Unknown};
    ModelReadiness readiness = ModelReadiness::Unknown;
    ModelStatus status = ModelStatus::Unknown;
    ModelSource source = ModelSource::Unknown;
    qint64 approximateDiskSizeBytes = 0;
    QString approximateDiskSizeLabel = QStringLiteral("Unknown");
    QString approximateRamClass = QStringLiteral("Unknown");
    int contextLength = 0;
    ModelRestriction restriction;
    ModelSafetyReport safetyReport;
    ModelRuntimeBadge runtimeBadge;
    QString summary;
};

enum class ModelRegistryStatus : std::uint8_t {
    Ready,
    Empty,
    Degraded,
    Disabled,
    Unknown,
};

struct ModelRegistrySummary {
    ModelRegistryStatus status = ModelRegistryStatus::Unknown;
    int modelCount = 0;
    int availableCount = 0;
    int placeholderCount = 0;
    QString selectedProviderId;
    QString selectedModelId;
    QString selectedReadiness = QStringLiteral("unknown");
    QString summary;
};

class ModelRegistry final {
public:
    ModelRegistry(QList<ModelSummary> models, QString selectedProviderId = QStringLiteral("ollama"),
                  QString selectedModelId = {});

    QList<ModelSummary> models() const;
    ModelRegistrySummary summary() const;
    ModelSummary selectedModel() const;
    bool hasAvailableModel(const QString& providerId, const QString& modelId) const;
    QStringList modelIdsForProvider(const QString& providerId) const;
    QStringList modelDisplaySummaries() const;
    QStringList selectedModelCapabilityLabels() const;
    QString selectedModelReadinessSummary() const;

private:
    QList<ModelSummary> models_;
    QString selectedProviderId_;
    QString selectedModelId_;
};

QString modelCapabilityName(ModelCapability capability);
QString modelReadinessName(ModelReadiness readiness);
QString modelStatusName(ModelStatus status);
QString modelSourceName(ModelSource source);
QString modelRegistryStatusName(ModelRegistryStatus status);
QString modelSummaryLine(const ModelSummary& model);
QStringList modelCapabilityLabels(const QList<ModelCapability>& capabilities);
QList<ModelSummary> modelSummariesFromOllama(const QList<OllamaModelSummary>& models);
ModelSummary disabledProviderModelPlaceholder(const QString& providerId,
                                              const QString& providerLabel);

} // namespace sentinel::core
