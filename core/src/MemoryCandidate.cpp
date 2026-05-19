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
    case MemoryReviewState::Archived:
        return QStringLiteral("Archived");
    }

    return QStringLiteral("Pending Review");
}

QString memoryCandidateReviewActionName(MemoryCandidateReviewAction action) {
    switch (action) {
    case MemoryCandidateReviewAction::Approve:
        return QStringLiteral("Approve");
    case MemoryCandidateReviewAction::Reject:
        return QStringLiteral("Reject");
    case MemoryCandidateReviewAction::ResetToPending:
        return QStringLiteral("Reset to Pending");
    case MemoryCandidateReviewAction::Archive:
        return QStringLiteral("Archive");
    }

    return QStringLiteral("Approve");
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

QString memoryCommitTargetName(MemoryCommitTarget target) {
    switch (target) {
    case MemoryCommitTarget::KeyValueMemory:
        return QStringLiteral("Key-value Memory");
    }

    return QStringLiteral("Key-value Memory");
}

QString memoryCommitReadinessStatusName(MemoryCommitReadinessStatus status) {
    switch (status) {
    case MemoryCommitReadinessStatus::Disabled:
        return QStringLiteral("Disabled");
    case MemoryCommitReadinessStatus::Ready:
        return QStringLiteral("Ready");
    case MemoryCommitReadinessStatus::PendingReview:
        return QStringLiteral("Pending Review");
    case MemoryCommitReadinessStatus::Rejected:
        return QStringLiteral("Rejected");
    case MemoryCommitReadinessStatus::Archived:
        return QStringLiteral("Archived");
    case MemoryCommitReadinessStatus::MissingCandidate:
        return QStringLiteral("Missing Candidate");
    case MemoryCommitReadinessStatus::StoreUnavailable:
        return QStringLiteral("Store Unavailable");
    case MemoryCommitReadinessStatus::AlreadyCommitted:
        return QStringLiteral("Already Committed");
    }

    return QStringLiteral("Disabled");
}

QString memoryCommitStatusName(MemoryCommitStatus status) {
    switch (status) {
    case MemoryCommitStatus::NotCommitted:
        return QStringLiteral("Not Committed");
    case MemoryCommitStatus::Committed:
        return QStringLiteral("Committed");
    case MemoryCommitStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Not Committed");
}

QString memoryCommitConflictPolicyName(MemoryCommitConflictPolicy policy) {
    switch (policy) {
    case MemoryCommitConflictPolicy::Refuse:
        return QStringLiteral("Refuse Existing Key");
    case MemoryCommitConflictPolicy::Overwrite:
        return QStringLiteral("Overwrite Existing Key");
    }

    return QStringLiteral("Refuse Existing Key");
}

namespace {

QString sanitizedMemoryKeyPart(QString value) {
    value = value.simplified().toLower();

    QString result;
    result.reserve(value.size());
    bool lastWasSeparator = false;
    for (const auto ch : value) {
        if (ch.isLetterOrNumber()) {
            result.append(ch);
            lastWasSeparator = false;
            continue;
        }
        if (!lastWasSeparator && !result.isEmpty()) {
            result.append(QLatin1Char('-'));
            lastWasSeparator = true;
        }
    }

    while (result.endsWith(QLatin1Char('-'))) {
        result.chop(1);
    }

    return result;
}

QString safeMemoryCommitKey(const MemoryCandidate& candidate) {
    QStringList parts;
    parts.append(QStringLiteral("memory"));
    parts.append(sanitizedMemoryKeyPart(memoryCandidateCategoryName(candidate.category)));
    parts.append(sanitizedMemoryKeyPart(candidate.title));
    parts.append(sanitizedMemoryKeyPart(candidate.id));
    parts.removeAll(QString());
    return parts.join(QLatin1Char('.'));
}

} // namespace

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
    summary.reviewedAt = candidate.reviewedAtUtc.isNull()
                             ? QStringLiteral("Not reviewed")
                             : candidate.reviewedAtUtc.toString(Qt::ISODateWithMs);
    summary.reviewer = candidate.reviewerSummary.trimmed().isEmpty()
                           ? QStringLiteral("Reviewer: User review")
                           : candidate.reviewerSummary.trimmed();
    summary.commitStatus = memoryCommitStatusName(candidate.commitStatus);
    summary.committedAt = candidate.committedAtUtc.isNull()
                              ? QStringLiteral("Not committed")
                              : candidate.committedAtUtc.toString(Qt::ISODateWithMs);
    summary.committedKey = candidate.committedKey.trimmed().isEmpty()
                               ? QStringLiteral("No committed key")
                               : candidate.committedKey.trimmed();
    summary.summary =
        QStringLiteral("%1 / %2 / %3 / %4 / %5 / %6 / %7 / %8 / %9")
            .arg(summary.title, summary.state, summary.category, summary.confidence, summary.source,
                 summary.reviewedAt, summary.reviewer, summary.commitStatus, summary.committedKey);
    return summary;
}

MemoryCommitPlan memoryCommitPlanForCandidate(const MemoryCandidate& candidate,
                                              MemoryCommitTarget target) {
    MemoryCommitPlan plan;
    plan.candidateId = candidate.id;
    plan.target = target;
    plan.key = safeMemoryCommitKey(candidate);
    plan.valuePreview = candidate.excerpt.simplified().left(160);
    plan.targetSummary = memoryCommitTargetName(target);
    plan.summary = QStringLiteral("%1 -> %2 / key: %3 / value preview: %4")
                       .arg(candidate.id, plan.targetSummary, plan.key,
                            plan.valuePreview.isEmpty() ? QStringLiteral("No candidate excerpt")
                                                        : plan.valuePreview);
    return plan;
}

MemoryCommitReadiness memoryCommitReadinessForCandidate(const MemoryCandidate* candidate,
                                                        bool memoryStoreAvailable,
                                                        const MemoryCommitPolicy& policy) {
    MemoryCommitReadiness readiness;
    readiness.status = MemoryCommitReadinessStatus::MissingCandidate;
    readiness.summary = QStringLiteral("Memory commit pending: candidate was not found.");
    readiness.checks.append(QStringLiteral("Candidate: missing"));

    if (!candidate) {
        return readiness;
    }

    readiness.candidateId = candidate->id;
    readiness.plan = memoryCommitPlanForCandidate(*candidate, policy.target);
    readiness.checks.append(QStringLiteral("Candidate: %1").arg(candidate->id));
    readiness.checks.append(
        QStringLiteral("Review state: %1").arg(memoryReviewStateName(candidate->reviewState)));
    readiness.checks.append(QStringLiteral("Target: %1").arg(readiness.plan.targetSummary));

    if (candidate->commitStatus == MemoryCommitStatus::Committed) {
        readiness.status = MemoryCommitReadinessStatus::AlreadyCommitted;
        readiness.summary = QStringLiteral("Memory commit complete: candidate is already stored "
                                           "as key '%1'.")
                                .arg(candidate->committedKey);
        readiness.checks.append(QStringLiteral("Commit: already committed"));
        return readiness;
    }

    if (candidate->reviewState == MemoryReviewState::PendingReview) {
        readiness.status = MemoryCommitReadinessStatus::PendingReview;
        readiness.summary =
            QStringLiteral("Memory commit pending: candidate still requires review.");
        readiness.checks.append(QStringLiteral("Commit: blocked until candidate is approved"));
        return readiness;
    }
    if (candidate->reviewState == MemoryReviewState::Rejected) {
        readiness.status = MemoryCommitReadinessStatus::Rejected;
        readiness.summary = QStringLiteral("Memory commit blocked: candidate was rejected.");
        readiness.checks.append(QStringLiteral("Commit: rejected candidates cannot be committed"));
        return readiness;
    }
    if (candidate->reviewState == MemoryReviewState::Archived) {
        readiness.status = MemoryCommitReadinessStatus::Archived;
        readiness.summary = QStringLiteral("Memory commit blocked: candidate is archived.");
        readiness.checks.append(
            QStringLiteral("Commit: archived candidates are terminal metadata"));
        return readiness;
    }

    if (!memoryStoreAvailable) {
        readiness.status = MemoryCommitReadinessStatus::StoreUnavailable;
        readiness.summary =
            QStringLiteral("Memory commit blocked: key-value memory store is unavailable.");
        readiness.checks.append(QStringLiteral("Commit: target store unavailable"));
        return readiness;
    }

    if (!policy.explicitUserCommitAllowed) {
        readiness.status = MemoryCommitReadinessStatus::Disabled;
        readiness.summary = policy.summary;
        readiness.checks.append(QStringLiteral("Commit: disabled by policy"));
        return readiness;
    }

    readiness.ready = true;
    readiness.status = MemoryCommitReadinessStatus::Ready;
    readiness.summary =
        QStringLiteral("Memory commit ready: approved candidate can be stored by explicit user "
                       "action.");
    readiness.checks.append(QStringLiteral("Commit: explicit user action allowed"));
    readiness.checks.append(QStringLiteral("Conflict policy: %1")
                                .arg(memoryCommitConflictPolicyName(policy.conflictPolicy)));
    return readiness;
}

} // namespace sentinel::core
