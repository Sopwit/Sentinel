// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core {

struct TodoItem {
    QString id;
    QString content;
    QString status{"pending"};
    QString priority{"medium"};
};

struct TodoSnapshot {
    QList<TodoItem> items;
};

class ITodoService {
public:
    virtual ~ITodoService() = default;

    virtual void setItems(const QList<TodoItem>& items) = 0;
    virtual QList<TodoItem> items() const = 0;
    virtual void updateItem(const QString& id, const QString& status, const QString& content = {}) = 0;
    virtual void addItem(const QString& content, const QString& priority = "medium") = 0;
    virtual void removeItem(const QString& id) = 0;
    virtual TodoSnapshot snapshot() const = 0;
    virtual void restoreSnapshot(const TodoSnapshot& snapshot) = 0;
    virtual QString formattedText() const = 0;
};

} // namespace sentinel::core
