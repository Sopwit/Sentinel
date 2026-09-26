// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/ProviderRequestRuntime.h"
#include "sentinel/core/interfaces/IRetryPolicy.h"

#include <QEventLoop>
#include <QNetworkRequest>
#include <QTimer>
#include <QUuid>

namespace sentinel::core {

QString ProviderRequestRuntime::requestId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

ChatProviderErrorCategory ProviderRequestRuntime::classify(
    int status, QNetworkReply::NetworkError error, bool timedOut, bool cancelled) {
    if (cancelled) return ChatProviderErrorCategory::Cancelled;
    if (timedOut || error == QNetworkReply::TimeoutError || status == 408)
        return ChatProviderErrorCategory::Timeout;
    if (status == 429) return ChatProviderErrorCategory::RateLimited;
    if (status == 401 || status == 403)
        return ChatProviderErrorCategory::AuthenticationRequired;
    if (status == 404) return ChatProviderErrorCategory::ModelNotFound;
    if (status == 400 || status == 422)
        return ChatProviderErrorCategory::RequestRejected;
    if (status == 501) return ChatProviderErrorCategory::CapabilityUnsupported;
    if (error == QNetworkReply::UnknownContentError)
        return ChatProviderErrorCategory::MalformedResponse;
    if (status >= 500) return ChatProviderErrorCategory::ProviderUnavailable;
    if (error != QNetworkReply::NoError)
        return ChatProviderErrorCategory::ConnectionFailed;
    return status >= 400 ? ChatProviderErrorCategory::RequestRejected
                         : ChatProviderErrorCategory::None;
}

ProviderTransportResult ProviderRequestRuntime::wait(
    QNetworkReply* reply, int timeoutMs,
    const std::shared_ptr<std::atomic_bool>& cancellationToken,
    const std::function<void()>& onReadyRead) {
    ProviderTransportResult result;
    if (cancellationToken && cancellationToken->load()) reply->abort();
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    bool deadlineExpired = false;
    QTimer cancellationTimer;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&deadline, &QTimer::timeout, &loop, [&] {
        deadlineExpired = true;
        reply->abort();
        loop.quit();
    });
    if (onReadyRead)
        QObject::connect(reply, &QNetworkReply::readyRead, &loop, onReadyRead);
    if (cancellationToken) {
        cancellationTimer.setInterval(25);
        QObject::connect(&cancellationTimer, &QTimer::timeout, &loop, [&] {
            if (cancellationToken->load()) {
                reply->abort();
                loop.quit();
            }
        });
        cancellationTimer.start();
    }
    deadline.start(timeoutMs > 0 ? timeoutMs : 30000);
    if (!reply->isFinished() && !(cancellationToken && cancellationToken->load())) loop.exec();
    cancellationTimer.stop();
    result.cancelled = cancellationToken && cancellationToken->load();
    result.timedOut = !result.cancelled && deadlineExpired;
    if (result.timedOut || result.cancelled) reply->abort();
    result.httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.networkError = reply->error();
    result.retryAfter = QString::fromLatin1(reply->rawHeader("Retry-After"));
    result.category = classify(result.httpStatus, result.networkError,
                               result.timedOut, result.cancelled);
    result.lifecycle = result.cancelled ? ChatRequestLifecycle::Cancelled
                       : result.timedOut ? ChatRequestLifecycle::TimedOut
                       : result.category == ChatProviderErrorCategory::RateLimited
                           ? ChatRequestLifecycle::RateLimited
                       : result.category == ChatProviderErrorCategory::None
                           ? ChatRequestLifecycle::Completed
                           : ChatRequestLifecycle::Failed;
    return result;
}

bool ProviderRequestRuntime::retryable(const ProviderTransportResult& result,
                                        ProviderRequestMode mode, bool outputEmitted) {
    if (result.cancelled || result.timedOut || outputEmitted) return false;
    const bool transientHttp = result.httpStatus == 429 || result.httpStatus == 502 ||
                               result.httpStatus == 503 || result.httpStatus == 504;
    const bool transientNetwork = result.httpStatus == 0 &&
        (result.networkError == QNetworkReply::TemporaryNetworkFailureError ||
         result.networkError == QNetworkReply::NetworkSessionFailedError ||
         result.networkError == QNetworkReply::ConnectionRefusedError ||
         result.networkError == QNetworkReply::HostNotFoundError);
    return (transientHttp || transientNetwork) &&
           (mode != ProviderRequestMode::HealthProbe || result.httpStatus != 429);
}

int ProviderRequestRuntime::retryDelayMs(int attempt, const QString& retryAfter,
                                          ProviderRequestMode mode) {
    RetryConfig config;
    config.maxRetries = mode == ProviderRequestMode::Discovery ||
                                mode == ProviderRequestMode::HealthProbe ? 1 : 2;
    config.initialDelayMs = 250;
    config.maxDelayMs = mode == ProviderRequestMode::Discovery ||
                                mode == ProviderRequestMode::HealthProbe ? 500 : 1500;
    return RetryPolicy(config).delayMs(attempt, retryAfter);
}

bool ProviderRequestRuntime::backoff(
    int delayMs, const std::shared_ptr<std::atomic_bool>& cancellationToken) {
    if (cancellationToken && cancellationToken->load()) return false;
    QEventLoop loop;
    QTimer::singleShot(qMax(0, delayMs), &loop, &QEventLoop::quit);
    QTimer cancellationTimer;
    if (cancellationToken) {
        cancellationTimer.setInterval(25);
        QObject::connect(&cancellationTimer, &QTimer::timeout, &loop, [&] {
            if (cancellationToken->load()) loop.quit();
        });
        cancellationTimer.start();
    }
    loop.exec();
    return !cancellationToken || !cancellationToken->load();
}

} // namespace sentinel::core
