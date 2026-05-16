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
        return JsonReply{false, {}, QStringLiteral("Ollama local request timed out.")};
    }

    if (reply->error() != QNetworkReply::NoError) {
        const auto error = reply->errorString();
        reply->deleteLater();
        return JsonReply{false, {}, error};
    }

    const auto payload = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return JsonReply{false, {}, QStringLiteral("Ollama local response was not valid JSON.")};
    }

    return JsonReply{true, document, {}};
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
    if (model.modifiedAt.isEmpty()) {
        return model.name;
    }
    return QStringLiteral("%1 (%2)").arg(model.name, model.modifiedAt);
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
    };
}

QList<OllamaModelSummary> NullOllamaRuntimeClient::installedModels() const {
    return {};
}

OllamaHttpRuntimeClient::OllamaHttpRuntimeClient(OllamaConfig config, int timeoutMs)
    : config_(std::move(config)), timeoutMs_(timeoutMs) {}

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
        };
    }

    const auto reply = getJson(endpointUrl(QStringLiteral("/api/version")), timeoutMs_);
    if (!reply.ok) {
        return OllamaHealthCheckResult{
            OllamaConnectionStatus::Unavailable,
            OllamaHealthStatus::Error,
            config_.endpoint.toString(),
            QStringLiteral("Ollama local health check failed: %1").arg(reply.error),
        };
    }

    return OllamaHealthCheckResult{
        OllamaConnectionStatus::Connected,
        OllamaHealthStatus::Healthy,
        config_.endpoint.toString(),
        QStringLiteral("Ollama local endpoint is reachable; no prompt or model execution was "
                       "performed."),
    };
}

QList<OllamaModelSummary> OllamaHttpRuntimeClient::installedModels() const {
    if (!config_.modelDiscoveryEnabled || !endpointAllowed()) {
        return {};
    }

    const auto reply = getJson(endpointUrl(QStringLiteral("/api/tags")), timeoutMs_);
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
