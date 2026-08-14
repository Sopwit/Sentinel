// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/enterprise/EnterpriseConfigService.h"

namespace sentinel::core {

EnterpriseConfigService::EnterpriseConfigService(QObject* parent) : QObject(parent) {}
EnterpriseConfigService::~EnterpriseConfigService() = default;

bool EnterpriseConfigService::loadPolicy(const QString& policyPath) {
    Q_UNUSED(policyPath)
    return true;
}

EnterprisePolicy EnterpriseConfigService::currentPolicy() const { return m_policy; }
bool EnterpriseConfigService::isEnforced() const { return m_policy.enforced; }

bool EnterpriseConfigService::settingAllowed(const QString& setting) const {
    if (!m_policy.enforced) return true;
    return !m_policy.restrictions.contains(setting);
}

bool EnterpriseConfigService::isRestricted(const QString& feature) const {
    return m_policy.restrictions[feature].toBool();
}

} // namespace sentinel::core
