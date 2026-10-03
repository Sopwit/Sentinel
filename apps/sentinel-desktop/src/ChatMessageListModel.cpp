// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/desktop/ChatMessageListModel.h"
#include "sentinel/core/chat/ChatContentRepresentation.h"

namespace sentinel::desktop {

ChatMessageListModel::ChatMessageListModel(QObject* parent) : QAbstractListModel(parent) {}

int ChatMessageListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(messages_.size());
}

QVariant ChatMessageListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= messages_.size()) {
        return {};
    }

    const auto& message = messages_.at(index.row());
    switch (role) {
    case IdRole:
        return message.id;
    case RoleRole:
        return core::chatRoleName(message.role);
    case ContentRole:
        return message.content;
    case TimestampRole:
        return message.timestamp.toString(Qt::ISODate);
    case StatusRole:
        return core::chatMessageStatusName(message.status);
    case PartialRole: return message.partial;
    case ProviderRole: return message.providerUsed;
    case ModelRole: return message.modelUsed;
    case ErrorCategoryRole:
        return core::chatProviderErrorCategoryName(message.errorCategory);
    case ReplyToRole: return message.replyToMessageId;
    case ReplacesRole: return message.replacesMessageId;
    case ContentPartsRole: return core::chatContentPartsVariant(message.content);
    case StateNoticeRole:
        switch (message.status) {
        case core::ChatMessageStatus::Interrupted:
            return tr("Response interrupted.");
        case core::ChatMessageStatus::Cancelled:
            return tr("Response cancelled.");
        case core::ChatMessageStatus::Failed:
        case core::ChatMessageStatus::Error:
            return tr("Response failed.");
        default:
            return QString{};
        }
    default:
        return {};
    }
}

QHash<int, QByteArray> ChatMessageListModel::roleNames() const {
    return {
        {IdRole, "messageId"},
        {RoleRole, "messageRole"},
        {ContentRole, "content"},
        {TimestampRole, "timestamp"},
        {StatusRole, "messageStatus"},
        {PartialRole, "partial"},
        {ProviderRole, "providerId"},
        {ModelRole, "modelId"},
        {ErrorCategoryRole, "errorCategory"},
        {ReplyToRole, "replyToMessageId"},
        {ReplacesRole, "replacesMessageId"},
        {ContentPartsRole, "contentParts"},
        {StateNoticeRole, "stateNotice"},
    };
}

void ChatMessageListModel::setMessages(const QList<core::ChatMessage>& messages) {
    if (messages_.isEmpty()) {
        beginResetModel();
        messages_ = messages;
        endResetModel();
        return;
    }

    if (messages.size() > messages_.size()) {
        bool prefixMatches = true;
        for (int i = 0; i < messages_.size(); ++i) {
            const auto& oldMsg = messages_.at(i);
            const auto& newMsg = messages.at(i);
            if (oldMsg.id != newMsg.id || oldMsg.role != newMsg.role) {
                prefixMatches = false;
                break;
            }
        }

        if (prefixMatches) {
            for (int i = 0; i < messages_.size(); ++i) {
                const auto& oldMsg = messages_.at(i);
                const auto& newMsg = messages.at(i);
                if (oldMsg.content != newMsg.content || oldMsg.status != newMsg.status ||
                    oldMsg.timestamp != newMsg.timestamp || oldMsg.partial != newMsg.partial ||
                    oldMsg.errorCategory != newMsg.errorCategory) {
                    messages_[i] = newMsg;
                    emit dataChanged(index(i), index(i));
                }
            }

            beginInsertRows(QModelIndex(), messages_.size(), messages.size() - 1);
            for (int i = messages_.size(); i < messages.size(); ++i) {
                messages_.append(messages.at(i));
            }
            endInsertRows();
            return;
        }
    }

    if (messages.size() == messages_.size()) {
        for (int i = 0; i < messages_.size(); ++i) {
            const auto& oldMsg = messages_.at(i);
            const auto& newMsg = messages.at(i);
            if (oldMsg.id != newMsg.id || oldMsg.role != newMsg.role) {
                beginResetModel();
                messages_ = messages;
                endResetModel();
                return;
            }
            if (oldMsg.content != newMsg.content || oldMsg.status != newMsg.status ||
                oldMsg.timestamp != newMsg.timestamp || oldMsg.partial != newMsg.partial ||
                oldMsg.errorCategory != newMsg.errorCategory) {
                messages_[i] = newMsg;
                emit dataChanged(index(i), index(i));
            }
        }
        return;
    }

    beginResetModel();
    messages_ = messages;
    endResetModel();
}

} // namespace sentinel::desktop
