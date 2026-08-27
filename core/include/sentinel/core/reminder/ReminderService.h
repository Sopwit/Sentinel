// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/reminder/IReminderService.h"
#include <QMap>
#include <QObject>

namespace sentinel::core {

class ReminderService : public QObject, public IReminderService {
    Q_OBJECT
public:
    explicit ReminderService(QObject* parent = nullptr);
    ~ReminderService() override;

    QString addReminder(const QString& sessionId, const QString& content,
                        int triggerAfter = 5) override;
    bool removeReminder(const QString& reminderId) override;
    QList<Reminder> activeReminders(const QString& sessionId) const override;
    void incrementMessageCount(const QString& sessionId) override;
    QStringList checkReminders(const QString& sessionId) override;
    void clearSession(const QString& sessionId) override;

private:
    QString generateId() const;
    QMap<QString, Reminder> m_reminders;
    QMap<QString, int> m_messageCounts;
};

} // namespace sentinel::core
