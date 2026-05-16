#pragma once

#include "sentinel/core/ModelRouting.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class MemoryType : std::uint8_t {
    Episodic,
    Semantic,
    Procedural,
    Reflective,
    Ambient,
};

enum class MemoryShardStatus : std::uint8_t {
    Available,
    Planned,
    Disabled,
};

enum class MemoryRetentionPolicy : std::uint8_t {
    Session,
    Durable,
    UserControlled,
};

enum class MemoryPrivacyLevel : std::uint8_t {
    LocalOnly,
    Private,
    Sensitive,
    PublicMetadata,
};

enum class MemoryRecallHint : std::uint8_t {
    None,
    RecentContext,
    StableFact,
    UserPreference,
    SafetyRelevant,
};

inline QString memoryTypeName(MemoryType type) {
    switch (type) {
    case MemoryType::Episodic:
        return QStringLiteral("Episodic");
    case MemoryType::Semantic:
        return QStringLiteral("Semantic");
    case MemoryType::Procedural:
        return QStringLiteral("Procedural");
    case MemoryType::Reflective:
        return QStringLiteral("Reflective");
    case MemoryType::Ambient:
        return QStringLiteral("Ambient");
    }

    return QStringLiteral("Episodic");
}

inline QString memoryShardStatusName(MemoryShardStatus status) {
    switch (status) {
    case MemoryShardStatus::Available:
        return QStringLiteral("Available");
    case MemoryShardStatus::Planned:
        return QStringLiteral("Planned");
    case MemoryShardStatus::Disabled:
        return QStringLiteral("Disabled");
    }

    return QStringLiteral("Disabled");
}

inline QString memoryRetentionPolicyName(MemoryRetentionPolicy policy) {
    switch (policy) {
    case MemoryRetentionPolicy::Session:
        return QStringLiteral("Session");
    case MemoryRetentionPolicy::Durable:
        return QStringLiteral("Durable");
    case MemoryRetentionPolicy::UserControlled:
        return QStringLiteral("User Controlled");
    }

    return QStringLiteral("Session");
}

inline QString memoryPrivacyLevelName(MemoryPrivacyLevel privacyLevel) {
    switch (privacyLevel) {
    case MemoryPrivacyLevel::LocalOnly:
        return QStringLiteral("Local Only");
    case MemoryPrivacyLevel::Private:
        return QStringLiteral("Private");
    case MemoryPrivacyLevel::Sensitive:
        return QStringLiteral("Sensitive");
    case MemoryPrivacyLevel::PublicMetadata:
        return QStringLiteral("Public Metadata");
    }

    return QStringLiteral("Local Only");
}

inline QString memoryRecallHintName(MemoryRecallHint hint) {
    switch (hint) {
    case MemoryRecallHint::None:
        return QStringLiteral("None");
    case MemoryRecallHint::RecentContext:
        return QStringLiteral("Recent Context");
    case MemoryRecallHint::StableFact:
        return QStringLiteral("Stable Fact");
    case MemoryRecallHint::UserPreference:
        return QStringLiteral("User Preference");
    case MemoryRecallHint::SafetyRelevant:
        return QStringLiteral("Safety Relevant");
    }

    return QStringLiteral("None");
}

struct MemoryAffinity {
    MemoryType type = MemoryType::Episodic;
    TaskType taskType = TaskType::Unknown;
    int weight = 0;
};

struct MemoryAssociationDescriptor {
    QString id;
    QString label;
    QString targetMemoryTypeId;
};

struct MemoryShardDescriptor {
    QString id;
    MemoryType type = MemoryType::Episodic;
    QString displayName;
    MemoryShardStatus status = MemoryShardStatus::Planned;
    MemoryRetentionPolicy retentionPolicy = MemoryRetentionPolicy::Session;
    MemoryPrivacyLevel privacyLevel = MemoryPrivacyLevel::LocalOnly;
    MemoryRecallHint recallHint = MemoryRecallHint::None;
    QString summary;
    QStringList tags;
    QList<MemoryAffinity> affinities;
    QList<MemoryAssociationDescriptor> associations;
};

inline bool isMemoryShardAvailable(MemoryShardStatus status) {
    return status == MemoryShardStatus::Available;
}

inline QString memoryShardSummary(const MemoryShardDescriptor& shard) {
    if (!shard.summary.isEmpty()) {
        return shard.summary;
    }

    return QStringLiteral("%1 (%2, %3, %4)")
        .arg(shard.displayName, memoryTypeName(shard.type), memoryShardStatusName(shard.status),
             memoryPrivacyLevelName(shard.privacyLevel));
}

} // namespace sentinel::core
