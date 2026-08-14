// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/enterprise/IEnterpriseConfigService.h"
#include <QObject>

namespace sentinel::core {

class EnterpriseConfigService : public QObject, public IEnterpriseConfigService {
    Q_OBJECT
public:
    explicit EnterpriseConfigService(QObject* parent = nullptr);
    ~EnterpriseConfigService() override;

    bool loadPolicy(const QString& policyPath) override;
    EnterprisePolicy currentPolicy() const override;
    bool isEnforced() const override;
    bool settingAllowed(const QString& setting) const override;
    bool isRestricted(const QString& feature) const override;

private:
    EnterprisePolicy m_policy;
};

} // namespace sentinel::core
