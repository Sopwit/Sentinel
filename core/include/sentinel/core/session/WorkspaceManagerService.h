// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/session/IWorkspaceManagerService.h"
#include <QList>
#include <functional>

namespace sentinel::core {

class WorkspaceManagerService : public IWorkspaceManagerService {
public:
    explicit WorkspaceManagerService();
    ~WorkspaceManagerService() override;

    Workspace create(const QString& name, const QString& path = {}) override;
    bool remove(const QString& name) override;
    bool switchTo(const QString& name) override;
    QList<Workspace> list() const override;
    std::optional<Workspace> current() const override;
    std::optional<Workspace> find(const QString& name) const override;
    bool addProject(const QString& workspaceName, const QString& projectPath) override;

    void onWorkspaceChanged(std::function<void(const QString&)> callback);

private:
    QList<Workspace> m_workspaces;
    QString m_currentName;
    std::function<void(const QString&)> m_changedCallback;
};

} // namespace sentinel::core
