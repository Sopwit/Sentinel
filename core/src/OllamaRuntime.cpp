#include "sentinel/core/OllamaRuntime.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
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

// ── OllamaModelPuller implementation ─────────────────────────────────────────

OllamaModelPuller::OllamaModelPuller(QObject* parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))
{}

bool OllamaModelPuller::pulling() const    { return pulling_; }
QString OllamaModelPuller::activeModel() const { return activeModel_; }
qreal OllamaModelPuller::progress() const  { return progress_; }
QString OllamaModelPuller::statusText() const { return statusText_; }
QString OllamaModelPuller::errorText() const  { return errorText_; }

void OllamaModelPuller::pull(const QString& modelId) {
    if (pulling_) {
        cancel();
    }

    const QString trimmed = modelId.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    // Build request to local Ollama /api/pull
    QUrl url(QStringLiteral("http://127.0.0.1:11434/api/pull"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QByteArrayLiteral("application/json"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);

    const QJsonObject body { { QStringLiteral("name"), trimmed } };
    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);

    setState(true, trimmed, 0.0, QStringLiteral("Starting pull…"), QString());

    reply_ = nam_->post(request, payload);

    // Stream chunks as they arrive
    QObject::connect(reply_, &QNetworkReply::readyRead, this, [this]() {
        if (!reply_) return;
        const QByteArray data = reply_->readAll();
        // Ollama sends newline-delimited JSON — split and process each line
        const auto lines = data.split('\n');
        for (const auto& line : lines) {
            const auto trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                processChunk(trimmedLine);
            }
        }
    });

    QObject::connect(reply_, &QNetworkReply::finished, this, [this]() {
        if (!reply_) return;

        const bool networkError = (reply_->error() != QNetworkReply::NoError &&
                                   reply_->error() != QNetworkReply::OperationCanceledError);
        const bool cancelled = (reply_->error() == QNetworkReply::OperationCanceledError);
        const QString model = activeModel_;

        reply_->deleteLater();
        reply_ = nullptr;

        if (cancelled) {
            setState(false, model, 0.0,
                     QStringLiteral("Cancelled."), QString());
            emit pullFinished(model, false);
            return;
        }

        if (networkError) {
            setState(false, model, 0.0, QString(),
                     QStringLiteral("Pull failed: Ollama is not running or unreachable."));
            emit pullFinished(model, false);
            return;
        }

        // If progress reached 1.0, it's done; otherwise mark complete
        if (progress_ >= 1.0 || statusText_.contains(QStringLiteral("success"),
                                                       Qt::CaseInsensitive)) {
            setState(false, model, 1.0, QStringLiteral("Installed"), QString());
            emit pullFinished(model, true);
        } else {
            setState(false, model, progress_, statusText_, errorText_);
            emit pullFinished(model, progress_ >= 0.99);
        }
    });
}

void OllamaModelPuller::cancel() {
    if (reply_) {
        reply_->abort();
    }
}

