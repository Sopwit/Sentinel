// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/DeterministicChatFixture.h"

#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/app/ModeManager.h"
#include "sentinel/core/chat/InMemoryConversationStore.h"
#include "sentinel/core/memory/InMemorySettingsStore.h"
#include "sentinel/core/memory/InMemoryStore.h"
#include "sentinel/desktop/DesktopShellViewModel.h"

#include <QtTest>

using namespace sentinel;

class ChatViewModelTest final : public QObject {
    Q_OBJECT

private slots:
    void presentsSelectedBindingAndCompletedReply();
    void presentsCancellationWithoutWaitingForLateProviderResult();
};

void ChatViewModelTest::presentsSelectedBindingAndCompletedReply() {
    test::DeterministicModelServiceFixture models;
    core::ApplicationControllerBuilder builder;
    builder.withMemoryStore(std::make_unique<core::InMemoryStore>())
        .withConversationStore(std::make_unique<core::InMemoryConversationStore>())
        .withModelService(models.takeModelService());
    auto controller = builder.build();
    core::ModeManager modes;
    core::AppSettings settings{std::make_unique<core::InMemorySettingsStore>()};
    settings.setSelectedRuntimeProvider(QStringLiteral("ollama"));
    settings.setSelectedLocalModel(QStringLiteral("sentinel-test-model"));
    desktop::DesktopShellViewModel viewModel{*controller, modes, settings};

    QVERIFY(viewModel.sendMessage(QStringLiteral("show reply")));
    QTRY_VERIFY_WITH_TIMEOUT(!viewModel.chatGenerationActive(), 3000);
    const auto index = viewModel.chatMessages()->index(viewModel.chatMessages()->rowCount() - 1, 0);
    QCOMPARE(viewModel.chatMessages()->data(index, desktop::ChatMessageListModel::ContentRole),
             QStringLiteral("SENTINEL_TEST_RESPONSE"));
    QCOMPARE(viewModel.chatMessages()->data(index, desktop::ChatMessageListModel::StatusRole),
             QStringLiteral("completed"));
    QCOMPARE(viewModel.activeChatProviderId(), QStringLiteral("ollama"));
    QCOMPARE(viewModel.activeChatModelId(), QStringLiteral("sentinel-test-model"));
}

void ChatViewModelTest::presentsCancellationWithoutWaitingForLateProviderResult() {
    test::DeterministicModelServiceFixture models{test::DeterministicChatReply::Delayed};
    models.state->delayMs = 150;
    core::ApplicationControllerBuilder builder;
    builder.withMemoryStore(std::make_unique<core::InMemoryStore>())
        .withConversationStore(std::make_unique<core::InMemoryConversationStore>())
        .withModelService(models.takeModelService());
    auto controller = builder.build();
    core::ModeManager modes;
    core::AppSettings settings{std::make_unique<core::InMemorySettingsStore>()};
    settings.setSelectedRuntimeProvider(QStringLiteral("ollama"));
    settings.setSelectedLocalModel(QStringLiteral("sentinel-test-model"));
    desktop::DesktopShellViewModel viewModel{*controller, modes, settings};

    QVERIFY(viewModel.sendMessage(QStringLiteral("cancel reply")));
    QTRY_VERIFY_WITH_TIMEOUT(viewModel.chatGenerationActive(), 1000);
    QVERIFY(viewModel.cancelLocalInference());
    QVERIFY(!viewModel.chatGenerationActive());
    QTest::qWait(250);
    const auto index = viewModel.chatMessages()->index(viewModel.chatMessages()->rowCount() - 1, 0);
    QCOMPARE(viewModel.chatMessages()->data(index, desktop::ChatMessageListModel::StatusRole),
             QStringLiteral("cancelled"));
}

QTEST_MAIN(ChatViewModelTest)
#include "test_chat_view_model.moc"
