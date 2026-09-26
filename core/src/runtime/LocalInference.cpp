// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/LocalInference.h"
#include "sentinel/core/runtime/ProviderRequestRuntime.h"
#include "sentinel/core/interfaces/IChatProvider.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QTimer>
#include <QVariant>

#include <atomic>
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

namespace sentinel::core {

namespace {

struct JsonReply {
    bool ok = false;
    bool timedOut = false;
    QJsonDocument document;
    QString error;
    QNetworkReply::NetworkError networkError = QNetworkReply::NoError;
    int httpStatus = 0;
    QString retryAfter;
    bool cancelled = false;
    int attempts = 1;
    QString retrySummary;
    QString requestId;
    ChatProviderErrorCategory category = ChatProviderErrorCategory::None;
};

JsonReply postJsonOnce(const QUrl& url, const QJsonObject& body, int timeoutMs,
                   const QMap<QByteArray, QByteArray>& headers = {},
                   const std::shared_ptr<std::atomic_bool>& cancellationToken = {}) {
    QNetworkAccessManager manager;
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        request.setRawHeader(it.key(), it.value());
    }

    QNetworkReply* reply =
        manager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    const auto transport = ProviderRequestRuntime::wait(reply, timeoutMs, cancellationToken);
    if (transport.cancelled) {
        reply->deleteLater();
        JsonReply cancelled;
        cancelled.cancelled = true;
        cancelled.error = QStringLiteral("Request cancelled.");
        return cancelled;
    }

    if (transport.timedOut) {
        reply->deleteLater();
        return JsonReply{false,
                         true,
                         {},
                         QStringLiteral("Ollama local generation timed out."),
                         QNetworkReply::TimeoutError};
    }

    if (reply->error() != QNetworkReply::NoError ||
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() >= 400) {
        const auto networkError = reply->error();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString error = httpStatus > 0
                            ? QStringLiteral("HTTP %1 request failed.").arg(httpStatus)
                            : QStringLiteral("Network request failed (%1).")
                                  .arg(static_cast<int>(networkError));
        const auto retryAfter = QString::fromLatin1(reply->rawHeader("Retry-After"));
        const auto payload = reply->readAll();
        const auto errorBody = QJsonDocument::fromJson(payload).object().value(QStringLiteral("error"));
        const auto apiMessage = errorBody.isObject()
                                    ? errorBody.toObject().value(QStringLiteral("message")).toString()
                                    : errorBody.toString();
        if (!apiMessage.isEmpty())
            error = apiMessage.left(512);
        reply->deleteLater();
        return JsonReply{false, false, {}, error, networkError, httpStatus, retryAfter};
    }

    const auto payload = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return JsonReply{false,
                         false,
                         {},
                         QStringLiteral("Ollama local generation response was "
                                        "not valid JSON."),
                         QNetworkReply::UnknownContentError};
    }

    return JsonReply{true, false, document, {}, QNetworkReply::NoError};
}

ChatProviderErrorCategory requestCategory(const JsonReply& reply) {
    return ProviderRequestRuntime::classify(reply.httpStatus, reply.networkError,
                                            reply.timedOut, reply.cancelled);
}

JsonReply executeJsonRequest(ProviderRequestMode mode,
                             const std::shared_ptr<std::atomic_bool>& cancellationToken,
                             const std::function<JsonReply()>& send) {
    const QString requestId = ProviderRequestRuntime::requestId();
    QStringList retries;
    const int maxAttempts = mode == ProviderRequestMode::Generation ? 3 : 2;
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        if (cancellationToken && cancellationToken->load()) {
            JsonReply cancelled;
            cancelled.cancelled = true;
            cancelled.error = QStringLiteral("Request cancelled.");
            cancelled.attempts = attempt;
            cancelled.retrySummary = retries.join(QStringLiteral("; "));
            cancelled.requestId = requestId;
            cancelled.category = ChatProviderErrorCategory::Cancelled;
            return cancelled;
        }
        auto reply = send();
        reply.attempts = attempt;
        reply.requestId = requestId;
        reply.category = requestCategory(reply);
        reply.retrySummary = retries.join(QStringLiteral("; "));
        ProviderTransportResult transport;
        transport.httpStatus = reply.httpStatus;
        transport.networkError = reply.networkError;
        transport.timedOut = reply.timedOut;
        transport.cancelled = reply.cancelled;
        if (reply.ok || !ProviderRequestRuntime::retryable(
                            transport, mode, false) || attempt == maxAttempts)
            return reply;
        const int delayMs = ProviderRequestRuntime::retryDelayMs(
            attempt - 1, reply.retryAfter, mode);
        retries.append(QStringLiteral("%1: %2 ms")
                           .arg(chatProviderErrorCategoryName(reply.category))
                           .arg(delayMs));
        if (!ProviderRequestRuntime::backoff(delayMs, cancellationToken)) {
            reply.cancelled = true;
            reply.category = ChatProviderErrorCategory::Cancelled;
            reply.error = QStringLiteral("Request cancelled.");
            return reply;
        }
    }
    return {};
}

JsonReply postJson(const QUrl& url, const QJsonObject& body, int timeoutMs,
                   const QMap<QByteArray, QByteArray>& headers = {},
                   const std::shared_ptr<std::atomic_bool>& cancellationToken = {}) {
    return executeJsonRequest(ProviderRequestMode::Generation, cancellationToken, [&] {
        return postJsonOnce(url, body, timeoutMs, headers, cancellationToken);
    });
}

bool cancellationRequested(const LocalInferenceRequest& request) {
    return request.options.cancellationRequested ||
           (request.options.cancellationToken && request.options.cancellationToken->load());
}

void recordRequestMetadata(LocalInferenceResponse& response, const JsonReply& reply) {
    response.httpStatus = reply.httpStatus;
    response.attempts = reply.attempts;
    response.retrySummary = reply.retrySummary;
    response.requestId = reply.requestId;
    response.providerErrorCategory = static_cast<int>(reply.category);
}

int approximateTokenCount(const QString& text) {
    const auto words = text.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts).size();
    return words <= 0 ? 0 : static_cast<int>(words);
}

double tokensPerSecond(int tokens, qint64 latencyMs) {
    if (tokens <= 0 || latencyMs <= 0) {
        return 0.0;
    }
    return static_cast<double>(tokens) / (static_cast<double>(latencyMs) / 1000.0);
}

bool hasRedirectStatus(QNetworkReply* reply) {
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return status >= 300 && status < 400;
}

LocalInferenceTrace trace(int sequence, const QString& stage, const QString& status,
                          const QString& summary) {
    return LocalInferenceTrace{sequence, stage, status, summary};
}

LocalInferenceError networkErrorCategory(const JsonReply& reply) {
    if (reply.timedOut || reply.networkError == QNetworkReply::TimeoutError) {
        return LocalInferenceError::Timeout;
    }
    if (reply.networkError == QNetworkReply::ConnectionRefusedError) {
        return LocalInferenceError::OllamaNotRunning;
    }
    if (reply.networkError == QNetworkReply::HostNotFoundError ||
        reply.networkError == QNetworkReply::NetworkSessionFailedError ||
        reply.networkError == QNetworkReply::TemporaryNetworkFailureError) {
        return LocalInferenceError::EndpointUnreachable;
    }
    if (reply.networkError == QNetworkReply::UnknownContentError) {
        return LocalInferenceError::InvalidResponse;
    }
    return LocalInferenceError::RequestFailed;
}

QString safeNetworkFailureSummary(const JsonReply& reply, const QString& operation, int timeoutMs) {
    const auto category = networkErrorCategory(reply);
    if (category == LocalInferenceError::Timeout) {
        return QStringLiteral("%1 timed out after %2 ms.").arg(operation).arg(timeoutMs);
    }
    if (category == LocalInferenceError::OllamaNotRunning) {
        return QStringLiteral("%1 failed: Ollama is not running or is not listening locally.")
            .arg(operation);
    }
    if (category == LocalInferenceError::EndpointUnreachable) {
        return QStringLiteral("%1 failed: local Ollama endpoint is unreachable.").arg(operation);
    }
    if (category == LocalInferenceError::InvalidResponse) {
        return QStringLiteral("%1 failed: Ollama returned malformed JSON.").arg(operation);
    }
    return QStringLiteral("%1 failed: %2").arg(operation, reply.error);
}

} // namespace

