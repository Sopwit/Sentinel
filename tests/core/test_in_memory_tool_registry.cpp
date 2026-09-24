// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/ToolExecutionGateway.h"

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
    void handlerRegistrationAndProviderRemoval();
    void builtInHandlersExecuteThroughRegistry();
    void rejectsNonExecutableDynamicTools();
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

void InMemoryToolRegistryTest::handlerRegistrationAndProviderRemoval() {
    class Handler final : public sentinel::core::IToolHandler {
    public:
        sentinel::core::IToolExecutor::Cancel
        execute(const sentinel::core::ToolExecutionRequest&, const QString&, const QString&,
                sentinel::core::IToolExecutor::Output,
                sentinel::core::IToolExecutor::Completion completion) override {
            completion({sentinel::core::ToolExecutionStatus::Succeeded, QStringLiteral("done")});
            return {};
        }
    };
    InMemoryToolRegistry registry;
    auto descriptor = makeTool(QStringLiteral("plugin.example.action"), QStringLiteral("Action"));
    descriptor.source = sentinel::core::ToolSource::Plugin;
    descriptor.providerId = QStringLiteral("example");
    descriptor.inputSchema = QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
                                         {QStringLiteral("additionalProperties"), false}};
    auto handler = std::make_shared<Handler>();
    QVERIFY(registry.registerTool({descriptor, handler}));
    QVERIFY(!registry.registerTool({descriptor, handler}));
    auto resolved = registry.findRegistration(descriptor.id);
    QVERIFY(resolved.has_value());
    QCOMPARE(resolved->handler, handler);
    QCOMPARE(registry.enabledTools().size(), 1);
    QCOMPARE(
        registry.unregisterProvider(sentinel::core::ToolSource::MCP, QStringLiteral("example")), 0);
    QCOMPARE(
        registry.unregisterProvider(sentinel::core::ToolSource::Plugin, QStringLiteral("example")),
        1);
    QVERIFY(!registry.findRegistration(descriptor.id).has_value());
    QVERIFY(resolved->handler == handler); // Active invocation snapshot retains its handler.
}

void InMemoryToolRegistryTest::rejectsNonExecutableDynamicTools() {
    InMemoryToolRegistry registry;
    auto descriptor = makeTool(QStringLiteral("mcp.server.search"), QStringLiteral("Search"));
    descriptor.source = sentinel::core::ToolSource::MCP;
    descriptor.providerId = QStringLiteral("mcp:server");
    QVERIFY(!registry.registerTool(descriptor));
    QVERIFY(registry.enabledTools().isEmpty());
    descriptor.id = QStringLiteral("read-file");
    QVERIFY(!registry.registerTool(descriptor));
}

void InMemoryToolRegistryTest::builtInHandlersExecuteThroughRegistry() {
    class NoFallback final : public sentinel::core::IToolExecutor {
    public:
        mutable int calls = 0;
        sentinel::core::ToolExecutionResult
        execute(const sentinel::core::ToolExecutionRequest&) const override {
            ++calls;
            return {sentinel::core::ToolExecutionStatus::Blocked, QStringLiteral("fallback")};
        }
    } fallback;
    sentinel::core::RealToolExecutor executor;
    InMemoryToolRegistry registry;
    QVERIFY(sentinel::core::BuiltInToolProvider::registerTools(registry, executor));
    QCOMPARE(registry.enabledTools().size(), 42);
    for (const auto& id :
         {QStringLiteral("read-file"), QStringLiteral("write-file"), QStringLiteral("grep"),
          QStringLiteral("run-command"), QStringLiteral("web-search"),
          QStringLiteral("memory-search"), QStringLiteral("spawn-agent")}) {
        const auto registration = registry.findRegistration(id);
        QVERIFY(registration && registration->handler);
        QCOMPARE(registration->descriptor.source, sentinel::core::ToolSource::BuiltIn);
        QCOMPARE(registration->descriptor.providerId, QStringLiteral("builtin"));
    }
    sentinel::core::ToolExecutionGateway gateway(&registry);
    auto run = [&](const QString& id, const QList<sentinel::core::ToolInvocationArgument>& args,
                   sentinel::core::ApprovalStatus approval =
                       sentinel::core::ApprovalStatus::Approved,
                   sentinel::core::SandboxStatus sandbox = sentinel::core::SandboxStatus::Allowed) {
        sentinel::core::ToolExecutionRequest request;
        request.plan.status = sentinel::core::ToolInvocationPlanStatus::Planned;
        sentinel::core::PlannedToolInvocation invocation;
        invocation.toolId = id;
        invocation.arguments = args;
        request.plan.invocations.append(invocation);
        request.knownToolIds = {id};
        request.approval.status = approval;
        request.sandbox.status = sandbox;
        sentinel::core::ToolExecutionResult result;
        bool done = false;
        auto active = gateway.executeAsync(request, fallback, QStringLiteral("session"),
                                           QStringLiteral("call"), {}, [&](auto value) {
                                               result = std::move(value);
                                               done = true;
                                           });
        if (!QTest::qWaitFor([&done] { return done; }, 5000))
            return sentinel::core::ToolExecutionResult{sentinel::core::ToolExecutionStatus::Blocked,
                                                       QStringLiteral("timed out")};
        return result;
    };
    QCOMPARE(
        run(QStringLiteral("read-file"), {{QStringLiteral("path"), QStringLiteral("AGENTS.md")}})
            .status,
        sentinel::core::ToolExecutionStatus::Succeeded);
    QVERIFY(run(QStringLiteral("grep"), {{QStringLiteral("pattern"), QStringLiteral("Sentinel")},
                                         {QStringLiteral("path"), QStringLiteral("AGENTS.md")}})
                .summary.contains(QStringLiteral("Sentinel")));
#ifndef Q_OS_WIN
    const auto commandResult =
        run(QStringLiteral("run-command"),
            {{QStringLiteral("command"), QStringLiteral("printf native-registry")}});
    QVERIFY2(commandResult.summary.contains(QStringLiteral("native-registry")),
             qPrintable(commandResult.summary));
    QCOMPARE(run(QStringLiteral("run-command"),
                 {{QStringLiteral("command"), QStringLiteral("printf denied")}},
                 sentinel::core::ApprovalStatus::Denied)
                 .status,
             sentinel::core::ToolExecutionStatus::Blocked);
    QCOMPARE(run(QStringLiteral("run-command"),
                 {{QStringLiteral("command"), QStringLiteral("printf denied")}},
                 sentinel::core::ApprovalStatus::Approved, sentinel::core::SandboxStatus::Denied)
                 .status,
             sentinel::core::ToolExecutionStatus::Blocked);
#endif
    QCOMPARE(fallback.calls, 0);
    QVERIFY(registry.setEnabled(QStringLiteral("grep"), false));
    QCOMPARE(run(QStringLiteral("grep"), {{QStringLiteral("pattern"), QStringLiteral("Sentinel")}})
                 .status,
             sentinel::core::ToolExecutionStatus::Blocked);
    QVERIFY(registry.setEnabled(QStringLiteral("grep"), true));
}

QTEST_MAIN(InMemoryToolRegistryTest)

#include "test_in_memory_tool_registry.moc"
