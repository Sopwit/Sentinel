// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/agent/ClaimGroundingResolver.h"
#include "sentinel/core/agent/ObservationPolicy.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <optional>

namespace sentinel::core {
namespace {
QString path(const QString& value) {
    return normalizedObservationResource(value, ObservationDomain::FileSystem);
}
std::optional<bool> observed(const ObservationRequirement& claim, const StructuredObservation& observation) {
    const auto data = observation.data;
    const auto target = path(claim.resourceHint);
    const bool complete = data.value(QStringLiteral("complete")).toBool(false) &&
                          !data.value(QStringLiteral("truncated")).toBool(true);
    const bool existence = claim.claimType == ClaimType::PathExists ||
                           claim.claimType == ClaimType::FileExists ||
                           claim.claimType == ClaimType::DirectoryExists;
    if (existence) {
        if (observation.kind == StructuredObservationKind::FileSystemFact &&
            path(data.value(QStringLiteral("path")).toString()) == target) {
            if (claim.claimType == ClaimType::PathExists)
                return data.value(QStringLiteral("exists")).toBool();
            if (claim.claimType == ClaimType::FileExists)
                return data.value(QStringLiteral("file")).toBool();
            if (claim.claimType == ClaimType::DirectoryExists)
                return data.value(QStringLiteral("directory")).toBool();
        }
        if (observation.kind == StructuredObservationKind::FileSystemFailure &&
            path(observation.failureResource) == target) {
            switch (observation.fileSystemFailure) {
            case FileSystemFailure::NotFound: return false;
            case FileSystemFailure::NotFile:
                if (claim.claimType == ClaimType::PathExists) return true;
                if (claim.claimType == ClaimType::FileExists) return false;
                return {};
            case FileSystemFailure::NotDirectory:
                if (claim.claimType == ClaimType::PathExists) return true;
                if (claim.claimType == ClaimType::DirectoryExists) return false;
                return {};
            case FileSystemFailure::AlreadyExists:
                if (claim.claimType == ClaimType::PathExists) return true;
                return {};
            default: return {};
            }
        }
        if (observation.kind == StructuredObservationKind::DirectoryListing) {
            const auto listed = path(data.value(QStringLiteral("path")).toString());
            if (listed == target)
                return claim.claimType != ClaimType::FileExists;
            if (listed != QFileInfo(target).absolutePath()) return {};
            for (const auto& entry : data.value(QStringLiteral("entries")).toArray()) {
                const auto object = entry.toObject();
                if (object.value(QStringLiteral("name")).toString() != QFileInfo(target).fileName()) continue;
                if (claim.claimType == ClaimType::PathExists) return true;
                const auto type = object.value(QStringLiteral("type")).toString();
                return claim.claimType == ClaimType::DirectoryExists
                    ? type == QLatin1String("directory") : type == QLatin1String("file");
            }
            if (QFileInfo(target).fileName().startsWith(QLatin1Char('.')) &&
                !data.value(QStringLiteral("includeHidden")).toBool(false)) return {};
            return complete ? std::optional<bool>{false} : std::optional<bool>{};
        }
        if (observation.kind == StructuredObservationKind::FileContent &&
            path(data.value(QStringLiteral("path")).toString()) == target)
            return claim.claimType != ClaimType::DirectoryExists;
        if (observation.kind == StructuredObservationKind::PathMatches &&
            claim.claimType != ClaimType::DirectoryExists) {
            const auto pattern = data.value(QStringLiteral("pattern")).toString();
            if (pattern != QFileInfo(target).fileName() ||
                path(data.value(QStringLiteral("root")).toString()) != QFileInfo(target).absolutePath()) return {};
            for (const auto& item : data.value(QStringLiteral("matches")).toArray())
                if (path(item.toString()) == target) return true;
            if (QFileInfo(target).fileName().startsWith(QLatin1Char('.')) &&
                !data.value(QStringLiteral("includeHidden")).toBool(false)) return {};
            return complete ? std::optional<bool>{false} : std::optional<bool>{};
        }
    }
    if (claim.claimType == ClaimType::TextContains && observation.kind == StructuredObservationKind::FileContent &&
        path(data.value(QStringLiteral("path")).toString()) == target) {
        if (data.value(QStringLiteral("content")).toString().contains(claim.claimQuery)) return true;
        return complete ? std::optional<bool>{false} : std::optional<bool>{};
    }
    if (claim.claimType == ClaimType::SearchHasMatches && observation.kind == StructuredObservationKind::TextSearch &&
        path(data.value(QStringLiteral("scope")).toString()) == target &&
        data.value(QStringLiteral("query")).toString() == claim.claimQuery) {
        if (data.value(QStringLiteral("matchCount")).toInt() > 0) return true;
        return complete ? std::optional<bool>{false} : std::optional<bool>{};
    }
    return {};
}
}
ClaimResolution ClaimGroundingResolver::resolve(const ObservationRequirement& claim,
                                                 const QList<EvidenceRecord>& evidence,
                                                 const ClaimAssertion& assertion) {
    ClaimResolution result;
    result.fact.id = claim.claimId;
    result.fact.resource = claim.resourceHint;
    if (claim.claimType == ClaimType::None || assertion.id != claim.claimId) return result;
    int latest = -1;
    for (const auto& item : evidence) {
        if ((item.outcome != EvidenceOutcome::Verified &&
             item.outcome != EvidenceOutcome::Partial &&
             !(item.outcome == EvidenceOutcome::Failed && item.structuredObservation &&
               item.structuredObservation->kind == StructuredObservationKind::FileSystemFailure)) ||
            !item.structuredObservation || item.stepIndex < latest) continue;
        const auto value = observed(claim, *item.structuredObservation);
        if (!value) continue;
        if (item.stepIndex > latest) result.fact.evidenceCallIds.clear();
        latest = item.stepIndex;
        result.fact.value = *value;
        result.fact.evidenceCallIds.append(item.toolCallId);
    }
    if (latest >= 0) result.verdict = result.fact.value == assertion.value ? ClaimVerdict::Supported : ClaimVerdict::Contradicted;
    return result;
}
QList<StructuredFact> ClaimGroundingResolver::facts(const ObservationIntent& intent,
                                                     const QList<EvidenceRecord>& evidence) {
    QList<StructuredFact> result;
    for (const auto& claim : intent.requirements) {
        if (claim.claimType == ClaimType::None) continue;
        auto resolved = resolve(claim, evidence, {claim.claimId, true});
        if (resolved.verdict != ClaimVerdict::Unknown) result.append(resolved.fact);
    }
    return result;
}
} // namespace sentinel::core
