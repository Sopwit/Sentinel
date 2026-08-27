// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>
#include <functional>

namespace sentinel::core {

struct ToolHook {
    QString toolName;
    std::function<void(QJsonObject&)> beforeExecute;
    std::function<void(QJsonObject&)> afterExecute;
    std::function<void(const QString&)> onError;
};

class IToolHookService {
public:
    virtual ~IToolHookService() = default;

    virtual void registerHook(const QString& toolName, ToolHook hook) = 0;
    virtual void unregisterHook(const QString& toolName) = 0;
    virtual void beforeToolExecution(const QString& toolName, QJsonObject& params) = 0;
    virtual void afterToolExecution(const QString& toolName, QJsonObject& result) = 0;
    virtual void onToolError(const QString& toolName, const QString& error) = 0;
    virtual bool hasHooks(const QString& toolName) const = 0;
};

} // namespace sentinel::core
