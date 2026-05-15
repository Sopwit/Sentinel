#pragma once

#include "sentinel/core/AgentPipelineResult.h"

#include <QString>
#include <QStringList>

namespace sentinel::core {

enum class RuntimeContextStatus {
    Empty,
    Active,
    Cleared,
};

inline QString runtimeContextStatusName(RuntimeContextStatus status) {
    switch (status) {
    case RuntimeContextStatus::Empty:
        return QStringLiteral("Empty");
    case RuntimeContextStatus::Active:
        return QStringLiteral("Active");
    case RuntimeContextStatus::Cleared:
        return QStringLiteral("Cleared");
    }

    return QStringLiteral("Empty");
}

struct RuntimeSessionId {
    QString value = QStringLiteral("runtime-session-1");
};

struct AgentRuntimeContext {
    RuntimeSessionId sessionId;
    RuntimeContextStatus status = RuntimeContextStatus::Empty;
    int revision = 0;
    AgentPipelineResult latestPipelineResult;
    QStringList activePlannedToolIds;
    QString summary = QStringLiteral("No runtime context yet.");

    QString statusName() const {
        return runtimeContextStatusName(status);
    }
};

inline QString safeAgentRuntimeContextSummary(const AgentRuntimeContext& context) {
    return context.summary.isEmpty() ? QStringLiteral("No runtime context yet.") : context.summary;
}

class RuntimeSession {
public:
    RuntimeSession() = default;
    explicit RuntimeSession(RuntimeSessionId sessionId);

    const AgentRuntimeContext& context() const;
    void attachPipelineResult(const AgentPipelineResult& pipelineResult);
    void reset();

private:
    AgentRuntimeContext context_;
};

} // namespace sentinel::core
