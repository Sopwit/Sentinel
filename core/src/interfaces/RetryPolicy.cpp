// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/interfaces/IRetryPolicy.h"
#include <QDateTime>
#include <QtMath>
#include <cstdlib>

namespace sentinel::core {

RetryPolicy::RetryPolicy(const RetryConfig& config) : m_config(config) {}

bool RetryPolicy::shouldRetry(int attempt, int statusCode, const QString& errorString) const {
    if (attempt >= m_config.maxRetries)
        return false;
    if (isRetryableStatus(statusCode))
        return true;
    if (isRetryableError(errorString))
        return true;
    return false;
}

int RetryPolicy::delayMs(int attempt, const QString& retryAfterHeader) const {
    if (!retryAfterHeader.isEmpty()) {
        int parsed = parseRetryAfter(retryAfterHeader);
        if (parsed > 0)
            return parsed;
    }

    double baseDelay = m_config.initialDelayMs * qPow(m_config.backoffMultiplier, attempt);
    baseDelay = qMin(baseDelay, static_cast<double>(m_config.maxDelayMs));

    double jitter = baseDelay * m_config.jitterRange * ((std::rand() % 2000 - 1000) / 1000.0);
    return static_cast<int>(baseDelay + jitter);
}

QString RetryPolicy::retryReason(int attempt, int statusCode, const QString& errorString) const {
    if (statusCode == 429)
        return "Rate limited (429)";
    if (statusCode >= 500 && statusCode < 600)
        return QStringLiteral("Server error (%1)").arg(statusCode);
    if (errorString.contains("timeout"))
        return "Request timed out";
    if (errorString.contains("network"))
        return "Network error";
    return QStringLiteral("Attempt %1 of %2").arg(attempt + 1).arg(m_config.maxRetries);
}

int RetryPolicy::parseRetryAfter(const QString& header) const {
    bool ok;
    int seconds = header.toInt(&ok);
    if (ok && seconds > 0)
        return seconds * 1000;

    QDateTime date = QDateTime::fromString(header, Qt::RFC2822Date);
    if (date.isValid()) {
        qint64 delayMs = date.toMSecsSinceEpoch() - QDateTime::currentMSecsSinceEpoch();
        if (delayMs > 0)
            return static_cast<int>(delayMs);
    }
    return 0;
}

bool RetryPolicy::isRetryableStatus(int statusCode) const {
    if (m_config.retryOn429 && statusCode == 429)
        return true;
    if (m_config.retryOn5xx && statusCode >= 500 && statusCode < 600)
        return true;
    return false;
}

bool RetryPolicy::isRetryableError(const QString& error) const {
    if (!m_config.retryOnNetworkError)
        return false;
    return error.contains("timeout") || error.contains("network") || error.contains("connection") ||
           error.contains("overload");
}

} // namespace sentinel::core
