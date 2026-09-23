// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
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
