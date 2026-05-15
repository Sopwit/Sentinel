#include "sentinel/desktop/ChatMessageListModel.h"

namespace sentinel::desktop {

ChatMessageListModel::ChatMessageListModel(QObject* parent) : QAbstractListModel(parent) {}

int ChatMessageListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : messages_.size();
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
    default:
        return {};
    }
}

QHash<int, QByteArray> ChatMessageListModel::roleNames() const {
    return {
        {IdRole, "messageId"},        {RoleRole, "messageRole"},     {ContentRole, "content"},
        {TimestampRole, "timestamp"}, {StatusRole, "messageStatus"},
    };
}

void ChatMessageListModel::setMessages(const QList<core::ChatMessage>& messages) {
    beginResetModel();
    messages_ = messages;
    endResetModel();
}

} // namespace sentinel::desktop