void OllamaModelPuller::processChunk(const QByteArray& chunk) {
    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(chunk, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const auto obj = doc.object();
    const auto status = obj.value(QStringLiteral("status")).toString();
    const qint64 completed = obj.value(QStringLiteral("completed")).toVariant().toLongLong();
    const qint64 total    = obj.value(QStringLiteral("total")).toVariant().toLongLong();

    qreal newProgress = progress_;
    if (total > 0 && completed > 0) {
        newProgress = static_cast<qreal>(completed) / static_cast<qreal>(total);
    } else if (status.contains(QStringLiteral("success"), Qt::CaseInsensitive)) {
        newProgress = 1.0;
    }

    // Build a human-readable status
    QString displayStatus = status;
    if (!displayStatus.isEmpty()) {
        // Capitalise first letter
        displayStatus[0] = displayStatus[0].toUpper();
    }
    if (total > 0 && completed > 0 && total != completed) {
        const double pct = newProgress * 100.0;
        displayStatus = QStringLiteral("%1 — %2%")
                            .arg(displayStatus)
                            .arg(static_cast<int>(pct));
    }

    if (qAbs(newProgress - progress_) > 0.001 || displayStatus != statusText_) {
        pulling_ = true;
        progress_ = newProgress;
        statusText_ = displayStatus;
        errorText_.clear();
        emit progressChanged();
        emit statusTextChanged();
        if (!pulling_) {
            pulling_ = true;
            emit pullingChanged();
        }
    }
}

void OllamaModelPuller::setState(bool pulling, const QString& model, qreal progress,
                                  const QString& status, const QString& error) {
    bool changed = false;

    if (pulling_ != pulling) { pulling_ = pulling; emit pullingChanged(); changed = true; }
    if (activeModel_ != model) { activeModel_ = model; emit activeModelChanged(); changed = true; }
    if (qAbs(progress_ - progress) > 0.0001) { progress_ = progress; emit progressChanged(); changed = true; }
    if (statusText_ != status) { statusText_ = status; emit statusTextChanged(); changed = true; }
    if (errorText_ != error) { errorText_ = error; emit errorTextChanged(); changed = true; }

    Q_UNUSED(changed)
}

// ── OllamaLibraryFetcher implementation ───────────────────────────────────────

OllamaLibraryFetcher::OllamaLibraryFetcher(QObject* parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))
{}

bool OllamaLibraryFetcher::fetching() const { return fetching_; }
QVariantList OllamaLibraryFetcher::models() const { return models_; }
QString OllamaLibraryFetcher::errorText() const { return errorText_; }

void OllamaLibraryFetcher::fetch(const QString& sort) {
    if (fetching_) {
        cancel();
    }

    fetching_ = true;
    errorText_.clear();
    emit fetchingChanged();
    emit errorTextChanged();

    QString sortParam = sort.trimmed().toLower();
    if (sortParam != QStringLiteral("newest")) {
        sortParam = QStringLiteral("popular");
    }

    QUrl url(QStringLiteral("https://ollama.com/library?sort=") + sortParam);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (Sentinel Assistant)"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    reply_ = nam_->get(request);

    connect(reply_, &QNetworkReply::finished, this, &OllamaLibraryFetcher::handleReplyFinished);
}

void OllamaLibraryFetcher::cancel() {
    if (reply_) {
        reply_->abort();
    }
}

void OllamaLibraryFetcher::handleReplyFinished() {
    if (!reply_) return;

    QNetworkReply* r = reply_;
    reply_ = nullptr;
    r->deleteLater();

    fetching_ = false;
    emit fetchingChanged();

    if (r->error() != QNetworkReply::NoError) {
        if (r->error() == QNetworkReply::OperationCanceledError) {
            errorText_ = QStringLiteral("Fetch cancelled.");
        } else {
            errorText_ = QStringLiteral("Failed to fetch Ollama models: %1").arg(r->errorString());
        }
        emit errorTextChanged();
        emit fetchFinished(false);
        return;
    }

    const QString html = QString::fromUtf8(r->readAll());
    parseHtml(html);
    emit fetchFinished(true);
}

