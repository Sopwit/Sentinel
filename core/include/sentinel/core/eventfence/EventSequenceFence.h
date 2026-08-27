// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

struct FenceState {
    quint64 aggregateSequence{0};
    QMap<QString, quint64> sequenceByChannel;
};

class EventSequenceFence {
public:
    quint64 nextSequence(const QString& channel);
    quint64 currentSequence(const QString& channel) const;
    FenceState state() const;
    bool isReady(const QString& channel, quint64 expectedSequence) const;
    void reset();

private:
    FenceState m_state;
};

} // namespace sentinel::core
