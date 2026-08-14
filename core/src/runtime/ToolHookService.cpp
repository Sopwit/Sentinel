// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/ToolHookService.h"

namespace sentinel::core {

ToolHookService::ToolHookService(QObject* parent) : QObject(parent) {}
ToolHookService::~ToolHookService() = default;

void ToolHookService::registerHook(const QString& toolName, ToolHook hook) {
    m_hooks[toolName] = hook;
}

void ToolHookService::unregisterHook(const QString& toolName) {
    m_hooks.remove(toolName);
}

void ToolHookService::beforeToolExecution(const QString& toolName, QJsonObject& params) {
    auto it = m_hooks.find(toolName);
    if (it != m_hooks.end() && it->beforeExecute) {
        it->beforeExecute(params);
    }
}

void ToolHookService::afterToolExecution(const QString& toolName, QJsonObject& result) {
    auto it = m_hooks.find(toolName);
    if (it != m_hooks.end() && it->afterExecute) {
        it->afterExecute(result);
    }
}

void ToolHookService::onToolError(const QString& toolName, const QString& error) {
    auto it = m_hooks.find(toolName);
    if (it != m_hooks.end() && it->onError) {
        it->onError(error);
    }
}

bool ToolHookService::hasHooks(const QString& toolName) const {
    return m_hooks.contains(toolName);
}

} // namespace sentinel::core
