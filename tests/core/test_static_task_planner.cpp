#include "sentinel/core/StaticProviderCatalog.h"
#include "sentinel/core/StaticTaskPlanner.h"

#include <QtTest>

using sentinel::core::CatalogAvailability;
using sentinel::core::CatalogPrivacyLevel;
using sentinel::core::ProviderCapabilityProfile;
using sentinel::core::ProviderCatalogEntry;
using sentinel::core::ProviderDescriptor;
using sentinel::core::ProviderKind;
using sentinel::core::RoutingMode;
using sentinel::core::StaticProviderCatalog;
using sentinel::core::StaticTaskPlanner;
using sentinel::core::TaskClassification;
using sentinel::core::TaskPlanningRequest;
using sentinel::core::TaskPlanStatus;
using sentinel::core::taskPlanStatusName;
using sentinel::core::TaskType;
using sentinel::core::taskTypeName;

class StaticTaskPlannerTest final : public QObject {
    Q_OBJECT

private slots:
    void namesTaskPlanStatus();
    void createsDeterministicLocalPlan();
    void sensitiveTaskStaysLocal();
    void cloudUnavailableUsesLocalFallback();
    void unknownTaskUsesSafeLocalFallback();
    void unavailableCloudOnlyCatalogIsBlocked();
    void plannedStepsStayOrdered();
};

static QList<ProviderCatalogEntry> defaultCatalogEntries() {
    return StaticProviderCatalog{}.entries();
}

static ProviderCatalogEntry unavailableCloudOnlyEntry() {
    return ProviderCatalogEntry{
        ProviderDescriptor{
            QStringLiteral("cloud-only"),
            QStringLiteral("Cloud Only"),
            ProviderKind::Cloud,
            ProviderCapabilityProfile{
                false,
                true,
                true,
                false,
                false,
                32768,
                QStringLiteral("unknown"),
                QStringLiteral("metered"),
                QStringLiteral("cloud-not-configured"),
                {taskTypeName(TaskType::Chat)},
            },
        },
        CatalogAvailability::NotConfigured,
        CatalogPrivacyLevel::CloudMetadataOnly,
        0,
        0,
        QStringLiteral("Cloud Only (Cloud, Not Configured)"),
        {},
    };
}

void StaticTaskPlannerTest::namesTaskPlanStatus() {
    QCOMPARE(taskPlanStatusName(TaskPlanStatus::NotRequested), QStringLiteral("Not Requested"));
    QCOMPARE(taskPlanStatusName(TaskPlanStatus::FallbackPlanned),
             QStringLiteral("Fallback Planned"));
}

void StaticTaskPlannerTest::createsDeterministicLocalPlan() {
    const StaticTaskPlanner planner;

    const auto plan = planner.plan(TaskPlanningRequest{
        TaskClassification{TaskType::Chat},
        RoutingMode::LocalOnly,
        defaultCatalogEntries(),
    });

    QCOMPARE(plan.status, TaskPlanStatus::Planned);
    QCOMPARE(plan.routingMode, RoutingMode::LocalOnly);
    QCOMPARE(plan.capabilityGraph.nodes.size(), 4);
    QCOMPARE(plan.steps.size(), 2);
    QCOMPARE(plan.steps.last().providerId, QStringLiteral("local-placeholder"));
    QCOMPARE(plan.steps.last().modelId, QStringLiteral("sentinel-local-placeholder"));
    QVERIFY(!plan.networkRequired);
    QVERIFY(!plan.modelExecutionAllowed);
    QCOMPARE(plan.summary,
             QStringLiteral("Task plan uses local metadata route: Local Metadata Provider / "
                            "Sentinel Local Placeholder."));
}

void StaticTaskPlannerTest::sensitiveTaskStaysLocal() {
    const StaticTaskPlanner planner;

    const auto plan = planner.plan(TaskPlanningRequest{
        TaskClassification{TaskType::SensitiveData, true, QStringLiteral("private context")},
        RoutingMode::CloudAllowed,
        defaultCatalogEntries(),
    });

    QCOMPARE(plan.status, TaskPlanStatus::Planned);
    QCOMPARE(plan.steps.last().providerKind, ProviderKind::Local);
    QCOMPARE(plan.steps.last().providerId, QStringLiteral("local-placeholder"));
    QVERIFY(plan.steps.last().localOnly);
    QVERIFY(!plan.networkRequired);
}

void StaticTaskPlannerTest::cloudUnavailableUsesLocalFallback() {
    const StaticTaskPlanner planner;

    const auto plan = planner.plan(TaskPlanningRequest{
        TaskClassification{TaskType::Chat},
        RoutingMode::CloudAllowed,
        defaultCatalogEntries(),
    });

    QCOMPARE(plan.status, TaskPlanStatus::FallbackPlanned);
    QCOMPARE(plan.steps.last().providerKind, ProviderKind::Local);
    QVERIFY(plan.summary.startsWith(QStringLiteral("Cloud metadata is unavailable")));
}

void StaticTaskPlannerTest::unknownTaskUsesSafeLocalFallback() {
    const StaticTaskPlanner planner;

    const auto plan = planner.plan(TaskPlanningRequest{
        TaskClassification{TaskType::Unknown},
        RoutingMode::LocalOnly,
        defaultCatalogEntries(),
    });

    QCOMPARE(plan.status, TaskPlanStatus::FallbackPlanned);
    QCOMPARE(plan.steps.last().providerId, QStringLiteral("local-placeholder"));
    QVERIFY(plan.summary.startsWith(QStringLiteral("Unknown task uses safe local metadata")));
}

void StaticTaskPlannerTest::unavailableCloudOnlyCatalogIsBlocked() {
    const StaticTaskPlanner planner;

    const auto plan = planner.plan(TaskPlanningRequest{
        TaskClassification{TaskType::Chat},
        RoutingMode::CloudAllowed,
        {unavailableCloudOnlyEntry()},
    });

    QCOMPARE(plan.status, TaskPlanStatus::Blocked);
    QVERIFY(plan.steps.isEmpty());
    QCOMPARE(plan.summary, QStringLiteral("Cloud metadata candidates are unavailable or not "
                                          "configured."));
    QVERIFY(!plan.networkRequired);
    QVERIFY(!plan.modelExecutionAllowed);
}

void StaticTaskPlannerTest::plannedStepsStayOrdered() {
    const StaticTaskPlanner planner;

    const auto plan = planner.plan(TaskPlanningRequest{
        TaskClassification{TaskType::Planning},
        RoutingMode::Balanced,
        defaultCatalogEntries(),
    });

    QCOMPARE(plan.status, TaskPlanStatus::Planned);
    QCOMPARE(plan.steps.size(), 2);
    QCOMPARE(plan.steps.at(0).order, 1);
    QCOMPARE(plan.steps.at(0).id, QStringLiteral("step-1-capability-graph"));
    QCOMPARE(plan.steps.at(1).order, 2);
    QCOMPARE(plan.steps.at(1).id, QStringLiteral("step-2-sentinel-local-placeholder"));
}

QTEST_MAIN(StaticTaskPlannerTest)

#include "test_static_task_planner.moc"
