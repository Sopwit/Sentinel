// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QUrlQuery>

using namespace sentinel::core::provider;

class TestGeminiGenerateContentProtocol : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testUrlGeneration();
    void testStreamUrlGeneration();
    void testBuildBody();
    void testParseTextSuccess();
    void testParseTextMissingCandidates();
    void testParseTextMissingContent();
    void testParseTextEmptyParts();
    void testByteEquivalence();
    void testNativeRequestBody();
    void testNativeRequestBodyWithToolsAndStructured();
    void testFunctionResponseParts();
};

void TestGeminiGenerateContentProtocol::testUrlGeneration() {
    const auto url = geminiGenerateContentUrl(QStringLiteral("gemini-1.5-pro"), QStringLiteral("secret_api_key"));
    QCOMPARE(url.host(), QStringLiteral("generativelanguage.googleapis.com"));
    QCOMPARE(url.path(), QStringLiteral("/v1beta/models/gemini-1.5-pro:generateContent"));
    const QUrlQuery query(url);
    QCOMPARE(query.queryItemValue(QStringLiteral("key")), QStringLiteral("secret_api_key"));
}

void TestGeminiGenerateContentProtocol::testStreamUrlGeneration() {
    const auto url = geminiStreamGenerateContentUrl(QStringLiteral("gemini-1.5-flash"), QStringLiteral("my_key"));
    QCOMPARE(url.host(), QStringLiteral("generativelanguage.googleapis.com"));
    QCOMPARE(url.path(), QStringLiteral("/v1beta/models/gemini-1.5-flash:streamGenerateContent"));
    const QUrlQuery query(url);
    QCOMPARE(query.queryItemValue(QStringLiteral("alt")), QStringLiteral("sse"));
    QCOMPARE(query.queryItemValue(QStringLiteral("key")), QStringLiteral("my_key"));
}

void TestGeminiGenerateContentProtocol::testBuildBody() {
    QJsonObject part;
    part.insert(QStringLiteral("text"), QStringLiteral("Translate this to French"));
    QJsonArray parts;
    parts.append(part);

    QJsonObject content;
    content.insert(QStringLiteral("role"), QStringLiteral("user"));
    content.insert(QStringLiteral("parts"), parts);
    QJsonArray contents;
    contents.append(content);

    const auto body = buildGeminiGenerateContentBody(contents, 1024, 0.7, 0.95);

    QCOMPARE(body.value(QStringLiteral("contents")).toArray().size(), 1);
    const auto genConfig = body.value(QStringLiteral("generationConfig")).toObject();
    QCOMPARE(genConfig.value(QStringLiteral("maxOutputTokens")).toInt(), 1024);
    QCOMPARE(genConfig.value(QStringLiteral("temperature")).toDouble(), 0.7);
    QCOMPARE(genConfig.value(QStringLiteral("topP")).toDouble(), 0.95);
}

void TestGeminiGenerateContentProtocol::testParseTextSuccess() {
    QJsonObject part1;
    part1.insert(QStringLiteral("text"), QStringLiteral("Bonjour "));

    QJsonObject part2;
    part2.insert(QStringLiteral("text"), QStringLiteral("le monde!"));

    QJsonArray parts;
    parts.append(part1);
    parts.append(part2);

    QJsonObject content;
    content.insert(QStringLiteral("parts"), parts);

    QJsonObject candidate;
    candidate.insert(QStringLiteral("content"), content);

    QJsonArray candidates;
    candidates.append(candidate);

    QJsonObject response;
    response.insert(QStringLiteral("candidates"), candidates);

    GeminiGenerateContentTextError error = GeminiGenerateContentTextError::None;
    const auto text = parseGeminiGenerateContentText(response, &error);

    QVERIFY(text.has_value());
    QCOMPARE(*text, QStringLiteral("Bonjour le monde!"));
    QCOMPARE(error, GeminiGenerateContentTextError::None);
}

void TestGeminiGenerateContentProtocol::testParseTextMissingCandidates() {
    QJsonObject response;
    GeminiGenerateContentTextError error = GeminiGenerateContentTextError::None;
    const auto text = parseGeminiGenerateContentText(response, &error);

    QVERIFY(!text.has_value());
    QCOMPARE(error, GeminiGenerateContentTextError::MissingCandidates);
}

void TestGeminiGenerateContentProtocol::testParseTextMissingContent() {
    QJsonObject candidate;
    QJsonArray candidates;
    candidates.append(candidate);

    QJsonObject response;
    response.insert(QStringLiteral("candidates"), candidates);

    GeminiGenerateContentTextError error = GeminiGenerateContentTextError::None;
    const auto text = parseGeminiGenerateContentText(response, &error);

    QVERIFY(!text.has_value());
    QCOMPARE(error, GeminiGenerateContentTextError::MissingContent);
}

void TestGeminiGenerateContentProtocol::testParseTextEmptyParts() {
    QJsonObject content;
    content.insert(QStringLiteral("parts"), QJsonArray());

    QJsonObject candidate;
    candidate.insert(QStringLiteral("content"), content);

    QJsonArray candidates;
    candidates.append(candidate);

    QJsonObject response;
    response.insert(QStringLiteral("candidates"), candidates);

    GeminiGenerateContentTextError error = GeminiGenerateContentTextError::None;
    const auto text = parseGeminiGenerateContentText(response, &error);

    QVERIFY(!text.has_value());
    QCOMPARE(error, GeminiGenerateContentTextError::MissingText);
}

