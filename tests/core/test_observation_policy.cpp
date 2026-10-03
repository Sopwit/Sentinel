// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "sentinel/core/agent/ObservationPolicy.h"
#include "sentinel/core/interfaces/IChatProvider.h"

using namespace sentinel::core;

namespace {

class FixedReplyProvider final : public IChatProvider {
public:
    QString name() const override {
        return QStringLiteral("fixed-reply");
    }
    ChatProviderStatus status() const override {
        return ChatProviderStatus::Ready;
    }
    ChatProviderReply sendMessage(const QString&) override {
        return {true, reply, {}};
    }
    ChatProviderReply sendRequest(const QString& message,
                                  const ChatRequestOptions& options) override {
        request = options;
        prompt = message;
        if (options.cancellationToken && options.cancellationToken->load()) {
            ChatProviderReply result;
            result.lifecycle = ChatRequestLifecycle::Cancelled;
            result.category = ChatProviderErrorCategory::Cancelled;
            result.errorMessage = QStringLiteral("Cancelled");
            return result;
        }
        return sendMessage(message);
    }

    QString reply;
    QString prompt;
    ChatRequestOptions request;
};

} // namespace

class ObservationPolicyTest final : public QObject {
    Q_OBJECT

private slots:
    void classifierPropagatesCancellationWithoutOtherPayload() {
        FixedReplyProvider provider;
        auto token = std::make_shared<std::atomic_bool>(true);
        ObservationIntentPolicy policy(&provider, token);
        const auto intent = policy.classify(QStringLiteral("List the workspace"), {}, {});
        QCOMPARE(provider.request.cancellationToken, token);
        QVERIFY(intent.indeterminate);
        QCOMPARE(intent.error, QStringLiteral("Cancelled"));
        QVERIFY(provider.request.tools.isEmpty());
        QVERIFY(!provider.request.nativeToolCalling);
        QVERIFY(!provider.request.structuredOutput);
        QVERIFY(provider.request.priorToolCalls.isEmpty());
        QVERIFY(provider.request.toolResults.isEmpty());
        QVERIFY(provider.prompt.contains(QStringLiteral("Return ONLY JSON")));
    }

    void responseContract_data() {
        QTest::addColumn<QString>("reply");
        QTest::addColumn<bool>("accepted");
        QTest::newRow("bare") << QStringLiteral("{\"requirements\":[]}") << true;
        QTest::newRow("whole-fence")
            << QStringLiteral("```json\n{\"requirements\":[]}\n```") << true;
        QTest::newRow("prose") << QStringLiteral("Result: {\"requirements\":[]}") << false;
        QTest::newRow("empty") << QString{} << false;
        QTest::newRow("reasoning-only")
            << QStringLiteral("<think>Need to inspect the workspace</think>") << false;
        QTest::newRow("malformed") << QStringLiteral("{\"requirements\":[}") << false;
        QTest::newRow("wrong-schema") << QStringLiteral("{\"intent\":\"filesystem\"}") << false;
        QTest::newRow("invalid-domain")
            << QStringLiteral("{\"requirements\":[{\"domain\":\"invented\"}]}") << false;
    }

    void responseContract() {
        QFETCH(QString, reply);
        QFETCH(bool, accepted);
        FixedReplyProvider provider;
        provider.reply = reply;
        ObservationIntentPolicy policy(&provider);
        const auto intent = policy.classify(QStringLiteral("List the workspace"), {}, {});
        QCOMPARE(!intent.indeterminate, accepted);
        QVERIFY(intent.requirements.isEmpty());
    }

    void acceptsSingleFencedJsonObject() {
        FixedReplyProvider provider;
        provider.reply = QStringLiteral("```json\n{\"requirements\":[{\"domain\":\"filesystem\","
                                        "\"resource\":\"\",\"purpose\":\"inspect\","
                                        "\"operation\":\"none\",\"query\":\"\"}]}\n```");
        ObservationIntentPolicy policy(&provider);

        const auto intent = policy.classify(QStringLiteral("List the workspace"), {}, {});

        QVERIFY(!intent.indeterminate);
        QCOMPARE(intent.requirements.size(), 1);
        QCOMPARE(intent.requirements.first().domain, ObservationDomain::FileSystem);
        QCOMPARE(intent.requirements.first().purpose, ObservationPurpose::Inspect);
    }

    void rejectsFencedJsonWithSurroundingProse() {
        FixedReplyProvider provider;
        provider.reply = QStringLiteral("Here is the result:\n```json\n{\"requirements\":[]}\n```");
        ObservationIntentPolicy policy(&provider);

        const auto intent = policy.classify(QStringLiteral("List the workspace"), {}, {});

        QVERIFY(intent.indeterminate);
    }
};

QTEST_APPLESS_MAIN(ObservationPolicyTest)

#include "test_observation_policy.moc"
