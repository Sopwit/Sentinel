// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

namespace sentinel::core {

enum class ObservationDomain {
    FileSystem, Workspace, Process, System, Clipboard, Network, Browser,
    Memory, ConversationHistory, ExternalService, Application, ProcessExecution
};
enum class EvidenceFreshness { Live, TurnScoped, SessionStable };
enum class EvidenceScope { ExactResource, DirectoryEntries, SearchScope, Provider, Operation };
enum class ObservationPurpose { Inspect, Operate };
enum class EvidenceOutcome { Verified, Unavailable, Denied, Failed };
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
};

struct ObservationIntent {
    QList<ObservationRequirement> requirements;
    bool indeterminate = false;
    QString error;
    bool requiresObservation() const { return !requirements.isEmpty(); }
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
};

struct FinalAnswerGrounding {
    GroundingMode mode = GroundingMode::Context;
    QStringList evidenceCallIds;
};

QString observationDomainName(ObservationDomain domain);
QString groundingModeName(GroundingMode mode);
} // namespace sentinel::core
