#include "sentinel/core/MemoryCandidate.h"

namespace sentinel::core {

QString memoryCandidateSourceName(MemoryCandidateSource source) {
    switch (source) {
    case MemoryCandidateSource::ConversationText:
        return QStringLiteral("Conversation Text");
    case MemoryCandidateSource::ManualMetadata:
        return QStringLiteral("Manual Metadata");
    }

    return QStringLiteral("Conversation Text");
}

QString memoryCandidateCategoryName(MemoryCandidateCategory category) {
    switch (category) {
    case MemoryCandidateCategory::Semantic:
        return QStringLiteral("Semantic");
    case MemoryCandidateCategory::Preference:
        return QStringLiteral("Preference");
    case MemoryCandidateCategory::ProjectContext:
        return QStringLiteral("Project Context");
    case MemoryCandidateCategory::SafetyNote:
        return QStringLiteral("Safety Note");
    }

    return QStringLiteral("Semantic");
}

QString memoryCandidateConfidenceName(MemoryCandidateConfidence confidence) {
    switch (confidence) {
    case MemoryCandidateConfidence::Low:
        return QStringLiteral("Low");
    case MemoryCandidateConfidence::Medium:
        return QStringLiteral("Medium");
    case MemoryCandidateConfidence::High:
        return QStringLiteral("High");
    }

    return QStringLiteral("Medium");
}

QString memoryReviewStateName(MemoryReviewState state) {
    switch (state) {
    case MemoryReviewState::PendingReview:
        return QStringLiteral("Pending Review");
    case MemoryReviewState::Approved:
        return QStringLiteral("Approved");
    case MemoryReviewState::Rejected:
        return QStringLiteral("Rejected");
    }

    return QStringLiteral("Pending Review");
}

QString memoryCapturePolicyName(MemoryCapturePolicy policy) {
    switch (policy) {
    case MemoryCapturePolicy::MetadataOnly:
        return QStringLiteral("Metadata Only");
    case MemoryCapturePolicy::ManualReviewRequired:
        return QStringLiteral("Manual Review Required");
    case MemoryCapturePolicy::Disabled:
        return QStringLiteral("Disabled");
    }

    return QStringLiteral("Manual Review Required");
}

MemoryCandidateSummary memoryCandidateSummary(const MemoryCandidate& candidate) {
    MemoryCandidateSummary summary;
    summary.id = candidate.id;
    summary.title =
        candidate.title.isEmpty() ? QStringLiteral("Untitled Memory Candidate") : candidate.title;
    summary.state = memoryReviewStateName(candidate.reviewState);
    summary.category = memoryCandidateCategoryName(candidate.category);
    summary.confidence = memoryCandidateConfidenceName(candidate.confidence);
    summary.retention = memoryRetentionPolicyName(candidate.retentionPolicy);
    summary.source = memoryCandidateSourceName(candidate.source);
    summary.summary = QStringLiteral("%1 / %2 / %3 / %4")
                          .arg(summary.title, summary.state, summary.category, summary.confidence);
    return summary;
}

} // namespace sentinel::core
