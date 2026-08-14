// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>
#include <QMap>

namespace sentinel::core {

enum class InstanceState : std::uint8_t {
    Created,
    Initializing,
    Running,
    Suspended,
    Stopped,
    Error
};

struct InstanceInfo {
    QString instanceId;
    InstanceState state{InstanceState::Created};
    QDateTime createdAt;
    QDateTime lastActiveAt;
    QJsonObject metadata;
};

class IInstanceStateService {
public:
    virtual ~IInstanceStateService() = default;

    virtual QString createInstance() = 0;
    virtual void setState(const QString& instanceId, InstanceState state) = 0;
    virtual InstanceState state(const QString& instanceId) const = 0;
    virtual InstanceInfo info(const QString& instanceId) const = 0;
    virtual void updateMetadata(const QString& instanceId, const QJsonObject& metadata) = 0;
    virtual void removeInstance(const QString& instanceId) = 0;
    virtual QList<InstanceInfo> allInstances() const = 0;
};

} // namespace sentinel::core
