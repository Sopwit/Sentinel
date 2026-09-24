// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/model/ModelRouting.h"

namespace sentinel::core {

class IModelRouter {
public:
    virtual ~IModelRouter() = default;

    virtual RoutingMode routingMode() const = 0;
    virtual void setRoutingMode(RoutingMode mode) = 0;
    virtual QList<ProviderDescriptor> providers() const = 0;
    virtual QList<ModelDescriptor> models() const = 0;
    virtual ModelRoute route(const TaskClassification& task) const = 0;
    // Resolves an explicit user selection without choosing another provider.
    virtual ModelRoute resolveSelection(const ModelBinding& selection) const {
        ModelRoute result;
        result.status = selection.isConfigured() ? ModelRoutingStatus::Routed
                                                : ModelRoutingStatus::NoAvailableModel;
        result.provider.id = selection.providerId;
        result.model.id = selection.modelId;
        result.model.providerId = selection.providerId;
        return result;
    }
};

} // namespace sentinel::core
