#include "sentinel/core/NullAgentRuntime.h"

#include <QtTest>

using sentinel::core::AgentRequest;
using sentinel::core::AgentStatus;
using sentinel::core::agentStatusName;
using sentinel::core::NullAgentRuntime;
using sentinel::core::ToolDescriptor;
using sentinel::core::ToolExecutionMode;
using sentinel::core::ToolInvocationPlanStatus;
using sentinel::core::ToolParameterDescriptor;
using sentinel::core::ToolRiskLevel;

class NullAgentRuntimeTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesDeterministicIdentityAndStatus();
    void exposesPlaceholderCapability();
    void exposesMetadataTools();
    void returnsDeterministicMetadataOnlyPlan();
    void reportsEmptyPlanRequest();
    void reportsNoToolsAvailableForPlanning();
    void reportsUnknownRequestedTool();
    void preservesPlanMetadata();
    void handlesEmptyRequests();
    void returnsDeterministicLocalResponse();
};

static ToolDescriptor makeTool(const QString& id, const QString& name, ToolRiskLevel riskLevel) {
    return ToolDescriptor{
        id,
        name,
        QStringLiteral("metadata"),
        riskLevel,
        ToolExecutionMode::MetadataOnly,
        {
            ToolParameterDescriptor{QStringLiteral("topic"), QStringLiteral("Topic"), true},
            ToolParameterDescriptor{QStringLiteral("style"), QStringLiteral("Style"), false},
        },
    };
}

void NullAgentRuntimeTest::exposesDeterministicIdentityAndStatus() {
    NullAgentRuntime runtime;

    QCOMPARE(runtime.name(), QStringLiteral("NullAgentRuntime"));
    QCOMPARE(runtime.status(), AgentStatus::Ready);
    QCOMPARE(agentStatusName(runtime.status()), QStringLiteral("Ready"));
}

void NullAgentRuntimeTest::exposesPlaceholderCapability() {
    NullAgentRuntime runtime;
    const auto capabilities = runtime.capabilities();

    QCOMPARE(capabilities.size(), 1);
    QCOMPARE(capabilities.first().id, QStringLiteral("placeholder-local-response"));
    QVERIFY(capabilities.first().enabled);
}

void NullAgentRuntimeTest::exposesMetadataTools() {
    NullAgentRuntime runtime;
    const auto tools = runtime.availableTools();

    QCOMPARE(tools.size(), 1);
    QCOMPARE(tools.first().id, QStringLiteral("local-plan-summary"));
    QCOMPARE(tools.first().executionMode, sentinel::core::ToolExecutionMode::MetadataOnly);
}

void NullAgentRuntimeTest::returnsDeterministicMetadataOnlyPlan() {
    NullAgentRuntime runtime(QList<ToolDescriptor>{
        makeTool(QStringLiteral("tool-z"), QStringLiteral("Tool Z"), ToolRiskLevel::Low),
        makeTool(QStringLiteral("tool-a"), QStringLiteral("Tool A"), ToolRiskLevel::Medium),
    });

    const auto first = runtime.plan(AgentRequest{QStringLiteral("plan next step")});
    const auto second = runtime.plan(AgentRequest{QStringLiteral("plan next step")});

    QCOMPARE(first.status, ToolInvocationPlanStatus::Planned);
    QCOMPARE(second.status, ToolInvocationPlanStatus::Planned);
    QCOMPARE(first.summary, second.summary);
    QCOMPARE(first.invocations.size(), 2);
    QCOMPARE(second.invocations.size(), 2);
    QCOMPARE(first.invocations.at(0).toolId, QStringLiteral("tool-a"));
    QCOMPARE(first.invocations.at(1).toolId, QStringLiteral("tool-z"));
    QCOMPARE(first.invocations.at(0).rationale, second.invocations.at(0).rationale);
}

void NullAgentRuntimeTest::reportsEmptyPlanRequest() {
    NullAgentRuntime runtime;

    const auto plan = runtime.plan(AgentRequest{QStringLiteral("   ")});

    QCOMPARE(plan.status, ToolInvocationPlanStatus::EmptyRequest);
    QCOMPARE(plan.summary, QStringLiteral("Agent request was empty."));
    QVERIFY(plan.invocations.isEmpty());
}

void NullAgentRuntimeTest::reportsNoToolsAvailableForPlanning() {
    NullAgentRuntime runtime(QList<ToolDescriptor>{});

    const auto plan = runtime.plan(AgentRequest{QStringLiteral("plan next step")});

    QCOMPARE(plan.status, ToolInvocationPlanStatus::NoToolsAvailable);
    QCOMPARE(plan.summary, QStringLiteral("No tool metadata is available for planning."));
    QVERIFY(plan.invocations.isEmpty());
}

void NullAgentRuntimeTest::reportsUnknownRequestedTool() {
    NullAgentRuntime runtime;

    const auto plan = runtime.plan(
        AgentRequest{QStringLiteral("plan next step"), QStringLiteral("missing-tool")});

    QCOMPARE(plan.status, ToolInvocationPlanStatus::UnknownTool);
    QCOMPARE(plan.summary, QStringLiteral("Requested tool metadata was not found: missing-tool"));
    QVERIFY(plan.invocations.isEmpty());
}

void NullAgentRuntimeTest::preservesPlanMetadata() {
    NullAgentRuntime runtime(QList<ToolDescriptor>{
        makeTool(QStringLiteral("tool-a"), QStringLiteral("Tool A"), ToolRiskLevel::High),
    });

    const auto plan =
        runtime.plan(AgentRequest{QStringLiteral("summarize topic"), QStringLiteral("tool-a")});

    QCOMPARE(plan.status, ToolInvocationPlanStatus::Planned);
    QCOMPARE(plan.invocations.size(), 1);
    const auto invocation = plan.invocations.first();
    QCOMPARE(invocation.toolId, QStringLiteral("tool-a"));
    QCOMPARE(invocation.toolName, QStringLiteral("Tool A"));
    QCOMPARE(invocation.riskLevel, ToolRiskLevel::High);
    QCOMPARE(invocation.executionMode, ToolExecutionMode::MetadataOnly);
    QCOMPARE(invocation.arguments.size(), 2);
    QCOMPARE(invocation.arguments.first().id, QStringLiteral("topic"));
    QCOMPARE(invocation.arguments.first().value, QStringLiteral("summarize topic"));
    QCOMPARE(invocation.arguments.last().id, QStringLiteral("style"));
    QVERIFY(invocation.arguments.last().value.isEmpty());
}

void NullAgentRuntimeTest::handlesEmptyRequests() {
    NullAgentRuntime runtime;
    const auto response = runtime.execute(AgentRequest{QStringLiteral("   ")});

    QVERIFY(!response.success);
    QCOMPARE(response.status, AgentStatus::Ready);
    QCOMPARE(response.message, QStringLiteral("Agent request was empty."));
}

void NullAgentRuntimeTest::returnsDeterministicLocalResponse() {
    NullAgentRuntime runtime;

    const auto first = runtime.execute(AgentRequest{QStringLiteral("plan next step")});
    const auto second = runtime.execute(AgentRequest{QStringLiteral("plan next step")});

    QVERIFY(first.success);
    QVERIFY(second.success);
    QCOMPARE(first.status, AgentStatus::Ready);
    QCOMPARE(second.status, AgentStatus::Ready);
    QCOMPARE(first.message, second.message);
    QCOMPARE(first.message, QStringLiteral("Agent executed command successfully: plan next step"));
}

QTEST_MAIN(NullAgentRuntimeTest)

#include "test_null_agent_runtime.moc"
