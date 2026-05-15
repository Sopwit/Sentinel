#include "sentinel/core/InMemoryToolRegistry.h"

#include <QtTest>

using sentinel::core::InMemoryToolRegistry;
using sentinel::core::ToolDescriptor;

class InMemoryToolRegistryTest final : public QObject {
    Q_OBJECT

private slots:
    void returnsEmptyStateByDefault();
    void registersAndFindsToolById();
    void listsToolsDeterministicallyById();
    void rejectsDuplicateIds();
};

static ToolDescriptor makeTool(const QString& id, const QString& name) {
    return ToolDescriptor{
        id,
        name,
        QStringLiteral("metadata"),
    };
}

void InMemoryToolRegistryTest::returnsEmptyStateByDefault() {
    InMemoryToolRegistry registry;

    QVERIFY(registry.listTools().isEmpty());
    QVERIFY(!registry.findToolById(QStringLiteral("missing")).has_value());
}

void InMemoryToolRegistryTest::registersAndFindsToolById() {
    InMemoryToolRegistry registry;

    QVERIFY(registry.registerTool(makeTool(QStringLiteral("tool-a"), QStringLiteral("Tool A"))));
    const auto found = registry.findToolById(QStringLiteral("tool-a"));

    QVERIFY(found.has_value());
    QCOMPARE(found->id, QStringLiteral("tool-a"));
    QCOMPARE(found->name, QStringLiteral("Tool A"));
}

void InMemoryToolRegistryTest::listsToolsDeterministicallyById() {
    InMemoryToolRegistry registry;

    QVERIFY(registry.registerTool(makeTool(QStringLiteral("tool-z"), QStringLiteral("Tool Z"))));
    QVERIFY(registry.registerTool(makeTool(QStringLiteral("tool-a"), QStringLiteral("Tool A"))));
    QVERIFY(registry.registerTool(makeTool(QStringLiteral("tool-m"), QStringLiteral("Tool M"))));

    const auto tools = registry.listTools();
    QCOMPARE(tools.size(), 3);
    QCOMPARE(tools.at(0).id, QStringLiteral("tool-a"));
    QCOMPARE(tools.at(1).id, QStringLiteral("tool-m"));
    QCOMPARE(tools.at(2).id, QStringLiteral("tool-z"));
}

void InMemoryToolRegistryTest::rejectsDuplicateIds() {
    InMemoryToolRegistry registry;

    QVERIFY(registry.registerTool(makeTool(QStringLiteral("tool-a"), QStringLiteral("Tool A"))));
    QVERIFY(!registry.registerTool(
        makeTool(QStringLiteral("tool-a"), QStringLiteral("Tool A Duplicate"))));

    const auto tools = registry.listTools();
    QCOMPARE(tools.size(), 1);
    QCOMPARE(tools.first().name, QStringLiteral("Tool A"));
}

QTEST_MAIN(InMemoryToolRegistryTest)

#include "test_in_memory_tool_registry.moc"
