// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/desktop/ExtensionsHubPresentation.h"
#include <QTest>
using sentinel::desktop::ExtensionsHubPresentation;
class TestExtensionsHub : public QObject {
    Q_OBJECT
private slots:
    void liveInventoryUsesLifecycleAndOmitsPrivateFields() {
        const QVariantMap row{{"id", "mcp:live"}, {"type", "mcp"}, {"name", "Live server"},
            {"available", false}, {"health", "Failed"}, {"metadataStale", true},
            {"actions", QStringList{"disable", "connect", "install"}},
            {"source", "private/path"}, {"credentials", "secret"}, {"failureDetail", "private stderr"}};
        auto result = ExtensionsHubPresentation::inventory(true, true,
            {{"entries", QVariantList{row}}, {"toolEntries", QVariantList{}}}, {}, {});
        const auto safe = result.value("entries").toList().first().toMap();
        QCOMPARE(safe.value("health").toString(), QString("Failed"));
        QVERIFY(!safe.value("available").toBool());
        QCOMPARE(safe.value("actions").toStringList(), QStringList{"disable"});
        QVERIFY(!safe.contains("source"));
        QVERIFY(!safe.contains("credentials"));
        QVERIFY(!safe.contains("failureDetail"));
        auto invalid = row;
        invalid["available"] = "true";
        result = ExtensionsHubPresentation::inventory(true, true,
            {{"entries", QVariantList{invalid}}, {"toolEntries", QVariantList{}}}, {}, {});
        QVERIFY(!result.value("entries").toList().first().toMap().contains("available"));
    }
    void disconnectedClearsStaleMetadata() {
        const QVariantMap data{{"value", QVariantList{QVariantMap{{"id", "extensions.plugin:stale"},
                                                                  {"value", true}}}}};
        const auto result = ExtensionsHubPresentation::inventory(false, true, data, {"stale.tool"},
                                                                 {"stale.policy"});
        QCOMPARE(result.value("phase").toString(), "disconnected");
        QVERIFY(result.value("entries").toList().isEmpty());
        QVERIFY(result.value("tools").toStringList().isEmpty());
        QCOMPARE(ExtensionsHubPresentation::detail(false, "plugin:stale", {{"available", true}})
                     .value("phase")
                     .toString(),
                 "disconnected");
    }
    void loadingErrorAndUnsupportedAreDistinct() {
        QCOMPARE(
            ExtensionsHubPresentation::inventory(true, false, {}, {}, {}).value("phase").toString(),
            "loading");
        QCOMPARE(ExtensionsHubPresentation::inventory(true, true,
                                                      {{"code", "UnknownSettingsAction"}}, {}, {})
                     .value("phase")
                     .toString(),
                 "error");
        QCOMPARE(ExtensionsHubPresentation::inventory(true, true, {{"unexpected", true}}, {}, {})
                     .value("phase")
                     .toString(),
                 "unavailable");
        const auto empty =
            ExtensionsHubPresentation::inventory(true, true, {{"value", QVariantList{}}}, {}, {});
        QCOMPARE(empty.value("phase").toString(), "ready");
        QVERIFY(empty.value("entries").toList().isEmpty());
    }
    void authoritativeNamespacesAndEnablement() {
        QVariantList rows;
        for (const auto& id : {"plugin:real", "mcp:server", "skill:guide", "future:unknown"})
            rows.append(QVariantMap{{"id", "extensions." + QString(id)},
                                    {"value", false},
                                    {"keywords", QStringList{id, "Actual name"}}});
        rows.append(rows.first());
        rows.append(QVariantMap{{"id", "extensions.plugin:unknown-flag"}, {"value", "true"}});
        const auto result = ExtensionsHubPresentation::inventory(
            true, true, {{"value", rows}}, {"tool.registered"}, {"Refused by policy"});
        const auto entries = result.value("entries").toList();
        QCOMPARE(entries.size(), 4);
        QCOMPARE(entries.first().toMap().value("name").toString(), "Actual name");
        QVERIFY(!entries.first().toMap().value("enabled").toBool());
        QVERIFY(entries.first().toMap().value("enabledKnown").toBool());
        QVERIFY(!entries.last().toMap().value("enabledKnown").toBool());
        QVERIFY(!entries.first().toMap().contains("available"));
        QCOMPARE(result.value("tools").toStringList(), QStringList{"tool.registered"});
    }
    void detailsDoNotInferAvailabilityOrExposeCredentials() {
        QCOMPARE(ExtensionsHubPresentation::detail(true, "mcp:a", {}).value("phase").toString(),
                 "loading");
        QCOMPARE(
            ExtensionsHubPresentation::detail(true, "mcp:a", {}, true).value("phase").toString(),
            "unavailable");
        QCOMPARE(ExtensionsHubPresentation::detail(true, "mcp:a", {{"id", "mcp:b"}})
                     .value("phase")
                     .toString(),
                 "unavailable");
        const auto detail = ExtensionsHubPresentation::detail(true, "mcp:a",
                                                              {{"id", "mcp:a"},
                                                               {"effectiveEnabled", true},
                                                               {"available", false},
                                                               {"credentials", "secret"},
                                                               {"source", "https://private"}});
        QVERIFY(detail.value("effectiveEnabled").toBool());
        QVERIFY(!detail.value("available").toBool());
        QVERIFY(!detail.contains("credentials"));
        QVERIFY(!detail.contains("source"));
        const auto unknown = ExtensionsHubPresentation::detail(
            true, "plugin:p", {{"id", "plugin:p"}, {"available", "true"}});
        QVERIFY(!unknown.contains("available"));
    }
};
QTEST_GUILESS_MAIN(TestExtensionsHub)
#include "test_extensions_hub.moc"
