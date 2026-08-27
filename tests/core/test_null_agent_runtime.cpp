// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/NullAgentRuntime.h"

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
    void exposesLocalExecutionCapability();
    void exposesMetadataTools();
    void returnsDeterministicMetadataOnlyPlan();
    void reportsEmptyPlanRequest();
    void reportsNoToolsAvailableForPlanning();
    void reportsUnknownRequestedTool();
    void preservesPlanMetadata();
    void handlesEmptyRequests();
    void returnsDeterministicLocalResponse();
    void detectsTurkishWebSearchIntent();
    void detectsEnglishWebSearchIntent();
    void detectsWeatherWebSearchIntent();
    void keepsOrdinaryPromptsLocal();
    void routesDomainLaunchRequestToOpenUrl();
    void routesWwwAndHttpsLaunchRequestsToOpenUrl();
    void routesTurkishSuffixedDomainToOpenUrl();
    void keepsApplicationLaunchRequestsOnAppLaunch();
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

void NullAgentRuntimeTest::exposesLocalExecutionCapability() {
    NullAgentRuntime runtime;
    const auto capabilities = runtime.capabilities();

    QCOMPARE(capabilities.size(), 1);
    QCOMPARE(capabilities.first().id, QStringLiteral("local-plan-execution"));
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
    QCOMPARE(first.message, QStringLiteral("Executed: Local Plan Summary"));
}

void NullAgentRuntimeTest::detectsTurkishWebSearchIntent() {
    NullAgentRuntime runtime(NullAgentRuntime::standardTools());

    const auto plan =
        runtime.plan(AgentRequest{QStringLiteral("Bugünün en son teknoloji haberleri neler?")});

    QCOMPARE(plan.status, ToolInvocationPlanStatus::Planned);
    QCOMPARE(plan.invocations.size(), 1);
    QCOMPARE(plan.invocations.first().toolId, QStringLiteral("web-search"));
    QCOMPARE(plan.invocations.first().arguments.first().value,
             QStringLiteral("Bugünün en son teknoloji haberleri neler?"));
}

void NullAgentRuntimeTest::detectsEnglishWebSearchIntent() {
    NullAgentRuntime runtime(NullAgentRuntime::standardTools());

    const auto plan = runtime.plan(AgentRequest{QStringLiteral("What is the latest Qt release?")});

    QCOMPARE(plan.status, ToolInvocationPlanStatus::Planned);
    QCOMPARE(plan.invocations.first().toolId, QStringLiteral("web-search"));
}

void NullAgentRuntimeTest::detectsWeatherWebSearchIntent() {
    NullAgentRuntime runtime(NullAgentRuntime::standardTools());

    const auto plan =
        runtime.plan(AgentRequest{QStringLiteral("İstanbul'da bugün hava durumu nasıl?")});

    QCOMPARE(plan.status, ToolInvocationPlanStatus::Planned);
    QCOMPARE(plan.invocations.first().toolId, QStringLiteral("web-search"));
}

void NullAgentRuntimeTest::keepsOrdinaryPromptsLocal() {
    NullAgentRuntime runtime(NullAgentRuntime::standardTools());

    const auto plan = runtime.plan(AgentRequest{QStringLiteral("Write a short welcome message.")});

    QCOMPARE(plan.status, ToolInvocationPlanStatus::Planned);
    QCOMPARE(plan.invocations.first().toolId, QStringLiteral("run-command"));
}

void NullAgentRuntimeTest::routesDomainLaunchRequestToOpenUrl() {
    NullAgentRuntime runtime(NullAgentRuntime::standardTools());

    const auto plan = runtime.plan(AgentRequest{QStringLiteral("sahibinden.com aç")});

    QCOMPARE(plan.status, ToolInvocationPlanStatus::Planned);
    QCOMPARE(plan.invocations.first().toolId, QStringLiteral("open-url"));
    QCOMPARE(plan.invocations.first().arguments.first().id, QStringLiteral("url"));
    QCOMPARE(plan.invocations.first().arguments.first().value, QStringLiteral("sahibinden.com"));
}

void NullAgentRuntimeTest::routesWwwAndHttpsLaunchRequestsToOpenUrl() {
    NullAgentRuntime runtime(NullAgentRuntime::standardTools());

    const auto wwwPlan = runtime.plan(AgentRequest{QStringLiteral("aç www.sahibinden.com")});
    QCOMPARE(wwwPlan.invocations.first().toolId, QStringLiteral("open-url"));
    QCOMPARE(wwwPlan.invocations.first().arguments.first().value,
             QStringLiteral("www.sahibinden.com"));

    const auto httpsPlan = runtime.plan(AgentRequest{QStringLiteral("https://example.com open")});
    QCOMPARE(httpsPlan.invocations.first().toolId, QStringLiteral("open-url"));
    QCOMPARE(httpsPlan.invocations.first().arguments.first().value,
             QStringLiteral("https://example.com"));
}

void NullAgentRuntimeTest::routesTurkishSuffixedDomainToOpenUrl() {
    NullAgentRuntime runtime(NullAgentRuntime::standardTools());

    const auto plan = runtime.plan(AgentRequest{QStringLiteral("sahibinden.com'u aç")});

    QCOMPARE(plan.status, ToolInvocationPlanStatus::Planned);
    QCOMPARE(plan.invocations.first().toolId, QStringLiteral("open-url"));
    QCOMPARE(plan.invocations.first().arguments.first().value, QStringLiteral("sahibinden.com"));
}

void NullAgentRuntimeTest::keepsApplicationLaunchRequestsOnAppLaunch() {
    NullAgentRuntime runtime(NullAgentRuntime::standardTools());

    const auto plan = runtime.plan(AgentRequest{QStringLiteral("spotify aç")});

    QCOMPARE(plan.status, ToolInvocationPlanStatus::Planned);
    QCOMPARE(plan.invocations.first().toolId, QStringLiteral("app-launch"));
    QCOMPARE(plan.invocations.first().arguments.first().value, QStringLiteral("Spotify"));
}

QTEST_MAIN(NullAgentRuntimeTest)

#include "test_null_agent_runtime.moc"
