// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace sentinel::core {

enum class ProviderErrorCategory : std::uint8_t {
    Unknown,
    Authentication,
    RateLimit,
    ServerError,
    NetworkError,
    Timeout,
    InvalidRequest,
    ContentFilter,
    QuotaExceeded
};

struct ProviderError {
    QString message;
    ProviderErrorCategory category{ProviderErrorCategory::Unknown};
    int httpStatus{0};
    QString provider;
    bool retryable{false};
};

class ProviderErrorClassifier {
public:
    static ProviderError classify(int httpStatus, const QString& message, const QString& provider = {});
    static bool isRetryable(const ProviderError& error);
    static QString userMessage(const ProviderError& error);
    static ProviderErrorCategory categoryFromHttpStatus(int status);
};

} // namespace sentinel::core
