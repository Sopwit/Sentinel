#pragma once

#include "sentinel/core/MemoryMetadata.h"

#include <QDateTime>
#include <QList>
#include <QString>

#include <cstdint>

namespace sentinel::core {

using MemoryCandidateId = QString;

enum class MemoryCandidateSource : std::uint8_t {
    ConversationText,
    ManualMetadata,
};

enum class MemoryCandidateCategory : std::uint8_t {
    Semantic,
    Preference,
    ProjectContext,
    SafetyNote,
};

enum class MemoryCandidateConfidence : std::uint8_t {
    Low,
    Medium,
    High,
};

enum class MemoryReviewState : std::uint8_t {
    PendingReview,
    Approved,
    Rejected,
};

enum class MemoryCapturePolicy : std::uint8_t {
    MetadataOnly,
    ManualReviewRequired,
    Disabled,
};

QString memoryCandidateSourceName(MemoryCandidateSource source);
QString memoryCandidateCategoryName(MemoryCandidateCategory category);
QString memoryCandidateConfidenceName(MemoryCandidateConfidence confidence);
QString memoryReviewStateName(MemoryReviewState state);
QString memoryCapturePolicyName(MemoryCapturePolicy policy);

struct MemoryCandidate {
    MemoryCandidateId id;
    MemoryCandidateSource source = MemoryCandidateSource::ConversationText;
    MemoryCandidateCategory category = MemoryCandidateCategory::Semantic;
    MemoryCandidateConfidence confidence = MemoryCandidateConfidence::Medium;
    MemoryReviewState reviewState = MemoryReviewState::PendingReview;
    MemoryRetentionPolicy retentionPolicy = MemoryRetentionPolicy::UserControlled;
    MemoryCapturePolicy capturePolicy = MemoryCapturePolicy::ManualReviewRequired;
    QString title;
    QString excerpt;
    QString sourceSummary = QStringLiteral("Conversation text metadata only.");
    QString reviewSummary = QStringLiteral("Pending user review.");
    QDateTime createdAtUtc;
    QDateTime reviewedAtUtc;
};

struct MemoryCandidateSummary {
    MemoryCandidateId id;
    QString title;
    QString state;
    QString category;
    QString confidence;
    QString retention;
    QString source;
    QString summary;
};

MemoryCandidateSummary memoryCandidateSummary(const MemoryCandidate& candidate);

class IMemoryCandidateStore {
public:
    virtual ~IMemoryCandidateStore() = default;

    virtual MemoryCandidate createCandidate(MemoryCandidate candidate) = 0;
    virtual QList<MemoryCandidate> candidates() const = 0;
    virtual bool setReviewState(const MemoryCandidateId& id, MemoryReviewState state,
                                const QString& reviewSummary) = 0;
};

} // namespace sentinel::core
