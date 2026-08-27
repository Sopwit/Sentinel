// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QString>
#include <optional>

namespace sentinel::core {

struct RetryConfig {
    int maxRetries{3};
    int initialDelayMs{1000};
    int maxDelayMs{30000};
    double backoffMultiplier{2.0};
    double jitterRange{0.3};
    bool retryOn429{true};
    bool retryOn5xx{true};
    bool retryOnNetworkError{true};
};

class RetryPolicy {
public:
    explicit RetryPolicy(const RetryConfig& config = {});

    bool shouldRetry(int attempt, int statusCode, const QString& errorString = {}) const;
    int delayMs(int attempt, const QString& retryAfterHeader = {}) const;
    QString retryReason(int attempt, int statusCode, const QString& errorString = {}) const;

private:
    int parseRetryAfter(const QString& header) const;
    bool isRetryableStatus(int statusCode) const;
    bool isRetryableError(const QString& error) const;

    RetryConfig m_config;
};

} // namespace sentinel::core
