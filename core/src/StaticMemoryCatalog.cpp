#include "sentinel/core/StaticMemoryCatalog.h"

#include <algorithm>
#include <utility>

namespace sentinel::core {

namespace {

MemoryShardDescriptor shard(QString id, MemoryType type, QString name,
                            MemoryRetentionPolicy retentionPolicy, MemoryPrivacyLevel privacyLevel,
                            MemoryRecallHint recallHint, QString summary, QStringList tags,
                            QList<MemoryAffinity> affinities,
                            QList<MemoryAssociationDescriptor> associations = {}) {
    return MemoryShardDescriptor{
        std::move(id),
        type,
        std::move(name),
        MemoryShardStatus::Available,
        retentionPolicy,
        privacyLevel,
        recallHint,
        std::move(summary),
        std::move(tags),
        std::move(affinities),
        std::move(associations),
    };
}

QList<MemoryShardDescriptor> defaultShards() {
    return {
        shard(QStringLiteral("episodic"), MemoryType::Episodic, QStringLiteral("Episodic"),
              MemoryRetentionPolicy::UserControlled, MemoryPrivacyLevel::Private,
              MemoryRecallHint::RecentContext,
              QStringLiteral("Episodic (Available, Private, User Controlled)"),
              {QStringLiteral("timeline"), QStringLiteral("conversation-context")},
              {{MemoryType::Episodic, TaskType::Chat, 80},
               {MemoryType::Episodic, TaskType::LongContext, 70}},
              {{QStringLiteral("episodic-semantic"), QStringLiteral("can inform"),
                QStringLiteral("semantic")}}),
        shard(QStringLiteral("semantic"), MemoryType::Semantic, QStringLiteral("Semantic"),
              MemoryRetentionPolicy::Durable, MemoryPrivacyLevel::LocalOnly,
              MemoryRecallHint::StableFact,
              QStringLiteral("Semantic (Available, Local Only, Durable)"),
              {QStringLiteral("facts"), QStringLiteral("knowledge")},
              {{MemoryType::Semantic, TaskType::Summarization, 85},
               {MemoryType::Semantic, TaskType::LongContext, 80},
               {MemoryType::Semantic, TaskType::Coding, 45}}),
        shard(QStringLiteral("procedural"), MemoryType::Procedural, QStringLiteral("Procedural"),
              MemoryRetentionPolicy::Durable, MemoryPrivacyLevel::LocalOnly,
              MemoryRecallHint::UserPreference,
              QStringLiteral("Procedural (Available, Local Only, Durable)"),
              {QStringLiteral("workflow"), QStringLiteral("how-to")},
              {{MemoryType::Procedural, TaskType::Coding, 85},
               {MemoryType::Procedural, TaskType::ToolPlanning, 80},
               {MemoryType::Procedural, TaskType::Planning, 65}}),
        shard(QStringLiteral("reflective"), MemoryType::Reflective, QStringLiteral("Reflective"),
              MemoryRetentionPolicy::UserControlled, MemoryPrivacyLevel::Sensitive,
              MemoryRecallHint::SafetyRelevant,
              QStringLiteral("Reflective (Available, Sensitive, User Controlled)"),
              {QStringLiteral("safety"), QStringLiteral("preferences")},
              {{MemoryType::Reflective, TaskType::SensitiveData, 100},
               {MemoryType::Reflective, TaskType::Planning, 70}}),
        shard(QStringLiteral("ambient"), MemoryType::Ambient, QStringLiteral("Ambient"),
              MemoryRetentionPolicy::Session, MemoryPrivacyLevel::PublicMetadata,
              MemoryRecallHint::None,
              QStringLiteral("Ambient (Available, Public Metadata, Session)"),
              {QStringLiteral("environment"), QStringLiteral("state")},
              {{MemoryType::Ambient, TaskType::Unknown, 50},
               {MemoryType::Ambient, TaskType::Chat, 35}}),
    };
}

void sortShards(QList<MemoryShardDescriptor>& shards) {
    std::sort(shards.begin(), shards.end(),
              [](const MemoryShardDescriptor& left, const MemoryShardDescriptor& right) {
                  return left.id < right.id;
              });

    for (auto& shard : shards) {
        std::sort(shard.associations.begin(), shard.associations.end(),
                  [](const MemoryAssociationDescriptor& left,
                     const MemoryAssociationDescriptor& right) { return left.id < right.id; });
    }
}

} // namespace

StaticMemoryCatalog::StaticMemoryCatalog() : StaticMemoryCatalog(defaultShards()) {}

StaticMemoryCatalog::StaticMemoryCatalog(QList<MemoryShardDescriptor> shards)
    : shards_(std::move(shards)) {
    sortShards(shards_);
}

QList<MemoryShardDescriptor> StaticMemoryCatalog::shards() const {
    return shards_;
}

QStringList StaticMemoryCatalog::shardSummaries() const {
    QStringList summaries;
    for (const auto& shard : shards_) {
        summaries.append(memoryShardSummary(shard));
    }
    return summaries;
}

} // namespace sentinel::core
