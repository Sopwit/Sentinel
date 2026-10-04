// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <memory>

namespace sentinel::core {

enum class ObservationDomain {
    FileSystem,
    Workspace,
    Process,
    System,
    Clipboard,
    Network,
    Browser,
    Memory,
    ConversationHistory,
    ExternalService,
    Application,
    ProcessExecution
};
enum class EvidenceFreshness { Live, TurnScoped, SessionStable };
enum class EvidenceScope { ExactResource, DirectoryEntries, SearchScope, Provider, Operation };
enum class ObservationPurpose { Inspect, Operate };
enum class EvidenceOutcome { Verified, Partial, Unavailable, Denied, Failed, Stale };
enum class StructuredObservationKind {
    None,
    DirectoryListing,
    PathMatches,
    FileContent,
    TextSearch,
    CodeDefinitions,
    FileSystemFailure,
    FileSystemFact,
    PatchResult,
    Generic
};
enum class FileSystemFailure {
    None,
    NotFound,
    PermissionDenied,
    NotFile,
    NotDirectory,
    InvalidPath,
    AlreadyExists,
    ReadFailed,
    WriteFailed,
    IOError,
    Unavailable,
    ResourceChanged,
    SymlinkEscape,
    UnsafeParent,
    SecurityBoundaryViolation
};
enum class FileSystemOperation {
    Stat,
    ListDirectory,
    ReadFile,
    WriteFile,
    EditFile,
    Delete,
    Move,
    Glob,
    Grep,
    ApplyPatch
};
struct PatchPathOutcome {
    QString resource;
    QString action;
    FileSystemFailure failure = FileSystemFailure::None;
    bool committed = false;
    bool cancelled = false;
};
struct StructuredObservation {
    StructuredObservationKind kind = StructuredObservationKind::None;
    QJsonObject data;
    FileSystemFailure fileSystemFailure = FileSystemFailure::None;
    FileSystemOperation fileSystemOperation = FileSystemOperation::Stat;
    QString failureResource;
    QList<PatchPathOutcome> patchPaths;
};
using StructuredObservationPtr = std::shared_ptr<const StructuredObservation>;
enum class HiddenEntriesPolicy { Unknown, Included, Excluded };
struct FileSystemCoverage {
    QString scope;
    HiddenEntriesPolicy hiddenEntriesPolicy = HiddenEntriesPolicy::Unknown;
    bool complete = false;
    bool truncated = true;
    bool cancelled = true;
    bool permissionLimited = true;
    bool recursive = false;
    bool followSymlinks = false;
    bool symlinkPolicyKnown = false;
    bool enumeration = false;
    int skippedSymlinks = 0;
    int maxDepth = 0;
    int resultLimit = 0;
    static FileSystemCoverage fromObservation(const StructuredObservation& observation);
    bool supportsAbsence(bool includeHidden, bool requireRecursive) const;
};
enum class ClaimType {
    None,
    PathExists,
    FileExists,
    DirectoryExists,
    TextContains,
    SearchHasMatches,
    HiddenEntriesExist,
    PathPatternExists
};
struct ClaimAssertion {
    QString id;
    bool value = false;
};
struct StructuredFact {
    QString id;
    QString resource;
    bool value = false;
    QStringList evidenceCallIds;
};
enum class GroundingMode { Context, Verified, UnableToVerify };

struct ToolEvidenceDescriptor {
    ObservationDomain domain = ObservationDomain::ExternalService;
    EvidenceFreshness freshness = EvidenceFreshness::TurnScoped;
    EvidenceScope scope = EvidenceScope::ExactResource;
    QString resourceArgument;
    QString qualifierArgument;
};

struct ObservationRequirement {
    ObservationDomain domain = ObservationDomain::FileSystem;
    QString resourceHint;
    EvidenceFreshness freshness = EvidenceFreshness::TurnScoped;
    ObservationPurpose purpose = ObservationPurpose::Inspect;
    ClaimType claimType = ClaimType::None;
    QString claimId;
    QString claimQuery;
    bool recursive = false;
};

struct ObservationIntent {
    QList<ObservationRequirement> requirements;
    bool indeterminate = false;
    QString error;
    bool requiresObservation() const {
        return !requirements.isEmpty();
    }
};

struct EvidenceRecord {
    QString toolId;
    QString toolCallId;
    int stepIndex = 0;
    ObservationDomain domain = ObservationDomain::ExternalService;
    QString resource;
    EvidenceFreshness freshness = EvidenceFreshness::TurnScoped;
    EvidenceScope scope = EvidenceScope::ExactResource;
    QString qualifier;
    EvidenceOutcome outcome = EvidenceOutcome::Failed;
    QDateTime observedAtUtc;
    StructuredObservationPtr structuredObservation;
};

struct FinalAnswerGrounding {
    GroundingMode mode = GroundingMode::Context;
    QStringList evidenceCallIds;
};

QString observationDomainName(ObservationDomain domain);
QString groundingModeName(GroundingMode mode);
} // namespace sentinel::core
