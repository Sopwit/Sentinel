// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/ControlledAgentTasks.h"
#include "sentinel/core/agent/IAgentRuntime.h"
#include "sentinel/core/model/ModelService.h"

#include <QHash>
#include <QObject>
#include <functional>

namespace sentinel::core {
class AppSettings;

// Application ownership for controlled-task records and their live runtime sessions.
// AgentRuntime remains the only execution engine; this class projects its events.
class ControlledTaskService final : public QObject {
    Q_OBJECT
public:
    using BindResolvedModel = std::function<bool(const ModelBindingResolution&)>;
    using InteractiveBusy = std::function<bool()>;

    ControlledTaskService(AppSettings& settings, IAgentRuntime& runtime, ModelService& modelService,
                          BindResolvedModel bindModel, InteractiveBusy interactiveBusy,
                          QObject* parent = nullptr);
    ~ControlledTaskService() override;

    const QList<ControlledAgentTask>& tasks() const {
        return tasks_;
    }
    const QList<ControlledWorkspacePermission>& permissions() const {
        return permissions_;
    }
    ControlledAgentTask task(const QString& taskId) const;
    QString createTask(const QString& goal, const QString& workspaceId,
                       const QStringList& resources);
    bool editPlan(const QString& taskId, const QStringList& steps);
    bool approveTask(const QString& taskId, const QString& choice);
    bool denyTask(const QString& taskId);
    bool startTask(const QString& taskId);
    bool cancelTask(const QString& taskId);
    bool skipTask(const QString& taskId);
    bool retryTask(const QString& taskId);
    bool reorderTask(const QString& taskId, int index);
    bool setPermission(const QString& workspaceId, const QString& category, const QString& choice);
    bool sessionActive() const {
        return !sessionByTask_.isEmpty();
    }
    const ControlledAgentTaskService& presentation() const {
        return domain_;
    }

signals:
    void tasksChanged();

private:
    void onAgentEvent(const AgentEvent& event);
    void saveTasks();
    void savePermissions();
    void upsert(ControlledAgentTask task);

    AppSettings& settings_;
    IAgentRuntime& runtime_;
    ModelService& modelService_;
    BindResolvedModel bindModel_;
    InteractiveBusy interactiveBusy_;
    ControlledAgentTaskService domain_;
    QList<ControlledAgentTask> tasks_;
    QList<ControlledWorkspacePermission> permissions_;
    QHash<QString, QString> sessionByTask_;
    QHash<QString, QString> taskBySession_;
    QString subscriptionId_;
};
} // namespace sentinel::core
