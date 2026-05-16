#include "sentinel/core/RuntimeCapabilities.h"

#include <QSet>
#include <QtTest>

using sentinel::core::disabledRuntimeCapabilitySummaries;
using sentinel::core::enabledRuntimeCapabilitySummaries;
using sentinel::core::localOnlyRuntimeEnforcementSummary;
using sentinel::core::RuntimeCapabilityGroup;
using sentinel::core::runtimeCapabilityGroupName;
using sentinel::core::RuntimeCapabilityState;
using sentinel::core::runtimeCapabilityStateName;
using sentinel::core::runtimeCapabilitySummary;
using sentinel::core::safeRuntimeNegotiationProfileSummary;
using sentinel::core::safeRuntimeNegotiationSummary;
using sentinel::core::StaticRuntimeCapabilityRegistry;

class StaticRuntimeCapabilityRegistryTest final : public QObject {
    Q_OBJECT

private slots:
    void namesCapabilityMetadata();
    void exposesDeterministicOrderedCapabilities();
    void keepsCapabilityIdsUnique();
    void reportsEnabledAndDisabledCapabilitySummaries();
    void reportsLocalOnlyNegotiationMetadata();
};

void StaticRuntimeCapabilityRegistryTest::namesCapabilityMetadata() {
    QCOMPARE(runtimeCapabilityStateName(RuntimeCapabilityState::Enabled),
             QStringLiteral("Enabled"));
    QCOMPARE(runtimeCapabilityStateName(RuntimeCapabilityState::Disabled),
             QStringLiteral("Disabled"));
    QCOMPARE(runtimeCapabilityStateName(RuntimeCapabilityState::Unavailable),
             QStringLiteral("Unavailable"));
    QCOMPARE(runtimeCapabilityGroupName(RuntimeCapabilityGroup::Inference),
             QStringLiteral("Inference"));
    QCOMPARE(runtimeCapabilityGroupName(RuntimeCapabilityGroup::Memory), QStringLiteral("Memory"));
    QCOMPARE(runtimeCapabilityGroupName(RuntimeCapabilityGroup::Integration),
             QStringLiteral("Integration"));
    QCOMPARE(runtimeCapabilityGroupName(RuntimeCapabilityGroup::Security),
             QStringLiteral("Security"));
    QCOMPARE(runtimeCapabilityGroupName(RuntimeCapabilityGroup::Platform),
             QStringLiteral("Platform"));
}

void StaticRuntimeCapabilityRegistryTest::exposesDeterministicOrderedCapabilities() {
    const StaticRuntimeCapabilityRegistry registry;
    const auto capabilities = registry.capabilities();

    QCOMPARE(capabilities.size(), 13);
    QCOMPARE(capabilities.first().id, QStringLiteral("cloud-relay-support"));
    QCOMPARE(capabilities.last().id, QStringLiteral("tool-bridge"));
}

void StaticRuntimeCapabilityRegistryTest::keepsCapabilityIdsUnique() {
    const StaticRuntimeCapabilityRegistry registry;
    QSet<QString> ids;

    for (const auto& capability : registry.capabilities()) {
        QVERIFY2(!ids.contains(capability.id), qPrintable(capability.id));
        ids.insert(capability.id);
    }

    QCOMPARE(ids.size(), registry.capabilities().size());
}

void StaticRuntimeCapabilityRegistryTest::reportsEnabledAndDisabledCapabilitySummaries() {
    const StaticRuntimeCapabilityRegistry registry;
    const auto capabilities = registry.capabilities();
    const auto enabled = enabledRuntimeCapabilitySummaries(capabilities);
    const auto disabled = disabledRuntimeCapabilitySummaries(capabilities);

    QCOMPARE(enabled.size(), 2);
    QCOMPARE(disabled.size(), 11);
    QVERIFY(
        enabled.contains(QStringLiteral("Local-Only Enforcement (Security, Enabled): "
                                        "Negotiation metadata enforces a local-only posture.")));
    QVERIFY(disabled.contains(QStringLiteral("Local Inference (Inference, Disabled): Local model "
                                             "inference remains disabled.")));
    QVERIFY(disabled.contains(QStringLiteral("External Process Execution (Platform, "
                                             "Unavailable): External process launch is "
                                             "unavailable.")));
    QCOMPARE(runtimeCapabilitySummary(capabilities.first()),
             QStringLiteral("Cloud Relay Support (Integration, Unavailable): Cloud relay metadata "
                            "is unavailable and cannot execute."));
}

void StaticRuntimeCapabilityRegistryTest::reportsLocalOnlyNegotiationMetadata() {
    const StaticRuntimeCapabilityRegistry registry;
    const auto result = registry.negotiate();

    QVERIFY(result.profile.localOnlyEnforced);
    QCOMPARE(safeRuntimeNegotiationProfileSummary(result.profile),
             QStringLiteral("Metadata-only negotiation profile; no runtime capability is "
                            "activated."));
    QCOMPARE(safeRuntimeNegotiationSummary(result),
             QStringLiteral("Metadata-only runtime negotiation: 13 capabilities described, 2 "
                            "enabled as safety metadata."));
    QCOMPARE(localOnlyRuntimeEnforcementSummary(result),
             QStringLiteral("Local-only enforcement is active; cloud relay and external runtime "
                            "execution remain unavailable."));
}

QTEST_MAIN(StaticRuntimeCapabilityRegistryTest)

#include "test_static_runtime_capability_registry.moc"
