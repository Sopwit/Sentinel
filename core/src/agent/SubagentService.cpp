// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/SubagentService.h"
#include <QtConcurrent>
#include <QUuid>

namespace sentinel::core {

SubagentService::SubagentService(QObject* parent)
    : QObject(parent)
{
    m_threadPool.setMaxThreadCount(4);
}

SubagentService::~SubagentService() {
    m_threadPool.waitForDone();
}

QString SubagentService::submitTask(const TaskRequest& request, TaskFunction func) {
    QMutexLocker locker(&m_mutex);

    TaskRequest req = request;
    if (req.taskId.isEmpty()) {
        req.taskId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    }

    m_requests[req.taskId] = req;

    emit taskSubmitted(req.taskId);

    QtConcurrent::run(&m_threadPool, [this, req, func]() {
        auto progressCb = [](QString) {};

        TaskResult result;
        try {
            result = func(req, progressCb);
        } catch (const std::exception& e) {
            result.success = false;
            result.error = e.what();
        }

        QMutexLocker locker(&m_mutex);
        m_results[req.taskId] = result;

        emit taskCompleted(req.taskId, result.success);
    });

    return req.taskId;
}

bool SubagentService::cancelTask(const QString& taskId) {
    QMutexLocker locker(&m_mutex);
    auto it = m_requests.find(taskId);
    if (it == m_requests.end()) return false;
    if (it->cancellationToken) {
        it->cancellationToken->store(true);
    }
    return true;
}

std::optional<TaskResult> SubagentService::findResult(const QString& taskId) {
    QMutexLocker locker(&m_mutex);
    auto it = m_results.find(taskId);
    if (it == m_results.end()) return std::nullopt;
    return it.value();
}

bool SubagentService::isTaskComplete(const QString& taskId) const {
    QMutexLocker locker(&m_mutex);
    return m_results.contains(taskId);
}

void SubagentService::setMaxDepth(int depth) { m_maxDepth = depth; }
int SubagentService::maxDepth() const { return m_maxDepth; }

QString SubagentService::generateTaskId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

} // namespace sentinel::core
