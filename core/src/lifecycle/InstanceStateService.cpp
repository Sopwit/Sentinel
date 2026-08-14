// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/lifecycle/InstanceStateService.h"
#include <QUuid>

namespace sentinel::core {

InstanceStateService::InstanceStateService(QObject* parent) : QObject(parent) {}
InstanceStateService::~InstanceStateService() = default;

QString InstanceStateService::createInstance() {
    InstanceInfo info;
    info.instanceId = generateId();
    info.state = InstanceState::Created;
    info.createdAt = QDateTime::currentDateTime();
    info.lastActiveAt = info.createdAt;
    m_instances[info.instanceId] = info;
    return info.instanceId;
}

void InstanceStateService::setState(const QString& instanceId, InstanceState state) {
    auto it = m_instances.find(instanceId);
    if (it != m_instances.end()) {
        it->state = state;
        it->lastActiveAt = QDateTime::currentDateTime();
    }
}

InstanceState InstanceStateService::state(const QString& instanceId) const {
    auto it = m_instances.find(instanceId);
    return it != m_instances.end() ? it->state : InstanceState::Error;
}

InstanceInfo InstanceStateService::info(const QString& instanceId) const {
    return m_instances.value(instanceId);
}

void InstanceStateService::updateMetadata(const QString& instanceId, const QJsonObject& metadata) {
    auto it = m_instances.find(instanceId);
    if (it != m_instances.end()) {
        for (auto mit = metadata.begin(); mit != metadata.end(); ++mit) {
            it->metadata[mit.key()] = mit.value();
        }
    }
}

void InstanceStateService::removeInstance(const QString& instanceId) {
    m_instances.remove(instanceId);
}

QList<InstanceInfo> InstanceStateService::allInstances() const {
    return m_instances.values();
}

QString InstanceStateService::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
}

} // namespace sentinel::core
