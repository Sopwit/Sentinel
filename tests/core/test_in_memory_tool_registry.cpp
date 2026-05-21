#include "sentinel/core/InMemoryToolRegistry.h"

#include <QtTest>

using sentinel::core::InMemoryToolRegistry;
using sentinel::core::ToolDescriptor;
using sentinel::core::ToolExecutionMode;
using sentinel::core::ToolParameterDescriptor;
using sentinel::core::ToolRiskLevel;

class InMemoryToolRegistryTest final : public QObject {
    Q_OBJECT

private slots:
    void returnsEmptyStateByDefault();
    void registersAndFindsToolById();
    void trimsToolIdsOnRegistrationAndLookup();
    void rejectsBlankIds();
    void listsToolsDeterministicallyById();
    void rejectsDuplicateIds();
    void preservesDescriptorMetadata();
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

    if (!found.has_value()) {
        QFAIL("Expected tool-a to be registered.");
    }
    const auto& tool = *found;
    QCOMPARE(tool.id, QStringLiteral("tool-a"));
    QCOMPARE(tool.name, QStringLiteral("Tool A"));
}

void InMemoryToolRegistryTest::trimsToolIdsOnRegistrationAndLookup() {
    InMemoryToolRegistry registry;

    QVERIFY(
        registry.registerTool(makeTool(QStringLiteral("  tool-a  "), QStringLiteral("Tool A"))));

    const auto found = registry.findToolById(QStringLiteral(" tool-a "));
    if (!found.has_value()) {
        QFAIL("Expected trimmed tool-a lookup to succeed.");
    }
    const auto& tool = *found;
    QCOMPARE(tool.id, QStringLiteral("tool-a"));
}

void InMemoryToolRegistryTest::rejectsBlankIds() {
    InMemoryToolRegistry registry;

    QVERIFY(!registry.registerTool(makeTool(QStringLiteral("   "), QStringLiteral("Blank Tool"))));
    QVERIFY(registry.listTools().isEmpty());
    QVERIFY(!registry.findToolById(QStringLiteral("   ")).has_value());
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
        makeTool(QStringLiteral(" tool-a "), QStringLiteral("Tool A Duplicate"))));

    const auto tools = registry.listTools();
    QCOMPARE(tools.size(), 1);
    QCOMPARE(tools.first().name, QStringLiteral("Tool A"));
}

void InMemoryToolRegistryTest::preservesDescriptorMetadata() {
    InMemoryToolRegistry registry;

    QVERIFY(registry.registerTool(ToolDescriptor{
        QStringLiteral("tool-a"),
        QStringLiteral("Tool A"),
        QStringLiteral("Detailed metadata"),
        ToolRiskLevel::Medium,
        ToolExecutionMode::MetadataOnly,
        {
            ToolParameterDescriptor{
                QStringLiteral("topic"),
                QStringLiteral("Summary topic"),
                true,
            },
            ToolParameterDescriptor{
                QStringLiteral("style"),
                QStringLiteral("Output style"),
                false,
            },
        },
    }));

    const auto found = registry.findToolById(QStringLiteral("tool-a"));
    if (!found.has_value()) {
        QFAIL("Expected tool-a metadata lookup to succeed.");
    }
    const auto& tool = *found;
    QCOMPARE(tool.description, QStringLiteral("Detailed metadata"));
    QCOMPARE(tool.riskLevel, ToolRiskLevel::Medium);
    QCOMPARE(tool.executionMode, ToolExecutionMode::MetadataOnly);
    QCOMPARE(tool.parameters.size(), 2);
    QCOMPARE(tool.parameters.first().id, QStringLiteral("topic"));
    QVERIFY(tool.parameters.first().required);
    QCOMPARE(tool.parameters.last().id, QStringLiteral("style"));
    QVERIFY(!tool.parameters.last().required);
}

QTEST_MAIN(InMemoryToolRegistryTest)

#include "test_in_memory_tool_registry.moc"
