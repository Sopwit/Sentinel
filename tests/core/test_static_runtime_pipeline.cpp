#include "sentinel/core/RuntimePipeline.h"

#include <QtTest>

using sentinel::core::RuntimePermission;
using sentinel::core::RuntimePermissionDecision;
using sentinel::core::RuntimePermissionDecisionStatus;
using sentinel::core::RuntimePermissionLevel;
using sentinel::core::RuntimePermissionRequest;
using sentinel::core::RuntimePipelineStage;
using sentinel::core::runtimePipelineStageName;
using sentinel::core::RuntimePipelineStatus;
using sentinel::core::runtimePipelineStatusName;
using sentinel::core::runtimePipelineTraceSummaries;
using sentinel::core::RuntimeSafetyDecision;
using sentinel::core::RuntimeSafetyPolicy;
using sentinel::core::RuntimeSafetyReport;
using sentinel::core::RuntimeSafetyRule;
using sentinel::core::safeRuntimePipelineSummary;
using sentinel::core::StaticRuntimePipeline;

class StaticRuntimePipelineTest final : public QObject {
    Q_OBJECT

private slots:
    void namesPipelineMetadata();
    void producesOrderedTraceAndBlockedExecutionMetadata();
};

void StaticRuntimePipelineTest::namesPipelineMetadata() {
    QCOMPARE(runtimePipelineStageName(RuntimePipelineStage::RequestReceived),
             QStringLiteral("Request Received"));
    QCOMPARE(runtimePipelineStageName(RuntimePipelineStage::ExecutionBoundary),
             QStringLiteral("Execution Boundary"));
    QCOMPARE(runtimePipelineStatusName(RuntimePipelineStatus::NotRequested),
             QStringLiteral("Not Requested"));
    QCOMPARE(runtimePipelineStatusName(RuntimePipelineStatus::CompletedMetadata),
             QStringLiteral("Completed Metadata"));
    QCOMPARE(runtimePipelineStatusName(RuntimePipelineStatus::Blocked), QStringLiteral("Blocked"));
}

void StaticRuntimePipelineTest::producesOrderedTraceAndBlockedExecutionMetadata() {
    const StaticRuntimePipeline pipeline;
    const auto result = pipeline.evaluate(
        {
            QStringLiteral("runtime-pipeline-request-1"),
            QStringLiteral("Runtime request pipeline metadata evaluation."),
            RuntimePermissionRequest{
                RuntimePermission::LocalInference,
                RuntimePermissionLevel::Execute,
                QStringLiteral("runtime-request"),
                QStringLiteral("Evaluate execution permission metadata."),
            },
        },
        RuntimePermissionDecision{
            RuntimePermissionDecisionStatus::Denied,
            RuntimePermissionRequest{
                RuntimePermission::LocalInference,
                RuntimePermissionLevel::Execute,
                QStringLiteral("runtime-request"),
                QStringLiteral("Evaluate execution permission metadata."),
            },
            QStringLiteral("Runtime permission policy is metadata-only and denies execution by "
                           "default."),
        },
        RuntimeSafetyReport{
            RuntimeSafetyPolicy{
                QStringLiteral("runtime-safety-policy-1"),
                QStringLiteral("Static Runtime Safety Policy"),
                true,
                true,
                QStringLiteral("Local-only and no-execution runtime safety posture is active."),
            },
            QList<RuntimeSafetyRule>{
                RuntimeSafetyRule{
                    QStringLiteral("runtime-safety.no-execution"),
                    QStringLiteral("No Execution"),
                    true,
                    QStringLiteral(
                        "Runtime request metadata cannot execute models, tools, or providers."),
                },
            },
            RuntimeSafetyDecision::Compliant,
            QStringLiteral("Runtime safety policy report: local-only and no-execution posture is "
                           "enforced with deterministic metadata rules."),
        });

    QCOMPARE(result.status, RuntimePipelineStatus::Blocked);
    QVERIFY(result.executionBlocked);
    QCOMPARE(result.traces.size(), 4);
    QCOMPARE(result.traces[0].stage, RuntimePipelineStage::RequestReceived);
    QCOMPARE(result.traces[1].stage, RuntimePipelineStage::PermissionPolicy);
    QCOMPARE(result.traces[2].stage, RuntimePipelineStage::SafetyPolicy);
    QCOMPARE(result.traces[3].stage, RuntimePipelineStage::ExecutionBoundary);
    QCOMPARE(safeRuntimePipelineSummary(result),
             QStringLiteral("Runtime request pipeline blocked execution metadata by permission and "
                            "safety policy."));
    QVERIFY(runtimePipelineTraceSummaries(result.traces)
                .contains(QStringLiteral(
                    "Execution Boundary [Blocked]: Execution boundary remained blocked; no "
                    "runtime action was performed.")));
}

QTEST_MAIN(StaticRuntimePipelineTest)

#include "test_static_runtime_pipeline.moc"
