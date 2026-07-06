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

bool hasCapability(const ModelSummary& model, ModelCapability capability) {
    return model.capabilities.contains(capability);
}

ModelSummary catalogPlaceholder(const QString& providerId, const QString& providerLabel,
                                const QString& modelName, const QString& family,
                                const QString& sizeClass, QStringList capabilityLabels,
                                const QString& ramClass, int contextLength,
                                const QString& summary) {
    QList<ModelCapability> capabilities;
    for (const auto& label : capabilityLabels) {
        const auto normalized = label.trimmed().toLower();
        if (normalized == QStringLiteral("chat")) {
            capabilities.append(ModelCapability::Chat);
        } else if (normalized == QStringLiteral("code")) {
            capabilities.append(ModelCapability::Code);
        } else if (normalized == QStringLiteral("reasoning")) {
            capabilities.append(ModelCapability::Reasoning);
        } else if (normalized == QStringLiteral("vision")) {
            capabilities.append(ModelCapability::Vision);
        } else if (normalized == QStringLiteral("audio")) {
            capabilities.append(ModelCapability::Audio);
        } else if (normalized == QStringLiteral("embeddings")) {
            capabilities.append(ModelCapability::Embeddings);
        }
    }
    if (capabilities.isEmpty()) {
        capabilities.append(ModelCapability::Unknown);
    }

    return ModelSummary{
        QStringLiteral("%1/%2").arg(providerId, modelName),
        providerId,
        modelName,
        modelName,
        family,
        QStringLiteral("Catalog metadata"),
        sizeClass,
        capabilities,
        ModelReadiness::Disabled,
        ModelStatus::Placeholder,
        ModelSource::Placeholder,
        0,
        QStringLiteral("Unknown"),
        ramClass,
        contextLength,
        ModelRestriction{true, true, true,
                         QStringLiteral("%1 catalog entry is metadata-only; downloads, updates, "
                                        "deletes, and execution are disabled.")
                             .arg(providerLabel)},
        ModelSafetyReport{},
        ModelRuntimeBadge{providerLabel, QStringLiteral("Metadata Only"),
                          modelReadinessName(ModelReadiness::Disabled)},
        summary,
    };
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

QString modelRoleId(ModelRole role) {
    switch (role) {
    case ModelRole::PrimaryChat:
        return QStringLiteral("primary-chat");
    case ModelRole::Coding:
        return QStringLiteral("coding");
    case ModelRole::Summarizer:
        return QStringLiteral("summarizer");
    case ModelRole::Research:
        return QStringLiteral("research");
    case ModelRole::Fast:
        return QStringLiteral("fast");
    case ModelRole::Voice:
        return QStringLiteral("voice");
    case ModelRole::Embedding:
        return QStringLiteral("embedding");
    }
    return QStringLiteral("primary-chat");
}

QString modelRoleDisplayName(ModelRole role) {
    switch (role) {
    case ModelRole::PrimaryChat:
        return QStringLiteral("Primary Chat Model");
    case ModelRole::Coding:
        return QStringLiteral("Coding Model");
    case ModelRole::Summarizer:
        return QStringLiteral("Summarizer Model");
    case ModelRole::Research:
        return QStringLiteral("Research Model");
    case ModelRole::Fast:
        return QStringLiteral("Fast Model");
    case ModelRole::Voice:
        return QStringLiteral("Voice Model");
    case ModelRole::Embedding:
        return QStringLiteral("Embedding Model");
    }
    return QStringLiteral("Primary Chat Model");
}

QStringList modelRoleIds() {
    return {
        modelRoleId(ModelRole::PrimaryChat), modelRoleId(ModelRole::Coding),
        modelRoleId(ModelRole::Summarizer),  modelRoleId(ModelRole::Research),
        modelRoleId(ModelRole::Fast),        modelRoleId(ModelRole::Voice),
        modelRoleId(ModelRole::Embedding),
    };
}

QString modelDownloadStateName(ModelDownloadState state) {
    switch (state) {
    case ModelDownloadState::NotStarted:
        return QStringLiteral("Not Started");
    case ModelDownloadState::Queued:
        return QStringLiteral("Queued");
    case ModelDownloadState::Downloading:
        return QStringLiteral("Downloading");
    case ModelDownloadState::Paused:
        return QStringLiteral("Paused");
    case ModelDownloadState::Completed:
        return QStringLiteral("Completed");
    case ModelDownloadState::Failed:
        return QStringLiteral("Failed");
    case ModelDownloadState::Cancelled:
        return QStringLiteral("Cancelled");
    case ModelDownloadState::ExecutionDisabled:
        return QStringLiteral("Execution Disabled");
    }
    return QStringLiteral("Execution Disabled");
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

QString modelLibrarySummaryLine(const ModelSummary& model) {
    const auto sourceLabel = model.source == ModelSource::Local ? QStringLiteral("Installed")
                                                                : QStringLiteral("Discoverable");
    return QStringLiteral("%1 - %2 - %3 - size %4 - RAM %5 - context %6 - Turkish %7 - coding %8 "
                          "- reasoning %9 - vision/audio %10 - readiness %11")
        .arg(sourceLabel, model.runtimeBadge.providerLabel, model.displayName,
             model.sizeClass.isEmpty() ? QStringLiteral("Unknown") : model.sizeClass,
             model.approximateRamClass,
             model.contextLength > 0 ? QString::number(model.contextLength)
                                     : QStringLiteral("Unknown"),
             hasCapability(model, ModelCapability::Chat) ? QStringLiteral("general")
                                                         : QStringLiteral("unknown"),
             hasCapability(model, ModelCapability::Code) ? QStringLiteral("yes")
                                                         : QStringLiteral("unknown"),
             hasCapability(model, ModelCapability::Reasoning) ? QStringLiteral("yes")
                                                              : QStringLiteral("unknown"),
             (hasCapability(model, ModelCapability::Vision) ||
              hasCapability(model, ModelCapability::Audio))
                 ? QStringLiteral("planned")
                 : QStringLiteral("unknown"),
             modelReadinessName(model.readiness));
}

QString modelDetailSummaryLine(const ModelSummary& model) {
    return QStringLiteral("%1 / source %2 / provider %3 / quantization %4 / capabilities %5 / "
                          "performance estimate %6 / unavailable reason %7")
        .arg(model.displayName, modelSourceName(model.source), model.runtimeBadge.providerLabel,
             model.format, modelCapabilityLabels(model.capabilities).join(QStringLiteral(", ")),
             model.sizeClass == QStringLiteral("3B") || model.sizeClass == QStringLiteral("1B") ||
                     model.sizeClass == QStringLiteral("1.5B")
                 ? QStringLiteral("Fast class")
                 : model.sizeClass == QStringLiteral("7B") || model.sizeClass == QStringLiteral("8B")
                       ? QStringLiteral("Balanced class")
                       : QStringLiteral("Unknown/manual"),
             model.readiness == ModelReadiness::Available ? QStringLiteral("None")
                                                           : model.restriction.summary);
}

QList<ModelSummary> modelSummariesFromOllama(const QList<OllamaModelSummary>& models) {
    QList<ModelSummary> summaries;
    for (const auto& model : models) {
        const auto rawName = normalizedModelName(model.name);
        if (rawName.isEmpty()) {
            continue;
        }
        const auto diskLabel = bytesLabel(model.sizeBytes);
        auto capabilities = QList<ModelCapability>{ModelCapability::Chat, ModelCapability::Streaming};
        const auto lower = rawName.toLower();
        if (lower.contains(QStringLiteral("coder")) || lower.contains(QStringLiteral("code"))) {
            capabilities.append(ModelCapability::Code);
        }
        if (lower.contains(QStringLiteral("reason")) || lower.contains(QStringLiteral("deepseek")) ||
            lower.contains(QStringLiteral("qwen"))) {
            capabilities.append(ModelCapability::Reasoning);
        }
        if (lower.contains(QStringLiteral("embed")) || lower.contains(QStringLiteral("nomic"))) {
            capabilities.append(ModelCapability::Embeddings);
        }
        summaries.append(ModelSummary{
            QStringLiteral("ollama/%1").arg(rawName),
            QStringLiteral("ollama"),
            rawName,
            displayNameFor(rawName),
            familyFor(rawName),
            QStringLiteral("Ollama"),
            sizeClassFor(rawName),
            capabilities,
            ModelReadiness::Available,
            ModelStatus::Available,
            ModelSource::Local,
            model.sizeBytes,
            diskLabel,
            sizeClassFor(rawName) == QStringLiteral("3B") ? QStringLiteral("4-8 GB class")
                                                          : sizeClassFor(rawName) == QStringLiteral("7B") ||
                                                                    sizeClassFor(rawName) == QStringLiteral("8B")
                                                                ? QStringLiteral("8-16 GB class")
                                                                : QStringLiteral("Unknown"),
            sizeClassFor(rawName) == QStringLiteral("3B") ? 8192 : 0,
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

QList<ModelSummary> localAiCatalogPlaceholders() {
    return {
        catalogPlaceholder(QStringLiteral("lm-studio"), QStringLiteral("LM Studio"),
                           QStringLiteral("local-open-model"), QStringLiteral("Open Local Model"),
                           QStringLiteral("Unknown"),
                           {QStringLiteral("chat"), QStringLiteral("code"),
                            QStringLiteral("reasoning")},
                           QStringLiteral("Model dependent"), 0,
                           QStringLiteral("LM Studio local server catalog is represented for "
                                          "readiness only. Sentinel does not probe or call it.")),
        catalogPlaceholder(QStringLiteral("llama-cpp-server"),
                           QStringLiteral("llama.cpp server"), QStringLiteral("gguf-local-model"),
                           QStringLiteral("GGUF"), QStringLiteral("Unknown"),
                           {QStringLiteral("chat"), QStringLiteral("code")},
                           QStringLiteral("Quantization dependent"), 0,
                           QStringLiteral("llama.cpp server metadata is future-scoped and "
                                          "loopback-only when explicitly checked later.")),
        catalogPlaceholder(QStringLiteral("openai-compatible-local"),
                           QStringLiteral("OpenAI-compatible local endpoint"),
                           QStringLiteral("local-endpoint-model"), QStringLiteral("OpenAI-compatible"),
                           QStringLiteral("Unknown"),
                           {QStringLiteral("chat"), QStringLiteral("reasoning")},
                           QStringLiteral("Endpoint dependent"), 0,
                           QStringLiteral("OpenAI-compatible local endpoint metadata is "
                                          "disabled until explicit local configuration exists.")),
        catalogPlaceholder(QStringLiteral("huggingface-catalog"),
                           QStringLiteral("Hugging Face catalog placeholder"),
                           QStringLiteral("hf-metadata-entry"), QStringLiteral("Catalog"),
                           QStringLiteral("Unknown"),
                           {QStringLiteral("chat"), QStringLiteral("code"),
                            QStringLiteral("embeddings")},
                           QStringLiteral("Unknown"), 0,
                           QStringLiteral("Hugging Face appears as a catalog placeholder only; "
                                          "no catalog fetch or cloud call is performed.")),
        catalogPlaceholder(QStringLiteral("mlx-catalog"),
                           QStringLiteral("MLX local/community catalog"),
                           QStringLiteral("mlx-metadata-entry"), QStringLiteral("MLX"),
                           QStringLiteral("Unknown"),
                           {QStringLiteral("chat"), QStringLiteral("code"),
                            QStringLiteral("reasoning")},
                           QStringLiteral("Apple Silicon dependent"), 0,
                           QStringLiteral("MLX models catalog optimized for Apple Silicon; "
                                          "Sentinel does not build or run them directly.")),
        catalogPlaceholder(QStringLiteral("custom-catalog"),
                           QStringLiteral("Future custom catalogs"),
                           QStringLiteral("custom-metadata-entry"), QStringLiteral("Custom"),
                           QStringLiteral("Unknown"),
                           {QStringLiteral("chat"), QStringLiteral("embeddings")},
                           QStringLiteral("Unknown"), 0,
                           QStringLiteral("Custom catalogs are represented as future metadata "
                                          "without import, scan, or fetch behavior.")),
    };
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

QStringList ModelRegistry::installedModelLibrarySummaries() const {
    QStringList summaries;
    for (const auto& model : models_) {
        if (model.status == ModelStatus::Available && model.source == ModelSource::Local) {
            summaries.append(modelLibrarySummaryLine(model));
        }
    }
    return summaries.isEmpty()
               ? QStringList{QStringLiteral("No installed model metadata is available. Safe "
                                            "local Ollama discovery may populate this after an "
                                            "explicit foreground readiness check.")}
               : summaries;
}

QStringList ModelRegistry::availableModelLibrarySummaries() const {
    QStringList summaries;
    for (const auto& model : models_) {
        summaries.append(modelLibrarySummaryLine(model));
    }
    for (const auto& model : localAiCatalogPlaceholders()) {
        summaries.append(modelLibrarySummaryLine(model));
    }
    return summaries;
}

QStringList ModelRegistry::recommendedModelLibrarySummaries() const {
    QStringList summaries;
    for (const auto& model : models_) {
        if (model.status == ModelStatus::Available &&
            (model.sizeClass == QStringLiteral("3B") || model.sizeClass == QStringLiteral("7B") ||
             model.sizeClass == QStringLiteral("8B") || hasCapability(model, ModelCapability::Code))) {
            summaries.append(QStringLiteral("Recommended installed: %1 - %2 - best use: %3")
                                 .arg(model.displayName, model.approximateRamClass,
                                      hasCapability(model, ModelCapability::Code)
                                          ? QStringLiteral("coding and chat")
                                          : QStringLiteral("general chat")));
        }
    }
    if (summaries.isEmpty()) {
        summaries.append(QStringLiteral("Recommended future local baseline: Llama/Qwen/Gemma "
                                        "3B-8B class via Ollama, LM Studio, or llama.cpp server "
                                        "after explicit user setup."));
    }
    summaries.append(QStringLiteral("Avoid: cloud-only or unknown-license models until explicit "
                                    "provider, credential, license, and privacy review exists."));
    return summaries;
}

QStringList ModelRegistry::modelDetailSummaries() const {
    QStringList summaries;
    for (const auto& model : models_) {
        summaries.append(modelDetailSummaryLine(model));
    }
    for (const auto& model : localAiCatalogPlaceholders()) {
        summaries.append(modelDetailSummaryLine(model));
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

QStringList deterministicModelAdvisorRecommendations(const ModelAdvisorInput& input,
                                                     const ModelRegistry& registry) {
    QStringList recommendations;
    const auto goal = input.userGoal.toLower();
    const auto preference = input.speedQualityPreference.toLower();
    for (const auto& model : registry.models()) {
        if (model.status != ModelStatus::Available || model.source != ModelSource::Local) {
            continue;
        }
        const bool codeGoal = goal.contains(QStringLiteral("cod"));
        const bool fastGoal = preference.contains(QStringLiteral("speed")) ||
                              model.sizeClass == QStringLiteral("3B");
        if ((codeGoal && hasCapability(model, ModelCapability::Code)) || fastGoal ||
            recommendations.isEmpty()) {
            recommendations.append(
                QStringLiteral("%1 - expected RAM %2 - disk %3 - best use %4 - reason %5")
                    .arg(model.displayName, model.approximateRamClass,
                         model.approximateDiskSizeLabel,
                         hasCapability(model, ModelCapability::Code)
                             ? QStringLiteral("coding")
                             : QStringLiteral("primary chat"),
                         QStringLiteral("installed local metadata matches %1/%2 on %3 %4")
                             .arg(input.userGoal, input.speedQualityPreference, input.platform,
                                  input.cpuArchitecture)));
        }
    }
    if (recommendations.isEmpty()) {
        recommendations.append(
            QStringLiteral("3B-8B local instruct model - expected RAM 4-16 GB class - disk "
                           "model-dependent - best use primary chat - reason no installed model "
                           "metadata is available yet."));
    }
    recommendations.append(QStringLiteral("Embedding model: nomic/embed class only after explicit "
                                          "local provider setup; semantic execution remains "
                                          "disabled."));
    return recommendations;
}

QStringList deterministicModelAdvisorAvoidList(const ModelAdvisorInput& input) {
    Q_UNUSED(input);
    return {
        QStringLiteral("Avoid cloud-only models: cloud provider activation and credentials are "
                       "disabled."),
        QStringLiteral("Avoid very large 32B/70B models unless the user explicitly confirms RAM, "
                       "disk, thermal, and provider readiness."),
        QStringLiteral("Avoid unknown-license catalog entries until license review is complete."),
        QStringLiteral("External tools such as llmfit are future license-reviewed inspiration only."),
    };
}

QStringList downloadCenterPlaceholderSummaries(const QList<ModelSummary>& models) {
    QStringList summaries;
    summaries.append(QStringLiteral("Downloads Center: %1 / actions disabled / no background "
                                    "workers / no ollama pull / no subprocess.")
                         .arg(modelDownloadStateName(ModelDownloadState::ExecutionDisabled)));
    for (const auto& model : models) {
        summaries.append(QStringLiteral("%1 - %2 - progress 0% - size %3 - status %4 - actions "
                                        "unavailable")
                             .arg(model.displayName, model.runtimeBadge.providerLabel,
                                  model.approximateDiskSizeLabel,
                                  modelDownloadStateName(ModelDownloadState::ExecutionDisabled)));
    }
    if (models.isEmpty()) {
        summaries.append(QStringLiteral("No queued downloads. Not Started, Queued, Downloading, "
                                        "Paused, Completed, Failed, Cancelled, and Execution "
                                        "Disabled are metadata states only."));
    }
    return summaries;
}

QStringList benchmarkHubPlaceholderSummaries(const QList<ModelSummary>& models) {
    QStringList summaries;
    summaries.append(QStringLiteral("Benchmark Hub: previous/manual metrics only; benchmark "
                                    "execution is disabled."));
    for (const auto& model : models) {
        summaries.append(QStringLiteral("%1 - tokens/sec manual unknown - first token latency "
                                        "unknown - average response unknown - RAM %2 - class %3")
                             .arg(model.displayName, model.approximateRamClass,
                                  model.sizeClass == QStringLiteral("3B")
                                      ? QStringLiteral("battery-friendly")
                                      : model.sizeClass == QStringLiteral("7B") ||
                                                model.sizeClass == QStringLiteral("8B")
                                            ? QStringLiteral("balanced performance")
                                            : QStringLiteral("unknown")));
    }
    if (models.isEmpty()) {
        summaries.append(QStringLiteral("No benchmark records. Tokens/sec, first-token latency, "
                                        "average response time, RAM estimate, and battery class "
                                        "can be shown later from explicit/manual results."));
    }
    return summaries;
}

} // namespace sentinel::core
