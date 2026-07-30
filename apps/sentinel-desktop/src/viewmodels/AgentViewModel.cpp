// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sentinel/desktop/viewmodels/AgentViewModel.h>

namespace sentinel::desktop::viewmodels {

AgentViewModel::AgentViewModel(QObject* parent)
    : QObject(parent) {
}

void AgentViewModel::setIsAgentRunning(bool running) {
    if (m_isAgentRunning != running) {
        m_isAgentRunning = running;
        Q_EMIT isAgentRunningChanged();
    }
}

void AgentViewModel::setCurrentTaskName(const QString& name) {
    if (m_currentTaskName != name) {
        m_currentTaskName = name;
        Q_EMIT currentTaskNameChanged();
    }
}

void AgentViewModel::cancelCurrentTask() {
    if (m_isAgentRunning) {
        setIsAgentRunning(false);
        Q_EMIT taskCompleted(m_currentTaskName, false);
        setCurrentTaskName(QString());
    }
}

} // namespace sentinel::desktop::viewmodels
