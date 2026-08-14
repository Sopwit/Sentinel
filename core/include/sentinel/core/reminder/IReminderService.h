// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>
#include <QList>

namespace sentinel::core {

struct Reminder {
    QString id;
    QString content;
    QString sessionId;
    int triggerAfterMessages{5};
    int messagesSinceLast{0};
    bool active{true};
};

class IReminderService {
public:
    virtual ~IReminderService() = default;

    virtual QString addReminder(const QString& sessionId, const QString& content, int triggerAfter = 5) = 0;
    virtual bool removeReminder(const QString& reminderId) = 0;
    virtual QList<Reminder> activeReminders(const QString& sessionId) const = 0;
    virtual void incrementMessageCount(const QString& sessionId) = 0;
    virtual QStringList checkReminders(const QString& sessionId) = 0;
    virtual void clearSession(const QString& sessionId) = 0;
};

} // namespace sentinel::core
