// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QString>
#include <atomic>
#include <functional>
#include <memory>

namespace sentinel::core {

enum class JobState : std::uint8_t { Queued, Running, Completed, Failed, Cancelled };

struct JobProgress {
    int current{0};
    int total{0};
    QString status;
    double percentage() const {
        return total > 0 ? (current * 100.0 / total) : 0.0;
    }
};

struct Job {
    QString id;
    QString name;
    QString description;
    JobState state{JobState::Queued};
    JobProgress progress;
    QDateTime createdAt;
    QDateTime startedAt;
    QDateTime completedAt;
    QString errorString;
    int retryCount{0};
    int maxRetries{3};
    std::shared_ptr<std::atomic_bool> cancellationToken = std::make_shared<std::atomic_bool>(false);

    bool isCancellationRequested() const {
        return cancellationToken && cancellationToken->load();
    }
};

using JobFunction =
    std::function<bool(Job& job, std::function<void(JobProgress)> progressCallback)>;

class IJobService {
public:
    virtual ~IJobService() = default;

    // Job management
    virtual QString submitJob(const QString& name, JobFunction func,
                              const QString& description = {}) = 0;
    virtual bool cancelJob(const QString& jobId) = 0;
    virtual bool retryJob(const QString& jobId) = 0;
    virtual bool removeJob(const QString& jobId) = 0;

    // Query
    virtual std::optional<Job> findJob(const QString& jobId) const = 0;
    virtual QList<Job> jobs() const = 0;
    virtual QList<Job> activeJobs() const = 0;
    virtual QList<Job> completedJobs() const = 0;
    virtual QList<Job> failedJobs() const = 0;

    // Configuration
    virtual void setMaxConcurrentJobs(int maxJobs) = 0;
    virtual int maxConcurrentJobs() const = 0;
};

} // namespace sentinel::core
