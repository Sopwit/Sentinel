// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/ISubagentService.h"
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QThreadPool>

namespace sentinel::core {

class SubagentService : public QObject, public ISubagentService {
    Q_OBJECT
public:
    explicit SubagentService(QObject* parent = nullptr);
    ~SubagentService() override;

    QString submitTask(const TaskRequest& request, TaskFunction func) override;
    bool cancelTask(const QString& taskId) override;
    std::optional<TaskResult> findResult(const QString& taskId) override;
    bool isTaskComplete(const QString& taskId) const override;
    void setMaxDepth(int depth) override;
    int maxDepth() const override;

signals:
    void taskSubmitted(const QString& taskId);
    void taskCompleted(const QString& taskId, bool success);
    void taskFailed(const QString& taskId, const QString& error);

private:
    QString generateTaskId() const;

    int m_maxDepth{3};
    QThreadPool m_threadPool;
    QMap<QString, TaskRequest> m_requests;
    QMap<QString, TaskResult> m_results;
    mutable QMutex m_mutex;
};

} // namespace sentinel::core
