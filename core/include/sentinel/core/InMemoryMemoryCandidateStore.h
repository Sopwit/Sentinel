#pragma once

#include "sentinel/core/MemoryCandidate.h"

#include <QMap>

namespace sentinel::core {

class InMemoryMemoryCandidateStore final : public IMemoryCandidateStore {
public:
    MemoryCandidate createCandidate(MemoryCandidate candidate) override;
    QList<MemoryCandidate> candidates() const override;
    bool setReviewState(const MemoryCandidateId& id, MemoryReviewState state,
                        const QString& reviewSummary) override;

private:
    QMap<MemoryCandidateId, MemoryCandidate> candidates_;
    QList<MemoryCandidateId> candidateOrder_;
    int nextCandidateNumber_ = 1;
};

} // namespace sentinel::core
