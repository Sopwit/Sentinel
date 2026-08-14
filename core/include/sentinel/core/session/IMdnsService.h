// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core {

struct MdnsService {
    QString name;
    QString type;
    QString domain;
    int port;
};

class IMdnsService {
public:
    virtual ~IMdnsService() = default;

    virtual bool publish(const MdnsService& service) = 0;
    virtual bool unpublish(const QString& name) = 0;
    virtual QList<MdnsService> discoveredServices() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual bool isEnabled() const = 0;
};

} // namespace sentinel::core
