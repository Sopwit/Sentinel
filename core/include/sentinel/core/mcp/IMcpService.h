// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <functional>
#include <memory>

namespace sentinel::core {

struct McpToolDefinition {
    QString name;
    QString description;
    QString serverName;
    QJsonObject inputSchema;
    bool filesystemSemanticContract = false;
};

struct McpResource {
    QString uri;
    QString name;
    QString description;
    QString mimeType;
    QString serverName;
};

struct McpServerConfig {
    QString name;
    QString type;          // "local" or "remote"
    QString command;       // for local: executable path
    QStringList arguments; // for local: command arguments
    QString url;           // for remote: HTTP endpoint
    QJsonObject headers;   // for remote: custom headers
    bool enabled{true};
    bool allowPartialConfinement{false}; // Windows restricted token/Job Object lacks filesystem confinement
};

enum class McpConnectionState { Disconnected, Connecting, Connected, Error };
enum class McpFailureCategory {
    None, ServerUnavailable, StartupFailure, ProtocolError, Timeout, Cancelled,
    InvalidToolSchema, RemoteExecutionFailure, SecurityDenied, TransportFailure,
    AuthenticationFailure, ConfigurationFailure, UnsupportedProtocolVersion,
    InteractionRequired, InteractionRejected, UnsupportedLegacyTransport, Offline
};

inline QString mcpFailureCategoryName(McpFailureCategory category) {
    switch (category) {
    case McpFailureCategory::None: return QStringLiteral("None");
    case McpFailureCategory::ServerUnavailable: return QStringLiteral("ServerUnavailable");
    case McpFailureCategory::StartupFailure: return QStringLiteral("StartupFailure");
    case McpFailureCategory::ProtocolError: return QStringLiteral("ProtocolError");
    case McpFailureCategory::Timeout: return QStringLiteral("Timeout");
    case McpFailureCategory::Cancelled: return QStringLiteral("Cancelled");
    case McpFailureCategory::InvalidToolSchema: return QStringLiteral("InvalidToolSchema");
    case McpFailureCategory::RemoteExecutionFailure: return QStringLiteral("RemoteExecutionFailure");
    case McpFailureCategory::SecurityDenied: return QStringLiteral("SecurityDenied");
    case McpFailureCategory::TransportFailure: return QStringLiteral("TransportFailure");
    case McpFailureCategory::AuthenticationFailure: return QStringLiteral("AuthenticationFailure");
    case McpFailureCategory::ConfigurationFailure: return QStringLiteral("ConfigurationFailure");
    case McpFailureCategory::UnsupportedProtocolVersion: return QStringLiteral("UnsupportedProtocolVersion");
    case McpFailureCategory::InteractionRequired: return QStringLiteral("InteractionRequired");
    case McpFailureCategory::InteractionRejected: return QStringLiteral("InteractionRejected");
    case McpFailureCategory::UnsupportedLegacyTransport: return QStringLiteral("UnsupportedLegacyTransport");
    case McpFailureCategory::Offline: return QStringLiteral("Offline");
    }
    return QStringLiteral("ProtocolError");
}

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
    using ToolCompletion = std::function<void(QJsonObject)>;
    using Cancel = std::function<void()>;
    virtual Cancel callToolAsync(const QString& serverName, const QString& toolName,
                                 const QJsonObject& arguments, ToolCompletion completion) {
        Q_UNUSED(serverName)
        Q_UNUSED(toolName)
        Q_UNUSED(arguments)
        completion({{"error", QJsonObject{{"message", "Async MCP calls unavailable"}}}});
        return {};
    }

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
