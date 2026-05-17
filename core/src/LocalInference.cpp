#include "sentinel/core/LocalInference.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QVariant>

#include <functional>
#include <utility>

namespace sentinel::core {

namespace {

struct JsonReply {
    bool ok = false;
    bool timedOut = false;
    QJsonDocument document;
    QString error;
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
        return JsonReply{false, true, {}, QStringLiteral("Ollama local request timed out.")};
    }

    if (reply->error() != QNetworkReply::NoError) {
        const auto error = reply->errorString();
        reply->deleteLater();
        return JsonReply{false, false, {}, error};
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
                                        "JSON.")};
    }

    return JsonReply{true, false, document, {}};
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
        return JsonReply{false, true, {}, QStringLiteral("Ollama local generation timed out.")};
    }

    if (reply->error() != QNetworkReply::NoError) {
        const auto error = reply->errorString();
        reply->deleteLater();
        return JsonReply{false, false, {}, error};
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
                                        "not valid JSON.")};
    }

    return JsonReply{true, false, document, {}};
}

bool hasRedirectStatus(QNetworkReply* reply) {
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return status >= 300 && status < 400;
}

LocalInferenceTrace trace(int sequence, const QString& stage, const QString& status,
                          const QString& summary) {
    return LocalInferenceTrace{sequence, stage, status, summary};
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
    case LocalInferenceError::RequestFailed:
        return QStringLiteral("Request Failed");
    case LocalInferenceError::Timeout:
        return QStringLiteral("Timeout");
    case LocalInferenceError::InvalidResponse:
        return QStringLiteral("Invalid Response");
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

OllamaLocalInferenceClient::OllamaLocalInferenceClient(OllamaConfig config, int timeoutMs)
    : config_(std::move(config)), timeoutMs_(timeoutMs) {}

LocalInferenceResponse OllamaLocalInferenceClient::infer(const LocalInferenceRequest& request) {
    LocalInferenceResponse response;
    response.endpoint = config_.endpoint.toString();
    response.model = request.options.model.trimmed();
    response.traces.append(
        trace(1, QStringLiteral("Request"), QStringLiteral("Received"),
              QStringLiteral("Local inference request reached Ollama boundary.")));

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

    if (request.options.cancellationRequested) {
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

    const auto timeoutMs = request.options.timeoutMs > 0 ? request.options.timeoutMs : timeoutMs_;
    if (!hasInstalledModel(response.model, timeoutMs)) {
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

    response.traces.append(trace(2, QStringLiteral("Generation"), QStringLiteral("Started"),
                                 QStringLiteral("Calling local Ollama /api/generate without "
                                                "streaming.")));
    const auto reply = postJson(endpointUrl(QStringLiteral("/api/generate")), body, timeoutMs);
    if (!reply.ok) {
        response.status = LocalInferenceStatus::Error;
        response.error =
            reply.timedOut ? LocalInferenceError::Timeout : LocalInferenceError::RequestFailed;
        response.summary = QStringLiteral("Local Ollama generation failed: %1").arg(reply.error);
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

QList<OllamaModelSummary> OllamaLocalInferenceClient::installedModels(int timeoutMs) const {
    if (!config_.modelDiscoveryEnabled || !endpointAllowed()) {
        return {};
    }

    const auto reply = getJson(endpointUrl(QStringLiteral("/api/tags")), timeoutMs);
    if (!reply.ok) {
        return {};
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
    return models;
}

bool OllamaLocalInferenceClient::hasInstalledModel(const QString& model, int timeoutMs) const {
    for (const auto& installedModel : installedModels(timeoutMs)) {
        if (installedModel.name == model) {
            return true;
        }
    }
    return false;
}

OllamaLocalInferenceStreamClient::OllamaLocalInferenceStreamClient(OllamaConfig config,
                                                                   int timeoutMs)
    : config_(std::move(config)), timeoutMs_(timeoutMs) {}

LocalInferenceStreamResult OllamaLocalInferenceStreamClient::startStream(
    const LocalInferenceRequest& request,
    const std::function<void(const LocalInferenceStreamChunk&)>& onChunk) {
    LocalInferenceStreamResult result;
    result.status = LocalInferenceStreamStatus::NotStarted;
    result.model = request.options.model.trimmed();
    result.endpoint = config_.endpoint.toString();
    result.traces.append(trace(1, QStringLiteral("Stream Request"), QStringLiteral("Received"),
                               QStringLiteral("Local streaming request reached Ollama boundary.")));

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

    if (request.options.cancellationRequested) {
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

    QNetworkAccessManager manager;
    QNetworkRequest networkRequest{endpointUrl(QStringLiteral("/api/generate"))};
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                             QStringLiteral("application/json"));
    networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                QNetworkRequest::ManualRedirectPolicy);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QByteArray pending;
    int sequence = 0;
    bool done = false;

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
    timer.start(request.options.timeoutMs > 0 ? request.options.timeoutMs : timeoutMs_);
    result.status = LocalInferenceStreamStatus::Streaming;
    result.summary = QStringLiteral("Local Ollama streaming generation is active.");
    loop.exec();

    if (timer.isActive()) {
        timer.stop();
    } else {
        reply->abort();
        result.status = LocalInferenceStreamStatus::Error;
        result.error = LocalInferenceError::Timeout;
        result.summary = QStringLiteral("Local Ollama streaming generation timed out.");
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
        result.error = LocalInferenceError::RequestFailed;
        result.summary = QStringLiteral("Local Ollama streaming generation failed: %1").arg(error);
        reply->deleteLater();
        return result;
    }

    reply->deleteLater();

    if (request.options.cancellationRequested) {
        result.status = LocalInferenceStreamStatus::Cancelled;
        result.error = LocalInferenceError::RequestFailed;
        result.cancelled = true;
        result.summary = QStringLiteral("Local Ollama streaming generation was cancelled.");
        return result;
    }

    if (!done || result.accumulatedText.trimmed().isEmpty()) {
        result.status = LocalInferenceStreamStatus::Error;
        result.error = LocalInferenceError::InvalidResponse;
        result.summary = QStringLiteral("Local Ollama streaming response did not complete with "
                                        "assistant text.");
        return result;
    }

    result.status = LocalInferenceStreamStatus::Completed;
    result.error = LocalInferenceError::None;
    result.summary = result.malformedChunkCount > 0
                         ? QStringLiteral("Local Ollama streaming completed with %1 malformed "
                                          "chunk(s) ignored.")
                               .arg(result.malformedChunkCount)
                         : QStringLiteral("Local Ollama streaming completed.");
    result.traces.append(trace(result.traces.size() + 1, QStringLiteral("Stream Generation"),
                               QStringLiteral("Completed"), result.summary));
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

} // namespace sentinel::core
