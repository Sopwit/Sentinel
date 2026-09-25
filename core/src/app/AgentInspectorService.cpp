// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/app/AgentInspectorService.h"

#include <QtGlobal>

namespace sentinel::core {

QList<StoredAgentRun> AgentInspectorService::recentRuns(int limit) const {
    return store_ ? store_->recentRuns(qBound(1, limit, 50)) : QList<StoredAgentRun>{};
}

QList<StoredAgentRun> AgentInspectorService::runsBefore(const QDateTime& time,
                                                        const QString& runId, int limit) const {
    return store_ ? store_->runsBefore(time, runId, qBound(1, limit, 50))
                  : QList<StoredAgentRun>{};
}

AgentInspectorDetail AgentInspectorService::detail(const QString& runId) const {
    AgentInspectorDetail result;
    if (!store_ || runId.isEmpty()) return result;
    result.run = store_->runById(runId);
    if (result.run.runId.isEmpty()) return result;
    result.steps = store_->stepsForRun(runId, 100);
    result.tools = store_->toolCallsForRun(runId, 100);
    result.authorizations = store_->authorizationsForRun(runId, 100);
    result.evidence = store_->evidenceForRun(runId, 100);
    result.claims = store_->claimsForRun(runId, 64);
    result.children = store_->childRuns(runId, 30);
    return result;
}

QString AgentInspectorService::error() const {
    return store_ ? store_->lastError() : QStringLiteral("Agent history is unavailable.");
}

} // namespace sentinel::core
