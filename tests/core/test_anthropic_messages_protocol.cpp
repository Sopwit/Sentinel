// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace sentinel::core::provider;

class TestAnthropicMessagesProtocol : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testUrl();
    void testBuildBodyDefaultTokens();
    void testBuildBodyExplicitTokensAndSystem();
    void testParseTextSuccess();
    void testParseTextMixedBlocks();
    void testParseTextMissingContent();
    void testParseTextEmptyContent();
    void testByteEquivalence();
};

void TestAnthropicMessagesProtocol::testUrl() {
    const auto url = anthropicMessagesUrl();
    QCOMPARE(url.toString(), QStringLiteral("https://api.anthropic.com/v1/messages"));
}

void TestAnthropicMessagesProtocol::testBuildBodyDefaultTokens() {
    QJsonArray messages;
    QJsonObject msg;
    msg.insert(QStringLiteral("role"), QStringLiteral("user"));
    msg.insert(QStringLiteral("content"), QStringLiteral("Hello Anthropic"));
    messages.append(msg);

    const auto body = buildAnthropicMessagesBody(QStringLiteral("claude-3-5-sonnet-20241022"),
                                                messages, 0, false);

    QCOMPARE(body.value(QStringLiteral("model")).toString(),
             QStringLiteral("claude-3-5-sonnet-20241022"));
    QCOMPARE(body.value(QStringLiteral("max_tokens")).toInt(), 2048);
    QCOMPARE(body.value(QStringLiteral("stream")).toBool(), false);
    QVERIFY(!body.contains(QStringLiteral("system")));
    QCOMPARE(body.value(QStringLiteral("messages")).toArray().size(), 1);
}

void TestAnthropicMessagesProtocol::testBuildBodyExplicitTokensAndSystem() {
    QJsonArray messages;
    QJsonObject msg;
    msg.insert(QStringLiteral("role"), QStringLiteral("user"));
    msg.insert(QStringLiteral("content"), QStringLiteral("Ping"));
    messages.append(msg);

    const auto body = buildAnthropicMessagesBody(QStringLiteral("claude-3-opus"), messages, 4096,
                                                true, QStringLiteral("You are a helpful assistant."));

    QCOMPARE(body.value(QStringLiteral("model")).toString(), QStringLiteral("claude-3-opus"));
    QCOMPARE(body.value(QStringLiteral("max_tokens")).toInt(), 4096);
    QCOMPARE(body.value(QStringLiteral("stream")).toBool(), true);
    QCOMPARE(body.value(QStringLiteral("system")).toString(),
             QStringLiteral("You are a helpful assistant."));
}

void TestAnthropicMessagesProtocol::testParseTextSuccess() {
    QJsonObject block1;
    block1.insert(QStringLiteral("type"), QStringLiteral("text"));
    block1.insert(QStringLiteral("text"), QStringLiteral("Hello "));

    QJsonObject block2;
    block2.insert(QStringLiteral("type"), QStringLiteral("text"));
    block2.insert(QStringLiteral("text"), QStringLiteral("world!"));

    QJsonArray content;
    content.append(block1);
    content.append(block2);

    QJsonObject response;
    response.insert(QStringLiteral("content"), content);

    AnthropicMessagesTextError error = AnthropicMessagesTextError::None;
    const auto text = parseAnthropicMessagesText(response, &error);

    QVERIFY(text.has_value());
    QCOMPARE(*text, QStringLiteral("Hello world!"));
    QCOMPARE(error, AnthropicMessagesTextError::None);
}

void TestAnthropicMessagesProtocol::testParseTextMixedBlocks() {
    QJsonObject toolBlock;
    toolBlock.insert(QStringLiteral("type"), QStringLiteral("tool_use"));
    toolBlock.insert(QStringLiteral("id"), QStringLiteral("tool_123"));
    toolBlock.insert(QStringLiteral("name"), QStringLiteral("get_weather"));

    QJsonObject textBlock;
    textBlock.insert(QStringLiteral("type"), QStringLiteral("text"));
    textBlock.insert(QStringLiteral("text"), QStringLiteral("Checking weather..."));

    QJsonArray content;
    content.append(toolBlock);
    content.append(textBlock);

    QJsonObject response;
    response.insert(QStringLiteral("content"), content);

    AnthropicMessagesTextError error = AnthropicMessagesTextError::None;
    const auto text = parseAnthropicMessagesText(response, &error);

    QVERIFY(text.has_value());
    QCOMPARE(*text, QStringLiteral("Checking weather..."));
    QCOMPARE(error, AnthropicMessagesTextError::None);
}

void TestAnthropicMessagesProtocol::testParseTextMissingContent() {
    QJsonObject response;
    response.insert(QStringLiteral("id"), QStringLiteral("msg_123"));

    AnthropicMessagesTextError error = AnthropicMessagesTextError::None;
    const auto text = parseAnthropicMessagesText(response, &error);

    QVERIFY(!text.has_value());
    QCOMPARE(error, AnthropicMessagesTextError::MissingContent);
}

void TestAnthropicMessagesProtocol::testParseTextEmptyContent() {
    QJsonObject response;
    response.insert(QStringLiteral("content"), QJsonArray());

    AnthropicMessagesTextError error = AnthropicMessagesTextError::None;
    const auto text = parseAnthropicMessagesText(response, &error);

    QVERIFY(!text.has_value());
    QCOMPARE(error, AnthropicMessagesTextError::MissingText);
}

void TestAnthropicMessagesProtocol::testByteEquivalence() {
    QJsonArray messages;
    QJsonObject msg;
    msg.insert(QStringLiteral("role"), QStringLiteral("user"));
    msg.insert(QStringLiteral("content"), QStringLiteral("hello world"));
    messages.append(msg);

    const auto body = buildAnthropicMessagesBody(QStringLiteral("claude-test"), messages, 512, false);
    const auto doc = QJsonDocument(body);
    const auto raw = doc.toJson(QJsonDocument::Compact);

    // Verify raw JSON keys are exact
    QVERIFY(raw.contains("\"max_tokens\":512"));
    QVERIFY(raw.contains("\"model\":\"claude-test\""));
    QVERIFY(raw.contains("\"stream\":false"));
    QVERIFY(raw.contains("\"messages\":[{\"content\":\"hello world\",\"role\":\"user\"}]"));
}

QTEST_MAIN(TestAnthropicMessagesProtocol)
#include "test_anthropic_messages_protocol.moc"
