// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <functional>

namespace sentinel::core {

struct WebSearchResult {
    QString title;
    QString url;
    QString snippet;
    double score{0.0};
};

struct WebSearchResponse {
    bool success{false};
    QList<WebSearchResult> results;
    QString errorString;
    int totalResults{0};
};

class WebSearchTool : public QObject {
    Q_OBJECT
public:
    explicit WebSearchTool(QObject* parent = nullptr);
    ~WebSearchTool() override;

    // Configuration
    void setSearchProvider(const QString& provider);
    void setApiKey(const QString& apiKey);
    void setMaxResults(int maxResults);

    // Search operations
    WebSearchResponse search(const QString& query, int numResults = 5);
    void searchAsync(const QString& query, int numResults,
                     std::function<void(WebSearchResponse)> callback);

    // Supported providers. "duckduckgo" needs no API key; "exa" and "parallel"
    // require one and fall back to DuckDuckGo when the key is missing.
    static QStringList supportedProviders();

signals:
    void searchCompleted(const WebSearchResponse& response);
    void searchError(const QString& error);

private slots:
    void onSearchReply(QNetworkReply* reply);

private:
    QJsonObject buildSearchRequest(const QString& query, int numResults) const;
    WebSearchResponse parseSearchResponse(const QByteArray& responseData) const;
    QUrl buildSearchUrl(const QString& query, int numResults) const;
    WebSearchResponse searchDuckDuckGo(const QString& query, int numResults);

public:
    // Exposed for tests: parses the DuckDuckGo HTML endpoint response.
    static WebSearchResponse parseDuckDuckGoResponse(const QByteArray& responseData,
                                                     int numResults);

private:
    bool apiKeyConfigured() const;

    QString m_searchProvider{"duckduckgo"};
    QString m_apiKey;
    QString m_userAgent{"Sentinel/1.0"};
    int m_maxResults{5};
    QNetworkAccessManager m_networkManager;
};

} // namespace sentinel::core
