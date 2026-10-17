// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/desktop/DaemonClient.h"

#include <optional>

namespace sentinel::desktop {

// Desktop-shell failure categories that are routed to the single global error banner.
// Chat-, run-, provider-, and model-level failures are intentionally excluded: they already
// surface per-channel and must not become global alerts.
enum class DesktopErrorCategory {
    None,
    DaemonUnavailable,
    DaemonDisconnected,
    StartupRecovery,
};

// Maps a daemon connection state to the banner category it implies. Returns nullopt for
// transitions that must leave the current banner untouched (startup/connecting/shutdown), so
// connected-versus-ready semantics stay truthful and repeated equivalent failures do not
// produce duplicate banners.
inline std::optional<DesktopErrorCategory>
desktopErrorCategoryForDaemonState(DaemonClient::ConnectionState state) {
    switch (state) {
    case DaemonClient::ConnectionState::Unavailable:
        return DesktopErrorCategory::DaemonUnavailable;
    case DaemonClient::ConnectionState::Reconnecting:
        return DesktopErrorCategory::DaemonDisconnected;
    case DaemonClient::ConnectionState::Connected:
        return DesktopErrorCategory::None;
    default:
        return std::nullopt;
    }
}

inline bool isDaemonErrorCategory(DesktopErrorCategory category) {
    return category == DesktopErrorCategory::DaemonUnavailable ||
           category == DesktopErrorCategory::DaemonDisconnected;
}

} // namespace sentinel::desktop
