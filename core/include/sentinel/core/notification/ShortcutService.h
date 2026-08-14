// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/notification/IShortcutService.h"
#include <QObject>
#include <QList>
#include <QMap>

namespace sentinel::core {

class ShortcutService : public QObject, public IShortcutService {
    Q_OBJECT
public:
    explicit ShortcutService(QObject* parent = nullptr);
    ~ShortcutService() override;

    void registerShortcut(const QString& action, const QString& keys, const QString& description) override;
    void bindCallback(const QString& action, std::function<void()> callback) override;
    void triggerAction(const QString& action) override;
    bool processKeys(const QString& keySequence) override;
    QList<ShortcutBinding> bindings() const override;
    void setLeaderKey(const QString& key) override;
    QString leaderKey() const override;

private:
    QList<ShortcutBinding> m_bindings;
    QMap<QString, std::function<void()>> m_callbacks;
    QString m_leaderKey;
};

} // namespace sentinel::core
