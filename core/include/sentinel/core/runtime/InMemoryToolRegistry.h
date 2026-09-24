// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/IToolRegistry.h"

#include <QMap>
#include <QReadWriteLock>

namespace sentinel::core {

class InMemoryToolRegistry final : public IToolRegistry {
public:
    bool registerTool(ToolDescriptor descriptor) override;
    bool registerTool(Registration registration) override;
    bool unregisterTool(const QString& id) override;
    bool setEnabled(const QString& id, bool enabled) override;
    int unregisterProvider(ToolSource source, const QString& providerId) override;
    bool replaceProvider(ToolSource source, const QString& providerId,
                         QList<Registration> registrations) override;
    std::optional<Registration> findRegistration(const QString& id) const override;
    QList<ToolDescriptor> listTools() const override;
    QList<ToolDescriptor> enabledTools() const override;
    std::optional<ToolDescriptor> findToolById(const QString& id) const override;

private:
    mutable QReadWriteLock mutex_;
    QMap<QString, Registration> toolsById_;
};

} // namespace sentinel::core
