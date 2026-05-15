#include "sentinel/core/NullAgentRuntime.h"

#include <QtTest>

using sentinel::core::AgentRequest;
using sentinel::core::AgentStatus;
using sentinel::core::agentStatusName;
using sentinel::core::NullAgentRuntime;

class NullAgentRuntimeTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesDeterministicIdentityAndStatus();
    void exposesPlaceholderCapability();
    void exposesMetadataTools();
    void handlesEmptyRequests();
    void returnsDeterministicLocalResponse();
};

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
    QCOMPARE(first.message, QStringLiteral("Local agent placeholder processed: plan next step"));
}

QTEST_MAIN(NullAgentRuntimeTest)

#include "test_null_agent_runtime.moc"
