#include "sentinel/core/InMemoryMemoryCandidateStore.h"

#include <QDateTime>

namespace sentinel::core {

namespace {

MemoryReviewState targetStateForAction(MemoryCandidateReviewAction action) {
    switch (action) {
    case MemoryCandidateReviewAction::Approve:
        return MemoryReviewState::Approved;
    case MemoryCandidateReviewAction::Reject:
        return MemoryReviewState::Rejected;
    case MemoryCandidateReviewAction::ResetToPending:
        return MemoryReviewState::PendingReview;
    case MemoryCandidateReviewAction::Archive:
        return MemoryReviewState::Archived;
    }

    return MemoryReviewState::PendingReview;
}

bool transitionAllowed(MemoryReviewState currentState, MemoryCandidateReviewAction action) {
    if (currentState == MemoryReviewState::Archived) {
        return false;
    }

    switch (action) {
    case MemoryCandidateReviewAction::Approve:
    case MemoryCandidateReviewAction::Reject:
        return currentState == MemoryReviewState::PendingReview;
    case MemoryCandidateReviewAction::ResetToPending:
        return currentState == MemoryReviewState::Approved ||
               currentState == MemoryReviewState::Rejected;
    case MemoryCandidateReviewAction::Archive:
        return currentState == MemoryReviewState::Approved ||
               currentState == MemoryReviewState::Rejected;
    }

    return false;
}

QString defaultDecisionReason(MemoryCandidateReviewAction action) {
    switch (action) {
    case MemoryCandidateReviewAction::Approve:
        return QStringLiteral("Approved by user review metadata.");
    case MemoryCandidateReviewAction::Reject:
        return QStringLiteral("Rejected by user review metadata.");
    case MemoryCandidateReviewAction::ResetToPending:
        return QStringLiteral("Reset to pending review metadata.");
    case MemoryCandidateReviewAction::Archive:
        return QStringLiteral("Archived after review metadata.");
    }

    return QStringLiteral("Reviewed by user metadata.");
}

} // namespace

MemoryCandidate InMemoryMemoryCandidateStore::createCandidate(MemoryCandidate candidate) {
    if (candidate.id.trimmed().isEmpty()) {
        candidate.id = QStringLiteral("memory-candidate-%1").arg(nextCandidateNumber_++);
    }
    if (candidate.createdAtUtc.isNull()) {
        candidate.createdAtUtc = QDateTime::currentDateTimeUtc();
    }
    if (candidate.title.trimmed().isEmpty()) {
        candidate.title = QStringLiteral("Conversation Memory Candidate");
    }
    candidate.reviewState = MemoryReviewState::PendingReview;
    candidate.reviewedAtUtc = {};
    candidate.reviewSummary = QStringLiteral("Pending user review.");
    candidate.reviewerSummary = {};
    candidate.decisionReason = {};
    candidate.commitStatus = MemoryCommitStatus::NotCommitted;
    candidate.committedKey = {};
    candidate.commitSummary = QStringLiteral("Not committed.");
    candidate.committedAtUtc = {};

    candidates_.insert(candidate.id, candidate);
    if (!candidateOrder_.contains(candidate.id)) {
        candidateOrder_.append(candidate.id);
    }
    return candidate;
}

QList<MemoryCandidate> InMemoryMemoryCandidateStore::candidates() const {
    QList<MemoryCandidate> records;
    records.reserve(candidateOrder_.size());
    for (const auto& id : candidateOrder_) {
        const auto it = candidates_.constFind(id);
        if (it != candidates_.constEnd()) {
            records.append(*it);
        }
    }
    return records;
}

MemoryCandidateReviewResult InMemoryMemoryCandidateStore::reviewCandidate(
    const MemoryCandidateId& id, MemoryCandidateReviewAction action, const QString& reviewerSummary,
    const QString& decisionReason) {
    MemoryCandidateReviewResult result;
    result.candidateId = id.trimmed();
    result.action = action;
    result.newState = targetStateForAction(action);
    result.reviewerSummary = reviewerSummary.trimmed().isEmpty()
                                 ? QStringLiteral("Reviewer: User review")
                                 : reviewerSummary.trimmed();
    result.decisionReason = decisionReason.trimmed().isEmpty() ? defaultDecisionReason(action)
                                                               : decisionReason.trimmed();

    auto it = candidates_.find(id.trimmed());
    if (it == candidates_.end()) {
        result.status = QStringLiteral("Refused");
        result.summary = QStringLiteral("%1 refused: memory candidate was not found.")
                             .arg(memoryCandidateReviewActionName(action));
        return result;
    }

    result.previousState = it->reviewState;
    result.sourceSummary = it->sourceSummary.trimmed().isEmpty()
                               ? memoryCandidateSourceName(it->source)
                               : it->sourceSummary.trimmed();
    if (it->commitStatus == MemoryCommitStatus::Committed) {
        result.status = QStringLiteral("Refused");
        result.summary = QStringLiteral("%1 refused: candidate is already committed.")
                             .arg(memoryCandidateReviewActionName(action));
        return result;
    }
    if (!transitionAllowed(it->reviewState, action)) {
        result.status = QStringLiteral("Refused");
        result.summary = QStringLiteral("%1 refused: candidate is %2.")
                             .arg(memoryCandidateReviewActionName(action),
                                  memoryReviewStateName(it->reviewState));
        return result;
    }

    result.accepted = true;
    result.status = QStringLiteral("Accepted");
    result.reviewedAtUtc = QDateTime::currentDateTimeUtc();
    result.summary = QStringLiteral("%1 accepted: %2 -> %3. %4 %5")
                         .arg(memoryCandidateReviewActionName(action),
                              memoryReviewStateName(result.previousState),
                              memoryReviewStateName(result.newState), result.sourceSummary,
                              result.decisionReason);

    it->reviewState = result.newState;
    it->reviewedAtUtc = result.reviewedAtUtc;
    it->reviewSummary = result.summary;
    it->reviewerSummary = result.reviewerSummary;
    it->decisionReason = result.decisionReason;
    return result;
}

bool InMemoryMemoryCandidateStore::recordCommitResult(const MemoryCommitResult& result) {
    auto it = candidates_.find(result.candidateId.trimmed());
    if (it == candidates_.end()) {
        return false;
    }

    if (!result.accepted || result.commitStatus != MemoryCommitStatus::Committed) {
        return false;
    }

    it->commitStatus = result.commitStatus;
    it->committedAtUtc = result.committedAtUtc;
    it->committedKey = result.committedKey;
    it->commitSummary = result.summary;
    return true;
}

} // namespace sentinel::core
