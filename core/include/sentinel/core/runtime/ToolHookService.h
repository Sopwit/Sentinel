// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/IToolHookService.h"
#include <QMap>
#include <QObject>

namespace sentinel::core {

class ToolHookService : public QObject, public IToolHookService {
    Q_OBJECT
public:
    explicit ToolHookService(QObject* parent = nullptr);
    ~ToolHookService() override;

    void registerHook(const QString& toolName, ToolHook hook) override;
    void unregisterHook(const QString& toolName) override;
    void beforeToolExecution(const QString& toolName, QJsonObject& params) override;
    void afterToolExecution(const QString& toolName, QJsonObject& result) override;
    void onToolError(const QString& toolName, const QString& error) override;
    bool hasHooks(const QString& toolName) const override;

private:
    QMap<QString, ToolHook> m_hooks;
};

} // namespace sentinel::core
