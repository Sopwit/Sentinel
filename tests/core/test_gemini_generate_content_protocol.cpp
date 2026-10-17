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
    void testResponseTextOnly();
    void testResponseFunctionCallSynthesis();
    void testResponseMixedParts();
    void testResponseTerminalStates_data();
    void testResponseTerminalStates();
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

void TestGeminiGenerateContentProtocol::testResponseTextOnly() {
    const QJsonObject body{
        {QStringLiteral("candidates"),
         QJsonArray{QJsonObject{
             {QStringLiteral("content"),
              QJsonObject{
                  {QStringLiteral("role"), QStringLiteral("model")},
                  {QStringLiteral("parts"),
                   QJsonArray{QJsonObject{{QStringLiteral("text"), QStringLiteral("hi ")}},
                              QJsonObject{{QStringLiteral("text"), QStringLiteral("there")}}}}}},
             {QStringLiteral("finishReason"), QStringLiteral("STOP")}}}}};
    const auto parsed = parseGeminiGenerateContentResponse(body, QStringLiteral("req-1"));
    QCOMPARE(parsed.status, GeminiGenerateContentResponseStatus::Ok);
    QCOMPARE(parsed.finishReason, QStringLiteral("STOP"));
    QCOMPARE(parsed.text, QStringLiteral("hi there"));
    QVERIFY(parsed.functionCalls.isEmpty());
    QCOMPARE(parsed.modelContent.value(QStringLiteral("role")).toString(), QStringLiteral("model"));
}

void TestGeminiGenerateContentProtocol::testResponseFunctionCallSynthesis() {
    const QJsonObject firstCall{
        {QStringLiteral("name"), QStringLiteral("read-file")},
        {QStringLiteral("args"), QJsonObject{{QStringLiteral("path"), QStringLiteral("a")}}}};
    const QJsonObject explicitCall{{QStringLiteral("id"), QStringLiteral("call_explicit")},
                                   {QStringLiteral("name"), QStringLiteral("write-file")},
                                   {QStringLiteral("args"), QJsonObject{}}};
    const QJsonObject thirdCall{{QStringLiteral("name"), QStringLiteral("read-file")},
                                {QStringLiteral("args"), QJsonObject{}}};
    const QJsonObject body{
        {QStringLiteral("candidates"),
         QJsonArray{QJsonObject{
             {QStringLiteral("content"),
              QJsonObject{{QStringLiteral("parts"),
                           QJsonArray{QJsonObject{{QStringLiteral("functionCall"), firstCall},
                                                  {QStringLiteral("thoughtSignature"),
                                                   QStringLiteral("sig_a")}},
                                      QJsonObject{{QStringLiteral("functionCall"), explicitCall}},
                                      QJsonObject{{QStringLiteral("functionCall"), thirdCall}}}}}},
             {QStringLiteral("finishReason"), QStringLiteral("STOP")}}}}};
    const auto parsed = parseGeminiGenerateContentResponse(body, QStringLiteral("req-7"));
    QCOMPARE(parsed.status, GeminiGenerateContentResponseStatus::Ok);
    QCOMPARE(parsed.functionCalls.size(), 3);
    // Missing ids are synthesized per response; provider ids are forwarded.
    QCOMPARE(parsed.functionCalls.at(0).callId, QStringLiteral("sentinel-gemini-req-7-1"));
    QCOMPARE(parsed.functionCalls.at(0).name, QStringLiteral("read-file"));
    QCOMPARE(parsed.functionCalls.at(0).args.toObject(),
             (QJsonObject{{QStringLiteral("path"), QStringLiteral("a")}}));
    QCOMPARE(parsed.functionCalls.at(0).thoughtSignature, QStringLiteral("sig_a"));
    QCOMPARE(parsed.functionCalls.at(1).callId, QStringLiteral("call_explicit"));
    QVERIFY(parsed.functionCalls.at(1).thoughtSignature.isEmpty());
    QCOMPARE(parsed.functionCalls.at(2).callId, QStringLiteral("sentinel-gemini-req-7-2"));
}

void TestGeminiGenerateContentProtocol::testResponseMixedParts() {
    const QJsonObject call{{QStringLiteral("name"), QStringLiteral("read-file")},
                           {QStringLiteral("args"), QJsonObject{}}};
    const QJsonObject body{
        {QStringLiteral("candidates"),
         QJsonArray{QJsonObject{
             {QStringLiteral("content"),
              QJsonObject{
                  {QStringLiteral("parts"),
                   QJsonArray{QJsonObject{{QStringLiteral("text"), QStringLiteral("thinking")}},
                              QJsonObject{{QStringLiteral("functionCall"), call}},
                              QJsonObject{{QStringLiteral("text"), QStringLiteral(" done")}}}}}},
             {QStringLiteral("finishReason"), QStringLiteral("STOP")}}}}};
    const auto parsed = parseGeminiGenerateContentResponse(body, QStringLiteral("req-9"));
    QCOMPARE(parsed.text, QStringLiteral("thinking done"));
    QCOMPARE(parsed.functionCalls.size(), 1);
    QCOMPARE(parsed.functionCalls.first().callId, QStringLiteral("sentinel-gemini-req-9-1"));
}

void TestGeminiGenerateContentProtocol::testResponseTerminalStates_data() {
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<int>("status");
    const int blocked = int(GeminiGenerateContentResponseStatus::Blocked);
    const int rejected = int(GeminiGenerateContentResponseStatus::Rejected);
    const int noContent = int(GeminiGenerateContentResponseStatus::NoContent);
    const int ok = int(GeminiGenerateContentResponseStatus::Ok);

    QTest::newRow("blocked") << QByteArray(R"({"promptFeedback":{"blockReason":"SAFETY"}})")
                             << blocked;
    QTest::newRow("empty-object") << QByteArray("{}") << noContent;
    QTest::newRow("empty-candidates") << QByteArray(R"({"candidates":[]})") << noContent;
    QTest::newRow("safety")
        << QByteArray(
               R"({"candidates":[{"content":{"parts":[{"text":"x"}]},"finishReason":"SAFETY"}]})")
        << rejected;
    QTest::newRow("prohibited")
        << QByteArray(
               R"({"candidates":[{"content":{"parts":[{"text":"x"}]},"finishReason":"PROHIBITED_CONTENT"}]})")
        << rejected;
    QTest::newRow("no-parts") << QByteArray(R"({"candidates":[{"finishReason":"STOP"}]})")
                              << noContent;
    QTest::newRow("empty-parts")
        << QByteArray(R"({"candidates":[{"content":{"parts":[]},"finishReason":"STOP"}]})")
        << noContent;
    QTest::newRow("content-not-object")
        << QByteArray(R"({"candidates":[{"content":"x","finishReason":"STOP"}]})") << noContent;
    // A block reason is only terminal when the candidates array is empty.
    QTest::newRow("block-reason-ignored-with-candidates")
        << QByteArray(
               R"({"candidates":[{"content":{"parts":[{"text":"x"}]},"finishReason":"STOP"}],"promptFeedback":{"blockReason":"SAFETY"}})")
        << ok;
}

void TestGeminiGenerateContentProtocol::testResponseTerminalStates() {
    QFETCH(QByteArray, body);
    QFETCH(int, status);
    const auto parsed = parseGeminiGenerateContentResponse(QJsonDocument::fromJson(body).object(),
                                                           QStringLiteral("req"));
    QCOMPARE(int(parsed.status), status);
}

QTEST_MAIN(TestGeminiGenerateContentProtocol)
#include "test_gemini_generate_content_protocol.moc"
