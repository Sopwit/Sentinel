// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMap>
#include <QString>

namespace sentinel::core {

enum class SessionStatus : std::uint8_t { Idle, Busy };

class ISessionStatusService {
public:
    virtual ~ISessionStatusService() = default;

    virtual void setStatus(const QString& sessionId, SessionStatus status) = 0;
    virtual SessionStatus status(const QString& sessionId) const = 0;
    virtual bool isBusy(const QString& sessionId) const = 0;
    virtual bool acquireLock(const QString& sessionId) = 0;
    virtual void releaseLock(const QString& sessionId) = 0;
    virtual QMap<QString, SessionStatus> allStatuses() const = 0;
};

} // namespace sentinel::core
