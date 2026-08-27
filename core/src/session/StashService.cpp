// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/StashService.h"
#include <QUuid>

namespace sentinel::core {

StashService::StashService(QObject* parent) : QObject(parent) {}
StashService::~StashService() = default;

QString StashService::stash(const QString& sessionId, const QJsonObject& data,
                            const QString& description) {
    StashedChanges changes;
    changes.stashId = generateStashId();
    changes.sessionId = sessionId;
    changes.data = data;
    changes.description = description;
    changes.createdAt = QDateTime::currentDateTime();
    m_stashes.append(changes);
    return changes.stashId;
}

bool StashService::unstash(const QString& stashId, QJsonObject& data) {
    for (int i = 0; i < m_stashes.size(); ++i) {
        if (m_stashes[i].stashId == stashId) {
            data = m_stashes[i].data;
            m_stashes.removeAt(i);
            return true;
        }
    }
    return false;
}

bool StashService::removeStash(const QString& stashId) {
    for (int i = 0; i < m_stashes.size(); ++i) {
        if (m_stashes[i].stashId == stashId) {
            m_stashes.removeAt(i);
            return true;
        }
    }
    return false;
}

QList<StashedChanges> StashService::stashes(const QString& sessionId) const {
    if (sessionId.isEmpty())
        return m_stashes;
    QList<StashedChanges> result;
    for (const auto& s : m_stashes) {
        if (s.sessionId == sessionId)
            result.append(s);
    }
    return result;
}

std::optional<StashedChanges> StashService::findStash(const QString& stashId) const {
    for (const auto& s : m_stashes) {
        if (s.stashId == stashId)
            return s;
    }
    return std::nullopt;
}

QString StashService::generateStashId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

} // namespace sentinel::core