QString localInferenceStatusName(LocalInferenceStatus status) {
    switch (status) {
    case LocalInferenceStatus::NotRequested:
        return QStringLiteral("Not Requested");
    case LocalInferenceStatus::Busy:
        return QStringLiteral("Busy");
    case LocalInferenceStatus::Refused:
        return QStringLiteral("Refused");
    case LocalInferenceStatus::Blocked:
        return QStringLiteral("Blocked");
    case LocalInferenceStatus::InvalidRequest:
        return QStringLiteral("Invalid Request");
    case LocalInferenceStatus::ModelUnavailable:
        return QStringLiteral("Model Unavailable");
    case LocalInferenceStatus::Succeeded:
        return QStringLiteral("Succeeded");
    case LocalInferenceStatus::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Not Requested");
}

QString localInferenceStreamStatusName(LocalInferenceStreamStatus status) {
    switch (status) {
    case LocalInferenceStreamStatus::Disabled:
        return QStringLiteral("Disabled");
    case LocalInferenceStreamStatus::NotStarted:
        return QStringLiteral("Not Started");
    case LocalInferenceStreamStatus::Refused:
        return QStringLiteral("Refused");
    case LocalInferenceStreamStatus::Streaming:
        return QStringLiteral("Streaming");
    case LocalInferenceStreamStatus::Completed:
        return QStringLiteral("Completed");
    case LocalInferenceStreamStatus::Cancelled:
        return QStringLiteral("Cancelled");
    case LocalInferenceStreamStatus::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Disabled");
}

QString localInferenceErrorName(LocalInferenceError error) {
    switch (error) {
    case LocalInferenceError::None:
        return QStringLiteral("None");
    case LocalInferenceError::ClientUnavailable:
        return QStringLiteral("Client Unavailable");
    case LocalInferenceError::OllamaNotRunning:
        return QStringLiteral("Ollama Not Running");
    case LocalInferenceError::EndpointUnreachable:
        return QStringLiteral("Endpoint Unreachable");
    case LocalInferenceError::BlankPrompt:
        return QStringLiteral("Blank Prompt");
    case LocalInferenceError::MissingModel:
        return QStringLiteral("Missing Model");
    case LocalInferenceError::EndpointBlocked:
        return QStringLiteral("Endpoint Blocked");
    case LocalInferenceError::ModelUnavailable:
        return QStringLiteral("Model Unavailable");
    case LocalInferenceError::PermissionDenied:
        return QStringLiteral("Permission Denied");
    case LocalInferenceError::SafetyBlocked:
        return QStringLiteral("Safety Blocked");
    case LocalInferenceError::BusyRequest:
        return QStringLiteral("Busy Request");
    case LocalInferenceError::RequestFailed:
        return QStringLiteral("Request Failed");
    case LocalInferenceError::Timeout:
        return QStringLiteral("Timeout");
    case LocalInferenceError::InvalidResponse:
        return QStringLiteral("Invalid Response");
    case LocalInferenceError::StreamInterrupted:
        return QStringLiteral("Stream Interrupted");
    }

    return QStringLiteral("None");
}

QString localInferenceTraceSummary(const LocalInferenceTrace& trace) {
    return trace.summary.isEmpty()
               ? QStringLiteral("%1. %2 [%3]").arg(trace.sequence).arg(trace.stage, trace.status)
               : QStringLiteral("%1. %2 [%3]: %4")
                     .arg(trace.sequence)
                     .arg(trace.stage, trace.status, trace.summary);
}

QStringList localInferenceTraceSummaries(const QList<LocalInferenceTrace>& traces) {
    QStringList summaries;
    for (const auto& item : traces) {
        summaries.append(localInferenceTraceSummary(item));
    }
    return summaries;
}

QString safeLocalInferenceSummary(const LocalInferenceResponse& response) {
    if (!response.summary.isEmpty()) {
        return response.summary;
    }

    return QStringLiteral("Local inference %1.").arg(localInferenceStatusName(response.status));
}

QString safeLocalInferenceResponseSummary(const LocalInferenceResponse& response) {
    if (response.status == LocalInferenceStatus::Succeeded && !response.text.trimmed().isEmpty()) {
        return QStringLiteral("Local inference completed for %1 (%2 characters).")
            .arg(response.model, QString::number(response.text.size()));
    }

    return safeLocalInferenceSummary(response);
}

LocalInferenceResponse NullLocalInferenceClient::infer(const LocalInferenceRequest& request) {
    Q_UNUSED(request);
    LocalInferenceResponse response;
    response.status = LocalInferenceStatus::Refused;
    response.error = LocalInferenceError::ClientUnavailable;
    response.summary =
        QStringLiteral("Local inference client is unavailable; no prompt was executed.");
    response.traces = {
        trace(1, QStringLiteral("Client"), QStringLiteral("Refused"),
              QStringLiteral("Null local inference client refused execution.")),
    };
    return response;
}

QString NullLocalInferenceClient::statusSummary() const {
    return QStringLiteral("Local inference client is unavailable.");
}

LocalInferenceStreamResult NullLocalInferenceStreamClient::startStream(
    const LocalInferenceRequest& request,
    const std::function<void(const LocalInferenceStreamChunk&)>& onChunk) {
    Q_UNUSED(request);
    Q_UNUSED(onChunk);
    return LocalInferenceStreamResult{
        LocalInferenceStreamStatus::Disabled,
        LocalInferenceError::ClientUnavailable,
        QStringLiteral("Local inference streaming is disabled; no stream was opened."),
        {},
        {},
        {},
        false,
        0,
        {},
        {},
    };
}

QString NullLocalInferenceStreamClient::statusSummary() const {
    return QStringLiteral("Local inference streaming is disabled.");
}

bool NullLocalInferenceStreamClient::isAvailable() const {
    return false;
}

LocalInferenceWorker::LocalInferenceWorker(
    std::unique_ptr<ILocalInferenceClient> inferenceClient,
    std::unique_ptr<ILocalInferenceStreamClient> streamClient, QObject* callbackContext,
    bool threadedInference, bool threadedStream)
    : inferenceClient_(inferenceClient ? std::move(inferenceClient)
                                       : std::make_unique<NullLocalInferenceClient>()),
      streamClient_(streamClient ? std::move(streamClient)
                                 : std::make_unique<NullLocalInferenceStreamClient>()),
      callbackContext_(callbackContext), threadedInference_(threadedInference),
      threadedStream_(threadedStream) {}

LocalInferenceWorker::~LocalInferenceWorker() {
    if (activeThread_ && activeThread_->isRunning()) {
        activeThread_->wait();
    }
    delete activeThread_;
    activeThread_ = nullptr;
}

bool LocalInferenceWorker::hasActiveThread() const {
    return activeThread_ && activeThread_->isRunning();
}

bool LocalInferenceWorker::requestCancelled(const LocalInferenceRequest& request) const {
    return cancellationRequested(request);
}

bool LocalInferenceWorker::startInference(const LocalInferenceRequest& request,
                                          LocalInferenceFinishedCallback onFinished) {
    if (!inferenceClient_ || hasActiveThread()) {
        return false;
    }

    auto requestWithToken = request;
    requestWithToken.options.cancellationToken = std::make_shared<std::atomic_bool>(false);
    activeCancellationToken_ = requestWithToken.options.cancellationToken;

    if (!threadedInference_) {
        QElapsedTimer timer;
        timer.start();
        auto response = inferenceClient_->infer(requestWithToken);
        response.latencyMs = timer.elapsed();
        if (!response.text.isEmpty() && response.firstTokenLatencyMs < 0) {
            response.firstTokenLatencyMs = response.latencyMs;
        }
        response.approximateOutputTokens = approximateTokenCount(response.text);
        response.approximateTokensPerSecond =
            tokensPerSecond(response.approximateOutputTokens, response.latencyMs);
        if (onFinished) {
            onFinished(requestWithToken.id, response);
        }
        activeCancellationToken_.reset();
        return true;
    }

    const QPointer<QObject> context{callbackContext_};
    auto* thread = QThread::create([this, request = std::move(requestWithToken),
                                    onFinished = std::move(onFinished), context]() mutable {
        QElapsedTimer timer;
        timer.start();
        auto response = inferenceClient_->infer(request);
        response.latencyMs = timer.elapsed();
        if (!response.text.isEmpty() && response.firstTokenLatencyMs < 0) {
            response.firstTokenLatencyMs = response.latencyMs;
        }
        response.approximateOutputTokens = approximateTokenCount(response.text);
        response.approximateTokensPerSecond =
            tokensPerSecond(response.approximateOutputTokens, response.latencyMs);
        if (!context) {
            return;
        }
        QMetaObject::invokeMethod(
            context,
            [this, requestId = request.id, response = std::move(response),
             onFinished = std::move(onFinished)]() mutable {
                if (onFinished) {
                    onFinished(requestId, response);
                }
                if (activeThread_) {
                    activeThread_->deleteLater();
                }
                activeThread_ = nullptr;
                activeCancellationToken_.reset();
            },
            Qt::QueuedConnection);
    });
    thread->setObjectName(QStringLiteral("SentinelLocalInferenceWorker"));
    activeThread_ = thread;
    thread->start();
    return true;
}

bool LocalInferenceWorker::startStream(const LocalInferenceRequest& request,
                                       LocalInferenceStreamChunkCallback onChunk,
                                       LocalInferenceStreamFinishedCallback onFinished) {
    if (!streamClient_ || !streamClient_->isAvailable() || hasActiveThread()) {
        return false;
    }

    auto requestWithToken = request;
    requestWithToken.options.cancellationToken = std::make_shared<std::atomic_bool>(false);
    activeCancellationToken_ = requestWithToken.options.cancellationToken;

    if (!threadedStream_) {
        auto result = streamClient_->startStream(
            requestWithToken,
            [&requestWithToken, &onChunk](const LocalInferenceStreamChunk& chunk) {
                if (onChunk) {
                    onChunk(requestWithToken.id, chunk);
                }
            });
        if (onFinished) {
            onFinished(requestWithToken.id, result);
        }
        activeCancellationToken_.reset();
        return true;
    }

    const QPointer<QObject> context{callbackContext_};
    auto* thread =
        QThread::create([this, request = std::move(requestWithToken), onChunk = std::move(onChunk),
                         onFinished = std::move(onFinished), context]() mutable {
            auto result = streamClient_->startStream(
                request, [requestId = request.id, onChunk,
                          context](const LocalInferenceStreamChunk& chunk) mutable {
                    if (!context) {
                        return;
                    }
                    QMetaObject::invokeMethod(
                        context,
                        [requestId, chunk, onChunk]() mutable {
                            if (onChunk) {
                                onChunk(requestId, chunk);
                            }
                        },
                        Qt::QueuedConnection);
                });
            if (!context) {
                return;
            }
            QMetaObject::invokeMethod(
                context,
                [this, requestId = request.id, result = std::move(result),
                 onFinished = std::move(onFinished)]() mutable {
                    if (onFinished) {
                        onFinished(requestId, result);
                    }
                    if (activeThread_) {
                        activeThread_->deleteLater();
                    }
                    activeThread_ = nullptr;
                    activeCancellationToken_.reset();
                },
                Qt::QueuedConnection);
        });
    thread->setObjectName(QStringLiteral("SentinelLocalInferenceStreamWorker"));
    activeThread_ = thread;
    thread->start();
    return true;
}

void LocalInferenceWorker::cancel(const QString& requestId) {
    Q_UNUSED(requestId);
    if (activeCancellationToken_) {
        activeCancellationToken_->store(true);
    }
}

QString LocalInferenceWorker::statusSummary() const {
    return inferenceClient_ ? inferenceClient_->statusSummary()
                            : QStringLiteral("Local inference client is unavailable.");
}

QString LocalInferenceWorker::streamStatusSummary() const {
    return streamClient_ ? streamClient_->statusSummary()
                         : QStringLiteral("Local inference streaming client is unavailable.");
}

bool LocalInferenceWorker::streamingAvailable() const {
    return streamClient_ && streamClient_->isAvailable();
}

OllamaLocalInferenceClient::OllamaLocalInferenceClient(OllamaConfig config, int timeoutMs)
    : config_(std::move(config)), timeoutMs_(timeoutMs) {
    config_.generateTimeoutMs = timeoutMs_;
}

LocalInferenceResponse OllamaLocalInferenceClient::infer(const LocalInferenceRequest& request) {
    LocalInferenceResponse response;
    response.endpoint = config_.endpoint.toString();
    response.model = request.options.model.trimmed();
    response.timeoutMs =
        request.options.timeoutMs > 0 ? request.options.timeoutMs : config_.generateTimeoutMs;
    response.traces.append(
        trace(1, QStringLiteral("Request"), QStringLiteral("Received"),
              QStringLiteral("Local inference request reached Ollama boundary with %1 ms "
                             "timeout metadata.")
                  .arg(response.timeoutMs)));

    if (request.prompt.trimmed().isEmpty()) {
        response.status = LocalInferenceStatus::InvalidRequest;
        response.error = LocalInferenceError::BlankPrompt;
        response.summary = QStringLiteral("Local inference request rejected: prompt is blank.");
        response.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Rejected"), response.summary));
        return response;
    }

    if (request.options.streamingRequested) {
        response.status = LocalInferenceStatus::InvalidRequest;
        response.error = LocalInferenceError::RequestFailed;
        response.summary =
            QStringLiteral("Local inference request rejected: streaming is out of scope.");
        response.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Rejected"), response.summary));
        return response;
    }

    if (cancellationRequested(request)) {
        response.status = LocalInferenceStatus::Blocked;
        response.error = LocalInferenceError::RequestFailed;
        response.summary =
            QStringLiteral("Local inference request was cancelled before generation.");
        response.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Cancelled"), response.summary));
        return response;
    }

    if (!endpointAllowed()) {
        response.status = LocalInferenceStatus::Blocked;
        response.error = LocalInferenceError::EndpointBlocked;
        response.summary =
            QStringLiteral("Local inference blocked: endpoint must be local loopback HTTP.");
        response.traces.append(
            trace(2, QStringLiteral("Endpoint"), QStringLiteral("Blocked"), response.summary));
        return response;
    }

    const auto timeoutMs = response.timeoutMs > 0 ? response.timeoutMs : timeoutMs_;
    if (request.options.discoverySnapshot) {
        const auto& discovery = *request.options.discoverySnapshot;
        if (!discovery.succeeded()) {
            response.httpStatus = discovery.httpStatus;
            response.providerErrorCategory = static_cast<int>(
                discovery.errorCategory == ChatProviderErrorCategory::None
                    ? ChatProviderErrorCategory::ProviderUnavailable : discovery.errorCategory);
            response.requestId = discovery.requestId;
            response.attempts = discovery.attemptCount;
            response.status = discovery.lifecycle == ChatRequestLifecycle::Cancelled
                                  ? LocalInferenceStatus::Blocked : LocalInferenceStatus::Error;
            response.error = discovery.errorCategory == ChatProviderErrorCategory::Timeout
                                 ? LocalInferenceError::Timeout
                             : discovery.errorCategory == ChatProviderErrorCategory::ConnectionFailed
                                 ? LocalInferenceError::EndpointUnreachable
                                 : LocalInferenceError::RequestFailed;
            response.summary = discovery.safeDetail;
            response.traces.append(trace(2, QStringLiteral("Model Discovery"),
                                         QStringLiteral("Error"), response.summary));
            return response;
        }
        if (discovery.models.isEmpty()) {
            response.status = LocalInferenceStatus::ModelUnavailable;
            response.error = LocalInferenceError::MissingModel;
            response.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::ModelNotFound);
            response.summary = discovery.safeDetail;
            return response;
        }
        if (response.model.isEmpty()) {
            response.status = LocalInferenceStatus::InvalidRequest;
            response.error = LocalInferenceError::MissingModel;
            response.summary = QStringLiteral("Local inference request rejected: model is required.");
            return response;
        }
        const bool modelAvailable = std::any_of(
            discovery.models.cbegin(), discovery.models.cend(),
            [&](const OllamaModelSummary& model) { return model.name == response.model; });
        if (!modelAvailable) {
            response.status = LocalInferenceStatus::ModelUnavailable;
            response.error = LocalInferenceError::ModelUnavailable;
            response.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::ModelNotFound);
            response.summary = QStringLiteral("Local inference request rejected: selected model is unavailable.");
            response.traces.append(trace(2, QStringLiteral("Model Discovery"),
                                         QStringLiteral("Unavailable"), response.summary));
            return response;
        }
    }

    if (response.model.isEmpty()) {
        response.status = LocalInferenceStatus::InvalidRequest;
        response.error = LocalInferenceError::MissingModel;
        response.summary = QStringLiteral("Local inference request rejected: model is required.");
        return response;
    }

    QJsonObject body;
    body.insert(QStringLiteral("model"), response.model);
    body.insert(QStringLiteral("prompt"), request.prompt.trimmed());
    body.insert(QStringLiteral("stream"), false);

    QJsonObject options;
    options.insert(QStringLiteral("temperature"), request.options.temperature);
    options.insert(QStringLiteral("top_p"), request.options.topP);
    options.insert(QStringLiteral("num_predict"), request.options.maxTokens);
    body.insert(QStringLiteral("options"), options);

    response.traces.append(trace(2, QStringLiteral("Generation"), QStringLiteral("Started"),
                                 QStringLiteral("Calling local Ollama /api/generate without "
                                                "streaming; timeout %1 ms.")
                                     .arg(timeoutMs)));
    const auto reply = postJson(endpointUrl(QStringLiteral("/api/generate")), body, timeoutMs,
                                {}, request.options.cancellationToken);
    recordRequestMetadata(response, reply);
    if (!reply.ok) {
        response.status = LocalInferenceStatus::Error;
        response.error = networkErrorCategory(reply);
        response.httpStatus = reply.httpStatus;
        response.summary =
            safeNetworkFailureSummary(reply, QStringLiteral("Local Ollama generation"), timeoutMs);
        response.traces.append(
            trace(3, QStringLiteral("Generation"), QStringLiteral("Error"), response.summary));
        return response;
    }

    const auto text = reply.document.object().value(QStringLiteral("response")).toString();
    if (text.isEmpty()) {
        response.status = LocalInferenceStatus::Error;
        response.error = LocalInferenceError::InvalidResponse;
        response.summary = QStringLiteral("Local Ollama generation response did not include text.");
        response.traces.append(trace(3, QStringLiteral("Generation"),
                                     QStringLiteral("Invalid Response"), response.summary));
        return response;
    }

    response.status = LocalInferenceStatus::Succeeded;
    response.error = LocalInferenceError::None;
    response.text = text;
    response.approximateOutputTokens = approximateTokenCount(text);
    response.summary = QStringLiteral("Local inference completed through Ollama /api/generate.");
    response.traces.append(trace(3, QStringLiteral("Generation"), QStringLiteral("Succeeded"),
                                 QStringLiteral("Local Ollama generation completed without "
                                                "streaming or tools.")));
    return response;
}

