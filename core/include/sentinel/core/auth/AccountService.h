// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/auth/IAccountService.h"
#include <QObject>
#include <QMap>

namespace sentinel::core {

class AccountService : public QObject, public IAccountService {
    Q_OBJECT
public:
    explicit AccountService(QObject* parent = nullptr);
    ~AccountService() override;

    Account currentAccount() const override;
    void setCurrentAccount(const Account& account) override;
    QList<Account> accounts() const override;
    Organization organization(const QString& orgId) const override;
    bool isOrganizationMember(const QString& orgId) const override;
    void addOrganization(const Organization& org) override;

private:
    Account m_currentAccount;
    QList<Account> m_accounts;
    QMap<QString, Organization> m_organizations;
};

} // namespace sentinel::core
