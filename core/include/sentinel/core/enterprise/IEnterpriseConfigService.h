// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core {

struct EnterprisePolicy {
    QString organizationId;
    QJsonObject settings;
    QJsonObject restrictions;
    QDateTime effectiveDate;
    bool enforced{false};
};

class IEnterpriseConfigService {
public:
    virtual ~IEnterpriseConfigService() = default;

    virtual bool loadPolicy(const QString& policyPath) = 0;
    virtual EnterprisePolicy currentPolicy() const = 0;
    virtual bool isEnforced() const = 0;
    virtual bool settingAllowed(const QString& setting) const = 0;
    virtual bool isRestricted(const QString& feature) const = 0;
};

} // namespace sentinel::core
