// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/security/IPermissionService.h"
#include <QHash>
#include <QMap>
#include <QObject>

namespace sentinel::core {

class PermissionService : public QObject, public IPermissionService {
    Q_OBJECT
public:
    explicit PermissionService(QObject* parent = nullptr);
    ~PermissionService() override;

    // IPermissionService interface
    bool addRule(const PermissionRule& rule) override;
    bool removeRule(int index) override;
    QList<PermissionRule> rules() const override;
    bool updateRule(int index, const PermissionRule& rule) override;

    PermissionDecision evaluate(const QString& action, const QString& resource) const override;
    bool isAllowed(const QString& action, const QString& resource) const override;
    bool requiresApproval(const QString& action, const QString& resource) const override;

    bool savePermission(const QString& action, const QString& resource,
                        PermissionEffect effect) override;
    bool clearSavedPermissions() override;
    QList<PermissionRule> savedPermissions() const override;

    void setAgentRules(const QString& agentId, const QList<PermissionRule>& rules) override;
    QList<PermissionRule> agentRules(const QString& agentId) const override;

    QString requestPermission(const QString& action, const QString& resource);
    bool resolvePermission(const QString& requestId, bool allowed, bool alwaysAllow = false);
    int rejectPendingForSession(const QString& sessionId);
    QStringList pendingPermissionIds() const;

    // Default rules
    void loadDefaultRules();

signals:
    void ruleAdded(int index);
    void ruleRemoved(int index);
    void ruleUpdated(int index);
    void permissionRequested(const QString& action, const QString& resource);
    void permissionDecisionMade(const QString& action, const QString& resource, bool allowed);
    void permissionPending(const QString& requestId, const QString& action,
                           const QString& resource);

private:
    bool matchPattern(const QString& pattern, const QString& value) const;
    PermissionEffect findMatchingEffect(const QString& action, const QString& resource,
                                        const QList<PermissionRule>& rulesList) const;

    QList<PermissionRule> m_rules;
    QList<PermissionRule> m_savedPermissions;
    QMap<QString, QList<PermissionRule>> m_agentRules;

    struct PendingPermission {
        QString action;
        QString resource;
        QString sessionId;
    };
    QHash<QString, PendingPermission> m_pendingPermissions;
};

} // namespace sentinel::core
