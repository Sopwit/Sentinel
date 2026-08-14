// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/tools/WebSearchTool.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrlQuery>
#include <QTimer>
#include <QDebug>

namespace sentinel::core {

WebSearchTool::WebSearchTool(QObject* parent)
    : QObject(parent)
{
}

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

    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_apiKey).toUtf8());
    }

    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSearchReply(reply);
    });

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
        if (callback) callback(response);
        return;
    }

    if (numResults <= 0) {
        numResults = m_maxResults;
    }

    QUrl url = buildSearchUrl(query, numResults);
    if (!url.isValid()) {
        WebSearchResponse response;
        response.errorString = "Invalid search URL";
        if (callback) callback(response);
        return;
    }

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", m_userAgent.toUtf8());

    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_apiKey).toUtf8());
    }

    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        WebSearchResponse response = parseSearchResponse(reply->readAll());
        if (reply->error() != QNetworkReply::NoError) {
            response.errorString = reply->errorString();
        }
        reply->deleteLater();

        if (callback) callback(response);
        emit searchCompleted(response);
    });
}

QStringList WebSearchTool::supportedProviders() {
    return {"exa", "parallel", "custom"};
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