QString OllamaLocalInferenceClient::statusSummary() const {
    return endpointAllowed() ? QStringLiteral("Ollama local inference boundary is configured for "
                                              "loopback-only /api/generate.")
                             : QStringLiteral("Ollama local inference boundary is blocked by "
                                              "endpoint policy.");
}

QUrl OllamaLocalInferenceClient::endpointUrl(const QString& path) const {
    QUrl url = config_.endpoint.url;
    url.setPath(path);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

bool OllamaLocalInferenceClient::endpointAllowed() const {
    return config_.endpoint.isLoopbackHttp();
}

OllamaLocalInferenceStreamClient::OllamaLocalInferenceStreamClient(OllamaConfig config,
                                                                   int timeoutMs)
    : config_(std::move(config)), timeoutMs_(timeoutMs) {
    config_.streamTimeoutMs = timeoutMs_;
}

LocalInferenceStreamResult OllamaLocalInferenceStreamClient::startStream(
    const LocalInferenceRequest& request,
    const std::function<void(const LocalInferenceStreamChunk&)>& onChunk) {
    LocalInferenceStreamResult result;
    result.requestId = ProviderRequestRuntime::requestId();
    result.status = LocalInferenceStreamStatus::NotStarted;
    result.model = request.options.model.trimmed();
    result.endpoint = config_.endpoint.toString();
    result.timeoutMs =
        request.options.timeoutMs > 0 ? request.options.timeoutMs : config_.streamTimeoutMs;
    result.traces.append(trace(1, QStringLiteral("Stream Request"), QStringLiteral("Received"),
                               QStringLiteral("Local streaming request reached Ollama boundary "
                                              "with %1 ms timeout metadata.")
                                   .arg(result.timeoutMs)));

    if (request.prompt.trimmed().isEmpty()) {
        result.status = LocalInferenceStreamStatus::Refused;
        result.lifecycle = ChatRequestLifecycle::Failed;
        result.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::RequestRejected);
        result.error = LocalInferenceError::BlankPrompt;
        result.summary = QStringLiteral("Local streaming request rejected: prompt is blank.");
        result.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Rejected"), result.summary));
        return result;
    }

    if (result.model.isEmpty()) {
        result.status = LocalInferenceStreamStatus::Refused;
        result.lifecycle = ChatRequestLifecycle::Failed;
        result.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::ModelNotFound);
        result.error = LocalInferenceError::MissingModel;
        result.summary = QStringLiteral("Local streaming request rejected: model is required.");
        result.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Rejected"), result.summary));
        return result;
    }

    if (cancellationRequested(request)) {
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.lifecycle = ChatRequestLifecycle::Cancelled;
        result.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::Cancelled);
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local streaming request was cancelled before generation.");
        result.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Cancelled"), result.summary));
        return result;
    }

    if (!endpointAllowed()) {
        result.status = LocalInferenceStreamStatus::Refused;
        result.lifecycle = ChatRequestLifecycle::Failed;
        result.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::RequestRejected);
        result.error = LocalInferenceError::EndpointBlocked;
        result.summary =
            QStringLiteral("Local streaming blocked: endpoint must be local loopback HTTP.");
        result.traces.append(
            trace(2, QStringLiteral("Endpoint"), QStringLiteral("Blocked"), result.summary));
        return result;
    }

    QJsonObject body;
    body.insert(QStringLiteral("model"), result.model);
    body.insert(QStringLiteral("prompt"), request.prompt.trimmed());
    body.insert(QStringLiteral("stream"), true);

    QJsonObject options;
    options.insert(QStringLiteral("temperature"), request.options.temperature);
    options.insert(QStringLiteral("top_p"), request.options.topP);
    options.insert(QStringLiteral("num_predict"), request.options.maxTokens);
    body.insert(QStringLiteral("options"), options);

    QNetworkAccessManager manager;
    QNetworkRequest networkRequest{endpointUrl(QStringLiteral("/api/generate"))};
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                             QStringLiteral("application/json"));
    networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                QNetworkRequest::ManualRedirectPolicy);

    QByteArray pending;
    int sequence = 0;
    bool done = false;
    QElapsedTimer elapsed;
    elapsed.start();

    QNetworkReply* reply =
        manager.post(networkRequest, QJsonDocument(body).toJson(QJsonDocument::Compact));

    auto processLine = [&](const QByteArray& line) {
        const auto trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) {
            return;
        }

        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(trimmedLine, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            ++sequence;
            ++result.malformedChunkCount;
            LocalInferenceStreamChunk chunk{
                sequence, {}, false, true, QStringLiteral("Malformed local stream chunk ignored."),
            };
            result.chunks.append(chunk);
            result.traces.append(trace(sequence + 2, QStringLiteral("Stream Chunk"),
                                       QStringLiteral("Malformed"), chunk.summary));
            if (onChunk) {
                onChunk(chunk);
            }
            return;
        }

        const auto object = document.object();
        const auto text = object.value(QStringLiteral("response")).toString();
        const auto finalChunk = object.value(QStringLiteral("done")).toBool(false);
        if (text.isEmpty() && !finalChunk) {
            ++sequence;
            ++result.malformedChunkCount;
            LocalInferenceStreamChunk chunk{
                sequence,
                {},
                false,
                true,
                QStringLiteral("Local stream chunk did not include response text."),
            };
            result.chunks.append(chunk);
            result.traces.append(trace(sequence + 2, QStringLiteral("Stream Chunk"),
                                       QStringLiteral("Malformed"), chunk.summary));
            if (onChunk) {
                onChunk(chunk);
            }
            return;
        }

        ++sequence;
        done = done || finalChunk;
        result.accumulatedText.append(text);
        if (!text.isEmpty() && result.firstTokenLatencyMs < 0) {
            result.firstTokenLatencyMs = elapsed.elapsed();
        }
        LocalInferenceStreamChunk chunk{
            sequence,
            text,
            finalChunk,
            false,
            finalChunk ? QStringLiteral("Final local stream chunk received.")
                       : QStringLiteral("Local stream chunk received."),
        };
        result.chunks.append(chunk);
        if (onChunk) {
            onChunk(chunk);
        }
    };

    auto consumeReady = [&]() {
        pending.append(reply->readAll());
        while (true) {
            const auto newlineIndex = pending.indexOf('\n');
            if (newlineIndex < 0) {
                break;
            }
            const auto line = pending.left(newlineIndex);
            pending.remove(0, newlineIndex + 1);
            processLine(line);
        }
    };
    const auto timeoutMs = result.timeoutMs > 0 ? result.timeoutMs : timeoutMs_;
    result.status = LocalInferenceStreamStatus::Streaming;
    result.summary = QStringLiteral("Local Ollama streaming generation is active.");
    result.lifecycle = ChatRequestLifecycle::Running;
    const auto transport = ProviderRequestRuntime::wait(
        reply, timeoutMs, request.options.cancellationToken, consumeReady);
    result.httpStatus = transport.httpStatus;
    result.providerErrorCategory = static_cast<int>(transport.category);
    result.lifecycle = transport.lifecycle;
    result.latencyMs = elapsed.elapsed();

    if (transport.cancelled || cancellationRequested(request)) {
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local Ollama streaming generation was cancelled.");
        reply->deleteLater();
        return result;
    }

    if (transport.timedOut) {
        result.status = LocalInferenceStreamStatus::Error;
        result.error = LocalInferenceError::Timeout;
        result.summary = QStringLiteral("Local Ollama streaming generation timed out after %1 ms.")
                             .arg(timeoutMs);
        reply->deleteLater();
        return result;
    }

    pending.append(reply->readAll());
    if (!pending.trimmed().isEmpty()) {
        processLine(pending);
    }

    if (hasRedirectStatus(reply)) {
        result.status = LocalInferenceStreamStatus::Error;
        result.lifecycle = ChatRequestLifecycle::Failed;
        result.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::RequestRejected);
        result.error = LocalInferenceError::EndpointBlocked;
        result.summary = QStringLiteral("Local Ollama streaming blocked: redirects are not "
                                        "allowed.");
        reply->deleteLater();
        return result;
    }

    if (transport.category != ChatProviderErrorCategory::None) {
        const auto error = QStringLiteral("HTTP %1 or network error %2.")
                               .arg(transport.httpStatus)
                               .arg(static_cast<int>(transport.networkError));
        result.status = LocalInferenceStreamStatus::Error;
        const JsonReply failedReply{false, false, {}, error, transport.networkError,
                                    transport.httpStatus};
        result.error = networkErrorCategory(failedReply);
        result.summary = safeNetworkFailureSummary(
            failedReply, QStringLiteral("Local Ollama streaming generation"), timeoutMs);
        reply->deleteLater();
        return result;
    }

    reply->deleteLater();

    if (cancellationRequested(request)) {
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.lifecycle = ChatRequestLifecycle::Cancelled;
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local Ollama streaming generation was cancelled.");
        return result;
    }

    if (!done || result.accumulatedText.trimmed().isEmpty()) {
        result.status = LocalInferenceStreamStatus::Error;
        result.lifecycle = ChatRequestLifecycle::Failed;
        result.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::MalformedResponse);
        result.error =
            done ? LocalInferenceError::InvalidResponse : LocalInferenceError::StreamInterrupted;
        result.summary = QStringLiteral("Local Ollama streaming response did not complete with "
                                        "assistant text.");
        return result;
    }

    result.status = LocalInferenceStreamStatus::Completed;
    result.error = LocalInferenceError::None;
    result.approximateOutputTokens = approximateTokenCount(result.accumulatedText);
    result.approximateTokensPerSecond =
        tokensPerSecond(result.approximateOutputTokens, result.latencyMs);
    result.summary = result.malformedChunkCount > 0
                         ? QStringLiteral("Local Ollama streaming completed with %1 malformed "
                                          "chunk(s) ignored.")
                               .arg(result.malformedChunkCount)
                         : QStringLiteral("Local Ollama streaming completed.");
    result.traces.append(trace(static_cast<int>(result.traces.size()) + 1,
                               QStringLiteral("Stream Generation"), QStringLiteral("Completed"),
                               result.summary));
    return result;
}

