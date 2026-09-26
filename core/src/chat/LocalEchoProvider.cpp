// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/chat/LocalEchoProvider.h"

namespace sentinel::core {

QString LocalEchoProvider::name() const {
    return QStringLiteral("LocalEchoProvider");
}

ChatProviderStatus LocalEchoProvider::status() const {
    return ChatProviderStatus::Ready;
}

ChatProviderReply LocalEchoProvider::sendMessage(const QString& message) {
    ChatProviderReply reply;
    reply.success = true;
    reply.lifecycle = ChatRequestLifecycle::Completed;
    reply.message = QStringLiteral("Sentinel Core online. Local chat pipeline is active.\n\n[echo] %1")
                        .arg(message.trimmed());
    return reply;
}

} // namespace sentinel::core
