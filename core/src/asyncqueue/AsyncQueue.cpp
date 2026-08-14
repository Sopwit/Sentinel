// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/asyncqueue/AsyncQueue.h"
#include <QtConcurrent>

namespace sentinel::core {

AsyncQueue::AsyncQueue(int maxConcurrent, QObject* parent)
    : QObject(parent), m_maxConcurrent(maxConcurrent) {}

AsyncQueue::~AsyncQueue() = default;

void AsyncQueue::enqueue(AsyncTask task) {
    QMutexLocker locker(&m_mutex);
    m_queue.enqueue(task);
    processNext();
}

void AsyncQueue::setMaxConcurrent(int max) { m_maxConcurrent = max; }

int AsyncQueue::pendingCount() const {
    QMutexLocker locker(&m_mutex);
    return m_queue.size();
}

int AsyncQueue::runningCount() const {
    QMutexLocker locker(&m_mutex);
    return m_running;
}

void AsyncQueue::clear() {
    QMutexLocker locker(&m_mutex);
    m_queue.clear();
}

void AsyncQueue::processNext() {
    if (m_running >= m_maxConcurrent || m_queue.isEmpty()) {
        if (m_queue.isEmpty() && m_running == 0) emit queueEmpty();
        return;
    }

    m_running++;
    AsyncTask task = m_queue.dequeue();
    emit taskStarted();

    QtConcurrent::run([this, task]() {
        try { task(); } catch (...) {}
        QMutexLocker locker(&m_mutex);
        m_running--;
        emit taskCompleted();
        processNext();
    });
}

} // namespace sentinel::core
