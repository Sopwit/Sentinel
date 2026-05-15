#pragma once

#include "sentinel/core/IModelRouter.h"

namespace sentinel::core {

class StaticModelRouter final : public IModelRouter {
public:
    StaticModelRouter();
    StaticModelRouter(RoutingMode routingMode, QList<ProviderDescriptor> providers,
                      QList<ModelDescriptor> models);

    RoutingMode routingMode() const override;
    void setRoutingMode(RoutingMode mode) override;
    QList<ProviderDescriptor> providers() const override;
    QList<ModelDescriptor> models() const override;
    ModelRoute route(const TaskClassification& task) const override;

private:
    ProviderDescriptor providerForModel(const ModelDescriptor& model) const;

    RoutingMode routingMode_ = RoutingMode::LocalOnly;
    QList<ProviderDescriptor> providers_;
    QList<ModelDescriptor> models_;
};

} // namespace sentinel::core
