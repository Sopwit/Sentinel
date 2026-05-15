#include "sentinel/core/ChatSession.h"

#include <QtTest>

using sentinel::core::ChatMessageStatus;
using sentinel::core::ChatRole;
using sentinel::core::ChatSession;
using sentinel::core::IClock;

class FixedClock final : public IClock {
public:
    QDateTime nowUtc() const override {
        return QDateTime(QDate(2026, 5, 15), QTime(12, 0), QTimeZone::UTC);
    }
};

class ChatSessionTest final : public QObject {
    Q_OBJECT

private slots:
    void appendsMessagesWithIdsRolesAndTimestamps();
    void clearsHistoryAndResetsIds();
    void roleAndStatusNamesAreStable();
};

void ChatSessionTest::appendsMessagesWithIdsRolesAndTimestamps() {
    ChatSession session(std::make_unique<FixedClock>());

    const auto system =
        session.appendSystemMessage(QStringLiteral("online"), ChatMessageStatus::Received);
    const auto user = session.appendUserMessage(QStringLiteral("status"));
    const auto assistant =
        session.appendAssistantMessage(QStringLiteral("ready"), ChatMessageStatus::Received);

    QCOMPARE(session.messages().size(), 3);
    QCOMPARE(system.id, 1);
    QCOMPARE(user.id, 2);
    QCOMPARE(assistant.id, 3);
    QCOMPARE(user.role, ChatRole::User);
    QCOMPARE(user.status, ChatMessageStatus::Sent);
    QCOMPARE(user.timestamp.toString(Qt::ISODate), QStringLiteral("2026-05-15T12:00:00Z"));
}

void ChatSessionTest::clearsHistoryAndResetsIds() {
    ChatSession session(std::make_unique<FixedClock>());

    session.appendUserMessage(QStringLiteral("status"));
    session.clear();
    const auto message =
        session.appendAssistantMessage(QStringLiteral("reset"), ChatMessageStatus::Received);

    QCOMPARE(session.messages().size(), 1);
    QCOMPARE(message.id, 1);
}

void ChatSessionTest::roleAndStatusNamesAreStable() {
    QCOMPARE(sentinel::core::chatRoleName(ChatRole::System), QStringLiteral("system"));
    QCOMPARE(sentinel::core::chatRoleName(ChatRole::User), QStringLiteral("user"));
    QCOMPARE(sentinel::core::chatRoleName(ChatRole::Assistant), QStringLiteral("assistant"));
    QCOMPARE(sentinel::core::chatMessageStatusName(ChatMessageStatus::Sent),
             QStringLiteral("sent"));
    QCOMPARE(sentinel::core::chatMessageStatusName(ChatMessageStatus::Received),
             QStringLiteral("received"));
    QCOMPARE(sentinel::core::chatMessageStatusName(ChatMessageStatus::Error),
             QStringLiteral("error"));
}

QTEST_MAIN(ChatSessionTest)

#include "test_chat_session.moc"
