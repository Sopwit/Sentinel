// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

struct TransientRetryConfig {
    int maxRetries{3};
    int initialDelayMs{500};
    int maxDelayMs{10000};
    double backoffMultiplier{2.0};
    bool retryOnTimeout{true};
    bool retryOnNetworkError{true};
};

class TransientRetryPolicy {
public:
    explicit TransientRetryPolicy(const TransientRetryConfig& config = {});

    bool shouldRetry(int attempt, int httpStatus, const QString& error = {}) const;
    int delayMs(int attempt) const;
    QString describeRetry(int attempt, int httpStatus) const;

private:
    TransientRetryConfig m_config;
};

} // namespace sentinel::core
