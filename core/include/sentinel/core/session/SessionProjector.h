#pragma once

#include "sentinel/core/event/DurableEventLog.h"
#include <QHash>

namespace sentinel::core {

class SessionProjector final {
public:
    void apply(const DurableEvent& event) {
        lastSequence_[event.aggregateId] = event.sequence;
        if (event.type == QStringLiteral("message.created")) ++messageCount_[event.aggregateId];
    }
    qint64 lastSequence(const QString& aggregateId) const { return lastSequence_.value(aggregateId); }
    int messageCount(const QString& aggregateId) const { return messageCount_.value(aggregateId); }

private:
    QHash<QString, qint64> lastSequence_;
    QHash<QString, int> messageCount_;
};

} // namespace sentinel::core
