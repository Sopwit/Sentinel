// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/session/ICompactionService.h"
#include <QFuture>
#include <QObject>
#include <QPromise>

namespace sentinel::core {

class IChatProvider;

class CompactionService : public QObject, public ICompactionService {
    Q_OBJECT
public:
    explicit CompactionService(QObject* parent = nullptr);
    ~CompactionService() override;

    // ICompactionService interface
    CompactionResult compact(QList<ChatMessage>& messages, int currentTokens = 0) override;
    bool shouldCompact(int currentTokens) const override;
    int estimateTokenCount(const QList<ChatMessage>& messages) const override;
    void setConfig(const CompactionConfig& config) override;
    CompactionConfig config() const override;

    QString currentSummary() const override;
    void clearSummary() override;

    // Async compaction
    void compactAsync(QList<ChatMessage>& messages, int currentTokens,
                      std::function<void(CompactionResult)> callback);

    // Set chat provider for LLM summarization
    void setChatProvider(IChatProvider* provider);

signals:
    void compactionStarted();
    void compactionCompleted(const CompactionResult& result);
    void compactionFailed(const QString& error);

private:
    // Token estimation
    int estimateTokens(const QString& text) const;

    // Message selection
    QList<int> selectMessagesToCompact(const QList<ChatMessage>& messages,
                                       int preserveRecentCount) const;

    // Summary generation
    QString buildCompactionPrompt(const QList<ChatMessage>& messagesToCompact,
                                  const QString& previousSummary) const;
    QString generateSummary(const QString& prompt);

    // Message serialization
    QString serializeMessage(const ChatMessage& message) const;

    // Helper methods
    int calculatePreserveCount(int totalMessages, int currentTokens) const;

    CompactionConfig m_config;
    QString m_currentSummary;
    IChatProvider* m_chatProvider{nullptr};
};

} // namespace sentinel::core
