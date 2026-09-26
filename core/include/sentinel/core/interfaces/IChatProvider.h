// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>
#include <QList>
#include "sentinel/core/runtime/ToolDescriptor.h"
#include <QtGlobal>
#include <atomic>
#include <functional>
#include <memory>
#include <optional>

namespace sentinel::core {

enum class ChatProviderStatus {
    Unavailable,
    Ready,
    Error,
};

enum class ChatProviderConcurrency {
    Supported,
    Unsupported,
    Unknown,
};

enum class ChatRequestLifecycle {
    Pending,
    Running,
    Completed,
    Cancelled,
    TimedOut,
    RateLimited,
    Failed,
};

enum class ChatProviderErrorCategory {
    None,
    AuthenticationRequired,
    ModelNotFound,
    ProviderUnavailable,
    ConnectionFailed,
    Timeout,
    RateLimited,
    RequestRejected,
    CapabilityUnsupported,
    MalformedResponse,
    Cancelled,
};

inline QString chatProviderErrorCategoryName(ChatProviderErrorCategory category) {
    switch (category) {
    case ChatProviderErrorCategory::None: return QStringLiteral("None");
    case ChatProviderErrorCategory::AuthenticationRequired:
        return QStringLiteral("AuthenticationRequired");
    case ChatProviderErrorCategory::ModelNotFound: return QStringLiteral("ModelNotFound");
    case ChatProviderErrorCategory::ProviderUnavailable:
        return QStringLiteral("ProviderUnavailable");
    case ChatProviderErrorCategory::ConnectionFailed: return QStringLiteral("ConnectionFailed");
    case ChatProviderErrorCategory::Timeout: return QStringLiteral("Timeout");
    case ChatProviderErrorCategory::RateLimited: return QStringLiteral("RateLimited");
    case ChatProviderErrorCategory::RequestRejected: return QStringLiteral("RequestRejected");
    case ChatProviderErrorCategory::CapabilityUnsupported:
        return QStringLiteral("CapabilityUnsupported");
    case ChatProviderErrorCategory::MalformedResponse: return QStringLiteral("MalformedResponse");
    case ChatProviderErrorCategory::Cancelled: return QStringLiteral("Cancelled");
    }
    return QStringLiteral("None");
}

struct ProviderFailureMetadata {
    ChatRequestLifecycle lifecycle = ChatRequestLifecycle::Failed;
    ChatProviderErrorCategory category = ChatProviderErrorCategory::None;
    int httpStatus = 0;
    int attempts = 1;
    QString requestId;
};

struct ChatProviderReply {
    bool success = false;
    QString message;
    QString errorMessage;
    enum class Error { None, CapabilityRejected, InvalidResponse, ProviderFailure };
    Error error = Error::None;
    ChatRequestLifecycle lifecycle = ChatRequestLifecycle::Pending;
    ChatProviderErrorCategory category = ChatProviderErrorCategory::None;
    int httpStatus = 0;
    int attempts = 1;
    QString retrySummary;
    QString requestId;
    struct ToolCall {
        QString callId;
        QString toolId;
        QJsonObject arguments;
    };
    QList<ToolCall> toolCalls;
    std::optional<QJsonObject> structuredResult;
};

struct ChatRequestOptions {
    std::shared_ptr<std::atomic_bool> cancellationToken;
    bool structuredOutput = false;
    bool nativeToolCalling = false;
    QString structuredSchemaName;
    QJsonObject structuredSchema;
    bool strictStructuredOutput = false;
    QList<ToolDescriptor> tools;
    QList<ChatProviderReply::ToolCall> priorToolCalls;
    struct ToolResult {
        QString callId;
        QString content;
    };
    QList<ToolResult> toolResults;
};

inline QString chatProviderStatusName(ChatProviderStatus status) {
    switch (status) {
    case ChatProviderStatus::Unavailable:
        return QStringLiteral("Unavailable");
    case ChatProviderStatus::Ready:
        return QStringLiteral("Ready");
    case ChatProviderStatus::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Unavailable");
}

class IChatProvider {
public:
    Q_DISABLE_COPY(IChatProvider)
    IChatProvider() = default;
    virtual ~IChatProvider() = default;

    virtual QString name() const = 0;
    virtual ChatProviderStatus status() const = 0;
    virtual ChatProviderReply sendMessage(const QString& message) = 0;
    virtual ChatProviderReply sendRequest(const QString& message, const ChatRequestOptions& options) {
        if (options.structuredOutput || options.nativeToolCalling) {
            ChatProviderReply reply;
            reply.errorMessage = QStringLiteral("Requested model capability is unavailable through this provider.");
            reply.error = ChatProviderReply::Error::CapabilityRejected;
            reply.category = ChatProviderErrorCategory::CapabilityUnsupported;
            reply.lifecycle = ChatRequestLifecycle::Failed;
            return reply;
        }
        return sendMessage(message);
    }
    virtual bool supportsStreaming() const {
        return false;
    }
    virtual ChatProviderConcurrency concurrency() const {
        return ChatProviderConcurrency::Unknown;
    }
    virtual ChatProviderReply sendMessageStreaming(const QString& message,
                                                   const std::function<void(const QString&)>&,
                                                   const std::shared_ptr<std::atomic_bool>&) {
        return sendMessage(message);
    }
};

} // namespace sentinel::core

namespace sentinel::core::interfaces {
using IChatProvider = ::sentinel::core::IChatProvider;
using ChatProviderStatus = ::sentinel::core::ChatProviderStatus;
using ChatProviderReply = ::sentinel::core::ChatProviderReply;
} // namespace sentinel::core::interfaces
