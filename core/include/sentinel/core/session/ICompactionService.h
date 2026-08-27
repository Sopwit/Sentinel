// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <functional>
#include <memory>

namespace sentinel::core {

struct ChatMessage;

struct CompactionConfig {
    int maxTokensBeforeCompaction{3500};
    int reservedTokens{20000};
    int preserveRecentTokens{5000};
    int minPreserveTokens{2000};
    int maxPreserveTokens{15000};
    double preserveRecentRatio{0.25};
    bool autoCompactEnabled{true};
    bool autoContinueAfterCompaction{true};
};

struct CompactionResult {
    bool success{false};
    QString summary;
    int messagesCompacted{0};
    int messagesPreserved{0};
    int estimatedTokensSaved{0};
    QString errorString;
};

class ICompactionService {
public:
    virtual ~ICompactionService() = default;

    virtual CompactionResult compact(QList<ChatMessage>& messages, int currentTokens = 0) = 0;
    virtual bool shouldCompact(int currentTokens) const = 0;
    virtual int estimateTokenCount(const QList<ChatMessage>& messages) const = 0;
    virtual void setConfig(const CompactionConfig& config) = 0;
    virtual CompactionConfig config() const = 0;

    // Summary management
    virtual QString currentSummary() const = 0;
    virtual void clearSummary() = 0;
};

} // namespace sentinel::core
