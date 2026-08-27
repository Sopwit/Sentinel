// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

struct AcpSession {
    QString sessionId;
    QString clientInfo;
    QString serverUrl;
    bool connected{false};
};

struct AcpMessage {
    QString role;
    QString content;
    QJsonObject metadata;
};

class IAcpService {
public:
    virtual ~IAcpService() = default;

    virtual bool connect(const QString& serverUrl) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual AcpSession session() const = 0;
    virtual bool sendMessage(const AcpMessage& message) = 0;
    virtual AcpMessage receiveMessage() = 0;
    virtual void setClientInfo(const QString& info) = 0;
};

} // namespace sentinel::core
