// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

struct PtyTicket {
    QString ticketId;
    QString sessionId;
    QDateTime createdAt;
    QDateTime expiresAt;
    bool used{false};
};

class PtyTicketService {
public:
    PtyTicket createTicket(const QString& sessionId, int ttlSeconds = 300);
    bool validateTicket(const QString& ticketId);
    void invalidateTicket(const QString& ticketId);
    bool isExpired(const QString& ticketId) const;

private:
    QMap<QString, PtyTicket> m_tickets;
};

} // namespace sentinel::core
