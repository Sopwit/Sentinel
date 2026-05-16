#include "sentinel/core/ExecutionLifecycle.h"

#include <utility>

namespace sentinel::core {

namespace {

ExecutionLifecycleTrace trace(int sequence, ExecutionLifecycleState state,
                              ExecutionTraceLevel level, const QString& summary) {
    return ExecutionLifecycleTrace{sequence, state, level, summary};
}

} // namespace

QString executionLifecycleTraceSummary(const ExecutionLifecycleTrace& trace) {
    return QStringLiteral("%1. %2 [%3]: %4")
        .arg(trace.sequence)
        .arg(executionLifecycleStateName(trace.state), executionTraceLevelName(trace.level),
             trace.summary);
}

QStringList executionLifecycleTraceSummaries(const QList<ExecutionLifecycleTrace>& traces) {
    QStringList summaries;
    for (const auto& trace : traces) {
        summaries.append(executionLifecycleTraceSummary(trace));
    }
    return summaries;
}

QString safeExecutionLifecycleSummary(const ExecutionLifecycleResult& result) {
    if (!result.summary.isEmpty()) {
        return result.summary;
    }

    return QStringLiteral("Execution lifecycle %1 in %2 state (%3 traces).")
        .arg(executionLifecycleStatusName(result.status), executionLifecycleStateName(result.state))
        .arg(result.traces.size());
}

QString safeExecutionSessionSummary(const ExecutionSession& session) {
    if (!session.summary.isEmpty()) {
        return session.summary;
    }

    return QStringLiteral("%1: %2 %3 session.")
        .arg(session.id.value, executionOwnershipName(session.ownership),
             executionSessionStatusName(session.status));
}

QString safeExecutionCoordinationSnapshotSummary(const ExecutionCoordinationSnapshot& snapshot) {
    if (!snapshot.summary.isEmpty()) {
        return snapshot.summary;
    }

    return QStringLiteral("Execution coordination snapshot is read-only; execution is blocked.");
}

ExecutionLifecycleResult StaticExecutionLifecycle::evaluate(const ExecutionRequest& request) const {
    ExecutionLifecycleResult result;
    result.request = request;
    result.state = ExecutionLifecycleState::Blocked;
    result.status = ExecutionLifecycleStatus::Blocked;
    result.transitionAccepted = true;
    result.executable = false;
    result.summary = QStringLiteral("Execution lifecycle reached blocked metadata state; no "
                                    "execution is permitted.");
    result.traces = {
        trace(1, ExecutionLifecycleState::Requested, ExecutionTraceLevel::Info,
              request.summary.isEmpty() ? QStringLiteral("Execution request metadata received.")
                                        : request.summary),
        trace(2, ExecutionLifecycleState::Validating, ExecutionTraceLevel::Info,
              QStringLiteral("Execution request metadata validated without dispatch.")),
        trace(3, ExecutionLifecycleState::PermissionCheck, ExecutionTraceLevel::Info,
              QStringLiteral("Permission check is metadata-only and cannot grant execution.")),
        trace(4, ExecutionLifecycleState::SafetyCheck, ExecutionTraceLevel::Info,
              QStringLiteral("Safety check preserves local no-execution posture.")),
        trace(5, ExecutionLifecycleState::Coordination, ExecutionTraceLevel::Info,
              QStringLiteral("Session coordination recorded ownership metadata only.")),
        trace(6, ExecutionLifecycleState::ReadyPlaceholder, ExecutionTraceLevel::Warning,
              QStringLiteral("Ready placeholder is descriptive and not executable.")),
        trace(7, ExecutionLifecycleState::Blocked, ExecutionTraceLevel::Blocked,
              QStringLiteral("Execution remains intentionally blocked.")),
    };
    return result;
}

ExecutionLifecycleResult
StaticExecutionLifecycle::transition(ExecutionLifecycleState current, ExecutionLifecycleState next,
                                     const ExecutionRequest& request) const {
    ExecutionLifecycleResult result;
    result.request = request;
    result.state = canTransition(current, next) ? next : current;
    result.transitionAccepted = canTransition(current, next);
    result.executable = false;

    if (!result.transitionAccepted) {
        result.status = ExecutionLifecycleStatus::Rejected;
        result.summary =
            QStringLiteral("Execution lifecycle transition rejected: %1 to %2.")
                .arg(executionLifecycleStateName(current), executionLifecycleStateName(next));
        result.traces = {trace(1, current, ExecutionTraceLevel::Blocked, result.summary)};
        return result;
    }

    result.status = next == ExecutionLifecycleState::Blocked ? ExecutionLifecycleStatus::Blocked
                    : next == ExecutionLifecycleState::ReadyPlaceholder
                        ? ExecutionLifecycleStatus::ReadyPlaceholder
                        : ExecutionLifecycleStatus::MetadataOnly;
    result.summary =
        QStringLiteral("Execution lifecycle transition accepted: %1 to %2.")
            .arg(executionLifecycleStateName(current), executionLifecycleStateName(next));
    result.traces = {trace(1, next, ExecutionTraceLevel::Info, result.summary)};
    return result;
}

bool StaticExecutionLifecycle::canTransition(ExecutionLifecycleState current,
                                             ExecutionLifecycleState next) {
    switch (current) {
    case ExecutionLifecycleState::NotRequested:
        return next == ExecutionLifecycleState::Requested;
    case ExecutionLifecycleState::Requested:
        return next == ExecutionLifecycleState::Validating;
    case ExecutionLifecycleState::Validating:
        return next == ExecutionLifecycleState::PermissionCheck;
    case ExecutionLifecycleState::PermissionCheck:
        return next == ExecutionLifecycleState::SafetyCheck;
    case ExecutionLifecycleState::SafetyCheck:
        return next == ExecutionLifecycleState::Coordination;
    case ExecutionLifecycleState::Coordination:
        return next == ExecutionLifecycleState::ReadyPlaceholder ||
               next == ExecutionLifecycleState::Blocked;
    case ExecutionLifecycleState::ReadyPlaceholder:
        return next == ExecutionLifecycleState::Blocked;
    case ExecutionLifecycleState::Blocked:
        return false;
    }

    return false;
}

ExecutionCoordinator::ExecutionCoordinator(ExecutionSession session)
    : session_(std::move(session)) {}

ExecutionCoordinationSnapshot
ExecutionCoordinator::coordinate(const ExecutionLifecycleResult& lifecycle) const {
    ExecutionCoordinationSnapshot snapshot;
    snapshot.session = session_;
    snapshot.lifecycle = lifecycle;
    snapshot.readOnly = true;
    snapshot.executable = false;
    snapshot.summary = QStringLiteral("Execution coordination snapshot is read-only for %1; "
                                      "lifecycle is %2 and execution is blocked.")
                           .arg(session_.id.value, executionLifecycleStatusName(lifecycle.status));
    return snapshot;
}

const ExecutionSession& ExecutionCoordinator::session() const {
    return session_;
}

} // namespace sentinel::core
