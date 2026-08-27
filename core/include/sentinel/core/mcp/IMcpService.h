// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <memory>

namespace sentinel::core {

struct McpToolDefinition {
    QString name;
    QString description;
    QString serverName;
    QJsonObject inputSchema;
};

struct McpServerConfig {
    QString name;
    QString type;          // "local" or "remote"
    QString command;       // for local: executable path
    QStringList arguments; // for local: command arguments
    QString url;           // for remote: HTTP endpoint
    QJsonObject headers;   // for remote: custom headers
    bool enabled{true};
};

enum class McpConnectionState { Disconnected, Connecting, Connected, Error };

class IMcpService {
public:
    virtual ~IMcpService() = default;

    // Server management
    virtual bool addServer(const McpServerConfig& config) = 0;
    virtual bool removeServer(const QString& serverName) = 0;
    virtual QList<McpServerConfig> servers() const = 0;
    virtual McpServerConfig serverConfig(const QString& serverName) const = 0;

    // Connection management
    virtual bool connectToServer(const QString& serverName) = 0;
    virtual bool disconnectFromServer(const QString& serverName) = 0;
    virtual McpConnectionState connectionState(const QString& serverName) const = 0;

    // Tool operations
    virtual QList<McpToolDefinition> tools(const QString& serverName = QString()) const = 0;
    virtual QJsonObject callTool(const QString& serverName, const QString& toolName,
                                 const QJsonObject& arguments = {}) = 0;

    // Batch operations
    virtual bool connectToAll() = 0;
    virtual void disconnectFromAll() = 0;

signals:
    virtual void serverConnected(const QString& serverName) = 0;
    virtual void serverDisconnected(const QString& serverName) = 0;
    virtual void serverError(const QString& serverName, const QString& error) = 0;
    virtual void toolsUpdated(const QString& serverName) = 0;
};

} // namespace sentinel::core
