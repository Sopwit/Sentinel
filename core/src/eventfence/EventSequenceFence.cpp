// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/eventfence/EventSequenceFence.h"

namespace sentinel::core {

quint64 EventSequenceFence::nextSequence(const QString& channel) {
    quint64 seq = ++m_state.sequenceByChannel[channel];
    m_state.aggregateSequence = seq;
    return seq;
}

quint64 EventSequenceFence::currentSequence(const QString& channel) const {
    return m_state.sequenceByChannel.value(channel, 0);
}

FenceState EventSequenceFence::state() const {
    return m_state;
}

bool EventSequenceFence::isReady(const QString& channel, quint64 expectedSequence) const {
    return currentSequence(channel) >= expectedSequence;
}

void EventSequenceFence::reset() {
    m_state = FenceState{};
}

} // namespace sentinel::core
