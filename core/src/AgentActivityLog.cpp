#include "sentinel/core/AgentActivityLog.h"

namespace sentinel::core {

const QList<AgentActivityEntry>& AgentActivityLog::entries() const {
    return entries_;
}

AgentActivityEntry AgentActivityLog::append(AgentActivityType type, AgentActivityStatus status,
                                            const QString& summary) {
    AgentActivityEntry entry{
        nextSequenceId_,
        type,
        status,
        summary,
    };
    nextSequenceId_ += 1;
    entries_.append(entry);
    return entry;
}

void AgentActivityLog::clear() {
    entries_.clear();
    nextSequenceId_ = 1;
}

int AgentActivityLog::count() const {
    return static_cast<int>(entries_.size());
}

QString AgentActivityLog::latestSummary() const {
    return entries_.isEmpty() ? QStringLiteral("No agent activity yet.") : entries_.last().summary;
}

} // namespace sentinel::core
