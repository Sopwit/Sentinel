// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/reminder/ReminderService.h"
#include <QUuid>

namespace sentinel::core {

ReminderService::ReminderService(QObject* parent) : QObject(parent) {}
ReminderService::~ReminderService() = default;

QString ReminderService::addReminder(const QString& sessionId, const QString& content, int triggerAfter) {
    Reminder r;
    r.id = generateId();
    r.content = content;
    r.sessionId = sessionId;
    r.triggerAfterMessages = triggerAfter;
    m_reminders[r.id] = r;
    return r.id;
}

bool ReminderService::removeReminder(const QString& reminderId) {
    return m_reminders.remove(reminderId) > 0;
}

QList<Reminder> ReminderService::activeReminders(const QString& sessionId) const {
    QList<Reminder> result;
    for (const auto& r : m_reminders) {
        if (r.sessionId == sessionId && r.active) result.append(r);
    }
    return result;
}

void ReminderService::incrementMessageCount(const QString& sessionId) {
    m_messageCounts[sessionId]++;
}

QStringList ReminderService::checkReminders(const QString& sessionId) {
    QStringList triggered;
    int count = m_messageCounts.value(sessionId, 0);

    for (auto it = m_reminders.begin(); it != m_reminders.end(); ++it) {
        if (it->sessionId == sessionId && it->active) {
            it->messagesSinceLast++;
            if (it->messagesSinceLast >= it->triggerAfterMessages) {
                triggered.append(it->content);
                it->messagesSinceLast = 0;
            }
        }
    }
    return triggered;
}

void ReminderService::clearSession(const QString& sessionId) {
    for (auto it = m_reminders.begin(); it != m_reminders.end(); ++it) {
        if (it->sessionId == sessionId) {
            it = m_reminders.erase(it);
        }
    }
    m_messageCounts.remove(sessionId);
}

QString ReminderService::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

} // namespace sentinel::core
