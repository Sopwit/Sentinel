// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/IToolHandler.h"
#include "sentinel/core/runtime/ToolDescriptor.h"

#include <QList>
#include <QString>

#include <memory>
#include <optional>

namespace sentinel::core {

class IToolRegistry {
public:
    virtual ~IToolRegistry() = default;

    virtual bool registerTool(ToolDescriptor descriptor) = 0;
    struct Registration {
        ToolDescriptor descriptor;
        std::shared_ptr<IToolHandler> handler;
    };
    virtual bool registerTool(Registration registration) = 0;
    virtual bool unregisterTool(const QString& id) = 0;
    virtual bool setEnabled(const QString& id, bool enabled) = 0;
    virtual int unregisterProvider(ToolSource source, const QString& providerId) = 0;
    virtual bool replaceProvider(ToolSource source, const QString& providerId,
                                 QList<Registration> registrations) = 0;
    virtual std::optional<Registration> findRegistration(const QString& id) const = 0;
    virtual QList<ToolDescriptor> listTools() const = 0;
    virtual QList<ToolDescriptor> enabledTools() const = 0;
    virtual std::optional<ToolDescriptor> findToolById(const QString& id) const = 0;
};

} // namespace sentinel::core
