#include "sentinel/core/RuntimePermissions.h"

#include <QtTest>

using sentinel::core::LocalOnlyRuntimePermissionPolicy;
using sentinel::core::RuntimePermission;
using sentinel::core::RuntimePermissionDecisionStatus;
using sentinel::core::runtimePermissionDecisionStatusName;
using sentinel::core::RuntimePermissionLevel;
using sentinel::core::runtimePermissionLevelName;
using sentinel::core::runtimePermissionName;
using sentinel::core::RuntimePermissionRequest;
using sentinel::core::safeRuntimePermissionDecisionSummary;
using sentinel::core::StaticRuntimePermissionPolicy;

class StaticRuntimePermissionPolicyTest final : public QObject {
    Q_OBJECT

private slots:
    void namesPermissionMetadata();
    void deniesExecutionByDefault();
    void allowsOnlyExplicitLocalInference();
};

void StaticRuntimePermissionPolicyTest::namesPermissionMetadata() {
    QCOMPARE(runtimePermissionName(RuntimePermission::LocalInference),
             QStringLiteral("Local Inference"));
    QCOMPARE(runtimePermissionName(RuntimePermission::NetworkAccess),
             QStringLiteral("Network Access"));
    QCOMPARE(runtimePermissionLevelName(RuntimePermissionLevel::None), QStringLiteral("None"));
    QCOMPARE(runtimePermissionLevelName(RuntimePermissionLevel::ReadOnly),
             QStringLiteral("Read Only"));
    QCOMPARE(runtimePermissionLevelName(RuntimePermissionLevel::Execute),
             QStringLiteral("Execute"));
    QCOMPARE(runtimePermissionDecisionStatusName(RuntimePermissionDecisionStatus::NotRequested),
             QStringLiteral("Not Requested"));
    QCOMPARE(runtimePermissionDecisionStatusName(RuntimePermissionDecisionStatus::Allowed),
             QStringLiteral("Allowed"));
    QCOMPARE(runtimePermissionDecisionStatusName(RuntimePermissionDecisionStatus::Denied),
             QStringLiteral("Denied"));
}

void StaticRuntimePermissionPolicyTest::deniesExecutionByDefault() {
    const StaticRuntimePermissionPolicy policy;

    const auto decision = policy.evaluate(RuntimePermissionRequest{
        RuntimePermission::LocalInference,
        RuntimePermissionLevel::Execute,
        QStringLiteral("runtime-request"),
        QStringLiteral("Evaluate execution permission metadata."),
    });

    QCOMPARE(decision.status, RuntimePermissionDecisionStatus::Denied);
    QCOMPARE(safeRuntimePermissionDecisionSummary(decision),
             QStringLiteral("Runtime permission policy is metadata-only and denies execution by "
                            "default."));
}

void StaticRuntimePermissionPolicyTest::allowsOnlyExplicitLocalInference() {
    const LocalOnlyRuntimePermissionPolicy policy;

    const auto localDecision = policy.evaluate(RuntimePermissionRequest{
        RuntimePermission::LocalInference,
        RuntimePermissionLevel::Execute,
        QStringLiteral("local-chat-request"),
        QStringLiteral("Evaluate explicit local chat inference."),
    });
    QCOMPARE(localDecision.status, RuntimePermissionDecisionStatus::Allowed);
    QCOMPARE(safeRuntimePermissionDecisionSummary(localDecision),
             QStringLiteral("Local-only inference permission is allowed for explicit user chat "
                            "requests."));

    const auto toolDecision = policy.evaluate(RuntimePermissionRequest{
        RuntimePermission::ToolInvocation,
        RuntimePermissionLevel::Execute,
        QStringLiteral("tool-request"),
        QStringLiteral("Evaluate tool execution."),
    });
    QCOMPARE(toolDecision.status, RuntimePermissionDecisionStatus::Denied);
    QCOMPARE(safeRuntimePermissionDecisionSummary(toolDecision),
             QStringLiteral("Runtime permission denied: only explicit local inference is "
                            "allowed."));
}

QTEST_MAIN(StaticRuntimePermissionPolicyTest)

#include "test_static_runtime_permission_policy.moc"
