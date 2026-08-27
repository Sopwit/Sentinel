// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/tools/WebSearchTool.h"
#include <QDebug>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>
#include <QUrlQuery>

namespace sentinel::core {

WebSearchTool::WebSearchTool(QObject* parent) : QObject(parent) {}

WebSearchTool::~WebSearchTool() = default;

void WebSearchTool::setSearchProvider(const QString& provider) {
    m_searchProvider = provider;
}

void WebSearchTool::setApiKey(const QString& apiKey) {
    m_apiKey = apiKey;
}

void WebSearchTool::setMaxResults(int maxResults) {
    m_maxResults = maxResults;
}

WebSearchResponse WebSearchTool::search(const QString& query, int numResults) {
    WebSearchResponse response;

    if (query.isEmpty()) {
        response.errorString = "Search query is empty";
        return response;
    }

    if (numResults <= 0) {
        numResults = m_maxResults;
    }

    if (!apiKeyConfigured()) {
        return searchDuckDuckGo(query, numResults);
    }

    QUrl url = buildSearchUrl(query, numResults);
    if (!url.isValid()) {
        response.errorString = "Invalid search URL";
        return response;
    }

    QUrlQuery queryParams;
    queryParams.addQueryItem(QStringLiteral("query"), query);
    queryParams.addQueryItem(QStringLiteral("numResults"), QString::number(numResults));
    url.setQuery(queryParams);
    QNetworkRequest request{url};
    request.setRawHeader("User-Agent", m_userAgent.toUtf8());

    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("x-api-key", m_apiKey.toUtf8());
    QNetworkReply* reply = m_networkManager.post(
        request,
        QJsonDocument(buildSearchRequest(query, numResults)).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onSearchReply(reply); });

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000);
    loop.exec();
    if (!reply->isFinished()) {
        reply->abort();
        response.errorString = QStringLiteral("Search request timed out.");
    } else {
        response = parseSearchResponse(reply->readAll());
        if (reply->error() != QNetworkReply::NoError) {
            response.success = false;
            response.errorString = reply->errorString();
        }
    }
    reply->deleteLater();
    return response;
}

void WebSearchTool::searchAsync(const QString& query, int numResults,
                                std::function<void(WebSearchResponse)> callback) {
    if (query.isEmpty()) {
        WebSearchResponse response;
        response.errorString = "Search query is empty";
        if (callback)
            callback(response);
        return;
    }

    if (numResults <= 0) {
        numResults = m_maxResults;
    }

    if (!apiKeyConfigured()) {
        WebSearchResponse response = searchDuckDuckGo(query, numResults);
        if (callback)
            callback(response);
        return;
    }

    QUrl url = buildSearchUrl(query, numResults);
    if (!url.isValid()) {
        WebSearchResponse response;
        response.errorString = "Invalid search URL";
        if (callback)
            callback(response);
        return;
    }

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", m_userAgent.toUtf8());

    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("x-api-key", m_apiKey.toUtf8());
    QNetworkReply* reply = m_networkManager.post(
        request,
        QJsonDocument(buildSearchRequest(query, numResults)).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        WebSearchResponse response = parseSearchResponse(reply->readAll());
        if (reply->error() != QNetworkReply::NoError) {
            response.errorString = reply->errorString();
        }
        reply->deleteLater();

        if (callback)
            callback(response);
        emit searchCompleted(response);
    });
}

QStringList WebSearchTool::supportedProviders() {
    return {"duckduckgo", "exa", "parallel", "custom"};
}

bool WebSearchTool::apiKeyConfigured() const {
    // Only the keyless DuckDuckGo provider runs without credentials; API-backed
    // providers fall back to DuckDuckGo until a key is configured.
    if (m_searchProvider.compare(QStringLiteral("duckduckgo"), Qt::CaseInsensitive) == 0) {
        return false;
    }
    return !m_apiKey.trimmed().isEmpty();
}

WebSearchResponse WebSearchTool::searchDuckDuckGo(const QString& query, int numResults) {
    WebSearchResponse response;

    QUrl url(QStringLiteral("https://html.duckduckgo.com/html/"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("q"), query);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
    // DuckDuckGo rejects clearly non-browser agents; use a conventional UA.
    request.setRawHeader("User-Agent",
                         "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
                         "Chrome/124.0 Safari/537.36");

    QNetworkReply* reply =
        m_networkManager.post(request, params.toString(QUrl::FullyEncoded).toUtf8());

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000);
    loop.exec();
    if (!reply->isFinished()) {
        reply->abort();
        response.errorString = QStringLiteral("Search request timed out.");
    } else {
        response = parseDuckDuckGoResponse(reply->readAll(), numResults);
        if (reply->error() != QNetworkReply::NoError && !response.success) {
            response.success = false;
            response.errorString = reply->errorString();
        }
    }
    reply->deleteLater();
    return response;
}

