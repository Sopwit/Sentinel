// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <atomic>
#include <functional>
#include <memory>

namespace sentinel::core {

struct TaskRequest {
    QString taskId;
    QString prompt;
    QString description;
    bool background{false};
    QString parentTaskId;
    int depth{0};
    int maxDepth{3};
    std::shared_ptr<std::atomic_bool> cancellationToken = std::make_shared<std::atomic_bool>(false);

    bool isCancellationRequested() const {
        return cancellationToken && cancellationToken->load();
    }
};

struct TaskResult {
    QString taskId;
    bool success{false};
    QString output;
    QString error;
};

using TaskFunction = std::function<TaskResult(const TaskRequest& request,
                                              std::function<void(QString)> progressCallback)>;

class ISubagentService {
public:
    virtual ~ISubagentService() = default;

    virtual QString submitTask(const TaskRequest& request, TaskFunction func) = 0;
    virtual bool cancelTask(const QString& taskId) = 0;
    virtual std::optional<TaskResult> findResult(const QString& taskId) = 0;
    virtual bool isTaskComplete(const QString& taskId) const = 0;
    virtual void setMaxDepth(int depth) = 0;
    virtual int maxDepth() const = 0;
};

} // namespace sentinel::core
