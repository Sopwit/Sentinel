// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <functional>
#include <cstdint>

namespace sentinel::core {

enum class PermissionEffect : std::uint8_t { Allow, Ask, Deny };

struct PermissionRule {
    QString action; // e.g., "edit", "shell", "read", "write", "websearch", "webfetch", "lsp",
                    // "skill", "task"
    QString
        resource; // pattern with wildcards, e.g., "*.cpp", "/path/to/*", "https://example.com/*"
    PermissionEffect effect{PermissionEffect::Ask};
    QString description;
    bool isDefault{false};
};

struct PermissionDecision {
    bool allowed{false};
    bool requiresApproval{false};
    QString matchedRule;
    PermissionEffect effect{PermissionEffect::Ask};
};

} // namespace sentinel::core
