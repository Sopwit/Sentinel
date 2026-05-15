#include "sentinel/core/AgentRuntimeContext.h"

#include <utility>

namespace sentinel::core {

namespace {

QStringList plannedToolIds(const ToolInvocationPlan& plan) {
    QStringList ids;
    for (const auto& invocation : plan.invocations) {
        ids.append(invocation.toolId);
    }
    return ids;
}

} // namespace

RuntimeSession::RuntimeSession(RuntimeSessionId sessionId) {
    context_.sessionId = std::move(sessionId);
}

const AgentRuntimeContext& RuntimeSession::context() const {
    return context_;
}

void RuntimeSession::attachPipelineResult(const AgentPipelineResult& pipelineResult) {
    context_.status = RuntimeContextStatus::Active;
    context_.revision += 1;
    context_.latestPipelineResult = pipelineResult;
    context_.activePlannedToolIds = plannedToolIds(pipelineResult.plan);
    context_.summary = QStringLiteral("Runtime context captured pipeline result: %1")
                           .arg(agentPipelineStatusName(pipelineResult));
}

void RuntimeSession::reset() {
    const auto sessionId = context_.sessionId;
    context_ = AgentRuntimeContext{};
    context_.sessionId = sessionId;
    context_.status = RuntimeContextStatus::Cleared;
    context_.summary = QStringLiteral("Runtime context cleared.");
}

} // namespace sentinel::core
