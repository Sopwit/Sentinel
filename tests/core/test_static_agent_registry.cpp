#include "sentinel/core/StaticAgentRegistry.h"

#include <QSet>
#include <QtTest>

#include <algorithm>

using sentinel::core::agentDescriptorSummary;
using sentinel::core::AgentPriority;
using sentinel::core::agentPriorityName;
using sentinel::core::AgentRole;
using sentinel::core::agentRoleName;
using sentinel::core::CatalogPrivacyLevel;
using sentinel::core::ProviderKind;
using sentinel::core::StaticAgentRegistry;
using sentinel::core::TaskType;

class StaticAgentRegistryTest final : public QObject {
    Q_OBJECT

private slots:
    void namesAgentMetadata();
    void exposesDeterministicOrderedAgents();
    void keepsAgentIdsUnique();
    void mapsPreferredTaskAffinities();
    void exposesMetadataOnlyAffinities();
};

void StaticAgentRegistryTest::namesAgentMetadata() {
    QCOMPARE(agentRoleName(AgentRole::Coordinator), QStringLiteral("Coordinator"));
    QCOMPARE(agentPriorityName(AgentPriority::Critical), QStringLiteral("Critical"));
}

void StaticAgentRegistryTest::exposesDeterministicOrderedAgents() {
    const StaticAgentRegistry registry;
    const auto agents = registry.agents();

    QCOMPARE(agents.size(), 6);
    QCOMPARE(agents.at(0).id, QStringLiteral("atlas"));
    QCOMPARE(agents.at(1).id, QStringLiteral("orin"));
    QCOMPARE(agents.at(2).id, QStringLiteral("vela"));
    QCOMPARE(agents.at(3).id, QStringLiteral("kaze"));
    QCOMPARE(agents.at(4).id, QStringLiteral("nyx"));
    QCOMPARE(agents.at(5).id, QStringLiteral("sol"));
    QCOMPARE(registry.agentById(QStringLiteral("nyx")).displayName, QStringLiteral("Nyx"));
}

void StaticAgentRegistryTest::keepsAgentIdsUnique() {
    const StaticAgentRegistry registry;
    QSet<QString> ids;

    for (const auto& agent : registry.agents()) {
        QVERIFY(!agent.id.isEmpty());
        QVERIFY(!ids.contains(agent.id));
        ids.insert(agent.id);
    }
}

void StaticAgentRegistryTest::mapsPreferredTaskAffinities() {
    const StaticAgentRegistry registry;

    const auto nyx = registry.agentById(QStringLiteral("nyx"));
    const auto kaze = registry.agentById(QStringLiteral("kaze"));
    QVERIFY(std::any_of(
        nyx.taskAffinities.cbegin(), nyx.taskAffinities.cend(), [](const auto& affinity) {
            return affinity.taskType == TaskType::SensitiveData && affinity.weight == 100;
        }));
    QVERIFY(std::any_of(kaze.taskAffinities.cbegin(), kaze.taskAffinities.cend(),
                        [](const auto& affinity) {
                            return affinity.taskType == TaskType::Coding && affinity.weight == 95;
                        }));
}

void StaticAgentRegistryTest::exposesMetadataOnlyAffinities() {
    const StaticAgentRegistry registry;
    const auto vela = registry.agentById(QStringLiteral("vela"));

    QCOMPARE(vela.capabilityProfile.providerAffinity, ProviderKind::Cloud);
    QCOMPARE(vela.capabilityProfile.privacyAffinity, CatalogPrivacyLevel::CloudMetadataOnly);
    QVERIFY(agentDescriptorSummary(vela).contains(QStringLiteral("Cloud")));
}

QTEST_MAIN(StaticAgentRegistryTest)

#include "test_static_agent_registry.moc"
