// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/IToolRegistry.h"

#include <QMap>

namespace sentinel::core {

class InMemoryToolRegistry final : public IToolRegistry {
public:
    bool registerTool(ToolDescriptor descriptor) override;
    QList<ToolDescriptor> listTools() const override;
    std::optional<ToolDescriptor> findToolById(const QString& id) const override;

private:
    QMap<QString, ToolDescriptor> toolsById_;
};

} // namespace sentinel::core
