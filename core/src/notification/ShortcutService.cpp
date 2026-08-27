// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/notification/ShortcutService.h"

namespace sentinel::core {

ShortcutService::ShortcutService(QObject* parent) : QObject(parent) {}
ShortcutService::~ShortcutService() = default;

void ShortcutService::registerShortcut(const QString& action, const QString& keys,
                                       const QString& description) {
    for (int i = 0; i < m_bindings.size(); ++i) {
        if (m_bindings[i].action == action) {
            m_bindings[i].keys = keys;
            m_bindings[i].description = description;
            return;
        }
    }
    m_bindings.append({action, keys, description, nullptr});
}

void ShortcutService::bindCallback(const QString& action, std::function<void()> callback) {
    m_callbacks[action] = callback;
    for (auto& binding : m_bindings) {
        if (binding.action == action) {
            binding.callback = callback;
        }
    }
}

void ShortcutService::triggerAction(const QString& action) {
    if (m_callbacks.contains(action)) {
        m_callbacks[action]();
    }
}

bool ShortcutService::processKeys(const QString& keySequence) {
    for (const auto& binding : m_bindings) {
        if (binding.keys == keySequence) {
            triggerAction(binding.action);
            return true;
        }
    }
    return false;
}

QList<ShortcutBinding> ShortcutService::bindings() const {
    return m_bindings;
}

void ShortcutService::setLeaderKey(const QString& key) {
    m_leaderKey = key;
}
QString ShortcutService::leaderKey() const {
    return m_leaderKey;
}

} // namespace sentinel::core