void TestGeminiGenerateContentProtocol::testByteEquivalence() {
    QJsonObject part;
    part.insert(QStringLiteral("text"), QStringLiteral("hello world"));
    QJsonArray parts;
    parts.append(part);

    QJsonObject content;
    content.insert(QStringLiteral("role"), QStringLiteral("user"));
    content.insert(QStringLiteral("parts"), parts);
    QJsonArray contents;
    contents.append(content);

    const auto body = buildGeminiGenerateContentBody(contents, 2048, 0.0, 1.0);
    const auto doc = QJsonDocument(body);
    const auto raw = doc.toJson(QJsonDocument::Compact);

    QVERIFY(raw.contains("\"maxOutputTokens\":2048"));
    QVERIFY(raw.contains("\"temperature\":0"));
    QVERIFY(raw.contains("\"topP\":1"));
    QVERIFY(raw.contains("\"contents\":[{\"parts\":[{\"text\":\"hello world\"}],\"role\":\"user\"}]"));
}

void TestGeminiGenerateContentProtocol::testNativeRequestBody() {
    QJsonObject part;
    part.insert(QStringLiteral("text"), QStringLiteral("hi"));
    QJsonObject content;
    content.insert(QStringLiteral("role"), QStringLiteral("user"));
    content.insert(QStringLiteral("parts"), QJsonArray{part});
    const QJsonArray contents{content};

    const auto body = buildGeminiNativeRequestBody(QStringLiteral("gemini-test"), contents, 1024);
    // Golden bytes captured from the pre-refactor ModelService::nativeEndpointReply
    // inline assembly. The native body keeps `model` (the transport derives the URL
    // and removes it before dispatch).
    QCOMPARE(
        QJsonDocument(body).toJson(QJsonDocument::Compact),
        QByteArrayLiteral(
            R"({"contents":[{"parts":[{"text":"hi"}],"role":"user"}],"generationConfig":{"maxOutputTokens":1024},"model":"gemini-test"})"));
}

void TestGeminiGenerateContentProtocol::testNativeRequestBodyWithToolsAndStructured() {
    QJsonObject part;
    part.insert(QStringLiteral("text"), QStringLiteral("hi"));
    QJsonObject content;
    content.insert(QStringLiteral("role"), QStringLiteral("user"));
    content.insert(QStringLiteral("parts"), QJsonArray{part});
    const QJsonArray contents{content};
    const QJsonArray declarations{
        QJsonObject{{QStringLiteral("name"), QStringLiteral("read-file")},
                    {QStringLiteral("parametersJsonSchema"),
                     QJsonObject{{QStringLiteral("type"), QStringLiteral("object")}}}}};
    const QJsonObject schema{{QStringLiteral("type"), QStringLiteral("object")}};

    const auto body = buildGeminiNativeRequestBody(QStringLiteral("gemini-test"), contents, 0,
                                                   declarations, schema);
    QCOMPARE(body.value(QStringLiteral("model")).toString(), QStringLiteral("gemini-test"));
    const auto tools = body.value(QStringLiteral("tools")).toArray();
    QCOMPARE(tools.size(), 1);
    QCOMPARE(tools.first().toObject().value(QStringLiteral("functionDeclarations")).toArray(),
             declarations);
    const auto generationConfig = body.value(QStringLiteral("generationConfig")).toObject();
    // maxOutputTokens <= 0 is omitted; structured output still emits the config.
    QVERIFY(!generationConfig.contains(QStringLiteral("maxOutputTokens")));
    QCOMPARE(generationConfig.value(QStringLiteral("responseMimeType")).toString(),
             QStringLiteral("application/json"));
    QCOMPARE(generationConfig.value(QStringLiteral("responseSchema")).toObject(), schema);

    // With no tools and no structured schema, generationConfig is omitted entirely.
    const auto bare = buildGeminiNativeRequestBody(QStringLiteral("gemini-test"), contents, 0);
    QVERIFY(!bare.contains(QStringLiteral("tools")));
    QVERIFY(!bare.contains(QStringLiteral("generationConfig")));
}

void TestGeminiGenerateContentProtocol::testFunctionResponseParts() {
    const QList<GeminiFunctionResult> results{
        {QStringLiteral("call_explicit"), QStringLiteral("read-file"),
         QStringLiteral("case contents")},
        {QStringLiteral("sentinel-gemini-7-1"), QStringLiteral("read-file"),
         QString(7000, QLatin1Char('y'))}};
    const auto parts = geminiFunctionResponseParts(results);
    QCOMPARE(parts.size(), 2);
    const auto explicitResponse =
        parts.at(0).toObject().value(QStringLiteral("functionResponse")).toObject();
    QCOMPARE(explicitResponse.value(QStringLiteral("name")).toString(),
             QStringLiteral("read-file"));
    QCOMPARE(explicitResponse.value(QStringLiteral("response"))
                 .toObject()
                 .value(QStringLiteral("output"))
                 .toString(),
             QStringLiteral("case contents"));
    // A provider-supplied id is forwarded verbatim.
    QCOMPARE(explicitResponse.value(QStringLiteral("id")).toString(),
             QStringLiteral("call_explicit"));
    // A synthesized Sentinel id is internal bookkeeping and never echoed back.
    const auto synthesizedResponse =
        parts.at(1).toObject().value(QStringLiteral("functionResponse")).toObject();
    QVERIFY(!synthesizedResponse.contains(QStringLiteral("id")));
    // Result text is clipped to the native continuation limit.
    QCOMPARE(synthesizedResponse.value(QStringLiteral("response"))
                 .toObject()
                 .value(QStringLiteral("output"))
                 .toString()
                 .size(),
             6800);
}

QTEST_MAIN(TestGeminiGenerateContentProtocol)
#include "test_gemini_generate_content_protocol.moc"
