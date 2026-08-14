// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <functional>

namespace sentinel::core {

using AsyncTask = std::function<void()>;

class AsyncQueue : public QObject {
    Q_OBJECT
public:
    explicit AsyncQueue(int maxConcurrent = 4, QObject* parent = nullptr);
    ~AsyncQueue() override;

    void enqueue(AsyncTask task);
    void setMaxConcurrent(int max);
    int pendingCount() const;
    int runningCount() const;
    void clear();

signals:
    void taskStarted();
    void taskCompleted();
    void queueEmpty();

private slots:
    void processNext();

private:
    int m_maxConcurrent;
    QQueue<AsyncTask> m_queue;
    int m_running{0};
    mutable QMutex m_mutex;
};

} // namespace sentinel::core
