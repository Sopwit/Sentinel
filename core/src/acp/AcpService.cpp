// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/acp/AcpService.h"

namespace sentinel::core {

AcpService::AcpService(QObject* parent) : QObject(parent) {}
AcpService::~AcpService() = default;

bool AcpService::connect(const QString& serverUrl) {
    m_session.serverUrl = serverUrl;
    m_session.connected = true;
    emit connected();
    return true;
}

void AcpService::disconnect() {
    m_session.connected = false;
    emit disconnected();
}

bool AcpService::isConnected() const { return m_session.connected; }
AcpSession AcpService::session() const { return m_session; }

bool AcpService::sendMessage(const AcpMessage& message) {
    Q_UNUSED(message)
    return m_session.connected;
}

AcpMessage AcpService::receiveMessage() {
    return {};
}

void AcpService::setClientInfo(const QString& info) { m_session.clientInfo = info; }

} // namespace sentinel::core
