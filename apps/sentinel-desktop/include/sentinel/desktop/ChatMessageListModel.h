// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/chat/ChatMessage.h"

#include <QAbstractListModel>
#include <QList>

namespace sentinel::desktop {

class ChatMessageListModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        RoleRole,
        ContentRole,
        TimestampRole,
        StatusRole,
    };

    explicit ChatMessageListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMessages(const QList<core::ChatMessage>& messages);

private:
    QList<core::ChatMessage> messages_;
};

} // namespace sentinel::desktop
