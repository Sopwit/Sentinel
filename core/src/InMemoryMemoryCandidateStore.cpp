#include "sentinel/core/InMemoryMemoryCandidateStore.h"

#include <QDateTime>

namespace sentinel::core {

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

bool InMemoryMemoryCandidateStore::setReviewState(const MemoryCandidateId& id,
                                                  MemoryReviewState state,
                                                  const QString& reviewSummary) {
    auto it = candidates_.find(id.trimmed());
    if (it == candidates_.end()) {
        return false;
    }

    it->reviewState = state;
    it->reviewedAtUtc = QDateTime::currentDateTimeUtc();
    it->reviewSummary =
        reviewSummary.trimmed().isEmpty() ? memoryReviewStateName(state) : reviewSummary.trimmed();
    return true;
}

} // namespace sentinel::core
