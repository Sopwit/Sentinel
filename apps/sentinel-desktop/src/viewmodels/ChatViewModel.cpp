// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sentinel/desktop/viewmodels/ChatViewModel.h>

#include <QDateTime>

namespace sentinel::desktop::viewmodels {

using sentinel::core::ChatMessage;
using sentinel::core::ChatMessageStatus;
using sentinel::core::ChatRole;

ChatViewModel::ChatViewModel(sentinel::core::IChatProvider* provider,
                             sentinel::core::IChatHistoryStore* historyStore, QObject* parent)
    : QObject(parent), m_provider(provider), m_historyStore(historyStore) {
}

void ChatViewModel::setPrompt(const QString& prompt) {
    if (m_prompt != prompt) {
        m_prompt = prompt;
        Q_EMIT promptChanged();
    }
}

void ChatViewModel::setIsStreaming(bool streaming) {
    if (m_isStreaming != streaming) {
        m_isStreaming = streaming;
        Q_EMIT isStreamingChanged();
    }
}

void ChatViewModel::clearPrompt() {
    setPrompt(QString());
}

void ChatViewModel::sendPrompt() {
    if (m_prompt.trimmed().isEmpty()) {
        return;
    }
    const QString text = m_prompt;
    clearPrompt();
    Q_EMIT messageReceived(QStringLiteral("user"), text);

    if (m_historyStore) {
        ChatMessage userMessage;
        userMessage.role = ChatRole::User;
        userMessage.content = text;
        userMessage.timestamp = QDateTime::currentDateTime();
        userMessage.status = ChatMessageStatus::Sent;
        m_historyStore->appendMessage(userMessage);
    }

    if (!m_provider) {
        Q_EMIT messageReceived(QStringLiteral("assistant"),
                              QStringLiteral("No chat provider is configured."));
        return;
    }

    setIsStreaming(true);
    const auto reply = m_provider->sendMessage(text);
    setIsStreaming(false);

    const auto assistantText =
        reply.success ? reply.message
                      : QStringLiteral("Provider error: %1").arg(reply.errorMessage);
    Q_EMIT messageReceived(QStringLiteral("assistant"), assistantText);

    if (m_historyStore) {
        ChatMessage assistantMessage;
        assistantMessage.role = ChatRole::Assistant;
        assistantMessage.content = assistantText;
        assistantMessage.timestamp = QDateTime::currentDateTime();
        assistantMessage.status =
            reply.success ? ChatMessageStatus::Received : ChatMessageStatus::Error;
        m_historyStore->appendMessage(assistantMessage);
    }
}

} // namespace sentinel::desktop::viewmodels
