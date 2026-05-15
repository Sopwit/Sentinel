#include "sentinel/core/StaticProviderCatalog.h"

#include <algorithm>
#include <utility>

namespace sentinel::core {

namespace {

ModelCatalogEntry localPlaceholderModel() {
    return ModelCatalogEntry{
        ModelDescriptor{
            QStringLiteral("sentinel-local-placeholder"),
            QStringLiteral("Sentinel Local Placeholder"),
            QStringLiteral("local-placeholder"),
            true,
            true,
            8192,
            QStringLiteral("metadata"),
            QStringLiteral("low"),
            {
                taskTypeName(TaskType::Chat),
                taskTypeName(TaskType::Summarization),
                taskTypeName(TaskType::Planning),
                taskTypeName(TaskType::SensitiveData),
                taskTypeName(TaskType::Unknown),
            },
        },
        CatalogAvailability::Available,
        CatalogPrivacyLevel::LocalOnly,
        2048,
        0,
        QStringLiteral("Available local metadata placeholder. No model execution."),
    };
}

ProviderCatalogEntry localMetadataProvider() {
    return ProviderCatalogEntry{
        ProviderDescriptor{
            QStringLiteral("local-placeholder"),
            QStringLiteral("Local Metadata Provider"),
            ProviderKind::Local,
            ProviderCapabilityProfile{
                true,
                false,
                true,
                false,
                true,
                8192,
                QStringLiteral("low"),
                QStringLiteral("none"),
                QStringLiteral("local-only"),
                {
                    taskTypeName(TaskType::Chat),
                    taskTypeName(TaskType::Summarization),
                    taskTypeName(TaskType::Planning),
                    taskTypeName(TaskType::SensitiveData),
                    taskTypeName(TaskType::Unknown),
                },
            },
        },
        CatalogAvailability::Available,
        CatalogPrivacyLevel::LocalOnly,
        2048,
        0,
        QStringLiteral("Local Metadata Provider (Local, Available)"),
        {localPlaceholderModel()},
    };
}

ModelCatalogEntry unavailableModel(QString id, QString name, QString providerId, bool localOnly,
                                   int contextWindowTokens, QStringList tasks) {
    return ModelCatalogEntry{
        ModelDescriptor{
            std::move(id),
            std::move(name),
            std::move(providerId),
            localOnly,
            false,
            contextWindowTokens,
            QStringLiteral("metadata"),
            QStringLiteral("unknown"),
            std::move(tasks),
        },
        CatalogAvailability::NotConfigured,
        localOnly ? CatalogPrivacyLevel::LocalOnly : CatalogPrivacyLevel::CloudMetadataOnly,
        0,
        0,
        QStringLiteral("Metadata placeholder only. Not configured."),
    };
}

ProviderCatalogEntry ollamaPlaceholder() {
    return ProviderCatalogEntry{
        ProviderDescriptor{
            QStringLiteral("ollama-local"),
            QStringLiteral("Ollama Local"),
            ProviderKind::Local,
            ProviderCapabilityProfile{
                true,
                false,
                true,
                false,
                true,
                8192,
                QStringLiteral("unknown"),
                QStringLiteral("none"),
                QStringLiteral("local-only"),
                {taskTypeName(TaskType::Chat), taskTypeName(TaskType::Coding)},
            },
        },
        CatalogAvailability::NotConfigured,
        CatalogPrivacyLevel::LocalOnly,
        8192,
        8192,
        QStringLiteral("Ollama Local (Local, Not Configured)"),
        {unavailableModel(QStringLiteral("ollama-local-placeholder"),
                          QStringLiteral("Ollama Local Placeholder"),
                          QStringLiteral("ollama-local"), true, 8192,
                          {taskTypeName(TaskType::Chat), taskTypeName(TaskType::Coding)})},
    };
}

ProviderCatalogEntry cloudPlaceholder(QString id, QString name, QString modelId,
                                      QString modelName) {
    const auto providerId = id;
    const auto providerName = name;
    return ProviderCatalogEntry{
        ProviderDescriptor{
            std::move(id),
            std::move(name),
            ProviderKind::Cloud,
            ProviderCapabilityProfile{
                false,
                true,
                true,
                false,
                false,
                32768,
                QStringLiteral("unknown"),
                QStringLiteral("metered"),
                QStringLiteral("cloud-not-configured"),
                {
                    taskTypeName(TaskType::Chat),
                    taskTypeName(TaskType::Summarization),
                    taskTypeName(TaskType::Coding),
                    taskTypeName(TaskType::Planning),
                    taskTypeName(TaskType::LongContext),
                },
            },
        },
        CatalogAvailability::NotConfigured,
        CatalogPrivacyLevel::CloudMetadataOnly,
        0,
        0,
        QStringLiteral("%1 (Cloud, Not Configured)").arg(providerName),
        {unavailableModel(std::move(modelId), std::move(modelName), providerId, false, 32768,
                          {taskTypeName(TaskType::Chat), taskTypeName(TaskType::LongContext)})},
    };
}

QList<ProviderCatalogEntry> defaultEntries() {
    return {
        localMetadataProvider(),
        ollamaPlaceholder(),
        cloudPlaceholder(QStringLiteral("anthropic-cloud"), QStringLiteral("Anthropic Cloud"),
                         QStringLiteral("anthropic-cloud-placeholder"),
                         QStringLiteral("Anthropic Cloud Placeholder")),
        cloudPlaceholder(QStringLiteral("openai-cloud"), QStringLiteral("OpenAI Cloud"),
                         QStringLiteral("openai-cloud-placeholder"),
                         QStringLiteral("OpenAI Cloud Placeholder")),
    };
}

void sortEntries(QList<ProviderCatalogEntry>& entries) {
    std::sort(entries.begin(), entries.end(),
              [](const ProviderCatalogEntry& left, const ProviderCatalogEntry& right) {
                  return left.descriptor.id < right.descriptor.id;
              });
    for (auto& entry : entries) {
        std::sort(entry.models.begin(), entry.models.end(),
                  [](const ModelCatalogEntry& left, const ModelCatalogEntry& right) {
                      return left.descriptor.id < right.descriptor.id;
                  });
    }
}

} // namespace

StaticProviderCatalog::StaticProviderCatalog() : StaticProviderCatalog(defaultEntries()) {}

StaticProviderCatalog::StaticProviderCatalog(QList<ProviderCatalogEntry> entries)
    : entries_(std::move(entries)) {
    sortEntries(entries_);
}

QList<ProviderCatalogEntry> StaticProviderCatalog::entries() const {
    return entries_;
}

QList<ProviderDescriptor> StaticProviderCatalog::availableProviders() const {
    QList<ProviderDescriptor> providers;
    for (const auto& entry : entries_) {
        if (isCatalogEntryAvailable(entry.availability)) {
            providers.append(entry.descriptor);
        }
    }
    return providers;
}

QList<ModelDescriptor> StaticProviderCatalog::availableModels() const {
    QList<ModelDescriptor> models;
    for (const auto& entry : entries_) {
        if (!isCatalogEntryAvailable(entry.availability)) {
            continue;
        }
        for (const auto& model : entry.models) {
            if (isCatalogEntryAvailable(model.availability)) {
                models.append(model.descriptor);
            }
        }
    }
    return models;
}

QStringList StaticProviderCatalog::providerSummaries() const {
    QStringList summaries;
    for (const auto& entry : entries_) {
        summaries.append(providerCatalogEntrySummary(entry));
    }
    return summaries;
}

} // namespace sentinel::core