void OllamaLibraryFetcher::parseHtml(const QString& html) {
    QVariantList parsedList;
    int index = 0;
    while (true) {
        int nextLink = html.indexOf(QStringLiteral("href=\"/library/"), index);
        if (nextLink == -1) {
            break;
        }

        int startQuote = nextLink + 15;
        int endQuote = html.indexOf(QStringLiteral("\""), startQuote);
        if (endQuote == -1) {
            index = startQuote;
            continue;
        }

        QString modelPath = html.mid(startQuote, endQuote - startQuote);
        if (modelPath.contains('/') || modelPath.contains('?')) {
            index = endQuote;
            continue;
        }

        int liStart = html.lastIndexOf(QStringLiteral("<li"), nextLink);
        if (liStart == -1) liStart = nextLink;

        int liEnd = html.indexOf(QStringLiteral("</li>"), nextLink);
        if (liEnd == -1) liEnd = nextLink + 2000;

        QString block = html.mid(liStart, liEnd - liStart);
        index = liEnd;

        QString ollamaId = modelPath;

        // Name
        int nameStart = block.indexOf(QStringLiteral("class=\"group-hover:underline truncate\">"));
        QString name = ollamaId;
        if (nameStart != -1) {
            nameStart += 39;
            int nameEnd = block.indexOf(QStringLiteral("</span>"), nameStart);
            if (nameEnd != -1) {
                name = block.mid(nameStart, nameEnd - nameStart).trimmed();
            }
        }

        // Description
        int descStart = block.indexOf(QStringLiteral("class=\"max-w-lg break-words text-neutral-800 text-md\">"));
        if (descStart == -1) {
            descStart = block.indexOf(QStringLiteral("text-neutral-800"));
            if (descStart != -1) {
                descStart = block.indexOf(QStringLiteral(">"), descStart);
                if (descStart != -1) descStart += 1;
            }
        } else {
            descStart += 54;
        }
        QString description;
        if (descStart != -1) {
            int descEnd = block.indexOf(QStringLiteral("</p>"), descStart);
            if (descEnd != -1) {
                description = block.mid(descStart, descEnd - descStart).trimmed();
                description.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
            }
        }

        // Capabilities
        QStringList caps;
        int capIndex = 0;
        while (true) {
            int capStart = block.indexOf(QStringLiteral("x-test-capability"), capIndex);
            if (capStart == -1) break;
            capStart = block.indexOf(QStringLiteral(">"), capStart);
            if (capStart == -1) break;
            capStart += 1;
            int capEnd = block.indexOf(QStringLiteral("</span>"), capStart);
            if (capEnd == -1) break;
            QString cap = block.mid(capStart, capEnd - capStart).trimmed();
            if (!cap.isEmpty() && !caps.contains(cap)) {
                caps.append(cap);
            }
            capIndex = capEnd;
        }

        // Pull count
        QString pulls = QStringLiteral("—");
        int pullStart = block.indexOf(QStringLiteral("x-test-pull-count>"));
        if (pullStart != -1) {
            pullStart += 18;
            int pullEnd = block.indexOf(QStringLiteral("</span>"), pullStart);
            if (pullEnd != -1) {
                pulls = block.mid(pullStart, pullEnd - pullStart).trimmed();
            }
        }

        // Updated time
        QString updated = QStringLiteral("—");
        int updatedStart = block.indexOf(QStringLiteral("x-test-updated>"));
        if (updatedStart != -1) {
            updatedStart += 15;
            int updatedEnd = block.indexOf(QStringLiteral("</span>"), updatedStart);
            if (updatedEnd != -1) {
                updated = block.mid(updatedStart, updatedEnd - updatedStart).trimmed();
            }
        }

        QString category = QStringLiteral("LLM");
        if (caps.contains(QStringLiteral("vision")) || ollamaId.contains(QStringLiteral("llava")) || ollamaId.contains(QStringLiteral("bakllava"))) {
            category = QStringLiteral("Vision");
        } else if (caps.contains(QStringLiteral("thinking")) || ollamaId.contains(QStringLiteral("deepseek-r1")) || ollamaId.contains(QStringLiteral("phi4"))) {
            category = QStringLiteral("Think");
        } else if (ollamaId.contains(QStringLiteral("stable-diffusion")) || ollamaId.contains(QStringLiteral("flux"))) {
            category = QStringLiteral("Image");
        }

        QVariantMap modelObj;
        modelObj[QStringLiteral("id")] = ollamaId;
        modelObj[QStringLiteral("ollamaId")] = ollamaId;
        modelObj[QStringLiteral("category")] = category;
        modelObj[QStringLiteral("name")] = name;
        modelObj[QStringLiteral("provider")] = QStringLiteral("Ollama Library");
        modelObj[QStringLiteral("size")] = pulls + QStringLiteral(" pulls");
        modelObj[QStringLiteral("description")] = description;

        QString badge = caps.isEmpty() ? QStringLiteral("Ollama") : caps.first();
        QString badgeColor = QStringLiteral("#7c3aed");
        if (badge == QStringLiteral("tools")) {
            badgeColor = QStringLiteral("#10b981");
        } else if (badge == QStringLiteral("thinking")) {
            badgeColor = QStringLiteral("#2563eb");
        } else if (badge == QStringLiteral("vision")) {
            badgeColor = QStringLiteral("#e05fc4");
        }
        modelObj[QStringLiteral("badge")] = badge;
        modelObj[QStringLiteral("badgeColor")] = badgeColor;

        QStringList tagsList = caps;
        if (updated != QStringLiteral("—")) {
            tagsList.append(updated);
        }
        modelObj[QStringLiteral("tags")] = tagsList;
        modelObj[QStringLiteral("downloadable")] = true;

        parsedList.append(modelObj);
    }

    models_ = parsedList;
    emit modelsChanged();
}

