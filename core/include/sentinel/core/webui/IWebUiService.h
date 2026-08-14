// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core {

struct WebUiConfig {
    bool enabled{false};
    int port{8080};
    QString host{"127.0.0.1"};
    bool enableCsp{true};
    QString cspPolicy;
    bool allowCors{false};
};

class IWebUiService {
public:
    virtual ~IWebUiService() = default;

    virtual bool start(const WebUiConfig& config) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual QString url() const = 0;
    virtual void setContent(const QString& path, const QString& content) = 0;
    virtual void setApiHandler(const QString& path, std::function<QString(const QJsonObject&)> handler) = 0;
};

} // namespace sentinel::core
