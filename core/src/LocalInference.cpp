#include "sentinel/core/LocalInference.h"

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
};

JsonReply getJson(const QUrl& url, int timeoutMs) {
    QNetworkAccessManager manager;
    QNetworkRequest request{url};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QNetworkReply* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (timer.isActive()) {
        timer.stop();
    } else {
        reply->abort();
        reply->deleteLater();
        return JsonReply{false,
                         true,
                         {},
                         QStringLiteral("Ollama local request timed out."),
                         QNetworkReply::TimeoutError};
    }

    if (reply->error() != QNetworkReply::NoError) {
        const auto error = reply->errorString();
        const auto networkError = reply->error();
        reply->deleteLater();
        return JsonReply{false, false, {}, error, networkError};
    }

    const auto payload = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return JsonReply{false,
                         false,
                         {},
                         QStringLiteral("Ollama local response was not valid "
                                        "JSON."),
                         QNetworkReply::UnknownContentError};
    }

    return JsonReply{true, false, document, {}, QNetworkReply::NoError};
}

JsonReply postJson(const QUrl& url, const QJsonObject& body, int timeoutMs) {
    QNetworkAccessManager manager;
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QNetworkReply* reply =
        manager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (timer.isActive()) {
        timer.stop();
    } else {
        reply->abort();
        reply->deleteLater();
        return JsonReply{false,
                         true,
                         {},
                         QStringLiteral("Ollama local generation timed out."),
                         QNetworkReply::TimeoutError};
    }

    if (reply->error() != QNetworkReply::NoError) {
        const auto error = reply->errorString();
        const auto networkError = reply->error();
        reply->deleteLater();
        return JsonReply{false, false, {}, error, networkError};
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

bool cancellationRequested(const LocalInferenceRequest& request) {
    return request.options.cancellationRequested ||
           (request.options.cancellationToken && request.options.cancellationToken->load());
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
            [this, &requestWithToken, &onChunk](const LocalInferenceStreamChunk& chunk) {
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

    if (response.model.isEmpty()) {
        response.status = LocalInferenceStatus::InvalidRequest;
        response.error = LocalInferenceError::MissingModel;
        response.summary = QStringLiteral("Local inference request rejected: model is required.");
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
    const auto modelLookup = installedModels(timeoutMs);
    if (!modelLookup.ok) {
        response.status = LocalInferenceStatus::Error;
        response.error = networkErrorCategory(JsonReply{
            false, modelLookup.timedOut, {}, modelLookup.error, modelLookup.networkError});
        response.summary = safeNetworkFailureSummary(
            JsonReply{false, modelLookup.timedOut, {}, modelLookup.error, modelLookup.networkError},
            QStringLiteral("Local Ollama model discovery"), timeoutMs);
        response.traces.append(
            trace(2, QStringLiteral("Model Discovery"), QStringLiteral("Error"), response.summary));
        return response;
    }

    bool modelAvailable = false;
    for (const auto& installedModel : modelLookup.models) {
        if (installedModel.name == response.model) {
            modelAvailable = true;
            break;
        }
    }
    if (!modelAvailable) {
        response.status = LocalInferenceStatus::ModelUnavailable;
        response.error = LocalInferenceError::ModelUnavailable;
        response.summary =
            QStringLiteral("Local inference request rejected: model is not installed.");
        response.traces.append(trace(2, QStringLiteral("Model Discovery"),
                                     QStringLiteral("Unavailable"), response.summary));
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
    const auto reply = postJson(endpointUrl(QStringLiteral("/api/generate")), body, timeoutMs);
    if (!reply.ok) {
        response.status = LocalInferenceStatus::Error;
        response.error = networkErrorCategory(reply);
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

ModelLookupReply OllamaLocalInferenceClient::installedModels(int timeoutMs) const {
    if (!config_.modelDiscoveryEnabled || !endpointAllowed()) {
        return ModelLookupReply{true, false, {}, {}, QNetworkReply::NoError};
    }

    const auto reply = getJson(endpointUrl(QStringLiteral("/api/tags")), timeoutMs);
    if (!reply.ok) {
        return ModelLookupReply{false, reply.timedOut, {}, reply.error, reply.networkError};
    }

    QList<OllamaModelSummary> models;
    const auto modelValues = reply.document.object().value(QStringLiteral("models")).toArray();
    for (const auto& value : modelValues) {
        const auto object = value.toObject();
        const auto name = object.value(QStringLiteral("name")).toString().trimmed();
        if (name.isEmpty()) {
            continue;
        }
        models.append(OllamaModelSummary{
            name,
            object.value(QStringLiteral("modified_at")).toString(),
            object.value(QStringLiteral("size")).toVariant().toLongLong(),
        });
    }
    return ModelLookupReply{true, false, models, {}, QNetworkReply::NoError};
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
        result.error = LocalInferenceError::BlankPrompt;
        result.summary = QStringLiteral("Local streaming request rejected: prompt is blank.");
        result.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Rejected"), result.summary));
        return result;
    }

    if (result.model.isEmpty()) {
        result.status = LocalInferenceStreamStatus::Refused;
        result.error = LocalInferenceError::MissingModel;
        result.summary = QStringLiteral("Local streaming request rejected: model is required.");
        result.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Rejected"), result.summary));
        return result;
    }

    if (cancellationRequested(request)) {
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local streaming request was cancelled before generation.");
        result.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Cancelled"), result.summary));
        return result;
    }

    if (!endpointAllowed()) {
        result.status = LocalInferenceStreamStatus::Refused;
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

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QTimer cancellationTimer;
    cancellationTimer.setInterval(25);

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

    QObject::connect(reply, &QNetworkReply::readyRead, &loop, [&]() {
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
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&cancellationTimer, &QTimer::timeout, &loop, [&]() {
        if (cancellationRequested(request)) {
            reply->abort();
            loop.quit();
        }
    });
    const auto timeoutMs = result.timeoutMs > 0 ? result.timeoutMs : timeoutMs_;
    timer.start(timeoutMs);
    cancellationTimer.start();
    result.status = LocalInferenceStreamStatus::Streaming;
    result.summary = QStringLiteral("Local Ollama streaming generation is active.");
    loop.exec();
    cancellationTimer.stop();
    result.latencyMs = elapsed.elapsed();

    if (cancellationRequested(request)) {
        reply->abort();
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local Ollama streaming generation was cancelled.");
        reply->deleteLater();
        return result;
    }

    if (timer.isActive()) {
        timer.stop();
    } else {
        reply->abort();
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
        result.error = LocalInferenceError::EndpointBlocked;
        result.summary = QStringLiteral("Local Ollama streaming blocked: redirects are not "
                                        "allowed.");
        reply->deleteLater();
        return result;
    }

    if (reply->error() != QNetworkReply::NoError) {
        const auto error = reply->errorString();
        result.status = LocalInferenceStreamStatus::Error;
        const JsonReply failedReply{false, false, {}, error, reply->error()};
        result.error = networkErrorCategory(failedReply);
        result.summary = safeNetworkFailureSummary(
            failedReply, QStringLiteral("Local Ollama streaming generation"), timeoutMs);
        reply->deleteLater();
        return result;
    }

    reply->deleteLater();

    if (cancellationRequested(request)) {
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local Ollama streaming generation was cancelled.");
        return result;
    }

    if (!done || result.accumulatedText.trimmed().isEmpty()) {
        result.status = LocalInferenceStreamStatus::Error;
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
            QStringLiteral("Local inference blocked: endpoint must be local loopback HTTP.");
        response.traces.append(
            trace(2, QStringLiteral("Endpoint"), QStringLiteral("Blocked"), response.summary));
        return response;
    }

    const auto timeoutMs = response.timeoutMs > 0 ? response.timeoutMs : timeoutMs_;
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
        response.summary =
            QStringLiteral("Local inference request rejected: model is not loaded in LM Studio.");
        response.traces.append(trace(2, QStringLiteral("Model Discovery"),
                                     QStringLiteral("Unavailable"), response.summary));
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
        postJson(endpointUrl(QStringLiteral("/v1/chat/completions")), body, timeoutMs);
    if (!reply.ok) {
        response.status = LocalInferenceStatus::Error;
        response.error = networkErrorCategory(reply);
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
    url.setPath(path);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

bool LMStudioLocalInferenceClient::endpointAllowed() const {
    return config_.isLoopbackHttp();
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
    result.model = request.options.model.trimmed();
    result.endpoint = config_.toString();
    result.timeoutMs =
        request.options.timeoutMs > 0 ? request.options.timeoutMs : config_.timeoutMs;
    result.traces.append(
        trace(1, QStringLiteral("Request"), QStringLiteral("Received"),
              QStringLiteral("Local streaming request reached LM Studio boundary with %1 ms "
                             "timeout metadata.")
                  .arg(result.timeoutMs)));

    if (request.prompt.trimmed().isEmpty()) {
        result.status = LocalInferenceStreamStatus::Refused;
        result.error = LocalInferenceError::BlankPrompt;
        result.summary = QStringLiteral("Local streaming request rejected: prompt is blank.");
        result.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Rejected"), result.summary));
        return result;
    }

    if (cancellationRequested(request)) {
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local streaming request was cancelled before generation.");
        result.traces.append(
            trace(2, QStringLiteral("Validation"), QStringLiteral("Cancelled"), result.summary));
        return result;
    }

    if (!endpointAllowed()) {
        result.status = LocalInferenceStreamStatus::Refused;
        result.error = LocalInferenceError::EndpointBlocked;
        result.summary =
            QStringLiteral("Local streaming blocked: endpoint must be local loopback HTTP.");
        result.traces.append(
            trace(2, QStringLiteral("Endpoint"), QStringLiteral("Blocked"), result.summary));
        return result;
    }

    QJsonObject messageObj;
    messageObj.insert(QStringLiteral("role"), QStringLiteral("user"));
    messageObj.insert(QStringLiteral("content"), request.prompt.trimmed());

    QJsonArray messagesArr;
    messagesArr.append(messageObj);

    QJsonObject body;
    body.insert(QStringLiteral("model"), result.model);
    body.insert(QStringLiteral("messages"), messagesArr);
    body.insert(QStringLiteral("stream"), true);

    QNetworkAccessManager manager;
    QNetworkRequest networkRequest{endpointUrl(QStringLiteral("/v1/chat/completions"))};
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                             QStringLiteral("application/json"));
    networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                QNetworkRequest::ManualRedirectPolicy);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QTimer cancellationTimer;
    cancellationTimer.setInterval(25);

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
        const auto choices = object.value(QStringLiteral("choices")).toArray();
        if (choices.isEmpty()) {
            return;
        }
        const auto choice = choices.first().toObject();
        const auto delta = choice.value(QStringLiteral("delta")).toObject();
        const auto text = delta.value(QStringLiteral("content")).toString();

        const auto finishReason = choice.value(QStringLiteral("finish_reason")).toString();
        const bool finalChunk = !finishReason.isEmpty() || done;

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

    QObject::connect(reply, &QNetworkReply::readyRead, &loop, [&]() {
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
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&cancellationTimer, &QTimer::timeout, &loop, [&]() {
        if (cancellationRequested(request)) {
            reply->abort();
            loop.quit();
        }
    });
    const auto timeoutMs = result.timeoutMs > 0 ? result.timeoutMs : timeoutMs_;
    timer.start(timeoutMs);
    cancellationTimer.start();
    result.status = LocalInferenceStreamStatus::Streaming;
    result.summary = QStringLiteral("Local LM Studio streaming generation is active.");
    loop.exec();
    cancellationTimer.stop();
    result.latencyMs = elapsed.elapsed();

    if (cancellationRequested(request)) {
        reply->abort();
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local LM Studio streaming generation was cancelled.");
        reply->deleteLater();
        return result;
    }

    if (timer.isActive()) {
        timer.stop();
    } else {
        reply->abort();
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
        result.error = LocalInferenceError::EndpointBlocked;
        result.summary =
            QStringLiteral("Local LM Studio streaming blocked: redirects are not allowed.");
        reply->deleteLater();
        return result;
    }

    if (reply->error() != QNetworkReply::NoError) {
        const auto error = reply->errorString();
        result.status = LocalInferenceStreamStatus::Error;
        const JsonReply failedReply{false, false, {}, error, reply->error()};
        result.error = networkErrorCategory(failedReply);
        result.summary = safeNetworkFailureSummary(
            failedReply, QStringLiteral("Local LM Studio streaming generation"), timeoutMs);
        reply->deleteLater();
        return result;
    }

    reply->deleteLater();

    if (cancellationRequested(request)) {
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local LM Studio streaming generation was cancelled.");
        return result;
    }

    if (result.accumulatedText.trimmed().isEmpty()) {
        result.status = LocalInferenceStreamStatus::Error;
        result.error =
            done ? LocalInferenceError::InvalidResponse : LocalInferenceError::StreamInterrupted;
        result.summary = QStringLiteral(
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
    url.setPath(path);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

bool LMStudioLocalInferenceStreamClient::endpointAllowed() const {
    return config_.isLoopbackHttp();
}

} // namespace sentinel::core
