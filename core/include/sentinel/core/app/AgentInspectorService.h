// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/agent/IAgentRunStore.h"

namespace sentinel::core {

struct AgentInspectorDetail {
    StoredAgentRun run;
    QList<StoredAgentStep> steps;
    QList<StoredAgentToolCall> tools;
    QList<StoredAgentAuthorization> authorizations;
    QList<StoredAgentEvidence> evidence;
    QList<StoredAgentClaim> claims;
    QList<StoredAgentRun> children;
};

class AgentInspectorService {
public:
    explicit AgentInspectorService(const IAgentRunStore* store) : store_(store) {}

    virtual ~AgentInspectorService() = default;
    virtual QList<StoredAgentRun> recentRuns(int limit) const;
    virtual QList<StoredAgentRun> runsBefore(const QDateTime& time, const QString& runId,
                                             int limit) const;
    virtual AgentInspectorDetail detail(const QString& runId) const;
    virtual QString error() const;

private:
    const IAgentRunStore* store_ = nullptr;
};

} // namespace sentinel::core
