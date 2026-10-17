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
    void testToolDeclaration();
    void testNativeRequestBody();
    void testNativeRequestBodyWithToolsAndStructured();
    void testToolResultBlocks();
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

void TestAnthropicMessagesProtocol::testToolDeclaration() {
    // The OpenAI-style envelope is flattened into the Anthropic tool shape.
    const QJsonObject envelope{
        {QStringLiteral("type"), QStringLiteral("function")},
        {QStringLiteral("function"),
         QJsonObject{{QStringLiteral("name"), QStringLiteral("read-file")},
                     {QStringLiteral("description"), QStringLiteral("Read a file.")},
                     {QStringLiteral("parameters"),
                      QJsonObject{{QStringLiteral("type"), QStringLiteral("object")}}}}}};
    const auto declaration = anthropicToolDeclaration(envelope);
    QCOMPARE(declaration.value(QStringLiteral("name")).toString(), QStringLiteral("read-file"));
    QCOMPARE(declaration.value(QStringLiteral("description")).toString(),
             QStringLiteral("Read a file."));
    QCOMPARE(declaration.value(QStringLiteral("input_schema"))
                 .toObject()
                 .value(QStringLiteral("type"))
                 .toString(),
             QStringLiteral("object"));
    // The Anthropic shape must not reuse the nested OpenAI envelope.
    QVERIFY(!declaration.contains(QStringLiteral("type")));
    QVERIFY(!declaration.contains(QStringLiteral("function")));
}

void TestAnthropicMessagesProtocol::testNativeRequestBody() {
    QJsonArray messages;
    QJsonObject msg;
    msg.insert(QStringLiteral("role"), QStringLiteral("user"));
    msg.insert(QStringLiteral("content"), QStringLiteral("hello world"));
    messages.append(msg);

    const auto body = buildAnthropicNativeRequestBody(QStringLiteral("claude-test"), messages, 512);
    // Golden bytes captured from the pre-refactor ModelService::nativeEndpointReply
    // inline assembly. The native body never carries a `stream` key.
    QCOMPARE(
        QJsonDocument(body).toJson(QJsonDocument::Compact),
        QByteArrayLiteral(
            R"({"max_tokens":512,"messages":[{"content":"hello world","role":"user"}],"model":"claude-test"})"));
}

void TestAnthropicMessagesProtocol::testNativeRequestBodyWithToolsAndStructured() {
    QJsonArray messages;
    messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                {QStringLiteral("content"), QStringLiteral("hi")}});
    QJsonArray tools;
    tools.append(QJsonObject{{QStringLiteral("name"), QStringLiteral("read-file")},
                             {QStringLiteral("description"), QStringLiteral("Read.")},
                             {QStringLiteral("input_schema"),
                              QJsonObject{{QStringLiteral("type"), QStringLiteral("object")}}}});
    const QJsonObject schema{
        {QStringLiteral("type"), QStringLiteral("object")},
        {QStringLiteral("properties"),
         QJsonObject{{QStringLiteral("answer"),
                      QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}}}}};

    const auto body =
        buildAnthropicNativeRequestBody(QStringLiteral("claude-test"), messages, 0, tools, schema);
    QCOMPARE(body.value(QStringLiteral("model")).toString(), QStringLiteral("claude-test"));
    // maxOutputTokens <= 0 falls back to 2048.
    QCOMPARE(body.value(QStringLiteral("max_tokens")).toInt(), 2048);
    QCOMPARE(body.value(QStringLiteral("tools")).toArray(), tools);
    const auto format = body.value(QStringLiteral("output_config"))
                            .toObject()
                            .value(QStringLiteral("format"))
                            .toObject();
    QCOMPARE(format.value(QStringLiteral("type")).toString(), QStringLiteral("json_schema"));
    QCOMPARE(format.value(QStringLiteral("schema")).toObject(), schema);
    QVERIFY(!body.contains(QStringLiteral("stream")));

    // Empty tools/structured schema omit both envelopes.
    const auto bare = buildAnthropicNativeRequestBody(QStringLiteral("claude-test"), messages, 10);
    QVERIFY(!bare.contains(QStringLiteral("tools")));
    QVERIFY(!bare.contains(QStringLiteral("output_config")));
}

void TestAnthropicMessagesProtocol::testToolResultBlocks() {
    const QList<AnthropicToolResult> results{
        {QStringLiteral("toolu_1"), QStringLiteral("case contents")},
        {QStringLiteral("toolu_2"), QString(7000, QLatin1Char('x'))}};
    const auto blocks = anthropicToolResultBlocks(results);
    QCOMPARE(blocks.size(), 2);
    const auto first = blocks.at(0).toObject();
    QCOMPARE(first.value(QStringLiteral("type")).toString(), QStringLiteral("tool_result"));
    QCOMPARE(first.value(QStringLiteral("tool_use_id")).toString(), QStringLiteral("toolu_1"));
    QCOMPARE(first.value(QStringLiteral("content")).toString(), QStringLiteral("case contents"));
    // Result text is clipped to the native continuation limit.
    QCOMPARE(blocks.at(1).toObject().value(QStringLiteral("content")).toString().size(), 6800);
}

QTEST_MAIN(TestAnthropicMessagesProtocol)
#include "test_anthropic_messages_protocol.moc"
