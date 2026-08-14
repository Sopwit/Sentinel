// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/WorkspaceManagerService.h"

namespace sentinel::core {

WorkspaceManagerService::WorkspaceManagerService() = default;
WorkspaceManagerService::~WorkspaceManagerService() = default;

Workspace WorkspaceManagerService::create(const QString& name, const QString& path) {
    Workspace ws;
    ws.name = name;
    ws.path = path;
    ws.isActive = true;
    m_workspaces.append(ws);
    m_currentName = name;
    if (m_changedCallback) m_changedCallback(name);
    return ws;
}

bool WorkspaceManagerService::remove(const QString& name) {
    for (int i = 0; i < m_workspaces.size(); ++i) {
        if (m_workspaces[i].name == name) {
            m_workspaces.removeAt(i);
            return true;
        }
    }
    return false;
}

bool WorkspaceManagerService::switchTo(const QString& name) {
    for (auto& ws : m_workspaces) {
        ws.isActive = (ws.name == name);
    }
    m_currentName = name;
    if (m_changedCallback) m_changedCallback(name);
    return true;
}

QList<Workspace> WorkspaceManagerService::list() const { return m_workspaces; }

std::optional<Workspace> WorkspaceManagerService::current() const {
    return find(m_currentName);
}

std::optional<Workspace> WorkspaceManagerService::find(const QString& name) const {
    for (const auto& ws : m_workspaces) {
        if (ws.name == name) return ws;
    }
    return std::nullopt;
}

bool WorkspaceManagerService::addProject(const QString& workspaceName, const QString& projectPath) {
    for (auto& ws : m_workspaces) {
        if (ws.name == workspaceName) {
            if (!ws.projectPaths.contains(projectPath)) {
                ws.projectPaths.append(projectPath);
            }
            return true;
        }
    }
    return false;
}

void WorkspaceManagerService::onWorkspaceChanged(std::function<void(const QString&)> callback) {
    m_changedCallback = callback;
}

} // namespace sentinel::core
