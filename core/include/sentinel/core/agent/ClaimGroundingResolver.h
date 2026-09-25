// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/agent/ObservationEvidence.h"

namespace sentinel::core {

enum class ClaimVerdict { Supported, Contradicted, Unknown };
struct ClaimResolution {
    ClaimVerdict verdict = ClaimVerdict::Unknown;
    StructuredFact fact;
};

class ClaimGroundingResolver final {
public:
    static ClaimResolution resolve(const ObservationRequirement& claim,
                                   const QList<EvidenceRecord>& evidence,
                                   const ClaimAssertion& assertion);
    static QList<StructuredFact> facts(const ObservationIntent& intent,
                                       const QList<EvidenceRecord>& evidence);
};
} // namespace sentinel::core
