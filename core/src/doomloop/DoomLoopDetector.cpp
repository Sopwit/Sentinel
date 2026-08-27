// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/doomloop/DoomLoopDetector.h"

namespace sentinel::core {

DoomLoopDetector::DoomLoopDetector(const Config& config) : m_config(config) {}

DoomLoopIndicator DoomLoopDetector::detect(const QJsonObject& context) const {
    Q_UNUSED(context)
    DoomLoopIndicator indicator;
    return indicator;
}

bool DoomLoopDetector::isStuck(const QString& sessionId) const {
    auto it = m_actionHistory.find(sessionId);
    if (it == m_actionHistory.end())
        return false;
    return hasRepeatedPattern(*it);
}

void DoomLoopDetector::recordAction(const QString& sessionId, const QString& action) {
    m_actionHistory[sessionId].append(action);
    if (m_actionHistory[sessionId].size() > m_config.windowSize * 2) {
        m_actionHistory[sessionId].removeFirst();
    }
}

void DoomLoopDetector::reset(const QString& sessionId) {
    m_actionHistory.remove(sessionId);
    m_errorHistory.remove(sessionId);
}

QList<DoomLoopIndicator> DoomLoopDetector::analyzeHistory(const QString& sessionId) const {
    QList<DoomLoopIndicator> indicators;
    auto it = m_actionHistory.find(sessionId);
    if (it == m_actionHistory.end())
        return indicators;

    if (hasRepeatedPattern(*it)) {
        indicators.append({"repetition", "Repeated action pattern detected", 80});
    }

    auto eit = m_errorHistory.find(sessionId);
    if (eit != m_errorHistory.end() && hasErrorLoop(*eit)) {
        indicators.append({"error_loop", "Repeated error pattern detected", 70});
    }

    return indicators;
}

bool DoomLoopDetector::hasRepeatedPattern(const QStringList& actions) const {
    if (actions.size() < m_config.maxRepetitions * 2)
        return false;

    for (int len = 1; len <= actions.size() / 2; ++len) {
        int count = 0;
        for (int i = 0; i + len <= actions.size(); ++i) {
            if (i + len < actions.size() && actions[i] == actions[i + len]) {
                count++;
            }
        }
        if (count >= m_config.maxRepetitions)
            return true;
    }
    return false;
}

bool DoomLoopDetector::hasSelfModification(const QString& action) const {
    return action.contains("edit") || action.contains("write") || action.contains("modify");
}

bool DoomLoopDetector::hasErrorLoop(const QStringList& errors) const {
    if (errors.size() < m_config.maxRepetitions)
        return false;
    QString last = errors.last();
    int count = 0;
    for (const auto& e : errors) {
        if (e == last)
            count++;
    }
    return count >= m_config.maxRepetitions;
}

} // namespace sentinel::core
