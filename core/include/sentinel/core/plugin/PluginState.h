// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace sentinel::core::plugin {

enum class PluginState { Unloaded, Loaded, Initialized, Active, Error, Disabled };

inline QString pluginStateToString(PluginState state) {
    switch (state) {
    case PluginState::Unloaded:
        return QStringLiteral("Unloaded");
    case PluginState::Loaded:
        return QStringLiteral("Loaded");
    case PluginState::Initialized:
        return QStringLiteral("Initialized");
    case PluginState::Active:
        return QStringLiteral("Active");
    case PluginState::Error:
        return QStringLiteral("Error");
    case PluginState::Disabled:
        return QStringLiteral("Disabled");
    }
    return QStringLiteral("Unknown");
}

} // namespace sentinel::core::plugin
