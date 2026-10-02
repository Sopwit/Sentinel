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
    QString name() const override { return QStringLiteral("fixed-reply"); }
    ChatProviderStatus status() const override { return ChatProviderStatus::Ready; }
    ChatProviderReply sendMessage(const QString&) override { return {true, reply, {}}; }

    QString reply;
};

} // namespace

class ObservationPolicyTest final : public QObject {
    Q_OBJECT

private slots:
    void acceptsSingleFencedJsonObject() {
        FixedReplyProvider provider;
        provider.reply = QStringLiteral(
            "```json\n{\"requirements\":[{\"domain\":\"filesystem\","
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
        provider.reply = QStringLiteral(
            "Here is the result:\n```json\n{\"requirements\":[]}\n```");
        ObservationIntentPolicy policy(&provider);

        const auto intent = policy.classify(QStringLiteral("List the workspace"), {}, {});

        QVERIFY(intent.indeterminate);
    }
};

QTEST_APPLESS_MAIN(ObservationPolicyTest)

#include "test_observation_policy.moc"
