// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/webui/WebUiService.h"

namespace sentinel::core {

WebUiService::WebUiService(QObject* parent) : QObject(parent) {}
WebUiService::~WebUiService() { stop(); }

bool WebUiService::start(const WebUiConfig& config) {
    m_config = config;
    m_running = true;
    return true;
}

void WebUiService::stop() { m_running = false; }
bool WebUiService::isRunning() const { return m_running; }

QString WebUiService::url() const {
    return QStringLiteral("http://%1:%2").arg(m_config.host).arg(m_config.port);
}

void WebUiService::setContent(const QString& path, const QString& content) {
    m_content[path] = content;
}

void WebUiService::setApiHandler(const QString& path, std::function<QString(const QJsonObject&)> handler) {
    m_apiHandlers[path] = handler;
}

} // namespace sentinel::core
