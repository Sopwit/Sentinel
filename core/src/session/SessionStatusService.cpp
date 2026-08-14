// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/SessionStatusService.h"

namespace sentinel::core {

SessionStatusService::SessionStatusService(QObject* parent) : QObject(parent) {}
SessionStatusService::~SessionStatusService() = default;

void SessionStatusService::setStatus(const QString& sessionId, SessionStatus status) {
    QMutexLocker locker(&m_mutex);
    m_statuses[sessionId] = status;
    emit statusChanged(sessionId, status);
}

SessionStatus SessionStatusService::status(const QString& sessionId) const {
    QMutexLocker locker(&m_mutex);
    return m_statuses.value(sessionId, SessionStatus::Idle);
}

bool SessionStatusService::isBusy(const QString& sessionId) const {
    return status(sessionId) == SessionStatus::Busy;
}

bool SessionStatusService::acquireLock(const QString& sessionId) {
    QMutexLocker locker(&m_mutex);
    if (m_statuses[sessionId] == SessionStatus::Busy) {
        return false;
    }
    m_statuses[sessionId] = SessionStatus::Busy;
    emit statusChanged(sessionId, SessionStatus::Busy);
    return true;
}

void SessionStatusService::releaseLock(const QString& sessionId) {
    QMutexLocker locker(&m_mutex);
    m_statuses[sessionId] = SessionStatus::Idle;
    emit statusChanged(sessionId, SessionStatus::Idle);
}

QMap<QString, SessionStatus> SessionStatusService::allStatuses() const {
    QMutexLocker locker(&m_mutex);
    return m_statuses;
}

} // namespace sentinel::core
