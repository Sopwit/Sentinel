// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QMap>
#include <functional>

namespace sentinel::core {

struct ShortcutBinding {
    QString action;
    QString keys;
    QString description;
    std::function<void()> callback;
};

class IShortcutService {
public:
    virtual ~IShortcutService() = default;

    virtual void registerShortcut(const QString& action, const QString& keys, const QString& description) = 0;
    virtual void bindCallback(const QString& action, std::function<void()> callback) = 0;
    virtual void triggerAction(const QString& action) = 0;
    virtual bool processKeys(const QString& keySequence) = 0;
    virtual QList<ShortcutBinding> bindings() const = 0;
    virtual void setLeaderKey(const QString& key) = 0;
    virtual QString leaderKey() const = 0;
};

} // namespace sentinel::core
