// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/ToolDescriptor.h"

#include <QList>
#include <QString>

#include <optional>

namespace sentinel::core {

class IToolRegistry {
public:
    virtual ~IToolRegistry() = default;

    virtual bool registerTool(ToolDescriptor descriptor) = 0;
    virtual QList<ToolDescriptor> listTools() const = 0;
    virtual std::optional<ToolDescriptor> findToolById(const QString& id) const = 0;
};

} // namespace sentinel::core
