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

enum class WebFetchFormat { Text, Markdown, Html };

struct WebFetchResponse {
    bool success{false};
    QString content;
    QString contentType;
    int statusCode{0};
    QString errorString;
    qint64 contentLength{0};
};

class WebFetchTool : public QObject {
    Q_OBJECT
public:
    explicit WebFetchTool(QObject* parent = nullptr);
    ~WebFetchTool() override;

    // Configuration
    void setTimeout(int seconds);
    void setMaxContentSize(qint64 maxSize);
    void setUserAgent(const QString& userAgent);

    // Fetch operations
    WebFetchResponse fetch(const QString& url, WebFetchFormat format = WebFetchFormat::Markdown);
    void fetchAsync(const QString& url, WebFetchFormat format,
                    std::function<void(WebFetchResponse)> callback);

    // HTML to conversion
    static QString htmlToMarkdown(const QString& html);
    static QString htmlToText(const QString& html);

signals:
    void fetchCompleted(const WebFetchResponse& response);
    void fetchError(const QString& error);

private slots:
    void onFetchReply(QNetworkReply* reply);

private:
    QUrl validateUrl(const QString& url) const;
    QByteArray buildRequestHeaders() const;
    WebFetchResponse processResponse(QNetworkReply* reply, WebFetchFormat format) const;
    QString convertResponse(const QByteArray& content, const QString& contentType,
                            WebFetchFormat format) const;

    int m_timeoutSeconds{30};
    qint64 m_maxContentSize{5 * 1024 * 1024}; // 5MB
    QString m_userAgent{"Sentinel/1.0"};
    QNetworkAccessManager m_networkManager;
};

} // namespace sentinel::core
