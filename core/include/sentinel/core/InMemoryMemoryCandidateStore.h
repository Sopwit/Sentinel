#pragma once

#include "sentinel/core/MemoryCandidate.h"

#include <QMap>

namespace sentinel::core {

class InMemoryMemoryCandidateStore final : public IMemoryCandidateStore {
public:
    MemoryCandidate createCandidate(MemoryCandidate candidate) override;
    QList<MemoryCandidate> candidates() const override;
    MemoryCandidateReviewResult reviewCandidate(const MemoryCandidateId& id,
                                                MemoryCandidateReviewAction action,
                                                const QString& reviewerSummary,
                                                const QString& decisionReason) override;

private:
    QMap<MemoryCandidateId, MemoryCandidate> candidates_;
    QList<MemoryCandidateId> candidateOrder_;
    int nextCandidateNumber_ = 1;
};

} // namespace sentinel::core
