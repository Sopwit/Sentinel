// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/tools/WebFetchTool.h"
#include "sentinel/core/runtime/tools/HtmlToMarkdown.h"
#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace sentinel::core {

WebFetchTool::WebFetchTool(QObject* parent) : QObject(parent) {}

WebFetchTool::~WebFetchTool() = default;

void WebFetchTool::setTimeout(int seconds) {
    m_timeoutSeconds = seconds;
}

void WebFetchTool::setMaxContentSize(qint64 maxSize) {
    m_maxContentSize = maxSize;
}

void WebFetchTool::setUserAgent(const QString& userAgent) {
    m_userAgent = userAgent;
}

WebFetchResponse WebFetchTool::fetch(const QString& url, WebFetchFormat format) {
    WebFetchResponse response;

    QUrl validatedUrl = validateUrl(url);
    if (!validatedUrl.isValid()) {
        response.errorString = "Invalid URL: " + url;
        return response;
    }

    QNetworkRequest request{validatedUrl};
    request.setRawHeader("User-Agent", m_userAgent.toUtf8());
    request.setRawHeader("Accept",
                         "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");

    QNetworkReply* reply = m_networkManager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(m_timeoutSeconds * 1000);
    loop.exec();
    if (!reply->isFinished()) {
        reply->abort();
        response.errorString = QStringLiteral("Request timed out.");
    } else {
        response = processResponse(reply, format);
    }
    reply->deleteLater();
    return response;
}

void WebFetchTool::fetchAsync(const QString& url, WebFetchFormat format,
                              std::function<void(WebFetchResponse)> callback) {
    QUrl validatedUrl = validateUrl(url);
    if (!validatedUrl.isValid()) {
        WebFetchResponse response;
        response.errorString = "Invalid URL: " + url;
        if (callback)
            callback(response);
        return;
    }

    QNetworkRequest request(validatedUrl);
    request.setRawHeader("User-Agent", m_userAgent.toUtf8());
    request.setRawHeader("Accept",
                         "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");

    QNetworkReply* reply = m_networkManager.get(request);

    // Set timeout
    QTimer::singleShot(m_timeoutSeconds * 1000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, format, callback]() {
        WebFetchResponse response = processResponse(reply, format);
        reply->deleteLater();

        if (callback)
            callback(response);
        emit fetchCompleted(response);
    });
}

QString WebFetchTool::htmlToMarkdown(const QString& html) {
    return HtmlToMarkdown::convert(html);
}

QString WebFetchTool::htmlToText(const QString& html) {
    return HtmlToMarkdown::toText(html);
}

void WebFetchTool::onFetchReply(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit fetchError(reply->errorString());
        reply->deleteLater();
        return;
    }

    WebFetchResponse response = processResponse(reply, WebFetchFormat::Markdown);
    reply->deleteLater();

    emit fetchCompleted(response);
}

QUrl WebFetchTool::validateUrl(const QString& url) const {
    QUrl qurl(url);
    if (!qurl.isValid()) {
        return QUrl();
    }

    // Only allow http and https
    if (qurl.scheme() != "http" && qurl.scheme() != "https") {
        return QUrl();
    }

    return qurl;
}

QByteArray WebFetchTool::buildRequestHeaders() const {
    QByteArray headers;
    headers += "User-Agent: " + m_userAgent.toUtf8() + "\r\n";
    headers += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    return headers;
}

WebFetchResponse WebFetchTool::processResponse(QNetworkReply* reply, WebFetchFormat format) const {
    WebFetchResponse response;

    response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    response.contentLength = reply->bytesAvailable();

    if (response.contentLength > m_maxContentSize) {
        response.errorString = "Content too large";
        return response;
    }

    QByteArray content = reply->readAll();
    response.content = convertResponse(content, response.contentType, format);
    response.success = true;

    return response;
}

QString WebFetchTool::convertResponse(const QByteArray& content, const QString& contentType,
                                      WebFetchFormat format) const {
    QString html = QString::fromUtf8(content);

    switch (format) {
    case WebFetchFormat::Text:
        return htmlToText(html);
    case WebFetchFormat::Markdown:
        return htmlToMarkdown(html);
    case WebFetchFormat::Html:
    default:
        return html;
    }
}

} // namespace sentinel::core
