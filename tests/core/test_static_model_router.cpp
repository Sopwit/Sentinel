#include "sentinel/core/StaticModelRouter.h"

#include <QtTest>

using sentinel::core::ModelDescriptor;
using sentinel::core::ModelRoutingStatus;
using sentinel::core::modelRoutingStatusName;
using sentinel::core::ProviderCapabilityProfile;
using sentinel::core::ProviderDescriptor;
using sentinel::core::ProviderKind;
using sentinel::core::RoutingMode;
using sentinel::core::routingModeName;
using sentinel::core::safeModelRouteSummary;
using sentinel::core::StaticModelRouter;
using sentinel::core::TaskClassification;
using sentinel::core::TaskType;
using sentinel::core::taskTypeName;

class StaticModelRouterTest final : public QObject {
    Q_OBJECT

private slots:
    void namesRoutingMetadata();
    void preservesProviderCapabilityProfileMetadata();
    void defaultRouterSelectsLocalPlaceholderDeterministically();
    void unknownTaskFallsBackToLocalPlaceholder();
    void localOnlyRoutingRejectsCloudOnlyModels();
};

void StaticModelRouterTest::namesRoutingMetadata() {
    QCOMPARE(routingModeName(RoutingMode::LocalOnly), QStringLiteral("Local Only"));
    QCOMPARE(taskTypeName(TaskType::ToolPlanning), QStringLiteral("Tool Planning"));
    QCOMPARE(modelRoutingStatusName(ModelRoutingStatus::NoAvailableModel),
             QStringLiteral("No Available Model"));
}

void StaticModelRouterTest::preservesProviderCapabilityProfileMetadata() {
    const ProviderDescriptor provider{
        QStringLiteral("local-a"),
        QStringLiteral("Local A"),
        ProviderKind::Local,
        ProviderCapabilityProfile{
            true,
            false,
            true,
            false,
            true,
            4096,
            QStringLiteral("low"),
            QStringLiteral("none"),
            QStringLiteral("local-only"),
            {taskTypeName(TaskType::Chat), taskTypeName(TaskType::Planning)},
        },
    };

    QCOMPARE(provider.id, QStringLiteral("local-a"));
    QCOMPARE(provider.name, QStringLiteral("Local A"));
    QCOMPARE(provider.kind, ProviderKind::Local);
    QVERIFY(provider.capabilityProfile.local);
    QVERIFY(!provider.capabilityProfile.cloud);
    QVERIFY(provider.capabilityProfile.supportsChat);
    QVERIFY(!provider.capabilityProfile.supportsTools);
    QCOMPARE(provider.capabilityProfile.contextWindowTokens, 4096);
    QCOMPARE(provider.capabilityProfile.privacyPosture, QStringLiteral("local-only"));
    QCOMPARE(provider.capabilityProfile.supportedTaskTypes.size(), 2);
}

void StaticModelRouterTest::defaultRouterSelectsLocalPlaceholderDeterministically() {
    StaticModelRouter router;

    const auto route = router.route(TaskClassification{TaskType::Chat});

    QCOMPARE(router.routingMode(), RoutingMode::LocalOnly);
    QCOMPARE(route.status, ModelRoutingStatus::Routed);
    QCOMPARE(route.routingMode, RoutingMode::LocalOnly);
    QCOMPARE(route.provider.id, QStringLiteral("local-placeholder"));
    QCOMPARE(route.model.id, QStringLiteral("sentinel-local-placeholder"));
    QVERIFY(!route.networkRequired);
    QVERIFY(!route.modelExecutionAllowed);
    QCOMPARE(safeModelRouteSummary(route),
             QStringLiteral("Local Only -> Local Metadata Provider / Sentinel Local Placeholder"));
}

void StaticModelRouterTest::unknownTaskFallsBackToLocalPlaceholder() {
    StaticModelRouter router;

    const auto route = router.route(TaskClassification{TaskType::Unknown});

    QCOMPARE(route.status, ModelRoutingStatus::Routed);
    QCOMPARE(route.task.type, TaskType::Unknown);
    QCOMPARE(route.model.id, QStringLiteral("sentinel-local-placeholder"));
}

void StaticModelRouterTest::localOnlyRoutingRejectsCloudOnlyModels() {
    const QList<ProviderDescriptor> providers{
        ProviderDescriptor{
            QStringLiteral("cloud-a"),
            QStringLiteral("Cloud A"),
            ProviderKind::Cloud,
            ProviderCapabilityProfile{false,
                                      true,
                                      true,
                                      false,
                                      false,
                                      32768,
                                      QStringLiteral("medium"),
                                      QStringLiteral("metered"),
                                      QStringLiteral("cloud"),
                                      {taskTypeName(TaskType::Chat)}},
        },
    };
    const QList<ModelDescriptor> models{
        ModelDescriptor{
            QStringLiteral("cloud-model"),
            QStringLiteral("Cloud Model"),
            QStringLiteral("cloud-a"),
            false,
            true,
            32768,
            QStringLiteral("high"),
            QStringLiteral("medium"),
            {taskTypeName(TaskType::Chat)},
        },
    };
    StaticModelRouter router{RoutingMode::LocalOnly, providers, models};

    const auto route = router.route(TaskClassification{TaskType::Chat});

    QCOMPARE(route.status, ModelRoutingStatus::NoAvailableModel);
    QVERIFY(!route.networkRequired);
    QVERIFY(!route.modelExecutionAllowed);
}

QTEST_MAIN(StaticModelRouterTest)

#include "test_static_model_router.moc"
