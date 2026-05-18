#include "sentinel/core/OllamaRuntime.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QVariant>

#include <utility>

namespace sentinel::core {

namespace {

constexpr auto defaultOllamaEndpoint = "http://127.0.0.1:11434";

bool isLoopbackHost(const QString& host) {
    const auto normalized = host.trimmed().toLower();
    return normalized == QStringLiteral("127.0.0.1") || normalized == QStringLiteral("localhost") ||
           normalized == QStringLiteral("::1") || normalized == QStringLiteral("[::1]");
}

QUrl normalizedOllamaUrl(const QString& endpoint, bool* valid) {
    QUrl url = QUrl::fromUserInput(endpoint.trimmed());
    if (endpoint.trimmed().isEmpty()) {
        url = QUrl(QString::fromLatin1(defaultOllamaEndpoint));
    }

    const auto allowed = url.isValid() && url.scheme() == QStringLiteral("http") &&
                         isLoopbackHost(url.host()) && !url.hasQuery() && !url.hasFragment();
    if (!allowed) {
        if (valid) {
            *valid = false;
        }
        return QUrl(QString::fromLatin1(defaultOllamaEndpoint));
    }

    url.setPath(QString());
    if (url.port() < 0) {
        url.setPort(11434);
    }
    if (valid) {
        *valid = true;
    }
    return url;
}

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
                         QStringLiteral("Ollama local response was not valid JSON."),
                         QNetworkReply::UnknownContentError};
    }

    return JsonReply{true, false, document, {}, QNetworkReply::NoError};
}

QString safeOllamaNetworkFailureSummary(const JsonReply& reply, const QString& operation,
                                        int timeoutMs) {
    if (reply.timedOut) {
        return QStringLiteral("%1 timed out after %2 ms.").arg(operation).arg(timeoutMs);
    }

    if (reply.networkError == QNetworkReply::ConnectionRefusedError) {
        return QStringLiteral("%1 failed: Ollama is not running or is not listening locally.")
            .arg(operation);
    }

    if (reply.networkError == QNetworkReply::HostNotFoundError ||
        reply.networkError == QNetworkReply::NetworkSessionFailedError ||
        reply.networkError == QNetworkReply::TemporaryNetworkFailureError) {
        return QStringLiteral("%1 failed: local Ollama endpoint is unreachable.").arg(operation);
    }

    return QStringLiteral("%1 failed: %2").arg(operation, reply.error);
}

QString sizeSummary(qint64 sizeBytes) {
    if (sizeBytes <= 0) {
        return QStringLiteral("size unavailable");
    }

    constexpr qint64 kib = 1024;
    constexpr qint64 mib = kib * 1024;
    constexpr qint64 gib = mib * 1024;
    if (sizeBytes >= gib) {
        return QStringLiteral("%1 GiB").arg(
            QString::number(static_cast<double>(sizeBytes) / static_cast<double>(gib), 'f', 1));
    }
    if (sizeBytes >= mib) {
        return QStringLiteral("%1 MiB").arg(
            QString::number(static_cast<double>(sizeBytes) / static_cast<double>(mib), 'f', 1));
    }
    if (sizeBytes >= kib) {
        return QStringLiteral("%1 KiB").arg(
            QString::number(static_cast<double>(sizeBytes) / static_cast<double>(kib), 'f', 1));
    }
    return QStringLiteral("%1 B").arg(sizeBytes);
}

} // namespace

OllamaEndpoint OllamaEndpoint::defaultEndpoint() {
    return OllamaEndpoint{QUrl(QString::fromLatin1(defaultOllamaEndpoint)), true, false};
}

OllamaEndpoint OllamaEndpoint::fromUserInput(const QString& endpoint) {
    bool valid = true;
    const auto url = normalizedOllamaUrl(endpoint, &valid);
    return OllamaEndpoint{url, valid, !valid};
}

QString OllamaEndpoint::toString() const {
    auto text = url.toString();
    if (text.endsWith(QLatin1Char('/'))) {
        text.chop(1);
    }
    return text;
}

bool OllamaEndpoint::isLoopbackHttp() const {
    return valid && url.scheme() == QStringLiteral("http") && isLoopbackHost(url.host()) &&
           !url.hasQuery() && !url.hasFragment();
}

