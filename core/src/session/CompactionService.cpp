// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/CompactionService.h"
#include "sentinel/core/chat/ChatMessage.h"
#include "sentinel/core/interfaces/IChatProvider.h"
#include "sentinel/core/session/CompactionPrompt.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtConcurrentRun>

namespace sentinel::core {

CompactionService::CompactionService(QObject* parent) : QObject(parent) {}

CompactionService::~CompactionService() = default;

CompactionResult CompactionService::compact(QList<ChatMessage>& messages, int currentTokens) {
    CompactionResult result;

    if (messages.isEmpty()) {
        result.errorString = "No messages to compact";
        return result;
    }

    // Estimate tokens if not provided
    if (currentTokens <= 0) {
        currentTokens = estimateTokenCount(messages);
    }

    // Check if compaction is needed
    if (!shouldCompact(currentTokens)) {
        result.errorString = "Compaction not needed";
        return result;
    }

    // Calculate how many messages to preserve
    int preserveCount = calculatePreserveCount(messages.size(), currentTokens);
    int compactCount = messages.size() - preserveCount;

    if (compactCount <= 0) {
        result.errorString = "Not enough messages to compact";
        return result;
    }

    // Select messages to compact (oldest ones)
    QList<int> indicesToCompact = selectMessagesToCompact(messages, preserveCount);

    // Extract messages for summarization
    QList<ChatMessage> messagesToCompact;
    for (int idx : indicesToCompact) {
        if (idx >= 0 && idx < messages.size()) {
            messagesToCompact.append(messages[idx]);
        }
    }

    // Build compaction prompt
    QString prompt = buildCompactionPrompt(messagesToCompact, m_currentSummary);

    // Generate summary (this would call the LLM in production)
    QString summary = generateSummary(prompt);

    if (summary.isEmpty()) {
        result.errorString = "Failed to generate summary";
        return result;
    }

    // Update current summary
    m_currentSummary = summary;

    // Remove compacted messages (in reverse order to preserve indices)
    std::sort(indicesToCompact.begin(), indicesToCompact.end(), std::greater<int>());
    for (int idx : indicesToCompact) {
        if (idx >= 0 && idx < messages.size()) {
            messages.removeAt(idx);
        }
    }

    // Add summary as a system message at the beginning
    ChatMessage summaryMessage;
    summaryMessage.role = ChatRole::System;
    summaryMessage.content = QStringLiteral("[Conversation Summary]\n%1").arg(summary);
    summaryMessage.timestamp = QDateTime::currentDateTime();
    messages.prepend(summaryMessage);

    result.success = true;
    result.summary = summary;
    result.messagesCompacted = compactCount;
    result.messagesPreserved = preserveCount;
    result.estimatedTokensSaved = currentTokens - estimateTokenCount(messages);

    qDebug() << QStringLiteral("CompactionService: Compacted %1 messages, saved ~%2 tokens")
                    .arg(compactCount)
                    .arg(result.estimatedTokensSaved);

    return result;
}

bool CompactionService::shouldCompact(int currentTokens) const {
    if (!m_config.autoCompactEnabled) {
        return false;
    }
    return currentTokens > m_config.maxTokensBeforeCompaction;
}

int CompactionService::estimateTokenCount(const QList<ChatMessage>& messages) const {
    int totalTokens = 0;
    for (const auto& message : messages) {
        totalTokens += estimateTokens(message.content);
    }
    return totalTokens;
}

void CompactionService::setConfig(const CompactionConfig& config) {
    m_config = config;
}

CompactionConfig CompactionService::config() const {
    return m_config;
}

QString CompactionService::currentSummary() const {
    return m_currentSummary;
}

void CompactionService::clearSummary() {
    m_currentSummary.clear();
}

void CompactionService::compactAsync(QList<ChatMessage>& messages, int currentTokens,
                                     std::function<void(CompactionResult)> callback) {
    QFuture<CompactionResult> future = QtConcurrent::run(
        [this, &messages, currentTokens]() { return compact(messages, currentTokens); });

    QFutureWatcher<CompactionResult>* watcher = new QFutureWatcher<CompactionResult>(this);
    connect(watcher, &QFutureWatcher<CompactionResult>::finished, this, [watcher, callback]() {
        CompactionResult result = watcher->result();
        if (callback) {
            callback(result);
        }
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void CompactionService::setChatProvider(IChatProvider* provider) {
    m_chatProvider = provider;
}

int CompactionService::estimateTokens(const QString& text) const {
    // Rough estimation: ~4 characters per token
    return text.length() / 4;
}

QList<int> CompactionService::selectMessagesToCompact(const QList<ChatMessage>& messages,
                                                      int preserveRecentCount) const {
    QList<int> indices;

    // Compact oldest messages, preserve recent ones
    int compactEnd = messages.size() - preserveRecentCount;
    for (int i = 0; i < compactEnd; ++i) {
        indices.append(i);
    }

    return indices;
}

QString CompactionService::buildCompactionPrompt(const QList<ChatMessage>& messagesToCompact,
                                                 const QString& previousSummary) const {
    QJsonArray messagesArray;
    for (const auto& message : messagesToCompact) {
        QJsonObject msgObj;
        msgObj["role"] = chatRoleName(message.role);
        msgObj["content"] = message.content;
        messagesArray.append(msgObj);
    }

    return CompactionPrompt::buildPrompt(messagesArray, previousSummary);
}

QString CompactionService::generateSummary(const QString& prompt) {
    if (!m_chatProvider) {
        return {};
    }
    const ChatProviderReply reply = m_chatProvider->sendMessage(prompt);
    if (!reply.success || reply.message.trimmed().isEmpty()) {
        return {};
    }
    return reply.message.trimmed();
}

QString CompactionService::serializeMessage(const ChatMessage& message) const {
    return QStringLiteral("[%1]: %2").arg(chatRoleName(message.role), message.content);
}

int CompactionService::calculatePreserveCount(int totalMessages, int currentTokens) const {
    if (totalMessages <= 2) {
        return totalMessages;
    }

    // Calculate based on token ratio
    int targetPreserveTokens = static_cast<int>(currentTokens * m_config.preserveRecentRatio);
    targetPreserveTokens =
        qBound(m_config.minPreserveTokens, targetPreserveTokens, m_config.maxPreserveTokens);

    // Estimate messages needed for target tokens
    int estimatedTokensPerMessage = currentTokens / totalMessages;
    if (estimatedTokensPerMessage <= 0) {
        estimatedTokensPerMessage = 100; // Default estimate
    }

    int preserveCount = targetPreserveTokens / estimatedTokensPerMessage;
    preserveCount =
        qMax(2, qMin(preserveCount, totalMessages - 1)); // Keep at least 1 message for compaction

    return preserveCount;
}

} // namespace sentinel::core
