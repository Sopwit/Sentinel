// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/lifecycle/IInstanceStateService.h"
#include <QMap>
#include <QObject>

namespace sentinel::core {

class InstanceStateService : public QObject, public IInstanceStateService {
    Q_OBJECT
public:
    explicit InstanceStateService(QObject* parent = nullptr);
    ~InstanceStateService() override;

    QString createInstance() override;
    void setState(const QString& instanceId, InstanceState state) override;
    InstanceState state(const QString& instanceId) const override;
    InstanceInfo info(const QString& instanceId) const override;
    void updateMetadata(const QString& instanceId, const QJsonObject& metadata) override;
    void removeInstance(const QString& instanceId) override;
    QList<InstanceInfo> allInstances() const override;

private:
    QString generateId() const;
    QMap<QString, InstanceInfo> m_instances;
};

} // namespace sentinel::core