QString OllamaLocalInferenceStreamClient::statusSummary() const {
    return endpointAllowed() ? QStringLiteral("Ollama local streaming boundary is configured for "
                                              "loopback-only /api/generate.")
                             : QStringLiteral("Ollama local streaming boundary is blocked by "
                                              "endpoint policy.");
}

bool OllamaLocalInferenceStreamClient::isAvailable() const {
    return endpointAllowed();
}

QUrl OllamaLocalInferenceStreamClient::endpointUrl(const QString& path) const {
    QUrl url = config_.endpoint.url;
    url.setPath(path);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

bool OllamaLocalInferenceStreamClient::endpointAllowed() const {
    return config_.endpoint.isLoopbackHttp();
}

LMStudioLocalInferenceClient::LMStudioLocalInferenceClient(LMStudioConfig config, int timeoutMs)
    : config_(std::move(config)), timeoutMs_(timeoutMs) {
    config_.timeoutMs = timeoutMs_;
}

LMStudioLocalInferenceClient::OpenAiCompletionResult
LMStudioLocalInferenceClient::completeOpenAiChat(
    const QJsonObject& body,
    const std::shared_ptr<std::atomic_bool>& cancellationToken) const {
    if (!endpointAllowed())
        return {false, {}, QStringLiteral("OpenAI-compatible endpoint is unavailable."), 0};
    const auto host = config_.endpoint.host().toLower();
    if (host.contains(QLatin1String("anthropic.com")) ||
        host.contains(QLatin1String("googleapis.com")))
        return {false, {}, QStringLiteral("Endpoint does not use OpenAI chat completions."), 501};
    QMap<QByteArray, QByteArray> headers;
    if (!config_.apiKey.isEmpty())
        headers.insert("Authorization", QStringLiteral("Bearer %1").arg(config_.apiKey).toUtf8());
    const auto path = config_.endpoint.path().endsWith(QLatin1String("/v1"))
                          ? QStringLiteral("/chat/completions")
                          : QStringLiteral("/v1/chat/completions");
    const auto reply = postJson(endpointUrl(path), body,
                                timeoutMs_ > 0 ? timeoutMs_ : config_.timeoutMs, headers,
                                cancellationToken);
    if (!reply.ok)
        return {false, {}, safeNetworkFailureSummary(reply, QStringLiteral("Native tool request"),
                                                      timeoutMs_), reply.httpStatus, reply.retryAfter,
                reply.networkError, reply.timedOut,
                reply.networkError == QNetworkReply::UnknownContentError, reply.cancelled,
                reply.attempts, reply.retrySummary, reply.requestId,
                static_cast<int>(reply.category)};
    OpenAiCompletionResult result;
    result.ok = true;
    result.body = reply.document.object();
    result.attempts = reply.attempts;
    result.retrySummary = reply.retrySummary;
    result.requestId = reply.requestId;
    return result;
}

LocalInferenceResponse LMStudioLocalInferenceClient::infer(const LocalInferenceRequest& request) {
    LocalInferenceResponse response;
    response.endpoint = config_.toString();
    response.model = request.options.model.trimmed();
    response.timeoutMs =
        request.options.timeoutMs > 0 ? request.options.timeoutMs : config_.timeoutMs;
    response.traces.append(
        trace(1, QStringLiteral("Request"), QStringLiteral("Received"),
              QStringLiteral("Local inference request reached LM Studio boundary with %1 ms "
                             "timeout metadata.")
                  .arg(response.timeoutMs)));

    if (request.prompt.trimmed().isEmpty()) {
        response.status = LocalInferenceStatus::InvalidRequest;
        response.error = LocalInferenceError::BlankPrompt;
        response.summary = QStringLiteral("Local inference request rejected: prompt is blank.");
        response.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Rejected"), response.summary));
        return response;
    }

    if (!endpointAllowed()) {
        response.status = LocalInferenceStatus::Blocked;
        response.error = LocalInferenceError::EndpointBlocked;
        response.summary =
            config_.isCloud()
                ? QStringLiteral(
                      "%1 inference blocked: an API key is required for cloud endpoints.")
                      .arg(config_.providerDisplayName())
                : QStringLiteral("Local inference blocked: endpoint must be local loopback HTTP.");
        response.traces.append(
            trace(2, QStringLiteral("Endpoint"), QStringLiteral("Blocked"), response.summary));
        return response;
    }

    const auto timeoutMs = response.timeoutMs > 0 ? response.timeoutMs : timeoutMs_;
    if (endpointAllowed() && !config_.isCloud()) {
        const auto models =
            fetchOpenAiCompatibleModels(endpointUrl(QStringLiteral("/v1/models")), timeoutMs);
        bool modelAvailable = false;
        for (const auto& installedModel : models) {
            if (installedModel.name == response.model) {
                modelAvailable = true;
                break;
            }
        }
        if (!modelAvailable && !models.isEmpty()) {
            response.status = LocalInferenceStatus::ModelUnavailable;
            response.error = LocalInferenceError::ModelUnavailable;
            response.summary = QStringLiteral(
                "Local inference request rejected: model is not loaded in LM Studio.");
            response.traces.append(trace(2, QStringLiteral("Model Discovery"),
                                         QStringLiteral("Unavailable"), response.summary));
            return response;
        }
    }

    // Cloud providers need their own request shape and credentials; the
    // OpenAI-compatible body below only fits LM Studio / llama.cpp / local
    // OpenAI-compatible servers.
    if (config_.isCloud()) {
        const bool isAnthropic = config_.endpoint.host().contains(QLatin1String("anthropic.com"));
        const bool isGemini = config_.endpoint.host().contains(QLatin1String("googleapis.com"));

        if (isAnthropic) {
            QJsonObject messageObj;
            messageObj.insert(QStringLiteral("role"), QStringLiteral("user"));
            messageObj.insert(QStringLiteral("content"), request.prompt.trimmed());
            QJsonArray messagesArr;
            messagesArr.append(messageObj);

            QJsonObject body;
            body.insert(QStringLiteral("model"), response.model);
            body.insert(QStringLiteral("messages"), messagesArr);
            body.insert(QStringLiteral("max_tokens"),
                        request.options.maxTokens > 0 ? request.options.maxTokens : 2048);
            body.insert(QStringLiteral("stream"), false);

            response.traces.append(trace(
                2, QStringLiteral("Generation"), QStringLiteral("Started"),
                QStringLiteral("Calling Anthropic /v1/messages; timeout %1 ms.").arg(timeoutMs)));

            QMap<QByteArray, QByteArray> headers;
            headers.insert("x-api-key", config_.apiKey.toUtf8());
            headers.insert("anthropic-version", "2023-06-01");
            const auto reply =
                postJson(QUrl(QStringLiteral("https://api.anthropic.com/v1/messages")), body,
                         timeoutMs, headers, request.options.cancellationToken);
            recordRequestMetadata(response, reply);
            if (!reply.ok) {
                response.status = LocalInferenceStatus::Error;
                response.error = networkErrorCategory(reply);
                response.httpStatus = reply.httpStatus;
                response.summary = safeNetworkFailureSummary(
                    reply, QStringLiteral("Anthropic cloud generation"), timeoutMs);
                response.traces.append(trace(3, QStringLiteral("Generation"),
                                             QStringLiteral("Error"), response.summary));
                return response;
            }
            const auto contentParts =
                reply.document.object().value(QStringLiteral("content")).toArray();
            QString text;
            for (const auto& part : contentParts) {
                const auto partObj = part.toObject();
                if (partObj.value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
                    text += partObj.value(QStringLiteral("text")).toString();
                }
            }
            if (text.isEmpty()) {
                response.status = LocalInferenceStatus::Error;
                response.error = LocalInferenceError::InvalidResponse;
                response.summary =
                    QStringLiteral("Anthropic cloud generation response did not include text.");
                response.traces.append(trace(3, QStringLiteral("Generation"),
                                             QStringLiteral("Invalid Response"), response.summary));
                return response;
            }
            response.status = LocalInferenceStatus::Succeeded;
            response.error = LocalInferenceError::None;
            response.text = text;
            response.summary = QStringLiteral("Anthropic cloud generation succeeded.");
            response.traces.append(trace(3, QStringLiteral("Generation"),
                                         QStringLiteral("Succeeded"), response.summary));
            return response;
        }

        if (isGemini) {
            const QString modelId =
                response.model.isEmpty() ? QStringLiteral("gemini-2.0-flash") : response.model;

            QJsonObject partObj;
            partObj.insert(QStringLiteral("text"), request.prompt.trimmed());
            QJsonArray partsArr;
            partsArr.append(partObj);

            QJsonObject contentObj;
            contentObj.insert(QStringLiteral("role"), QStringLiteral("user"));
            contentObj.insert(QStringLiteral("parts"), partsArr);
            QJsonArray contentsArr;
            contentsArr.append(contentObj);

            QJsonObject body;
            body.insert(QStringLiteral("contents"), contentsArr);
            QJsonObject generationConfig;
            generationConfig.insert(QStringLiteral("temperature"), request.options.temperature);
            generationConfig.insert(QStringLiteral("topP"), request.options.topP);
            generationConfig.insert(QStringLiteral("maxOutputTokens"),
                                    request.options.maxTokens > 0 ? request.options.maxTokens
                                                                  : 2048);
            body.insert(QStringLiteral("generationConfig"), generationConfig);

            response.traces.append(trace(
                2, QStringLiteral("Generation"), QStringLiteral("Started"),
                QStringLiteral("Calling Gemini generateContent; timeout %1 ms.").arg(timeoutMs)));

            const QUrl url(QStringLiteral("https://generativelanguage.googleapis.com/v1beta/models/"
                                          "%1:generateContent?key=%2")
                               .arg(modelId, config_.apiKey));
            const auto reply = postJson(url, body, timeoutMs, {},
                                        request.options.cancellationToken);
            recordRequestMetadata(response, reply);
            if (!reply.ok) {
                response.status = LocalInferenceStatus::Error;
                response.error = networkErrorCategory(reply);
                response.httpStatus = reply.httpStatus;
                response.summary = safeNetworkFailureSummary(
                    reply, QStringLiteral("Gemini cloud generation"), timeoutMs);
                response.traces.append(trace(3, QStringLiteral("Generation"),
                                             QStringLiteral("Error"), response.summary));
                return response;
            }
            const auto candidates =
                reply.document.object().value(QStringLiteral("candidates")).toArray();
            QString text;
            if (!candidates.isEmpty()) {
                const auto parts = candidates.first()
                                       .toObject()
                                       .value(QStringLiteral("content"))
                                       .toObject()
                                       .value(QStringLiteral("parts"))
                                       .toArray();
                for (const auto& part : parts) {
                    text += part.toObject().value(QStringLiteral("text")).toString();
                }
            }
            if (text.isEmpty()) {
                response.status = LocalInferenceStatus::Error;
                response.error = LocalInferenceError::InvalidResponse;
                response.summary =
                    QStringLiteral("Gemini cloud generation response did not include text.");
                response.traces.append(trace(3, QStringLiteral("Generation"),
                                             QStringLiteral("Invalid Response"), response.summary));
                return response;
            }
            response.status = LocalInferenceStatus::Succeeded;
            response.error = LocalInferenceError::None;
            response.text = text;
            response.summary = QStringLiteral("Gemini cloud generation succeeded.");
            response.traces.append(trace(3, QStringLiteral("Generation"),
                                         QStringLiteral("Succeeded"), response.summary));
            return response;
        }

        // OpenAI-compatible cloud (OpenAI, DeepSeek, Groq, Mistral).
        QJsonObject messageObj;
        messageObj.insert(QStringLiteral("role"), QStringLiteral("user"));
        messageObj.insert(QStringLiteral("content"), request.prompt.trimmed());
        QJsonArray messagesArr;
        messagesArr.append(messageObj);

        QJsonObject body;
        body.insert(QStringLiteral("model"), response.model);
        body.insert(QStringLiteral("messages"), messagesArr);
        body.insert(QStringLiteral("stream"), false);
        body.insert(QStringLiteral("temperature"), request.options.temperature);
        body.insert(QStringLiteral("max_tokens"),
                    request.options.maxTokens > 0 ? request.options.maxTokens : 2048);

        response.traces.append(
            trace(2, QStringLiteral("Generation"), QStringLiteral("Started"),
                  QStringLiteral("Calling %1 /v1/chat/completions; timeout %2 ms.")
                      .arg(config_.providerDisplayName(), QString::number(timeoutMs))));

        QMap<QByteArray, QByteArray> headers;
        headers.insert("Authorization", QStringLiteral("Bearer %1").arg(config_.apiKey).toUtf8());
        const auto reply =
            postJson(endpointUrl(QStringLiteral("/v1/chat/completions")), body, timeoutMs,
                     headers, request.options.cancellationToken);
        recordRequestMetadata(response, reply);
        if (!reply.ok) {
            response.status = LocalInferenceStatus::Error;
            response.error = networkErrorCategory(reply);
            response.httpStatus = reply.httpStatus;
            response.summary = safeNetworkFailureSummary(
                reply, QStringLiteral("%1 cloud generation").arg(config_.providerDisplayName()),
                timeoutMs);
            response.traces.append(
                trace(3, QStringLiteral("Generation"), QStringLiteral("Error"), response.summary));
            return response;
        }
        const auto choices = reply.document.object().value(QStringLiteral("choices")).toArray();
        if (choices.isEmpty()) {
            response.status = LocalInferenceStatus::Error;
            response.error = LocalInferenceError::InvalidResponse;
            response.summary =
                QStringLiteral("%1 cloud generation response did not include choices.")
                    .arg(config_.providerDisplayName());
            response.traces.append(trace(3, QStringLiteral("Generation"),
                                         QStringLiteral("Invalid Response"), response.summary));
            return response;
        }
        const auto choiceObj = choices.first().toObject();
        const auto text = choiceObj.value(QStringLiteral("message"))
                              .toObject()
                              .value(QStringLiteral("content"))
                              .toString();
        if (text.isEmpty()) {
            response.status = LocalInferenceStatus::Error;
            response.error = LocalInferenceError::InvalidResponse;
            response.summary = QStringLiteral("%1 cloud generation response did not include text.")
                                   .arg(config_.providerDisplayName());
            response.traces.append(trace(3, QStringLiteral("Generation"),
                                         QStringLiteral("Invalid Response"), response.summary));
            return response;
        }
        response.status = LocalInferenceStatus::Succeeded;
        response.error = LocalInferenceError::None;
        response.text = text;
        response.summary =
            QStringLiteral("%1 cloud generation succeeded.").arg(config_.providerDisplayName());
        response.traces.append(
            trace(3, QStringLiteral("Generation"), QStringLiteral("Succeeded"), response.summary));
        return response;
    }

    QJsonObject messageObj;
    messageObj.insert(QStringLiteral("role"), QStringLiteral("user"));
    messageObj.insert(QStringLiteral("content"), request.prompt.trimmed());

    QJsonArray messagesArr;
    messagesArr.append(messageObj);

    QJsonObject body;
    body.insert(QStringLiteral("model"), response.model);
    body.insert(QStringLiteral("messages"), messagesArr);
    body.insert(QStringLiteral("stream"), false);

    response.traces.append(
        trace(2, QStringLiteral("Generation"), QStringLiteral("Started"),
              QStringLiteral("Calling local LM Studio /v1/chat/completions; timeout %1 ms.")
                  .arg(timeoutMs)));

    const auto reply =
        postJson(endpointUrl(QStringLiteral("/v1/chat/completions")), body, timeoutMs,
                 {}, request.options.cancellationToken);
    recordRequestMetadata(response, reply);
    if (!reply.ok) {
        response.status = LocalInferenceStatus::Error;
        response.error = networkErrorCategory(reply);
        response.httpStatus = reply.httpStatus;
        response.summary = safeNetworkFailureSummary(
            reply, QStringLiteral("Local LM Studio generation"), timeoutMs);
        response.traces.append(
            trace(3, QStringLiteral("Generation"), QStringLiteral("Error"), response.summary));
        return response;
    }

    const auto choices = reply.document.object().value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        response.status = LocalInferenceStatus::Error;
        response.error = LocalInferenceError::InvalidResponse;
        response.summary =
            QStringLiteral("Local LM Studio generation response did not include choices.");
        response.traces.append(trace(3, QStringLiteral("Generation"),
                                     QStringLiteral("Invalid Response"), response.summary));
        return response;
    }

    const auto choiceObj = choices.first().toObject();
    const auto messageVal = choiceObj.value(QStringLiteral("message")).toObject();
    const auto text = messageVal.value(QStringLiteral("content")).toString();
    if (text.isEmpty()) {
        response.status = LocalInferenceStatus::Error;
        response.error = LocalInferenceError::InvalidResponse;
        response.summary =
            QStringLiteral("Local LM Studio generation response did not include text.");
        response.traces.append(trace(3, QStringLiteral("Generation"),
                                     QStringLiteral("Invalid Response"), response.summary));
        return response;
    }

    response.status = LocalInferenceStatus::Succeeded;
    response.error = LocalInferenceError::None;
    response.text = text;
    response.approximateOutputTokens = approximateTokenCount(text);
    response.summary =
        QStringLiteral("Local inference completed through LM Studio /v1/chat/completions.");
    response.traces.append(trace(3, QStringLiteral("Generation"), QStringLiteral("Succeeded"),
                                 QStringLiteral("Local LM Studio generation completed.")));
    return response;
}

