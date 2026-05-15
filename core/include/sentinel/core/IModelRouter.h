#pragma once

#include "sentinel/core/ModelRouting.h"

namespace sentinel::core {

class IModelRouter {
public:
    virtual ~IModelRouter() = default;

    virtual RoutingMode routingMode() const = 0;
    virtual void setRoutingMode(RoutingMode mode) = 0;
    virtual QList<ProviderDescriptor> providers() const = 0;
    virtual QList<ModelDescriptor> models() const = 0;
    virtual ModelRoute route(const TaskClassification& task) const = 0;
};

} // namespace sentinel::core
