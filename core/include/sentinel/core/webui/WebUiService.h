// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/webui/IWebUiService.h"
#include <QObject>
#include <QMap>

namespace sentinel::core {

class WebUiService : public QObject, public IWebUiService {
    Q_OBJECT
public:
    explicit WebUiService(QObject* parent = nullptr);
    ~WebUiService() override;

    bool start(const WebUiConfig& config) override;
    void stop() override;
    bool isRunning() const override;
    QString url() const override;
    void setContent(const QString& path, const QString& content) override;
    void setApiHandler(const QString& path, std::function<QString(const QJsonObject&)> handler) override;

private:
    WebUiConfig m_config;
    bool m_running{false};
    QMap<QString, QString> m_content;
    QMap<QString, std::function<QString(const QJsonObject&)>> m_apiHandlers;
};

} // namespace sentinel::core
