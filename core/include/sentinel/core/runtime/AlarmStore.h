// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

#include <memory>
#include <mutex>

namespace sentinel::core {

struct AlarmEntry {
    QString id;
    QDateTime triggerAt;
    QString label;
    QDateTime createdAt;
};

// Thread-safe alarm store persisted as JSON. Tool execution threads schedule
// alarms; the UI thread polls due alarms on a timer.
class AlarmStore {
public:
    explicit AlarmStore(QString persistencePath = QString());

    AlarmEntry schedule(const QDateTime& triggerAt, const QString& label);
    bool remove(const QString& alarmId);
    QList<AlarmEntry> active() const;
    QList<AlarmEntry> takeDue(const QDateTime& now);
    bool load();
    bool save() const;

private:
    bool saveLocked() const;

    QString persistencePath_;
    mutable std::mutex mutex_;
    QList<AlarmEntry> alarms_;
    quint64 idSequence_ = 0;
};

} // namespace sentinel::core
