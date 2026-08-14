// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/provider/ProviderErrorClassifier.h"

namespace sentinel::core {

ProviderError ProviderErrorClassifier::classify(int httpStatus, const QString& message, const QString& provider) {
    ProviderError error;
    error.httpStatus = httpStatus;
    error.message = message;
    error.provider = provider;
    error.category = categoryFromHttpStatus(httpStatus);
    const QString lowerMessage = message.toLower();
    if (lowerMessage.contains(QStringLiteral("context length")) ||
        lowerMessage.contains(QStringLiteral("context window")) ||
        lowerMessage.contains(QStringLiteral("maximum context"))) {
        error.category = ProviderErrorCategory::InvalidRequest;
    } else if (lowerMessage.contains(QStringLiteral("quota")) ||
               lowerMessage.contains(QStringLiteral("billing"))) {
        error.category = ProviderErrorCategory::QuotaExceeded;
    } else if (lowerMessage.contains(QStringLiteral("content filter")) ||
               lowerMessage.contains(QStringLiteral("safety system"))) {
        error.category = ProviderErrorCategory::ContentFilter;
    } else if (httpStatus == 0 &&
               (lowerMessage.contains(QStringLiteral("timeout")) ||
                lowerMessage.contains(QStringLiteral("timed out")))) {
        error.category = ProviderErrorCategory::Timeout;
    } else if (httpStatus == 0 &&
               (lowerMessage.contains(QStringLiteral("network")) ||
                lowerMessage.contains(QStringLiteral("connection")) ||
                lowerMessage.contains(QStringLiteral("dns")))) {
        error.category = ProviderErrorCategory::NetworkError;
    }
    error.retryable = isRetryable(error);
    return error;
}

bool ProviderErrorClassifier::isRetryable(const ProviderError& error) {
    return error.category == ProviderErrorCategory::RateLimit ||
           error.category == ProviderErrorCategory::ServerError ||
           error.category == ProviderErrorCategory::NetworkError ||
           error.category == ProviderErrorCategory::Timeout;
}

QString ProviderErrorClassifier::userMessage(const ProviderError& error) {
    switch (error.category) {
        case ProviderErrorCategory::Authentication:
            return "Authentication failed. Please check your API key.";
        case ProviderErrorCategory::RateLimit:
            return "Rate limit exceeded. Please wait before retrying.";
        case ProviderErrorCategory::ServerError:
            return "Server error. Please try again later.";
        case ProviderErrorCategory::NetworkError:
            return "Network error. Please check your connection.";
        case ProviderErrorCategory::Timeout:
            return "Request timed out. Please try again.";
        case ProviderErrorCategory::InvalidRequest:
            return "Invalid request. Please check your input.";
        case ProviderErrorCategory::ContentFilter:
            return "Content was filtered by the provider.";
        case ProviderErrorCategory::QuotaExceeded:
            return "Quota exceeded. Please check your plan.";
        default:
            return QStringLiteral("Error: %1").arg(error.message);
    }
}

ProviderErrorCategory ProviderErrorClassifier::categoryFromHttpStatus(int status) {
    if (status == 401 || status == 403) return ProviderErrorCategory::Authentication;
    if (status == 429) return ProviderErrorCategory::RateLimit;
    if (status >= 500 && status < 600) return ProviderErrorCategory::ServerError;
    if (status == 408) return ProviderErrorCategory::Timeout;
    if (status >= 400 && status < 500) return ProviderErrorCategory::InvalidRequest;
    return ProviderErrorCategory::Unknown;
}

} // namespace sentinel::core
