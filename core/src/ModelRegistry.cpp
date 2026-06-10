#include "sentinel/core/ModelRegistry.h"

#include <algorithm>

namespace sentinel::core {

namespace {

QString normalizedModelName(QString name) {
    return name.trimmed();
}

QString displayNameFor(const QString& rawName) {
    const auto name = normalizedModelName(rawName);
    return name.isEmpty() ? QStringLiteral("Unknown model") : name;
}

QString familyFor(const QString& rawName) {
    const auto lower = rawName.toLower();
    if (lower.contains(QStringLiteral("llama"))) {
        return QStringLiteral("Llama");
    }
    if (lower.contains(QStringLiteral("mistral")) || lower.contains(QStringLiteral("mixtral"))) {
        return QStringLiteral("Mistral");
    }
    if (lower.contains(QStringLiteral("qwen"))) {
        return QStringLiteral("Qwen");
    }
    if (lower.contains(QStringLiteral("gemma"))) {
        return QStringLiteral("Gemma");
    }
    if (lower.contains(QStringLiteral("phi"))) {
        return QStringLiteral("Phi");
    }
    if (lower.contains(QStringLiteral("deepseek"))) {
        return QStringLiteral("DeepSeek");
    }
    if (lower.contains(QStringLiteral("nomic")) || lower.contains(QStringLiteral("embed"))) {
        return QStringLiteral("Embeddings");
    }
    return QStringLiteral("Unknown");
}

QString sizeClassFor(const QString& rawName) {
    const auto lower = rawName.toLower();
    const QStringList classes{
        QStringLiteral("1b"),  QStringLiteral("1.5b"), QStringLiteral("3b"),
        QStringLiteral("4b"),  QStringLiteral("7b"),   QStringLiteral("8b"),
        QStringLiteral("13b"), QStringLiteral("14b"),  QStringLiteral("32b"),
        QStringLiteral("70b"),
    };
    for (const auto& sizeClass : classes) {
        if (lower.contains(sizeClass)) {
            return sizeClass.toUpper();
        }
    }
    return QStringLiteral("Unknown");
}

QString bytesLabel(qint64 bytes) {
    if (bytes <= 0) {
        return QStringLiteral("Unknown");
    }

    constexpr double gib = 1024.0 * 1024.0 * 1024.0;
    constexpr double mib = 1024.0 * 1024.0;
    if (bytes >= static_cast<qint64>(gib)) {
        return QStringLiteral("about %1 GiB").arg(QString::number(bytes / gib, 'f', 1));
    }
    return QStringLiteral("about %1 MiB").arg(QString::number(bytes / mib, 'f', 1));
}

bool modelLessThan(const ModelSummary& left, const ModelSummary& right) {
    const auto providerCompare =
        QString::compare(left.providerId, right.providerId, Qt::CaseInsensitive);
    if (providerCompare != 0) {
        return providerCompare < 0;
    }
    const auto displayCompare =
        QString::compare(left.displayName, right.displayName, Qt::CaseInsensitive);
    if (displayCompare != 0) {
        return displayCompare < 0;
    }
    return QString::compare(left.rawName, right.rawName, Qt::CaseInsensitive) < 0;
}

bool isPlaceholder(const ModelSummary& model) {
    return model.status == ModelStatus::Placeholder || model.source == ModelSource::Placeholder;
}

} // namespace

QString modelCapabilityName(ModelCapability capability) {
    switch (capability) {
    case ModelCapability::Chat:
        return QStringLiteral("chat");
    case ModelCapability::Code:
        return QStringLiteral("code");
    case ModelCapability::Reasoning:
        return QStringLiteral("reasoning");
    case ModelCapability::Vision:
        return QStringLiteral("vision");
    case ModelCapability::Audio:
        return QStringLiteral("audio");
    case ModelCapability::Embeddings:
        return QStringLiteral("embeddings");
    case ModelCapability::Tools:
        return QStringLiteral("tools");
    case ModelCapability::Streaming:
        return QStringLiteral("streaming");
    case ModelCapability::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString modelReadinessName(ModelReadiness readiness) {
    switch (readiness) {
    case ModelReadiness::Available:
        return QStringLiteral("available");
    case ModelReadiness::Missing:
        return QStringLiteral("missing");
    case ModelReadiness::Disabled:
        return QStringLiteral("disabled");
    case ModelReadiness::Incompatible:
        return QStringLiteral("incompatible");
    case ModelReadiness::Unsupported:
        return QStringLiteral("unsupported");
    case ModelReadiness::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString modelStatusName(ModelStatus status) {
    switch (status) {
    case ModelStatus::Available:
        return QStringLiteral("available");
    case ModelStatus::Missing:
        return QStringLiteral("missing");
    case ModelStatus::Disabled:
        return QStringLiteral("disabled");
    case ModelStatus::Placeholder:
        return QStringLiteral("placeholder");
    case ModelStatus::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString modelSourceName(ModelSource source) {
    switch (source) {
    case ModelSource::Local:
        return QStringLiteral("local");
    case ModelSource::Cloud:
        return QStringLiteral("cloud");
    case ModelSource::Placeholder:
        return QStringLiteral("placeholder");
    case ModelSource::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString modelRegistryStatusName(ModelRegistryStatus status) {
    switch (status) {
    case ModelRegistryStatus::Ready:
        return QStringLiteral("ready");
    case ModelRegistryStatus::Empty:
        return QStringLiteral("empty");
    case ModelRegistryStatus::Degraded:
        return QStringLiteral("degraded");
    case ModelRegistryStatus::Disabled:
        return QStringLiteral("disabled");
    case ModelRegistryStatus::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QStringList modelCapabilityLabels(const QList<ModelCapability>& capabilities) {
    QStringList labels;
    for (const auto capability : capabilities) {
        const auto label = modelCapabilityName(capability);
        if (!labels.contains(label)) {
            labels.append(label);
        }
    }
    return labels;
}

QString modelSummaryLine(const ModelSummary& model) {
    return QStringLiteral("%1 / %2 / %3 / %4 / %5 / disk %6 / RAM %7 / context %8")
        .arg(model.providerId, model.displayName, modelReadinessName(model.readiness),
             modelSourceName(model.source), modelCapabilityLabels(model.capabilities).join(", "),
             model.approximateDiskSizeLabel, model.approximateRamClass,
             model.contextLength > 0 ? QString::number(model.contextLength)
                                     : QStringLiteral("Unknown"));
}

QList<ModelSummary> modelSummariesFromOllama(const QList<OllamaModelSummary>& models) {
    QList<ModelSummary> summaries;
    for (const auto& model : models) {
        const auto rawName = normalizedModelName(model.name);
        if (rawName.isEmpty()) {
            continue;
        }
        const auto diskLabel = bytesLabel(model.sizeBytes);
        summaries.append(ModelSummary{
            QStringLiteral("ollama/%1").arg(rawName),
            QStringLiteral("ollama"),
            rawName,
            displayNameFor(rawName),
            familyFor(rawName),
            QStringLiteral("Ollama"),
            sizeClassFor(rawName),
            {ModelCapability::Chat, ModelCapability::Streaming},
            ModelReadiness::Available,
            ModelStatus::Available,
            ModelSource::Local,
            model.sizeBytes,
            diskLabel,
            QStringLiteral("Unknown"),
            0,
            ModelRestriction{},
            ModelSafetyReport{},
            ModelRuntimeBadge{QStringLiteral("Local Ollama"), QStringLiteral("Local Only"),
                              modelReadinessName(ModelReadiness::Available)},
            QStringLiteral("%1 is available from local Ollama discovery metadata. Disk %2; RAM "
                           "and context are unknown unless provided by a later explicit metadata "
                           "phase.")
                .arg(rawName, diskLabel),
        });
    }
    std::sort(summaries.begin(), summaries.end(), modelLessThan);
    return summaries;
}

ModelSummary disabledProviderModelPlaceholder(const QString& providerId,
                                              const QString& providerLabel) {
    const auto normalizedProvider = providerId.trimmed().isEmpty()
                                        ? QStringLiteral("unknown-provider")
                                        : providerId.trimmed().toLower();
    const auto label = providerLabel.trimmed().isEmpty() ? QStringLiteral("Disabled Provider")
                                                         : providerLabel.trimmed();
    return ModelSummary{
        QStringLiteral("%1/placeholder").arg(normalizedProvider),
        normalizedProvider,
        QStringLiteral("placeholder"),
        QStringLiteral("Model metadata unavailable"),
        QStringLiteral("Unknown"),
        QStringLiteral("Unknown"),
        QStringLiteral("Unknown"),
        {ModelCapability::Unknown},
        ModelReadiness::Disabled,
        ModelStatus::Placeholder,
        ModelSource::Placeholder,
        0,
        QStringLiteral("Unknown"),
        QStringLiteral("Unknown"),
        0,
        ModelRestriction{false, true, true,
                         QStringLiteral("%1 is disabled; model actions are unavailable.")
                             .arg(label)},
        ModelSafetyReport{},
        ModelRuntimeBadge{label, QStringLiteral("Future Scoped"),
                          modelReadinessName(ModelReadiness::Disabled)},
        QStringLiteral("%1 model metadata is a disabled placeholder. Sentinel will not call, "
                       "download, import, export, or manage models for this provider.")
            .arg(label),
    };
}

ModelRegistry::ModelRegistry(QList<ModelSummary> models, QString selectedProviderId,
                             QString selectedModelId)
    : models_(std::move(models)), selectedProviderId_(selectedProviderId.trimmed().toLower()),
      selectedModelId_(selectedModelId.trimmed()) {
    std::sort(models_.begin(), models_.end(), modelLessThan);
    if (selectedProviderId_.isEmpty()) {
        selectedProviderId_ = QStringLiteral("ollama");
    }
}

QList<ModelSummary> ModelRegistry::models() const {
    return models_;
}

ModelSummary ModelRegistry::selectedModel() const {
    const auto expectedId = QStringLiteral("%1/%2").arg(selectedProviderId_, selectedModelId_);
    for (const auto& model : models_) {
        if (model.providerId == selectedProviderId_ &&
            (model.rawName == selectedModelId_ || model.id == expectedId)) {
            return model;
        }
    }
    return {};
}

bool ModelRegistry::hasAvailableModel(const QString& providerId, const QString& modelId) const {
    const auto normalizedProvider = providerId.trimmed().toLower();
    const auto normalizedModel = modelId.trimmed();
    for (const auto& model : models_) {
        if (model.providerId == normalizedProvider && model.rawName == normalizedModel &&
            model.readiness == ModelReadiness::Available &&
            model.status == ModelStatus::Available && model.source == ModelSource::Local) {
            return true;
        }
    }
    return false;
}

QStringList ModelRegistry::modelIdsForProvider(const QString& providerId) const {
    const auto normalizedProvider = providerId.trimmed().toLower();
    QStringList ids;
    for (const auto& model : models_) {
        if (model.providerId == normalizedProvider && !isPlaceholder(model)) {
            ids.append(model.rawName);
        }
    }
    return ids;
}

QStringList ModelRegistry::modelDisplaySummaries() const {
    QStringList summaries;
    for (const auto& model : models_) {
        summaries.append(modelSummaryLine(model));
    }
    return summaries;
}

QStringList ModelRegistry::selectedModelCapabilityLabels() const {
    const auto selected = selectedModel();
    return selected.rawName.isEmpty() ? QStringList{} : modelCapabilityLabels(selected.capabilities);
}

QString ModelRegistry::selectedModelReadinessSummary() const {
    if (selectedModelId_.isEmpty()) {
        return QStringLiteral("No model selected for %1.").arg(selectedProviderId_);
    }
    const auto selected = selectedModel();
    if (selected.rawName.isEmpty()) {
        return QStringLiteral("Selected model %1 is missing from %2 metadata.")
            .arg(selectedModelId_, selectedProviderId_);
    }
    return selected.summary;
}

ModelRegistrySummary ModelRegistry::summary() const {
    ModelRegistrySummary registrySummary;
    registrySummary.modelCount = static_cast<int>(models_.size());
    registrySummary.selectedProviderId = selectedProviderId_;
    registrySummary.selectedModelId = selectedModelId_;

    for (const auto& model : models_) {
        if (model.status == ModelStatus::Available && model.readiness == ModelReadiness::Available) {
            ++registrySummary.availableCount;
        }
        if (isPlaceholder(model)) {
            ++registrySummary.placeholderCount;
        }
    }

    const auto selected = selectedModel();
    registrySummary.selectedReadiness =
        selected.rawName.isEmpty() ? QStringLiteral("missing") : modelReadinessName(selected.readiness);
    if (models_.isEmpty()) {
        registrySummary.status = ModelRegistryStatus::Empty;
    } else if (registrySummary.availableCount > 0) {
        registrySummary.status = ModelRegistryStatus::Ready;
    } else if (registrySummary.placeholderCount == registrySummary.modelCount) {
        registrySummary.status = ModelRegistryStatus::Disabled;
    } else {
        registrySummary.status = ModelRegistryStatus::Degraded;
    }

    registrySummary.summary =
        QStringLiteral("Model registry %1: %2 models, %3 available, %4 placeholders, selected %5 "
                       "for %6 is %7.")
            .arg(modelRegistryStatusName(registrySummary.status))
            .arg(registrySummary.modelCount)
            .arg(registrySummary.availableCount)
            .arg(registrySummary.placeholderCount)
            .arg(selectedModelId_.isEmpty() ? QStringLiteral("none") : selectedModelId_,
                 selectedProviderId_, registrySummary.selectedReadiness);
    return registrySummary;
}

} // namespace sentinel::core