OllamaConfig OllamaConfig::fromEndpoint(const QString& endpoint) {
    OllamaConfig config;
    config.endpoint = OllamaEndpoint::fromUserInput(endpoint);
    return config;
}

QString ollamaModelSummary(const OllamaModelSummary& model) {
    QStringList details;
    details.append(sizeSummary(model.sizeBytes));
    if (!model.modifiedAt.isEmpty()) {
        details.append(QStringLiteral("modified %1").arg(model.modifiedAt));
    }
    details.append(QStringLiteral("Local Only"));
    return QStringLiteral("%1 (%2)").arg(model.name, details.join(QStringLiteral(", ")));
}

QStringList ollamaModelSummaries(const QList<OllamaModelSummary>& models) {
    QStringList summaries;
    for (const auto& model : models) {
        summaries.append(ollamaModelSummary(model));
    }
    return summaries;
}

QString safeOllamaHealthSummary(const OllamaHealthCheckResult& result) {
    if (!result.summary.isEmpty()) {
        return result.summary;
    }
    return QStringLiteral("Ollama health is %1 at %2.")
        .arg(ollamaHealthStatusName(result.healthStatus), result.endpoint);
}

NullOllamaRuntimeClient::NullOllamaRuntimeClient(OllamaConfig config)
    : config_(std::move(config)) {}

OllamaConfig NullOllamaRuntimeClient::config() const {
    return config_;
}

OllamaHealthCheckResult NullOllamaRuntimeClient::healthCheck() const {
    return OllamaHealthCheckResult{
        OllamaConnectionStatus::Unavailable,
        OllamaHealthStatus::Unavailable,
        config_.endpoint.toString(),
        QStringLiteral("Ollama runtime client is unavailable; no local health check was "
                       "performed."),
        config_.healthCheckTimeoutMs,
    };
}

QList<OllamaModelSummary> NullOllamaRuntimeClient::installedModels() const {
    return {};
}

OllamaHttpRuntimeClient::OllamaHttpRuntimeClient(OllamaConfig config, int timeoutMs)
    : config_(std::move(config)), timeoutMs_(timeoutMs) {
    config_.healthCheckTimeoutMs = timeoutMs_;
    config_.modelDiscoveryTimeoutMs = timeoutMs_;
}

OllamaConfig OllamaHttpRuntimeClient::config() const {
    return config_;
}

OllamaHealthCheckResult OllamaHttpRuntimeClient::healthCheck() const {
    if (!endpointAllowed()) {
        return OllamaHealthCheckResult{
            OllamaConnectionStatus::Blocked,
            OllamaHealthStatus::InvalidEndpoint,
            config_.endpoint.toString(),
            QStringLiteral("Ollama health check blocked: endpoint must be local loopback HTTP."),
            config_.healthCheckTimeoutMs,
        };
    }

    const auto timeoutMs =
        config_.healthCheckTimeoutMs > 0 ? config_.healthCheckTimeoutMs : timeoutMs_;
    const auto reply = getJson(endpointUrl(QStringLiteral("/api/version")), timeoutMs);
    if (!reply.ok) {
        return OllamaHealthCheckResult{
            OllamaConnectionStatus::Unavailable,
            OllamaHealthStatus::Error,
            config_.endpoint.toString(),
            safeOllamaNetworkFailureSummary(reply, QStringLiteral("Ollama local health check"),
                                            timeoutMs),
            timeoutMs,
        };
    }

    return OllamaHealthCheckResult{
        OllamaConnectionStatus::Connected,
        OllamaHealthStatus::Healthy,
        config_.endpoint.toString(),
        QStringLiteral("Ollama local endpoint is reachable; no prompt or model execution was "
                       "performed."),
        timeoutMs,
    };
}

QList<OllamaModelSummary> OllamaHttpRuntimeClient::installedModels() const {
    if (!config_.modelDiscoveryEnabled || !endpointAllowed()) {
        return {};
    }

    const auto timeoutMs =
        config_.modelDiscoveryTimeoutMs > 0 ? config_.modelDiscoveryTimeoutMs : timeoutMs_;
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

QUrl OllamaHttpRuntimeClient::endpointUrl(const QString& path) const {
    QUrl url = config_.endpoint.url;
    url.setPath(path);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

bool OllamaHttpRuntimeClient::endpointAllowed() const {
    return config_.healthCheckEnabled && config_.endpoint.isLoopbackHttp();
}

} // namespace sentinel::core