// ── OllamaModelDetailFetcher implementation ───────────────────────────────────

OllamaModelDetailFetcher::OllamaModelDetailFetcher(QObject* parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))
{}

bool OllamaModelDetailFetcher::fetching() const { return fetching_; }
QString OllamaModelDetailFetcher::errorText() const { return errorText_; }
QString OllamaModelDetailFetcher::readme() const { return readme_; }
QVariantList OllamaModelDetailFetcher::tags() const { return tags_; }
QString OllamaModelDetailFetcher::installCmd() const { return installCmd_; }

void OllamaModelDetailFetcher::fetchDetails(const QString& modelId) {
    if (fetching_) {
        cancel();
    }

    fetching_ = true;
    errorText_.clear();
    readme_.clear();
    tags_.clear();
    installCmd_.clear();
    modelId_ = modelId.trimmed();
    
    emit fetchingChanged();
    emit errorTextChanged();
    emit readmeChanged();
    emit tagsChanged();
    emit installCmdChanged();

    if (modelId_.isEmpty()) {
        fetching_ = false;
        emit fetchingChanged();
        emit fetchFinished(false);
        return;
    }

    QUrl url(QStringLiteral("https://ollama.com/library/") + modelId_.toLower());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (Sentinel Assistant)"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    reply_ = nam_->get(request);

    connect(reply_, &QNetworkReply::finished, this, &OllamaModelDetailFetcher::handleReplyFinished);
}

void OllamaModelDetailFetcher::cancel() {
    if (reply_) {
        reply_->abort();
    }
}

void OllamaModelDetailFetcher::handleReplyFinished() {
    if (!reply_) return;

    QNetworkReply* r = reply_;
    reply_ = nullptr;
    r->deleteLater();

    fetching_ = false;
    emit fetchingChanged();

    if (r->error() != QNetworkReply::NoError) {
        if (r->error() == QNetworkReply::OperationCanceledError) {
            errorText_ = QStringLiteral("Fetch cancelled.");
        } else {
            errorText_ = QStringLiteral("Failed to fetch model details: %1").arg(r->errorString());
        }
        emit errorTextChanged();
        emit fetchFinished(false);
        return;
    }

    const QString html = QString::fromUtf8(r->readAll());
    parseHtml(html);
    emit fetchFinished(true);
}

QString OllamaModelDetailFetcher::decodeHtml(QString text) {
    text.replace(QLatin1String("&lt;"), QLatin1String("<"));
    text.replace(QLatin1String("&gt;"), QLatin1String(">"));
    text.replace(QLatin1String("&amp;"), QLatin1String("&"));
    text.replace(QLatin1String("&quot;"), QLatin1String("\""));
    text.replace(QLatin1String("&#34;"), QLatin1String("\""));
    text.replace(QLatin1String("&#39;"), QLatin1String("'"));
    text.replace(QLatin1String("&#x27;"), QLatin1String("'"));
    return text;
}

