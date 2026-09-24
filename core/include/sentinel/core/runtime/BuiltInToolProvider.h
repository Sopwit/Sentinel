// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "sentinel/core/runtime/IToolRegistry.h"
#include "sentinel/core/runtime/RealToolExecutor.h"
namespace sentinel::core {
class BuiltInToolProvider {
public:
    static QList<ToolDescriptor> descriptors();
    static RealToolExecutor::BuiltInMethod methodFor(const QString& id);
    static bool registerTools(IToolRegistry& registry, RealToolExecutor& executor);
};
} // namespace sentinel::core
