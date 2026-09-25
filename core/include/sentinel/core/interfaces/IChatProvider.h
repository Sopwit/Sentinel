// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>
#include <QList>
#include <QtGlobal>
#include <atomic>
#include <functional>
#include <memory>

namespace sentinel::core {

enum class ChatProviderStatus {
    Unavailable,
    Ready,
    Error,
};

struct ChatProviderReply {
    bool success = false;
    QString message;
    QString errorMessage;
    struct ToolCall {
        QString toolId;
        QJsonObject arguments;
    };
    QList<ToolCall> toolCalls;
};

struct ChatRequestOptions {
    bool structuredOutput = false;
    bool nativeToolCalling = false;
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
        if (options.structuredOutput || options.nativeToolCalling)
            return {false, {}, QStringLiteral("Requested model capability is unavailable through this provider.")};
        return sendMessage(message);
    }
    virtual bool supportsStreaming() const {
        return false;
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
