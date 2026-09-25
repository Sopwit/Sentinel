// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/agent/IAgentRunStore.h"

#include <mutex>

namespace sentinel::core {

class SQLiteAgentRunStore final : public IAgentRunStore {
public:
    explicit SQLiteAgentRunStore(QString databasePath);
    bool record(const AgentEvent& event) override;
    QList<StoredAgentRun> recentRuns(int limit) const override;
    QList<StoredAgentRun> runsBefore(const QDateTime& startedAt,
                                     const QString& runId, int limit) const override;
    StoredAgentRun runById(const QString& runId) const override;
    QList<StoredAgentRun> childRuns(const QString& parentRunId, int limit) const override;
    QList<StoredAgentStep> stepsForRun(const QString& runId, int limit) const override;
    QList<StoredAgentToolCall> toolCallsForRun(const QString& runId, int limit) const override;
    QList<StoredAgentEvidence> evidenceForRun(const QString& runId, int limit) const override;
    QList<StoredAgentClaim> claimsForRun(const QString& runId, int limit) const override;
    QList<StoredAgentAuthorization> authorizationsForRun(const QString& runId,
                                                         int limit) const override;
    QString lastError() const override;

private:
    bool initialize();
    QString databasePath_;
    mutable std::mutex mutex_;
    mutable QString lastError_;
    bool ready_ = false;
};

} // namespace sentinel::core
