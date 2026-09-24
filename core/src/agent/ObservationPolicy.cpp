// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/agent/ObservationPolicy.h"

#include "sentinel/core/interfaces/IChatProvider.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>

namespace sentinel::core {
namespace {
QString argumentValue(const PlannedToolInvocation& invocation, const QString& id) {
    for (const auto& argument : invocation.arguments)
        if (argument.id == id)
            return argument.value.trimmed();
    return {};
}

QString normalizedResource(QString resource, ObservationDomain domain) {
    resource = resource.trimmed();
    if (resource.isEmpty())
        return {};
    if (domain == ObservationDomain::FileSystem || domain == ObservationDomain::Workspace) {
        if (resource == QLatin1String("~"))
            resource = QDir::homePath();
        else if (resource.startsWith(QLatin1String("~/")))
            resource = QDir::home().filePath(resource.mid(2));
        if (!QDir::isAbsolutePath(resource)) {
            for (const auto location : {QStandardPaths::DesktopLocation,
                                        QStandardPaths::DocumentsLocation,
                                        QStandardPaths::DownloadLocation}) {
                const auto locationPath = QStandardPaths::writableLocation(location);
                const auto name = QFileInfo(locationPath).fileName();
                if (!name.isEmpty() &&
                    (resource.compare(name, Qt::CaseInsensitive) == 0 ||
                     resource.startsWith(name + QLatin1Char('/'), Qt::CaseInsensitive))) {
                    resource = locationPath + resource.mid(name.size());
                    break;
                }
            }
        }
        if (QDir::isAbsolutePath(resource))
            return QDir::cleanPath(resource);
        return QDir::cleanPath(QDir::current().absoluteFilePath(resource));
    }
    if (domain == ObservationDomain::Network || domain == ObservationDomain::Browser) {
        const QUrl url(resource);
        if (url.isValid() && !url.host().isEmpty())
            return url.adjusted(QUrl::NormalizePathSegments).toString();
    }
    return resource.toCaseFolded();
}

std::optional<ObservationDomain> domainFromName(const QString& name) {
    static const QHash<QString, ObservationDomain> domains{
        {QStringLiteral("filesystem"), ObservationDomain::FileSystem},
        {QStringLiteral("workspace"), ObservationDomain::Workspace},
        {QStringLiteral("process"), ObservationDomain::Process},
        {QStringLiteral("system"), ObservationDomain::System},
        {QStringLiteral("clipboard"), ObservationDomain::Clipboard},
        {QStringLiteral("network"), ObservationDomain::Network},
        {QStringLiteral("browser"), ObservationDomain::Browser},
        {QStringLiteral("memory"), ObservationDomain::Memory},
        {QStringLiteral("history"), ObservationDomain::ConversationHistory},
        {QStringLiteral("external"), ObservationDomain::ExternalService},
        {QStringLiteral("application"), ObservationDomain::Application},
        {QStringLiteral("execution"), ObservationDomain::ProcessExecution}};
    const auto found = domains.constFind(name.toLower().trimmed());
    return found == domains.cend() ? std::nullopt
                                   : std::optional<ObservationDomain>(*found);
}

bool compatibleDomain(ObservationDomain required, ObservationDomain produced) {
    return required == produced ||
           (required == ObservationDomain::Workspace &&
            produced == ObservationDomain::FileSystem);
}

bool resourceMatches(const ObservationRequirement& requirement, const EvidenceRecord& evidence) {
    if (requirement.purpose == ObservationPurpose::Inspect &&
        evidence.scope == EvidenceScope::Operation)
        return false;
    if (requirement.purpose == ObservationPurpose::Operate &&
        evidence.scope != EvidenceScope::Operation)
        return false;
    if (requirement.domain == ObservationDomain::Process ||
        requirement.domain == ObservationDomain::System ||
        requirement.domain == ObservationDomain::Clipboard)
        return true;
    const auto target = normalizedResource(requirement.resourceHint, requirement.domain);
    if (target.isEmpty())
        return true;
    const auto observed = normalizedResource(evidence.resource, requirement.domain);
    if (observed.isEmpty())
        return false;
    if (target == observed)
        return true;
    if (evidence.scope == EvidenceScope::Provider &&
        observed.endsWith(QStringLiteral(":") + target))
        return true;
    if (evidence.scope == EvidenceScope::DirectoryEntries &&
        (requirement.domain == ObservationDomain::FileSystem ||
         requirement.domain == ObservationDomain::Workspace))
        return QFileInfo(target).absolutePath() == observed;
    if (evidence.scope == EvidenceScope::SearchScope &&
        (requirement.domain == ObservationDomain::FileSystem ||
         requirement.domain == ObservationDomain::Workspace) &&
        target.startsWith(observed + QLatin1Char('/'))) {
        const auto name = QFileInfo(target).fileName();
        return !name.isEmpty() && evidence.qualifier.contains(name, Qt::CaseInsensitive);
    }
    return false;
}

bool matches(const ObservationRequirement& requirement, const EvidenceRecord& evidence) {
    return compatibleDomain(requirement.domain, evidence.domain) &&
           resourceMatches(requirement, evidence);
}

QList<ObservationRequirement> explicitResourceQuestions(const QString& goal) {
    QList<ObservationRequirement> requirements;
    // Concrete path/URL syntax is an independent runtime signal. Requiring a
    // question mark avoids treating a resource mentioned in a writing task as
    // a request to inspect it. Semantic cases remain with the intent classifier.
    if (!goal.contains(QLatin1Char('?')))
        return requirements;
    static const QRegularExpression path(
        QStringLiteral(R"((~/[^\s?]+|\./[^\s?]+|/(?:Users|home|tmp|etc|var)/[^\s?]+))"));
    static const QRegularExpression url(QStringLiteral(R"(https?://[^\s?]+)"),
                                        QRegularExpression::CaseInsensitiveOption);
    QString withoutUrl = goal;
    const auto urlMatch = url.match(goal);
    if (urlMatch.hasMatch())
        withoutUrl.replace(urlMatch.capturedStart(), urlMatch.capturedLength(), QString{});
    const auto pathMatch = path.match(withoutUrl);
    if (pathMatch.hasMatch())
        requirements.append({ObservationDomain::FileSystem, pathMatch.captured(1),
                             EvidenceFreshness::TurnScoped, ObservationPurpose::Inspect});
    if (urlMatch.hasMatch())
        requirements.append({ObservationDomain::Network, urlMatch.captured(),
                             EvidenceFreshness::Live, ObservationPurpose::Inspect});
    return requirements;
}
} // namespace

QString observationDomainName(ObservationDomain domain) {
    switch (domain) {
    case ObservationDomain::FileSystem: return QStringLiteral("filesystem");
    case ObservationDomain::Workspace: return QStringLiteral("workspace");
    case ObservationDomain::Process: return QStringLiteral("process");
    case ObservationDomain::System: return QStringLiteral("system");
    case ObservationDomain::Clipboard: return QStringLiteral("clipboard");
    case ObservationDomain::Network: return QStringLiteral("network");
    case ObservationDomain::Browser: return QStringLiteral("browser");
    case ObservationDomain::Memory: return QStringLiteral("memory");
    case ObservationDomain::ConversationHistory: return QStringLiteral("history");
    case ObservationDomain::ExternalService: return QStringLiteral("external");
    case ObservationDomain::Application: return QStringLiteral("application");
    case ObservationDomain::ProcessExecution: return QStringLiteral("execution");
    }
    return QStringLiteral("external");
}

QString groundingModeName(GroundingMode mode) {
    switch (mode) {
    case GroundingMode::Context: return QStringLiteral("context");
    case GroundingMode::Verified: return QStringLiteral("verified");
    case GroundingMode::UnableToVerify: return QStringLiteral("unable_to_verify");
    }
    return QStringLiteral("context");
}

ObservationIntent ObservationIntentPolicy::classify(const QString& goal,
                                                     const QString& conversationContext,
                                                     const QList<ToolDescriptor>& tools) const {
    ObservationIntent intent;
    if (!provider_) {
        intent.indeterminate = true;
        intent.error = QStringLiteral("No model is configured for observation intent.");
        return intent;
    }
    QStringList capabilities;
    for (const auto& tool : tools)
        for (const auto& evidence : tool.evidenceProduced) {
            const auto name = observationDomainName(evidence.domain);
            if (!capabilities.contains(name))
                capabilities.append(name);
        }
    const auto prompt = QStringLiteral(
        "Classify the USER GOAL, independently of any proposed answer. Does completing it "
        "require observing current external state? Distinguish inspecting/verifying a resource "
        "from merely mentioning it in a hypothetical or writing task. Conversation text "
        "already visible is context; older history requires lookup. Return ONLY JSON: "
        "{\"requirements\":[{\"domain\":\"filesystem|workspace|process|system|"
        "clipboard|network|browser|memory|history|external|application|execution\","
        "\"resource\":\"absolute path, ~/path, URL, or empty\","
        "\"purpose\":\"inspect|operate\"}]}. Use an empty array for general knowledge, "
        "rewriting, or conversation-only reasoning. Multiple domains are allowed. "
        "Available evidence domains: %1. RECENT CONVERSATION: %2. GOAL: %3")
                            .arg(capabilities.join(QLatin1Char(',')),
                                 conversationContext.left(1500), goal);
    const auto reply = provider_->sendMessage(prompt);
    if (!reply.success) {
        intent.indeterminate = true;
        intent.error = reply.errorMessage;
        return intent;
    }
    const auto candidate = reply.message.trimmed();
    if (!candidate.startsWith(QLatin1Char('{')) || !candidate.endsWith(QLatin1Char('}'))) {
        intent.indeterminate = true;
        return intent;
    }
    const auto document = QJsonDocument::fromJson(candidate.toUtf8());
    if (!document.isObject() || !document.object().value(QStringLiteral("requirements")).isArray()) {
        intent.indeterminate = true;
        return intent;
    }
    const auto entries = document.object().value(QStringLiteral("requirements")).toArray();
    if (entries.size() > 4) {
        intent.indeterminate = true;
        return intent;
    }
    for (const auto& entry : entries) {
        if (!entry.isObject()) {
            intent.indeterminate = true;
            intent.requirements.clear();
            return intent;
        }
        const auto object = entry.toObject();
        const auto domain = domainFromName(object.value(QStringLiteral("domain")).toString());
        if (!domain) {
            intent.indeterminate = true;
            intent.requirements.clear();
            return intent;
        }
        ObservationRequirement requirement;
        requirement.domain = *domain;
        requirement.resourceHint = object.value(QStringLiteral("resource")).toString().left(512);
        requirement.purpose = object.value(QStringLiteral("purpose")).toString() ==
                                      QLatin1String("operate")
                                  ? ObservationPurpose::Operate
                                  : ObservationPurpose::Inspect;
        requirement.freshness = *domain == ObservationDomain::ConversationHistory
                                    ? EvidenceFreshness::SessionStable
                                    : EvidenceFreshness::TurnScoped;
        intent.requirements.append(std::move(requirement));
    }
    for (const auto& structural : explicitResourceQuestions(goal)) {
        bool alreadyRequired = false;
        for (const auto& semantic : intent.requirements)
            if (semantic.domain == structural.domain &&
                normalizedResource(semantic.resourceHint, semantic.domain) ==
                    normalizedResource(structural.resourceHint, structural.domain))
                alreadyRequired = true;
        if (!alreadyRequired)
            intent.requirements.append(structural);
    }
    return intent;
}

QList<EvidenceRecord> EvidencePolicy::record(const ToolDescriptor& descriptor,
                                              const PlannedToolInvocation& invocation,
                                              ToolExecutionStatus status, const QString& summary,
                                              int stepIndex, const QString& toolCallId) {
    QList<EvidenceRecord> records;
    if (status == ToolExecutionStatus::InvalidArguments ||
        status == ToolExecutionStatus::InvalidToolContract ||
        status == ToolExecutionStatus::EmptyPlan ||
        status == ToolExecutionStatus::NotRequested)
        return records;
    const bool succeeded = status == ToolExecutionStatus::Succeeded;
    const bool denied = status == ToolExecutionStatus::Blocked;
    const bool unavailable = status == ToolExecutionStatus::UnknownTool;
    for (const auto& produced : descriptor.evidenceProduced) {
        EvidenceRecord record;
        record.toolId = descriptor.id;
        record.toolCallId = toolCallId;
        record.stepIndex = stepIndex;
        record.domain = produced.domain;
        record.resource = produced.resourceArgument.isEmpty()
                              ? (produced.scope == EvidenceScope::Provider
                                     ? descriptor.providerId
                                     : QString{})
                              : argumentValue(invocation, produced.resourceArgument);
        if (record.resource.isEmpty() && produced.scope == EvidenceScope::SearchScope)
            record.resource = QDir::currentPath();
        record.qualifier = argumentValue(invocation, produced.qualifierArgument);
        record.freshness = produced.freshness;
        record.scope = produced.scope;
        record.outcome = succeeded ? EvidenceOutcome::Verified
                                 : denied ? EvidenceOutcome::Denied
                                 : unavailable ? EvidenceOutcome::Unavailable
                                               : EvidenceOutcome::Failed;
        // A bounded directory listing cannot prove absence outside its returned page.
        if (succeeded && produced.scope == EvidenceScope::DirectoryEntries) {
            const auto document = QJsonDocument::fromJson(summary.toUtf8());
            if (document.isObject() &&
                document.object().value(QStringLiteral("truncated")).toBool())
                record.outcome = EvidenceOutcome::Unavailable;
        }
        record.observedAtUtc = QDateTime::currentDateTimeUtc();
        records.append(std::move(record));
    }
    return records;
}

EvidenceGateResult EvidencePolicy::evaluate(const ObservationIntent& intent,
                                             const QList<EvidenceRecord>& evidence,
                                             GroundingMode mode, bool groundingDeclared) {
    EvidenceGateResult result;
    result.grounding.mode = mode;
    if (intent.indeterminate) {
        result.repair = QStringLiteral("Observation need is unclear. Choose an appropriate "
                                       "observation tool before answering, or report inability "
                                       "to determine a valid action.");
        return result;
    }
    if (intent.requirements.isEmpty()) {
        if (mode == GroundingMode::UnableToVerify) {
            result.repair = QStringLiteral("No failed observation supports unable_to_verify.");
            return result;
        }
        if (mode == GroundingMode::Verified) {
            for (const auto& item : evidence)
                if (item.outcome == EvidenceOutcome::Verified)
                    result.grounding.evidenceCallIds.append(item.toolCallId);
            if (result.grounding.evidenceCallIds.isEmpty()) {
                result.repair = QStringLiteral("Verified grounding requires a successful "
                                               "current-run observation.");
                return result;
            }
        }
        result.accepted = true;
        return result;
    }
    if (!groundingDeclared) {
        result.repair = QStringLiteral("Declare final grounding as verified or "
                                       "unable_to_verify.");
        return result;
    }
    if (mode == GroundingMode::Context) {
        result.repair = QStringLiteral("This goal requires fresh external evidence. Use an "
                                       "appropriate observation tool before answering.");
        return result;
    }
    QStringList failures;
    for (const auto& requirement : intent.requirements) {
        const EvidenceRecord* latest = nullptr;
        for (const auto& item : evidence) {
            if (!matches(requirement, item))
                continue;
            if (!latest || item.stepIndex >= latest->stepIndex)
                latest = &item;
        }
        if (latest && latest->outcome == EvidenceOutcome::Verified) {
            result.grounding.evidenceCallIds.append(latest->toolCallId);
            continue;
        }
        if (mode == GroundingMode::Verified) {
            result.repair = QStringLiteral("Verified answer lacks fresh %1 evidence for %2. "
                                           "Use an appropriate tool.")
                                .arg(observationDomainName(requirement.domain),
                                     requirement.resourceHint.isEmpty()
                                         ? QStringLiteral("the requested resource")
                                         : requirement.resourceHint);
            return result;
        }
        if (!latest) {
            result.repair = QStringLiteral("No observation attempt exists for required %1 "
                                           "evidence. Use an appropriate tool.")
                                .arg(observationDomainName(requirement.domain));
            return result;
        }
        result.grounding.evidenceCallIds.append(latest->toolCallId);
        failures.append(requirement.resourceHint.isEmpty()
                            ? observationDomainName(requirement.domain)
                            : requirement.resourceHint);
    }
    if (mode == GroundingMode::UnableToVerify) {
        if (failures.isEmpty()) {
            result.repair = QStringLiteral("All required observations succeeded; provide a "
                                           "verified answer.");
            return result;
        }
        // Do not publish the model's wording after failed observations: it could
        // claim presence or absence while declaring unable_to_verify.
        result.answerOverride = QStringLiteral("I could not verify the current state of %1 "
                                               "because the observation did not succeed.")
                                    .arg(failures.join(QStringLiteral(", ")));
    }
    result.grounding.evidenceCallIds.removeDuplicates();
    result.accepted = true;
    return result;
}
} // namespace sentinel::core
