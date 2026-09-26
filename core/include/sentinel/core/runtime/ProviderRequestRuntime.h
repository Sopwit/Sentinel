// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/interfaces/IChatProvider.h"

#include <QNetworkReply>
#include <QString>

#include <atomic>
#include <functional>
#include <memory>

namespace sentinel::core {

enum class ProviderRequestMode { Generation, StreamingGeneration, Discovery, HealthProbe };

struct ProviderTransportResult {
    ChatRequestLifecycle lifecycle = ChatRequestLifecycle::Pending;
    ChatProviderErrorCategory category = ChatProviderErrorCategory::None;
    QNetworkReply::NetworkError networkError = QNetworkReply::NoError;
    int httpStatus = 0;
    QString retryAfter;
    bool cancelled = false;
    bool timedOut = false;
};

class ProviderRequestRuntime {
public:
    static QString requestId();
    static ProviderTransportResult wait(
        QNetworkReply* reply, int timeoutMs,
        const std::shared_ptr<std::atomic_bool>& cancellationToken = {},
        const std::function<void()>& onReadyRead = {});
    static ChatProviderErrorCategory classify(int httpStatus,
                                               QNetworkReply::NetworkError error,
                                               bool timedOut = false,
                                               bool cancelled = false);
    static bool retryable(const ProviderTransportResult& result, ProviderRequestMode mode,
                          bool outputEmitted);
    static int retryDelayMs(int attempt, const QString& retryAfter, ProviderRequestMode mode);
    static bool backoff(int delayMs,
                        const std::shared_ptr<std::atomic_bool>& cancellationToken = {});
};

} // namespace sentinel::core
