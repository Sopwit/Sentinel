// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "sentinel/core/runtime/tools/WebSearchTool.h"

using namespace sentinel::core;

namespace {

// DuckDuckGo HTML endpoint fixture (escaped, not a raw string, because moc's
// lexer mishandles raw string literals spanning the file).
const QString kDuckDuckGoSampleHtml = QStringLiteral(
    "<div class=\"result results_links results_links_deep web-result \">"
    "<div class=\"links_main links_deep result__body\">"
    "<h2 class=\"result__title\">"
    "<a rel=\"nofollow\" class=\"result__a\" "
    "href=\"//duckduckgo.com/l/?uddg=https%3A%2F%2Fexample.com%2Fdocs&amp;rut=abc\">"
    "Example <b>Docs</b> &amp; Guides</a>"
    "</h2>"
    "<a class=\"result__snippet\" "
    "href=\"//duckduckgo.com/l/?uddg=https%3A%2F%2Fexample.com%2Fdocs&amp;rut=abc\">"
    "The official documentation for the &quot;Example&quot; project.</a>"
    "</div></div>"
    "<div class=\"result results_links results_links_deep web-result \">"
    "<div class=\"links_main links_deep result__body\">"
    "<h2 class=\"result__title\">"
    "<a rel=\"nofollow\" class=\"result__a\" href=\"https://direct.example.org/page\">"
    "Direct Link Result</a>"
    "</h2>"
    "<a class=\"result__snippet\" href=\"https://direct.example.org/page\">"
    "A snippet with &lt;embedded&gt; markup.</a>"
    "</div></div>");

} // namespace

class WebSearchToolTest final : public QObject {
    Q_OBJECT

private slots:
    void supportedProvidersIncludeKeylessFallback() {
        const auto providers = WebSearchTool::supportedProviders();
        QVERIFY(providers.contains(QStringLiteral("duckduckgo")));
        QVERIFY(providers.contains(QStringLiteral("exa")));
        QVERIFY(providers.contains(QStringLiteral("parallel")));
    }

    void emptyQueryFails() {
        WebSearchTool tool;
        const auto response = tool.search(QStringLiteral("  "));
        QVERIFY(!response.success);
        QVERIFY(!response.errorString.isEmpty());
    }

    void parseDuckDuckGoResponseExtractsResults() {
        const auto response =
            WebSearchTool::parseDuckDuckGoResponse(kDuckDuckGoSampleHtml.toUtf8(), 5);

        QCOMPARE(response.results.size(), 2);
        QVERIFY(response.success);

        // Wrapped DuckDuckGo redirect URL is unwrapped to the destination.
        QCOMPARE(response.results.at(0).url, QStringLiteral("https://example.com/docs"));
        // Markup in titles is stripped and entities are unescaped.
        QCOMPARE(response.results.at(0).title, QStringLiteral("Example Docs & Guides"));
        QVERIFY(response.results.at(0).snippet.contains(
            QStringLiteral("The official documentation for the \"Example\" project.")));

        // Direct (non-wrapped) URLs pass through untouched.
        QCOMPARE(response.results.at(1).url, QStringLiteral("https://direct.example.org/page"));
        QCOMPARE(response.results.at(1).title, QStringLiteral("Direct Link Result"));
        QVERIFY(response.results.at(1).snippet.contains(QStringLiteral("A snippet with")));
    }

    void parseDuckDuckGoResponseRespectsLimit() {
        const auto response =
            WebSearchTool::parseDuckDuckGoResponse(kDuckDuckGoSampleHtml.toUtf8(), 1);

        QCOMPARE(response.results.size(), 1);
        QCOMPARE(response.results.at(0).url, QStringLiteral("https://example.com/docs"));
    }

    void parseDuckDuckGoResponseHandlesUnknownLayout() {
        const auto response = WebSearchTool::parseDuckDuckGoResponse(
            QByteArrayLiteral("<html><body>nothing here</body></html>"), 5);

        QVERIFY(!response.success);
        QVERIFY(response.results.isEmpty());
        QVERIFY(!response.errorString.isEmpty());
    }
};

QTEST_MAIN(WebSearchToolTest)
#include "test_web_search_tool.moc"
