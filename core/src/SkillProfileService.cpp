#include "sentinel/core/SkillProfileService.h"

namespace sentinel::core {

namespace {

QList<SkillProfileMetadata> profiles() {
    return {
        {
            QStringLiteral("developer"),
            QStringLiteral("Developer"),
            QStringLiteral("Code-aware presentation for software work."),
            QStringLiteral("Prioritizes concise technical framing, implementation details, and "
                           "future coding capability metadata without changing prompts or "
                           "runtime authority."),
            QStringLiteral("Metadata only"),
            QStringLiteral("Technical summaries and developer-facing labels"),
            QStringLiteral("No runtime authority; tools, agents, workspace access, and prompt "
                           "mutation remain disabled unless separately scoped."),
            {
                QStringLiteral("Presentation: technical"),
                QStringLiteral("Future capability metadata: coding, debugging, code review"),
                QStringLiteral("Prompt mutation: disabled"),
                QStringLiteral("Tool and workspace authority: disabled"),
            },
        },
        {
            QStringLiteral("student"),
            QStringLiteral("Student"),
            QStringLiteral("Learning-oriented presentation for study sessions."),
            QStringLiteral("Emphasizes approachable summaries, study structure, and future "
                           "learning-support metadata while preserving the same execution "
                           "boundaries."),
            QStringLiteral("Metadata only"),
            QStringLiteral("Guided explanations and study-friendly labels"),
            QStringLiteral("No tutoring workflow automation, hidden memory writes, prompt "
                           "changes, or external content access."),
            {
                QStringLiteral("Presentation: instructional"),
                QStringLiteral("Future capability metadata: explanation, practice, review"),
                QStringLiteral("Prompt mutation: disabled"),
                QStringLiteral("Filesystem, cloud, and voice authority: disabled"),
            },
        },
        {
            QStringLiteral("researcher"),
            QStringLiteral("Researcher"),
            QStringLiteral("Evidence-oriented presentation for analysis work."),
            QStringLiteral("Highlights source discipline, comparison, and future research "
                           "workflow metadata without enabling retrieval, browsing, tools, or "
                           "background agents."),
            QStringLiteral("Metadata only"),
            QStringLiteral("Analytical summaries and source-aware labels"),
            QStringLiteral("No browsing, provider calls, filesystem scanning, retrieval "
                           "activation, or autonomous research behavior."),
            {
                QStringLiteral("Presentation: analytical"),
                QStringLiteral("Future capability metadata: compare, synthesize, cite"),
                QStringLiteral("Prompt mutation: disabled"),
                QStringLiteral("Network, retrieval, and agent authority: disabled"),
            },
        },
        {
            QStringLiteral("personal-assistant"),
            QStringLiteral("Personal Assistant"),
            QStringLiteral("Everyday planning presentation for personal organization."),
            QStringLiteral("Uses calmer task and schedule-oriented labels for future assistant "
                           "workflows while keeping calendars, notifications, files, voice, and "
                           "automation inactive."),
            QStringLiteral("Metadata only"),
            QStringLiteral("Personal organization summaries"),
            QStringLiteral("No calendar/mail connectors, notifications, voice activation, tools, "
                           "or autonomous actions."),
            {
                QStringLiteral("Presentation: personal organization"),
                QStringLiteral("Future capability metadata: planning, reminders, daily brief"),
                QStringLiteral("Prompt mutation: disabled"),
                QStringLiteral("Connectors, voice, and automation authority: disabled"),
            },
        },
        {
            QStringLiteral("custom"),
            QStringLiteral("Custom"),
            QStringLiteral("User-defined profile placeholder."),
            QStringLiteral("Reserved for future user-authored profile metadata. This phase stores "
                           "only the selected profile id and exposes inert readiness text."),
            QStringLiteral("Placeholder"),
            QStringLiteral("Custom metadata placeholder"),
            QStringLiteral("No custom instructions, hidden system prompts, plugins, tools, or "
                           "workspace permissions are loaded."),
            {
                QStringLiteral("Presentation: custom placeholder"),
                QStringLiteral("Future capability metadata: user-defined after explicit scope"),
                QStringLiteral("Prompt mutation: disabled"),
                QStringLiteral("Custom plugin and instruction loading: disabled"),
            },
        },
    };
}

SkillProfileMetadata defaultProfile() {
    return profiles().first();
}

} // namespace

QList<SkillProfileMetadata> SkillProfileService::availableProfiles() const {
    return profiles();
}

SkillProfileMetadata SkillProfileService::selectedProfile(const QString& selectedProfileId) const {
    const auto normalized = normalizedProfileId(selectedProfileId);
    for (const auto& profile : availableProfiles()) {
        if (profile.id == normalized) {
            return profile;
        }
    }
    return defaultProfile();
}

SkillProfileReadinessSummary SkillProfileService::readiness(const QString& selectedProfileId) const {
    const auto profile = selectedProfile(selectedProfileId);
    return {
        profile.readiness,
        QStringLiteral("%1 profile is presentation metadata only. It does not change prompts, "
                       "providers, tools, agents, workspace access, voice, or automation.")
            .arg(profile.name),
        {
            QStringLiteral("Selected profile: %1").arg(profile.name),
            QStringLiteral("Presentation posture: %1").arg(profile.presentationPosture),
            QStringLiteral("Prompt mutation: disabled"),
            QStringLiteral("Hidden system prompt changes: disabled"),
            QStringLiteral("Tool execution and agent activation: disabled"),
            QStringLiteral("Workspace, filesystem, cloud/API, STT/TTS, and subprocess authority: "
                           "disabled"),
        },
        {
            QStringLiteral("Profile id: %1").arg(profile.id),
            QStringLiteral("Readiness: %1").arg(profile.readiness),
            QStringLiteral("Policy posture: %1").arg(profile.policyPosture),
            QStringLiteral("Runtime authority: unchanged"),
            QStringLiteral("Data boundary: selected profile id in settings only"),
            QStringLiteral("Prompt boundary: no mutation, no hidden system prompt changes"),
        },
    };
}

QStringList SkillProfileService::profileSummaries() const {
    QStringList summaries;
    for (const auto& profile : availableProfiles()) {
        summaries.append(skillProfileSummary(profile));
    }
    return summaries;
}

QString SkillProfileService::normalizedProfileId(const QString& profileId) const {
    const auto normalized = profileId.trimmed().toLower();
    for (const auto& profile : availableProfiles()) {
        if (profile.id == normalized) {
            return profile.id;
        }
    }
    return defaultProfile().id;
}

QString skillProfileSummary(const SkillProfileMetadata& profile) {
    return QStringLiteral("%1 / %2 / %3")
        .arg(profile.name, profile.readiness, profile.summary);
}

} // namespace sentinel::core