void OllamaModelDetailFetcher::parseHtml(const QString& html) {
    // 1. Extract Readme (from <textarea id="editor")
    int textareaStart = html.indexOf(QStringLiteral("<textarea id=\"editor\""));
    if (textareaStart == -1) {
        textareaStart = html.indexOf(QStringLiteral("<textarea")); // Fallback
    }

    QString readmeText;
    if (textareaStart != -1) {
        int contentStart = html.indexOf(QStringLiteral(">"), textareaStart);
        if (contentStart != -1) {
            contentStart += 1;
            int textareaEnd = html.indexOf(QStringLiteral("</textarea>"), contentStart);
            if (textareaEnd != -1) {
                readmeText = html.mid(contentStart, textareaEnd - contentStart);
                readmeText = decodeHtml(readmeText).trimmed();
            }
        }
    }

    // If textarea not found, let's try to extract from the display div
    if (readmeText.isEmpty()) {
        int displayStart = html.indexOf(QStringLiteral("id=\"display\""));
        if (displayStart != -1) {
            displayStart = html.indexOf(QStringLiteral(">"), displayStart);
            if (displayStart != -1) {
                displayStart += 1;
                int displayEnd = html.indexOf(QStringLiteral("</div>"), displayStart); // A rough end
                if (displayEnd != -1) {
                    readmeText = html.mid(displayStart, displayEnd - displayStart);
                    // Strip html tags
                    readmeText.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
                    readmeText = decodeHtml(readmeText).trimmed();
                }
            }
        }
    }

    readme_ = readmeText;

    // 2. Extract Tags & Sizes
    QVariantList tagsList;
    QString searchStr = QStringLiteral("href=\"/library/") + modelId_.toLower() + QStringLiteral(":");
    int tagIndex = 0;
    while (true) {
        int pos = html.indexOf(searchStr, tagIndex);
        if (pos == -1) break;

        int startQuote = pos + 6; // points to "/library/..."
        int endQuote = html.indexOf(QStringLiteral("\""), startQuote);
        if (endQuote == -1) break;

        QString fullTagPath = html.mid(startQuote, endQuote - startQuote); // "/library/llama3.2:1b"
        int colonIdx = fullTagPath.indexOf(':');
        QString tagOnly;
        if (colonIdx != -1) {
            tagOnly = fullTagPath.mid(colonIdx + 1);
        } else {
            tagOnly = QStringLiteral("latest");
        }

        // Find size
        QString sizeStr = QStringLiteral("—");
        int sizePos = html.indexOf(QStringLiteral("x-test-model-tag-size"), endQuote);
        if (sizePos != -1 && sizePos - endQuote < 1000) {
            int sizeTextStart = html.indexOf(QStringLiteral(">"), sizePos);
            if (sizeTextStart != -1) {
                sizeTextStart += 1;
                int sizeTextEnd = html.indexOf(QStringLiteral("</p>"), sizeTextStart);
                if (sizeTextEnd != -1) {
                    sizeStr = html.mid(sizeTextStart, sizeTextEnd - sizeTextStart).trimmed();
                }
            }
        }

        QVariantMap tagObj;
        tagObj[QStringLiteral("tag")] = tagOnly;
        tagObj[QStringLiteral("fullTag")] = modelId_ + QStringLiteral(":") + tagOnly;
        tagObj[QStringLiteral("size")] = sizeStr;

        bool exists = false;
        for (const auto& existing : tagsList) {
            if (existing.toMap()[QStringLiteral("tag")].toString() == tagOnly) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            tagsList.append(tagObj);
        }

        tagIndex = endQuote;
    }

    tags_ = tagsList;

    // 3. Extract install command
    QString command = QStringLiteral("ollama run ") + modelId_;
    int cliPanel = html.indexOf(QStringLiteral("data-panel=\"cli\""));
    if (cliPanel != -1) {
        int preStart = html.indexOf(QStringLiteral("<pre"), cliPanel);
        if (preStart != -1) {
            int preTextStart = html.indexOf(QStringLiteral(">"), preStart);
            if (preTextStart != -1) {
                preTextStart += 1;
                int preTextEnd = html.indexOf(QStringLiteral("</pre>"), preTextStart);
                if (preTextEnd != -1) {
                    command = html.mid(preTextStart, preTextEnd - preTextStart).trimmed();
                }
            }
        }
    }
    installCmd_ = command;

    emit readmeChanged();
    emit tagsChanged();
    emit installCmdChanged();
}

// ── LMStudioLibraryFetcher implementation ─────────────────────────────────────

