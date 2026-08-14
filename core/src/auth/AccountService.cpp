// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/auth/AccountService.h"

namespace sentinel::core {

AccountService::AccountService(QObject* parent) : QObject(parent) {}
AccountService::~AccountService() = default;

Account AccountService::currentAccount() const { return m_currentAccount; }

void AccountService::setCurrentAccount(const Account& account) {
    m_currentAccount = account;
}

QList<Account> AccountService::accounts() const { return m_accounts; }

Organization AccountService::organization(const QString& orgId) const {
    return m_organizations.value(orgId);
}

bool AccountService::isOrganizationMember(const QString& orgId) const {
    return m_organizations.contains(orgId);
}

void AccountService::addOrganization(const Organization& org) {
    m_organizations[org.orgId] = org;
}

} // namespace sentinel::core