QString LMStudioLocalInferenceClient::statusSummary() const {
    return endpointAllowed()
               ? QStringLiteral("LM Studio local inference boundary is configured for "
                                "loopback-only /v1/chat/completions.")
               : QStringLiteral("LM Studio local inference boundary is blocked by "
                                "endpoint policy.");
}

QUrl LMStudioLocalInferenceClient::endpointUrl(const QString& path) const {
    QUrl url = config_.endpoint;
    QString basePath = url.path();
    if (basePath == QStringLiteral("/"))
        basePath.clear();
    if (basePath.endsWith(QLatin1Char('/')))
        basePath.chop(1);
    QString addPath = path;
    if (!addPath.startsWith(QLatin1Char('/')))
        addPath.prepend(QLatin1Char('/'));
    url.setPath(basePath + addPath);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

bool LMStudioLocalInferenceClient::endpointAllowed() const {
    return config_.isAllowedEndpoint();
}

LMStudioLocalInferenceStreamClient::LMStudioLocalInferenceStreamClient(LMStudioConfig config,
                                                                       int timeoutMs)
    : config_(std::move(config)), timeoutMs_(timeoutMs) {
    config_.timeoutMs = timeoutMs_;
}

LocalInferenceStreamResult LMStudioLocalInferenceStreamClient::startStream(
    const LocalInferenceRequest& request,
    const std::function<void(const LocalInferenceStreamChunk&)>& onChunk) {
    LocalInferenceStreamResult result;
    result.requestId = ProviderRequestRuntime::requestId();
    const QString providerHost = config_.endpoint.host().toLower();
    QString modelName = request.options.model.trimmed();

    if (providerHost.contains(QLatin1String("googleapis.com"))) {
        if (modelName.isEmpty() || !modelName.startsWith(QLatin1String("gemini-"))) {
            modelName = QStringLiteral("gemini-2.0-flash");
        }
    } else if (providerHost.contains(QLatin1String("anthropic.com"))) {
        if (modelName.isEmpty() || !modelName.startsWith(QLatin1String("claude-"))) {
            modelName = QStringLiteral("claude-3-5-sonnet-20241022");
        }
    } else if (providerHost.contains(QLatin1String("deepseek.com"))) {
        if (modelName.isEmpty() || !modelName.startsWith(QLatin1String("deepseek-"))) {
            modelName = QStringLiteral("deepseek-chat");
        }
    } else if (providerHost.contains(QLatin1String("groq.com"))) {
        if (modelName.isEmpty() || (!modelName.startsWith(QLatin1String("llama")) &&
                                    !modelName.startsWith(QLatin1String("mixtral")) &&
                                    !modelName.startsWith(QLatin1String("deepseek")))) {
            modelName = QStringLiteral("llama-3.3-70b-versatile");
        }
    } else if (providerHost.contains(QLatin1String("mistral.ai"))) {
        if (modelName.isEmpty() || (!modelName.startsWith(QLatin1String("mistral")) &&
                                    !modelName.startsWith(QLatin1String("pixtral")) &&
                                    !modelName.startsWith(QLatin1String("codestral")))) {
            modelName = QStringLiteral("mistral-large-latest");
        }
    } else if (providerHost.contains(QLatin1String("openai.com"))) {
        if (modelName.isEmpty() || (!modelName.startsWith(QLatin1String("gpt-")) &&
                                    !modelName.startsWith(QLatin1String("o1")) &&
                                    !modelName.startsWith(QLatin1String("o3")))) {
            modelName = QStringLiteral("gpt-4o");
        }
    }

    result.model = modelName;
    result.endpoint = config_.toString();
    result.timeoutMs =
        request.options.timeoutMs > 0 ? request.options.timeoutMs : config_.timeoutMs;
    result.traces.append(
        trace(1, QStringLiteral("Request"), QStringLiteral("Received"),
              QStringLiteral("%1 streaming request reached boundary with %2 ms timeout metadata.")
                  .arg(config_.providerDisplayName())
                  .arg(result.timeoutMs)));

    if (request.prompt.trimmed().isEmpty()) {
        result.status = LocalInferenceStreamStatus::Refused;
        result.lifecycle = ChatRequestLifecycle::Failed;
        result.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::RequestRejected);
        result.error = LocalInferenceError::BlankPrompt;
        result.summary = QStringLiteral("Local streaming request rejected: prompt is blank.");
        result.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Rejected"), result.summary));
        return result;
    }

    if (cancellationRequested(request)) {
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.lifecycle = ChatRequestLifecycle::Cancelled;
        result.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::Cancelled);
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local streaming request was cancelled before generation.");
        result.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Cancelled"), result.summary));
        return result;
    }

    if (!endpointAllowed()) {
        result.status = LocalInferenceStreamStatus::Refused;
        result.lifecycle = ChatRequestLifecycle::Failed;
        result.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::ProviderUnavailable);
        result.error = LocalInferenceError::EndpointBlocked;
        result.summary =
            config_.isCloud()
                ? QStringLiteral(
                      "%1 streaming blocked: API key is missing or endpoint is not allowed.")
                      .arg(config_.providerDisplayName())
                : QStringLiteral("Local streaming blocked: endpoint must be local loopback HTTP.");
        result.traces.append(
            trace(2, QStringLiteral("Endpoint"), QStringLiteral("Blocked"), result.summary));
        return result;
    }

    QNetworkAccessManager manager;
    QNetworkRequest networkRequest;
    QJsonObject body;

    const bool isAnthropic = config_.endpoint.host().contains(QLatin1String("anthropic.com"));
    const bool isGemini = config_.endpoint.host().contains(QLatin1String("googleapis.com"));

    if (isAnthropic) {
        networkRequest.setUrl(QUrl(QStringLiteral("https://api.anthropic.com/v1/messages")));
        if (!config_.apiKey.isEmpty()) {
            networkRequest.setRawHeader("x-api-key", config_.apiKey.toUtf8());
        }
        networkRequest.setRawHeader("anthropic-version", "2023-06-01");

        QJsonObject messageObj;
        messageObj.insert(QStringLiteral("role"), QStringLiteral("user"));
        messageObj.insert(QStringLiteral("content"), request.prompt.trimmed());
        QJsonArray messagesArr;
        messagesArr.append(messageObj);

        body.insert(QStringLiteral("model"), result.model);
        body.insert(QStringLiteral("messages"), messagesArr);
        body.insert(QStringLiteral("max_tokens"), 2048);
        body.insert(QStringLiteral("stream"), true);
    } else if (isGemini) {
        const QString modelId =
            result.model.isEmpty() ? QStringLiteral("gemini-2.0-flash") : result.model;
        networkRequest.setUrl(
            QUrl(QStringLiteral("https://generativelanguage.googleapis.com/v1beta/models/"
                                "%1:streamGenerateContent?key=%2&alt=sse")
                     .arg(modelId, config_.apiKey)));

        QJsonObject partObj;
        partObj.insert(QStringLiteral("text"), request.prompt.trimmed());
        QJsonArray partsArr;
        partsArr.append(partObj);

        QJsonObject contentObj;
        contentObj.insert(QStringLiteral("role"), QStringLiteral("user"));
        contentObj.insert(QStringLiteral("parts"), partsArr);
        QJsonArray contentsArr;
        contentsArr.append(contentObj);

        body.insert(QStringLiteral("contents"), contentsArr);

        QJsonObject generationConfig;
        generationConfig.insert(QStringLiteral("temperature"), request.options.temperature);
        generationConfig.insert(QStringLiteral("topP"), request.options.topP);
        generationConfig.insert(QStringLiteral("maxOutputTokens"), request.options.maxTokens);
        body.insert(QStringLiteral("generationConfig"), generationConfig);
    } else {
        networkRequest.setUrl(endpointUrl(config_.endpoint.path().endsWith(QLatin1String("v1"))
                                              ? QStringLiteral("/chat/completions")
                                              : QStringLiteral("/v1/chat/completions")));
        if (!config_.apiKey.isEmpty()) {
            networkRequest.setRawHeader("Authorization",
                                        QStringLiteral("Bearer %1").arg(config_.apiKey).toUtf8());
        }

        QJsonObject messageObj;
        messageObj.insert(QStringLiteral("role"), QStringLiteral("user"));
        messageObj.insert(QStringLiteral("content"), request.prompt.trimmed());
        QJsonArray messagesArr;
        messagesArr.append(messageObj);

        body.insert(QStringLiteral("model"), result.model);
        body.insert(QStringLiteral("messages"), messagesArr);
        body.insert(QStringLiteral("stream"), true);
    }
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                             QStringLiteral("application/json"));
    networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                QNetworkRequest::ManualRedirectPolicy);

    QByteArray pending;
    int sequence = 0;
    bool done = false;
    QElapsedTimer elapsed;
    elapsed.start();

    QNetworkReply* reply =
        manager.post(networkRequest, QJsonDocument(body).toJson(QJsonDocument::Compact));

    auto processLine = [&](const QByteArray& line) {
        const auto trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) {
            return;
        }

        if (!trimmedLine.startsWith("data:")) {
            return;
        }

        QByteArray dataPayload = trimmedLine.mid(5).trimmed();
        if (dataPayload == "[DONE]") {
            done = true;
            return;
        }

        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(dataPayload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            ++sequence;
            ++result.malformedChunkCount;
            LocalInferenceStreamChunk chunk{
                sequence, {}, false, true, QStringLiteral("Malformed stream chunk ignored."),
            };
            result.chunks.append(chunk);
            result.traces.append(trace(sequence + 2, QStringLiteral("Stream Chunk"),
                                       QStringLiteral("Malformed"), chunk.summary));
            if (onChunk) {
                onChunk(chunk);
            }
            return;
        }

        const auto object = document.object();
        QString text;
        bool isFinal = false;

        // OpenAI format (choices[0].delta.content)
        if (object.contains(QStringLiteral("choices"))) {
            const auto choices = object.value(QStringLiteral("choices")).toArray();
            if (!choices.isEmpty()) {
                const auto choice = choices.first().toObject();
                const auto delta = choice.value(QStringLiteral("delta")).toObject();
                text = delta.value(QStringLiteral("content")).toString();
                const auto finishReason = choice.value(QStringLiteral("finish_reason")).toString();
                if (!finishReason.isEmpty()) {
                    isFinal = true;
                    done = true;
                }
            }
        }
        // Anthropic Claude format
        else if (object.contains(QStringLiteral("delta")) ||
                 object.value(QStringLiteral("type")).toString() == QLatin1String("message_stop")) {
            const auto delta = object.value(QStringLiteral("delta")).toObject();
            text = delta.value(QStringLiteral("text")).toString();
            const auto msgType = object.value(QStringLiteral("type")).toString();
            const auto stopReason = delta.value(QStringLiteral("stop_reason")).toString();
            if (msgType == QLatin1String("message_stop") || !stopReason.isEmpty()) {
                isFinal = true;
                done = true;
            }
        }
        // Google Gemini format (candidates[0].content.parts[0].text)
        else if (object.contains(QStringLiteral("candidates"))) {
            const auto candidates = object.value(QStringLiteral("candidates")).toArray();
            if (!candidates.isEmpty()) {
                const auto candObj = candidates.first().toObject();
                const auto contentObj = candObj.value(QStringLiteral("content")).toObject();
                const auto parts = contentObj.value(QStringLiteral("parts")).toArray();
                if (!parts.isEmpty()) {
                    text = parts.first().toObject().value(QStringLiteral("text")).toString();
                }
                const auto finishReason = candObj.value(QStringLiteral("finishReason")).toString();
                if (!finishReason.isEmpty()) {
                    isFinal = true;
                    done = true; // Gemini does not send [DONE]; finishReason signals completion
                }
            }
        }

        const bool finalChunk = isFinal || done;

        if (text.isEmpty() && !finalChunk) {
            return;
        }

        ++sequence;
        done = done || finalChunk;
        result.accumulatedText.append(text);
        if (!text.isEmpty() && result.firstTokenLatencyMs < 0) {
            result.firstTokenLatencyMs = elapsed.elapsed();
        }
        LocalInferenceStreamChunk chunk{
            sequence,
            text,
            finalChunk,
            false,
            finalChunk ? QStringLiteral("Final local stream chunk received.")
                       : QStringLiteral("Local stream chunk received."),
        };
        result.chunks.append(chunk);
        if (onChunk) {
            onChunk(chunk);
        }
    };

    auto consumeReady = [&]() {
        pending.append(reply->readAll());
        while (true) {
            const auto newlineIndex = pending.indexOf('\n');
            if (newlineIndex < 0) {
                break;
            }
            const auto line = pending.left(newlineIndex);
            pending.remove(0, newlineIndex + 1);
            processLine(line);
        }
    };
    const auto timeoutMs = result.timeoutMs > 0 ? result.timeoutMs : timeoutMs_;
    result.status = LocalInferenceStreamStatus::Streaming;
    result.summary = QStringLiteral("Local LM Studio streaming generation is active.");
    result.lifecycle = ChatRequestLifecycle::Running;
    const auto transport = ProviderRequestRuntime::wait(
        reply, timeoutMs, request.options.cancellationToken, consumeReady);
    result.httpStatus = transport.httpStatus;
    result.providerErrorCategory = static_cast<int>(transport.category);
    result.lifecycle = transport.lifecycle;
    result.latencyMs = elapsed.elapsed();

    if (transport.cancelled || cancellationRequested(request)) {
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local LM Studio streaming generation was cancelled.");
        reply->deleteLater();
        return result;
    }

    if (transport.timedOut) {
        result.status = LocalInferenceStreamStatus::Error;
        result.error = LocalInferenceError::Timeout;
        result.summary =
            QStringLiteral("Local LM Studio streaming generation timed out after %1 ms.")
                .arg(timeoutMs);
        reply->deleteLater();
        return result;
    }

    pending.append(reply->readAll());
    if (!pending.trimmed().isEmpty()) {
        processLine(pending);
    }

    if (hasRedirectStatus(reply)) {
        result.status = LocalInferenceStreamStatus::Error;
        result.lifecycle = ChatRequestLifecycle::Failed;
        result.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::RequestRejected);
        result.error = LocalInferenceError::EndpointBlocked;
        result.summary =
            config_.isCloud()
                ? QStringLiteral("%1 streaming blocked: unexpected redirect from API endpoint.")
                      .arg(config_.providerDisplayName())
                : QStringLiteral("Local LM Studio streaming blocked: redirects are not allowed.");
        reply->deleteLater();
        return result;
    }

    if (transport.category != ChatProviderErrorCategory::None) {
        result.status = LocalInferenceStreamStatus::Error;
        result.error = LocalInferenceError::RequestFailed;
        result.summary = QStringLiteral("%1 streaming failed: %2%3")
                             .arg(config_.providerDisplayName(),
                                  chatProviderErrorCategoryName(transport.category),
                                  transport.httpStatus > 0
                                      ? QStringLiteral(" (HTTP %1)").arg(transport.httpStatus)
                                      : QString{});
        reply->deleteLater();
        return result;
    }

    reply->deleteLater();

    if (cancellationRequested(request)) {
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.lifecycle = ChatRequestLifecycle::Cancelled;
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local LM Studio streaming generation was cancelled.");
        return result;
    }

    if (!done || result.accumulatedText.trimmed().isEmpty()) {
        result.status = LocalInferenceStreamStatus::Error;
        result.lifecycle = ChatRequestLifecycle::Failed;
        result.providerErrorCategory = static_cast<int>(ChatProviderErrorCategory::MalformedResponse);
        result.error =
            done ? LocalInferenceError::InvalidResponse : LocalInferenceError::StreamInterrupted;
        result.summary =
            config_.isCloud()
                ? QStringLiteral("%1 streaming response did not contain any text.")
                      .arg(config_.providerDisplayName())
                : QStringLiteral(
                      "Local LM Studio streaming response did not complete with assistant text.");
        return result;
    }

    result.status = LocalInferenceStreamStatus::Completed;
    result.error = LocalInferenceError::None;
    result.approximateOutputTokens = approximateTokenCount(result.accumulatedText);
    result.approximateTokensPerSecond =
        tokensPerSecond(result.approximateOutputTokens, result.latencyMs);
    result.summary =
        result.malformedChunkCount > 0
            ? QStringLiteral(
                  "Local LM Studio streaming completed with %1 malformed chunk(s) ignored.")
                  .arg(result.malformedChunkCount)
            : QStringLiteral("Local LM Studio streaming completed.");
    result.traces.append(trace(static_cast<int>(result.traces.size()) + 1,
                               QStringLiteral("Stream Generation"), QStringLiteral("Completed"),
                               result.summary));
    return result;
}

QString LMStudioLocalInferenceStreamClient::statusSummary() const {
    return endpointAllowed()
               ? QStringLiteral("LM Studio local streaming boundary is configured for "
                                "loopback-only /v1/chat/completions.")
               : QStringLiteral("LM Studio local streaming boundary is blocked by "
                                "endpoint policy.");
}

bool LMStudioLocalInferenceStreamClient::isAvailable() const {
    return endpointAllowed();
}

QUrl LMStudioLocalInferenceStreamClient::endpointUrl(const QString& path) const {
    QUrl url = config_.endpoint;
    QString basePath = url.path();
    if (basePath == QStringLiteral("/"))
        basePath.clear();
    if (basePath.endsWith(QLatin1Char('/')))
        basePath.chop(1);
    QString addPath = path;
    if (!addPath.startsWith(QLatin1Char('/')))
        addPath.prepend(QLatin1Char('/'));
    url.setPath(basePath + addPath);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

bool LMStudioLocalInferenceStreamClient::endpointAllowed() const {
    return config_.isAllowedEndpoint();
}

} // namespace sentinel::core