LMStudioLibraryFetcher::LMStudioLibraryFetcher(QObject* parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))
{}

bool LMStudioLibraryFetcher::fetching() const { return fetching_; }
QVariantList LMStudioLibraryFetcher::models() const { return models_; }
QString LMStudioLibraryFetcher::errorText() const { return errorText_; }

void LMStudioLibraryFetcher::fetch() {
    if (fetching_) {
        cancel();
    }

    fetching_ = true;
    errorText_.clear();
    emit fetchingChanged();
    emit errorTextChanged();

    QUrl url(QStringLiteral("https://lmstudio.ai/models?sort=updated"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (Sentinel Assistant)"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    reply_ = nam_->get(request);

    connect(reply_, &QNetworkReply::finished, this, &LMStudioLibraryFetcher::handleReplyFinished);
}

void LMStudioLibraryFetcher::cancel() {
    if (reply_) {
        reply_->abort();
    }
}

void LMStudioLibraryFetcher::handleReplyFinished() {
    if (!reply_) return;

    QNetworkReply* r = reply_;
    reply_ = nullptr;
    r->deleteLater();

    fetching_ = false;
    emit fetchingChanged();

    if (r->error() != QNetworkReply::NoError) {
        if (r->error() == QNetworkReply::OperationCanceledError) {
            errorText_ = QStringLiteral("Fetch cancelled.");
        } else {
            errorText_ = QStringLiteral("Failed to fetch LM Studio models: %1").arg(r->errorString());
        }
        emit errorTextChanged();
        emit fetchFinished(false);
        return;
    }

    const QString html = QString::fromUtf8(r->readAll());
    parseHtml(html);
    emit fetchFinished(true);
}

void LMStudioLibraryFetcher::parseHtml(const QString& html) {
    QVariantList parsedList;
    int index = 0;
    while (true) {
        int nextLink = html.indexOf(QStringLiteral("href=\"/models/"), index);
        if (nextLink == -1) {
            break;
        }

        int startQuote = nextLink + 14;
        int endQuote = html.indexOf(QStringLiteral("\""), startQuote);
        if (endQuote == -1) {
            index = startQuote;
            continue;
        }

        QString modelId = html.mid(startQuote, endQuote - startQuote);
        if (modelId.contains('/') || modelId.contains('?')) {
            index = endQuote;
            continue;
        }

        int aStart = html.lastIndexOf(QStringLiteral("<a"), nextLink);
        if (aStart == -1) aStart = nextLink;

        int aEnd = html.indexOf(QStringLiteral("</a>"), nextLink);
        if (aEnd == -1) aEnd = nextLink + 2000;

        QString block = html.mid(aStart, aEnd - aStart);
        index = aEnd;

        // 1. Name
        int nameStart = block.indexOf(QStringLiteral("class=\"text-lg font-medium\">"));
        QString name = modelId;
        if (nameStart != -1) {
            nameStart += 28;
            int nameEnd = block.indexOf(QStringLiteral("</div>"), nameStart);
            if (nameEnd != -1) {
                name = block.mid(nameStart, nameEnd - nameStart).trimmed();
            }
        }

        // 2. Sizes / Tags
        QStringList tags;
        int tagPos = 0;
        while (true) {
            tagPos = block.indexOf(QStringLiteral("font-mono text-xs"), tagPos);
            if (tagPos == -1) break;

            int textStart = block.indexOf(QStringLiteral(">"), tagPos);
            if (textStart == -1) break;
            textStart += 1;

            int textEnd = block.indexOf(QStringLiteral("</div>"), textStart);
            if (textEnd == -1) break;

            QString tagVal = block.mid(textStart, textEnd - textStart).trimmed();
            if (!tagVal.isEmpty() && !tags.contains(tagVal)) {
                tags.append(tagVal);
            }

            tagPos = textEnd;
        }

        // 3. Description
        int descStart = block.indexOf(QStringLiteral("text-muted-foreground"));
        QString description;
        if (descStart != -1) {
            descStart = block.indexOf(QStringLiteral(">"), descStart);
            if (descStart != -1) {
                descStart += 1;
                int descEnd = block.indexOf(QStringLiteral("</div>"), descStart);
                if (descEnd != -1) {
                    description = block.mid(descStart, descEnd - descStart).trimmed();
                    description.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
                }
            }
        }

        // 4. Downloads count
        int downloadsStart = block.indexOf(QStringLiteral("<span class=\"font-medium\">"));
        QString downloads = QStringLiteral("—");
        QString stars = QStringLiteral("—");
        if (downloadsStart != -1) {
            int dStart = downloadsStart + 26;
            int dEnd = block.indexOf(QStringLiteral("</span>"), dStart);
            if (dEnd != -1) {
                downloads = block.mid(dStart, dEnd - dStart).trimmed();
                
                int starsStart = block.indexOf(QStringLiteral("<span class=\"font-medium\">"), dEnd);
                if (starsStart != -1) {
                    starsStart += 26;
                    int starsEnd = block.indexOf(QStringLiteral("</span>"), starsStart);
                    if (starsEnd != -1) {
                        stars = block.mid(starsStart, starsEnd - starsStart).trimmed();
                    }
                }
            }
        }

        // 5. Updated time
        int updatedStart = block.indexOf(QStringLiteral("Updated"));
        QString updated = QStringLiteral("—");
        if (updatedStart != -1) {
            int updatedEnd = block.indexOf(QStringLiteral("</div>"), updatedStart);
            if (updatedEnd != -1) {
                updated = block.mid(updatedStart, updatedEnd - updatedStart).trimmed();
                updated.remove(QRegularExpression(QStringLiteral("<!--[^>]*-->")));
                updated = updated.simplified();
            }
        }

        // 6. Category (Vision, Think, and Image check)
        QString category = QStringLiteral("LLM");
        if (block.contains(QStringLiteral("--lm-yellow")) || 
            block.contains(QStringLiteral("rgb(var(--lm-yellow))")) || 
            description.toLower().contains(QStringLiteral("vision")) ||
            modelId.contains(QStringLiteral("llava")) ||
            modelId.contains(QStringLiteral("bakllava"))) {
            category = QStringLiteral("Vision");
        } else if (description.toLower().contains(QStringLiteral("reasoning")) || 
                   description.toLower().contains(QStringLiteral("thinking")) ||
                   modelId.contains(QStringLiteral("r1")) ||
                   modelId.contains(QStringLiteral("deepseek")) ||
                   modelId.contains(QStringLiteral("phi-4"))) {
            category = QStringLiteral("Think");
        } else if (description.toLower().contains(QStringLiteral("image")) || 
                   modelId.contains(QStringLiteral("stable-diffusion")) ||
                   modelId.contains(QStringLiteral("flux"))) {
            category = QStringLiteral("Image");
        }

        QVariantMap modelObj;
        modelObj[QStringLiteral("id")] = QStringLiteral("lmstudio-") + modelId;
        modelObj[QStringLiteral("ollamaId")] = QStringLiteral("");
        modelObj[QStringLiteral("category")] = category;
        modelObj[QStringLiteral("name")] = name;
        modelObj[QStringLiteral("provider")] = QStringLiteral("LM Studio Catalog");
        modelObj[QStringLiteral("size")] = downloads + QStringLiteral(" downloads");
        modelObj[QStringLiteral("description")] = description;
        modelObj[QStringLiteral("badge")] = QStringLiteral("LM Studio");
        modelObj[QStringLiteral("badgeColor")] = QStringLiteral("#3b82f6");

        QStringList tagsList = tags;
        if (updated != QStringLiteral("—")) {
            tagsList.append(updated);
        }
        if (stars != QStringLiteral("—")) {
            tagsList.append(stars + QStringLiteral(" ★"));
        }
        modelObj[QStringLiteral("tags")] = tagsList;
        modelObj[QStringLiteral("downloadable")] = false;
        modelObj[QStringLiteral("externalUrl")] = QStringLiteral("https://lmstudio.ai/models/") + modelId;

        parsedList.append(modelObj);
    }

    models_ = parsedList;
    emit modelsChanged();
}




