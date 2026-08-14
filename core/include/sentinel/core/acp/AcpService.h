// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/acp/IAcpService.h"
#include <QObject>

namespace sentinel::core {

class AcpService : public QObject, public IAcpService {
    Q_OBJECT
public:
    explicit AcpService(QObject* parent = nullptr);
    ~AcpService() override;

    bool connect(const QString& serverUrl) override;
    void disconnect() override;
    bool isConnected() const override;
    AcpSession session() const override;
    bool sendMessage(const AcpMessage& message) override;
    AcpMessage receiveMessage() override;
    void setClientInfo(const QString& info) override;

signals:
    void connected();
    void disconnected();
    void messageReceived(const AcpMessage& message);

private:
    AcpSession m_session;
};

} // namespace sentinel::core
