// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

namespace sentinel::core {

struct Account {
    QString accountId;
    QString displayName;
    QString email;
    QString organizationId;
    QString role;
    QJsonObject metadata;
};

struct Organization {
    QString orgId;
    QString name;
    QList<QString> memberIds;
    QJsonObject settings;
};

class IAccountService {
public:
    virtual ~IAccountService() = default;

    virtual Account currentAccount() const = 0;
    virtual void setCurrentAccount(const Account& account) = 0;
    virtual QList<Account> accounts() const = 0;
    virtual Organization organization(const QString& orgId) const = 0;
    virtual bool isOrganizationMember(const QString& orgId) const = 0;
    virtual void addOrganization(const Organization& org) = 0;
};

} // namespace sentinel::core
