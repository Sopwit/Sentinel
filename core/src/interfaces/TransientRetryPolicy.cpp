// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/interfaces/TransientRetryPolicy.h"
#include <QtMath>
#include <cstdlib>

namespace sentinel::core {

TransientRetryPolicy::TransientRetryPolicy(const TransientRetryConfig& config) : m_config(config) {}

bool TransientRetryPolicy::shouldRetry(int attempt, int httpStatus, const QString& error) const {
    if (attempt >= m_config.maxRetries)
        return false;
    if (httpStatus == 429 || (httpStatus >= 500 && httpStatus < 600))
        return true;
    if (m_config.retryOnTimeout && error.contains("timeout"))
        return true;
    if (m_config.retryOnNetworkError && error.contains("network"))
        return true;
    return false;
}

int TransientRetryPolicy::delayMs(int attempt) const {
    double base = m_config.initialDelayMs * qPow(m_config.backoffMultiplier, attempt);
    base = qMin(base, static_cast<double>(m_config.maxDelayMs));
    double jitter = base * 0.3 * ((std::rand() % 2000 - 1000) / 1000.0);
    return static_cast<int>(base + jitter);
}

QString TransientRetryPolicy::describeRetry(int attempt, int httpStatus) const {
    return QStringLiteral("Retry %1/%2 after HTTP %3")
        .arg(attempt + 1)
        .arg(m_config.maxRetries)
        .arg(httpStatus);
}

} // namespace sentinel::core
