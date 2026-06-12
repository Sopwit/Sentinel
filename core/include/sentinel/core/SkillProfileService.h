#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {

struct SkillProfileMetadata {
    QString id;
    QString name;
    QString summary;
    QString description;
    QString readiness;
    QString presentationPosture;
    QString policyPosture;
    QStringList capabilitySummaries;
};

struct SkillProfileReadinessSummary {
    QString status;
    QString summary;
    QStringList checks;
    QStringList developerDiagnostics;
};

class SkillProfileService final {
public:
    QList<SkillProfileMetadata> availableProfiles() const;
    SkillProfileMetadata selectedProfile(const QString& selectedProfileId) const;
    SkillProfileReadinessSummary readiness(const QString& selectedProfileId) const;
    QStringList profileSummaries() const;
    QString normalizedProfileId(const QString& profileId) const;
};

QString skillProfileSummary(const SkillProfileMetadata& profile);

} // namespace sentinel::core