namespace {

QString htmlUnescape(const QString& text) {
    QString out = text;
    out.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    out.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    out.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    out.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    out.replace(QStringLiteral("&#x27;"), QStringLiteral("'"));
    out.replace(QStringLiteral("&#39;"), QStringLiteral("'"));
    out.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
    return out;
}

} // namespace

WebSearchResponse WebSearchTool::parseDuckDuckGoResponse(const QByteArray& responseData,
                                                         int numResults) {
    WebSearchResponse response;

    const QString html = QString::fromUtf8(responseData);
    // Each result row: <a rel="nofollow" class="result__a" href="...">Title</a>
    // followed by a snippet in <a class="result__snippet" ...>...</a>.
    static const QRegularExpression resultBlock(
        QStringLiteral("class=\"result__a\"[^>]*href=\"([^\"]+)\"[^>]*>(.*?)</a>"
                       "(?:.*?class=\"result__snippet\"[^>]*>(.*?)</a>)?"),
        QRegularExpression::DotMatchesEverythingOption);

    auto it = resultBlock.globalMatch(html);
    while (it.hasNext() && response.results.size() < numResults) {
        const auto match = it.next();

        QString url = htmlUnescape(match.captured(1));
        // DuckDuckGo wraps destinations in /l/?uddg=<encoded url>; unwrap it.
        if (url.startsWith(QStringLiteral("//duckduckgo.com/l/")) ||
            url.startsWith(QStringLiteral("/l/"))) {
            const int uddgStart = url.indexOf(QStringLiteral("uddg="));
            if (uddgStart >= 0) {
                QString embedded = url.mid(uddgStart + 5);
                const int amp = embedded.indexOf(QLatin1Char('&'));
                if (amp >= 0) {
                    embedded = embedded.left(amp);
                }
                url = QUrl::fromPercentEncoding(embedded.toUtf8());
            }
        }
        if (url.startsWith(QStringLiteral("//"))) {
            url.prepend(QStringLiteral("https:"));
        }

        WebSearchResult result;
        result.title = htmlUnescape(match.captured(2))
                           .remove(QRegularExpression(QStringLiteral("<[^>]*>")))
                           .simplified();
        result.url = url;
        result.snippet = htmlUnescape(match.captured(3))
                             .remove(QRegularExpression(QStringLiteral("<[^>]*>")))
                             .simplified();
        response.results.append(result);
    }

    if (response.results.isEmpty()) {
        response.errorString = QStringLiteral(
            "No results were returned (DuckDuckGo layout may have changed or the request was "
            "blocked).");
    } else {
        response.success = true;
    }
    response.totalResults = response.results.size();
    return response;
}

void WebSearchTool::onSearchReply(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit searchError(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    WebSearchResponse response = parseSearchResponse(responseData);
    reply->deleteLater();

    emit searchCompleted(response);
}

QJsonObject WebSearchTool::buildSearchRequest(const QString& query, int numResults) const {
    QJsonObject request;
    request["query"] = query;
    request["numResults"] = numResults;
    request["type"] = "auto";
    return request;
}

WebSearchResponse WebSearchTool::parseSearchResponse(const QByteArray& responseData) const {
    WebSearchResponse response;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        response.errorString = parseError.errorString();
        return response;
    }

    QJsonObject root = doc.object();
    response.success = true;

    // Parse results based on provider format
    if (root.contains("results")) {
        QJsonArray resultsArray = root["results"].toArray();
        for (const auto& resultValue : resultsArray) {
            QJsonObject resultObj = resultValue.toObject();
            WebSearchResult result;
            result.title = resultObj["title"].toString();
            result.url = resultObj["url"].toString();
            result.snippet = resultObj["snippet"].toString();
            if (result.snippet.isEmpty()) {
                result.snippet = resultObj["text"].toString();
            }
            result.score = resultObj["score"].toDouble();
            response.results.append(result);
        }
    }

    response.totalResults = response.results.size();
    return response;
}

QUrl WebSearchTool::buildSearchUrl(const QString& query, int numResults) const {
    if (m_searchProvider == "exa") {
        return QUrl("https://api.exa.ai/search");
    } else if (m_searchProvider == "parallel") {
        return QUrl("https://api.parallel.ai/search");
    }

    return QUrl();
}

} // namespace sentinel::core
