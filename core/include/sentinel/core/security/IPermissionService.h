// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QList>
#include <QMap>
#include <functional>

namespace sentinel::core {

enum class PermissionEffect : std::uint8_t {
    Allow,
    Ask,
    Deny
};

struct PermissionRule {
    QString action; // e.g., "edit", "shell", "read", "write", "websearch", "webfetch", "lsp", "skill", "task"
    QString resource; // pattern with wildcards, e.g., "*.cpp", "/path/to/*", "https://example.com/*"
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

class IPermissionService {
public:
    virtual ~IPermissionService() = default;

    // Rule management
    virtual bool addRule(const PermissionRule& rule) = 0;
    virtual bool removeRule(int index) = 0;
    virtual QList<PermissionRule> rules() const = 0;
    virtual bool updateRule(int index, const PermissionRule& rule) = 0;

    // Permission evaluation
    virtual PermissionDecision evaluate(const QString& action, const QString& resource) const = 0;
    virtual bool isAllowed(const QString& action, const QString& resource) const = 0;
    virtual bool requiresApproval(const QString& action, const QString& resource) const = 0;

    // Saved permissions (per-project)
    virtual bool savePermission(const QString& action, const QString& resource, PermissionEffect effect) = 0;
    virtual bool clearSavedPermissions() = 0;
    virtual QList<PermissionRule> savedPermissions() const = 0;

    // Agent-specific rules
    virtual void setAgentRules(const QString& agentId, const QList<PermissionRule>& rules) = 0;
    virtual QList<PermissionRule> agentRules(const QString& agentId) const = 0;
};

} // namespace sentinel::core
