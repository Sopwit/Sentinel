// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/DeterministicChatFixture.h"

#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/chat/InMemoryConversationStore.h"
#include "sentinel/core/memory/InMemoryStore.h"

#include <QtTest>

using namespace sentinel;

namespace {

std::unique_ptr<core::ApplicationController>
makeController(test::DeterministicModelServiceFixture& models) {
    core::ApplicationControllerBuilder builder;
    builder.withMemoryStore(std::make_unique<core::InMemoryStore>())
        .withConversationStore(std::make_unique<core::InMemoryConversationStore>())
        .withModelService(models.takeModelService());
    return builder.build();
}

} // namespace

class ChatApplicationControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void sendsThroughSelectedModelBinding();
    void failureIsVisibleAndTerminal();
    void cancellationIsImmediatelyTerminalAndLateResultIsIgnored();
};

void ChatApplicationControllerTest::sendsThroughSelectedModelBinding() {
    test::DeterministicModelServiceFixture models;
    const auto controller = makeController(models);

    QVERIFY(controller->sendMessage(QStringLiteral("first prompt")));
    QTRY_VERIFY_WITH_TIMEOUT(!controller->chatGenerationActive(), 3000);
    QVERIFY(controller->sendMessage(QStringLiteral("second prompt")));
    QTRY_VERIFY_WITH_TIMEOUT(!controller->chatGenerationActive(), 3000);

    const auto messages = controller->chatHistory();
    QCOMPARE(messages.size(), 5);
    QCOMPARE(messages.at(2).content, QStringLiteral("SENTINEL_TEST_RESPONSE"));
    QCOMPARE(messages.at(2).status, core::ChatMessageStatus::Completed);
    QCOMPARE(messages.at(4).content, QStringLiteral("SENTINEL_TEST_RESPONSE"));
    QCOMPARE(controller->activeChatProviderId(), QStringLiteral("ollama"));
    QCOMPARE(controller->activeChatModelId(), QStringLiteral("sentinel-test-model"));
}

void ChatApplicationControllerTest::failureIsVisibleAndTerminal() {
    test::DeterministicModelServiceFixture models{test::DeterministicChatReply::Failure};
    const auto controller = makeController(models);

    QVERIFY(controller->sendMessage(QStringLiteral("will fail")));
    QTRY_VERIFY_WITH_TIMEOUT(!controller->chatGenerationActive(), 3000);
    const auto message = controller->chatHistory().last();
    QCOMPARE(message.status, core::ChatMessageStatus::Failed);
    QVERIFY(!message.content.trimmed().isEmpty());
    QCOMPARE(controller->chatErrorCategory(), QStringLiteral("ConnectionFailed"));
}

void ChatApplicationControllerTest::cancellationIsImmediatelyTerminalAndLateResultIsIgnored() {
    test::DeterministicModelServiceFixture models{test::DeterministicChatReply::Delayed};
    models.state->delayMs = 150;
    const auto controller = makeController(models);

    QVERIFY(controller->sendMessage(QStringLiteral("cancel this")));
    QTRY_VERIFY_WITH_TIMEOUT(controller->chatGenerationActive(), 1000);
    QVERIFY(controller->stopChatGeneration());
    QVERIFY(!controller->chatGenerationActive());
    QTest::qWait(250);

    const auto message = controller->chatHistory().last();
    QCOMPARE(message.status, core::ChatMessageStatus::Cancelled);
    QCOMPARE(message.errorCategory, core::ChatProviderErrorCategory::Cancelled);
    QVERIFY(!message.content.trimmed().isEmpty());
}

QTEST_MAIN(ChatApplicationControllerTest)
#include "test_chat_application_controller.moc"
