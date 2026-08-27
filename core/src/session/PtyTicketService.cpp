// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/PtyTicketService.h"
#include <QUuid>

namespace sentinel::core {

PtyTicket PtyTicketService::createTicket(const QString& sessionId, int ttlSeconds) {
    PtyTicket ticket;
    ticket.ticketId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(16);
    ticket.sessionId = sessionId;
    ticket.createdAt = QDateTime::currentDateTime();
    ticket.expiresAt = ticket.createdAt.addSecs(ttlSeconds);
    m_tickets[ticket.ticketId] = ticket;
    return ticket;
}

bool PtyTicketService::validateTicket(const QString& ticketId) {
    auto it = m_tickets.find(ticketId);
    if (it == m_tickets.end())
        return false;
    if (it->expiresAt < QDateTime::currentDateTime())
        return false;
    it->used = true;
    return true;
}

void PtyTicketService::invalidateTicket(const QString& ticketId) {
    m_tickets.remove(ticketId);
}

bool PtyTicketService::isExpired(const QString& ticketId) const {
    auto it = m_tickets.find(ticketId);
    return it == m_tickets.end() || it->expiresAt < QDateTime::currentDateTime();
}

} // namespace sentinel::core
