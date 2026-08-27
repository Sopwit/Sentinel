// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/security/PermissionService.h"
#include <QDebug>
#include <QRegularExpression>
#include <QUuid>

namespace sentinel::core {

PermissionService::PermissionService(QObject* parent) : QObject(parent) {
    loadDefaultRules();
}

PermissionService::~PermissionService() = default;

bool PermissionService::addRule(const PermissionRule& rule) {
    m_rules.append(rule);
    emit ruleAdded(m_rules.size() - 1);
    return true;
}

bool PermissionService::removeRule(int index) {
    if (index < 0 || index >= m_rules.size()) {
        return false;
    }

    m_rules.removeAt(index);
    emit ruleRemoved(index);
    return true;
}

QList<PermissionRule> PermissionService::rules() const {
    return m_rules;
}

bool PermissionService::updateRule(int index, const PermissionRule& rule) {
    if (index < 0 || index >= m_rules.size()) {
        return false;
    }

    m_rules[index] = rule;
    emit ruleUpdated(index);
    return true;
}

PermissionDecision PermissionService::evaluate(const QString& action,
                                               const QString& resource) const {
    PermissionDecision decision;

    // Check agent-specific rules first
    for (auto it = m_agentRules.begin(); it != m_agentRules.end(); ++it) {
        PermissionEffect effect = findMatchingEffect(action, resource, it.value());
        if (effect != PermissionEffect::Ask) {
            decision.allowed = (effect == PermissionEffect::Allow);
            decision.requiresApproval = (effect == PermissionEffect::Ask);
            decision.effect = effect;
            return decision;
        }
    }

    // Check saved permissions
    PermissionEffect savedEffect = findMatchingEffect(action, resource, m_savedPermissions);
    if (savedEffect != PermissionEffect::Ask) {
        decision.allowed = (savedEffect == PermissionEffect::Allow);
        decision.effect = savedEffect;
        return decision;
    }

    // Check default rules
    PermissionEffect defaultEffect = findMatchingEffect(action, resource, m_rules);
    decision.effect = defaultEffect;
    decision.allowed = (defaultEffect == PermissionEffect::Allow);
    decision.requiresApproval = (defaultEffect == PermissionEffect::Ask);

    return decision;
}

bool PermissionService::isAllowed(const QString& action, const QString& resource) const {
    PermissionDecision decision = evaluate(action, resource);
    return decision.allowed;
}

bool PermissionService::requiresApproval(const QString& action, const QString& resource) const {
    PermissionDecision decision = evaluate(action, resource);
    return decision.requiresApproval;
}

bool PermissionService::savePermission(const QString& action, const QString& resource,
                                       PermissionEffect effect) {
    PermissionRule rule;
    rule.action = action;
    rule.resource = resource;
    rule.effect = effect;
    rule.description = QStringLiteral("Saved permission for %1 on %2").arg(action, resource);

    m_savedPermissions.append(rule);
    return true;
}

bool PermissionService::clearSavedPermissions() {
    m_savedPermissions.clear();
    return true;
}

QList<PermissionRule> PermissionService::savedPermissions() const {
    return m_savedPermissions;
}

void PermissionService::setAgentRules(const QString& agentId, const QList<PermissionRule>& rules) {
    m_agentRules[agentId] = rules;
}

QList<PermissionRule> PermissionService::agentRules(const QString& agentId) const {
    return m_agentRules.value(agentId);
}

QString PermissionService::requestPermission(const QString& action, const QString& resource) {
    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_pendingPermissions.insert(requestId, {action, resource, {}});
    emit permissionRequested(action, resource);
    emit permissionPending(requestId, action, resource);
    return requestId;
}

bool PermissionService::resolvePermission(const QString& requestId, bool allowed,
                                          bool alwaysAllow) {
    const auto it = m_pendingPermissions.find(requestId);
    if (it == m_pendingPermissions.end()) {
        return false;
    }

    const PendingPermission pending = it.value();
    m_pendingPermissions.erase(it);
    if (allowed && alwaysAllow) {
        savePermission(pending.action, pending.resource, PermissionEffect::Allow);
    }
    emit permissionDecisionMade(pending.action, pending.resource, allowed);
    return true;
}

int PermissionService::rejectPendingForSession(const QString& sessionId) {
    int rejected = 0;
    for (auto it = m_pendingPermissions.begin(); it != m_pendingPermissions.end();) {
        if (it.value().sessionId == sessionId) {
            const PendingPermission pending = it.value();
            it = m_pendingPermissions.erase(it);
            emit permissionDecisionMade(pending.action, pending.resource, false);
            ++rejected;
        } else {
            ++it;
        }
    }
    return rejected;
}

QStringList PermissionService::pendingPermissionIds() const {
    return m_pendingPermissions.keys();
}

void PermissionService::loadDefaultRules() {
    // Allow read operations by default
    m_rules.append({"read", "*", PermissionEffect::Allow, "Allow reading files", true});
    m_rules.append({"glob", "*", PermissionEffect::Allow, "Allow file pattern matching", true});
    m_rules.append({"grep", "*", PermissionEffect::Allow, "Allow content search", true});

    // Ask for write operations
    m_rules.append({"edit", "*", PermissionEffect::Ask, "Ask before editing files", true});
    m_rules.append({"write", "*", PermissionEffect::Ask, "Ask before writing files", true});

    // Ask for shell operations
    m_rules.append(
        {"shell", "*", PermissionEffect::Ask, "Ask before running shell commands", true});

    // Ask for web operations
    m_rules.append({"websearch", "*", PermissionEffect::Ask, "Ask before web search", true});
    m_rules.append({"webfetch", "*", PermissionEffect::Ask, "Ask before fetching URLs", true});

    // Allow LSP operations
    m_rules.append({"lsp", "*", PermissionEffect::Allow, "Allow LSP operations", true});

    // Allow skill operations
    m_rules.append({"skill", "*", PermissionEffect::Allow, "Allow skill loading", true});

    // Ask for task operations
    m_rules.append({"task", "*", PermissionEffect::Ask, "Ask before running tasks", true});

    // Deny dangerous operations by default
    m_rules.append({"shell", "rm -rf *", PermissionEffect::Deny, "Deny recursive delete", true});
    m_rules.append({"shell", "sudo *", PermissionEffect::Deny, "Deny sudo operations", true});
    m_rules.append(
        {"shell", "chmod 777 *", PermissionEffect::Deny, "Deny unsafe permissions", true});
}

bool PermissionService::matchPattern(const QString& pattern, const QString& value) const {
    if (pattern == "*") {
        return true;
    }

    if (pattern == value) {
        return true;
    }

    // Convert glob pattern to regex
    QString regexPattern = pattern;
    regexPattern.replace(".", "\\.");
    regexPattern.replace("*", ".*");
    regexPattern.replace("?", ".");

    QRegularExpression regex(regexPattern, QRegularExpression::CaseInsensitiveOption);
    return regex.match(value).hasMatch();
}

PermissionEffect
PermissionService::findMatchingEffect(const QString& action, const QString& resource,
                                      const QList<PermissionRule>& rulesList) const {
    // Find the last matching rule (most specific)
    PermissionEffect lastEffect = PermissionEffect::Ask;
    bool foundMatch = false;

    for (const auto& rule : rulesList) {
        if (rule.action == action || rule.action == "*") {
            if (matchPattern(rule.resource, resource)) {
                lastEffect = rule.effect;
                foundMatch = true;
            }
        }
    }

    return foundMatch ? lastEffect : PermissionEffect::Ask;
}

} // namespace sentinel::core
