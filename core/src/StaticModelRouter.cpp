#include "sentinel/core/StaticModelRouter.h"

#include "sentinel/core/StaticProviderCatalog.h"

#include <algorithm>
#include <utility>

namespace sentinel::core {

namespace {

bool routeRequiresLocalOnly(RoutingMode mode, const TaskClassification& task) {
    return mode == RoutingMode::LocalOnly || task.sensitive || task.type == TaskType::SensitiveData;
}

QList<ProviderDescriptor> availableProvidersFromCatalog(const IProviderCatalog& catalog) {
    QList<ProviderDescriptor> providers;
    for (const auto& entry : catalog.entries()) {
        if (isCatalogEntryAvailable(entry.availability)) {
            providers.append(entry.descriptor);
        }
    }
    return providers;
}

QList<ModelDescriptor> availableModelsFromCatalog(const IProviderCatalog& catalog) {
    QList<ModelDescriptor> models;
    for (const auto& entry : catalog.entries()) {
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

} // namespace

StaticModelRouter::StaticModelRouter() : StaticModelRouter(StaticProviderCatalog{}) {}

StaticModelRouter::StaticModelRouter(const IProviderCatalog& catalog)
    : StaticModelRouter(RoutingMode::LocalOnly, catalog) {}

StaticModelRouter::StaticModelRouter(RoutingMode routingMode, QList<ProviderDescriptor> providers,
                                     QList<ModelDescriptor> models)
    : routingMode_(routingMode), providers_(std::move(providers)), models_(std::move(models)) {
    std::sort(providers_.begin(), providers_.end(),
              [](const ProviderDescriptor& left, const ProviderDescriptor& right) {
                  return left.id < right.id;
              });
    std::sort(models_.begin(), models_.end(),
              [](const ModelDescriptor& left, const ModelDescriptor& right) {
                  return left.id < right.id;
              });
}

StaticModelRouter::StaticModelRouter(RoutingMode routingMode, const IProviderCatalog& catalog)
    : StaticModelRouter(routingMode, availableProvidersFromCatalog(catalog),
                        availableModelsFromCatalog(catalog)) {}

RoutingMode StaticModelRouter::routingMode() const {
    return routingMode_;
}

void StaticModelRouter::setRoutingMode(RoutingMode mode) {
    routingMode_ = mode;
}

QList<ProviderDescriptor> StaticModelRouter::providers() const {
    return providers_;
}

QList<ModelDescriptor> StaticModelRouter::models() const {
    return models_;
}

ModelRoute StaticModelRouter::route(const TaskClassification& task) const {
    const auto localOnly = routeRequiresLocalOnly(routingMode_, task);
    for (const auto& model : models_) {
        if (!model.installed) {
            continue;
        }
        if (localOnly && !model.localOnly) {
            continue;
        }

        const auto provider = providerForModel(model);
        if (provider.id.isEmpty()) {
            continue;
        }
        if (localOnly && provider.kind != ProviderKind::Local) {
            continue;
        }

        return ModelRoute{
            ModelRoutingStatus::Routed,
            routingMode_,
            task,
            provider,
            model,
            QStringLiteral("%1 -> %2 / %3")
                .arg(routingModeName(routingMode_), provider.name, model.name),
            false,
            false,
        };
    }

    return ModelRoute{
        ModelRoutingStatus::NoAvailableModel,
        routingMode_,
        task,
        {},
        {},
        QStringLiteral("No metadata-only model route is available."),
        false,
        false,
    };
}

ProviderDescriptor StaticModelRouter::providerForModel(const ModelDescriptor& model) const {
    const auto found = std::find_if(
        providers_.begin(), providers_.end(),
        [&model](const ProviderDescriptor& provider) { return provider.id == model.providerId; });
    return found == providers_.end() ? ProviderDescriptor{} : *found;
}

} // namespace sentinel::core
