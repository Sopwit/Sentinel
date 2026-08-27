// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/session/ISessionStatusService.h"
#include <QMap>
#include <QMutex>
#include <QObject>

namespace sentinel::core {

class SessionStatusService : public QObject, public ISessionStatusService {
    Q_OBJECT
public:
    explicit SessionStatusService(QObject* parent = nullptr);
    ~SessionStatusService() override;

    void setStatus(const QString& sessionId, SessionStatus status) override;
    SessionStatus status(const QString& sessionId) const override;
    bool isBusy(const QString& sessionId) const override;
    bool acquireLock(const QString& sessionId) override;
    void releaseLock(const QString& sessionId) override;
    QMap<QString, SessionStatus> allStatuses() const override;

signals:
    void statusChanged(const QString& sessionId, SessionStatus status);

private:
    QMap<QString, SessionStatus> m_statuses;
    mutable QMutex m_mutex;
};

} // namespace sentinel::core
