// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/desktop/ChatMessageListModel.h"
#include <QtTest>
using namespace sentinel;
class ChatMessageRecoveryTest final : public QObject {
    Q_OBJECT
private slots:
    void interruptedEmptyMessageHasTruthfulNotice() {
        desktop::ChatMessageListModel model;
        core::ChatMessage message;
        message.id = 1; message.role = core::ChatRole::Assistant;
        message.status = core::ChatMessageStatus::Interrupted;
        model.setMessages({message});
        const auto noticeRole = model.roleNames().key("stateNotice", -1);
        const auto row = model.index(0);
        QCOMPARE(model.data(row, desktop::ChatMessageListModel::StatusRole).toString(), QStringLiteral("interrupted"));
        QVERIFY(model.data(row, noticeRole).toString().contains("interrupted", Qt::CaseInsensitive));
        QVERIFY(model.data(row, desktop::ChatMessageListModel::ContentRole).toString().isEmpty());
    }
    void noticesPreservePartialContentAndClearOnCompletion() {
        desktop::ChatMessageListModel model;
        core::ChatMessage message;
        message.id = 1; message.role = core::ChatRole::Assistant;
        message.content = "partial output";
        message.status = core::ChatMessageStatus::Cancelled;
        model.setMessages({message});
        const auto noticeRole = model.roleNames().key("stateNotice", -1);
        const auto row = model.index(0);
        QVERIFY(model.data(row, noticeRole).toString().contains("cancelled", Qt::CaseInsensitive));
        QCOMPARE(model.data(row, desktop::ChatMessageListModel::ContentRole).toString(), message.content);
        message.status = core::ChatMessageStatus::Completed;
        model.setMessages({message});
        QVERIFY(model.data(row, noticeRole).toString().isEmpty());
        QCOMPARE(model.data(row, desktop::ChatMessageListModel::ContentRole).toString(), message.content);
    }
};
QTEST_MAIN(ChatMessageRecoveryTest)
#include "test_chat_message_recovery.moc"
