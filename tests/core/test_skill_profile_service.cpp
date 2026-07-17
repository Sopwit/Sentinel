#include "sentinel/core/SkillProfileService.h"

#include <QtTest>

using sentinel::core::SkillProfileService;

class SkillProfileServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesDefaultProfiles();
    void reportsMetadataOnlyReadiness();
    void normalizesUnknownSelection();
};

void SkillProfileServiceTest::exposesDefaultProfiles() {
    const SkillProfileService service;
    const auto profiles = service.availableProfiles();

    QCOMPARE(profiles.size(), 5);
    QCOMPARE(profiles.at(0).id, QStringLiteral("developer"));
    QCOMPARE(profiles.at(1).id, QStringLiteral("student"));
    QCOMPARE(profiles.at(2).id, QStringLiteral("researcher"));
    QCOMPARE(profiles.at(3).id, QStringLiteral("personal-assistant"));
    QCOMPARE(profiles.at(4).id, QStringLiteral("custom"));
    QVERIFY(profiles.at(0)
                .capabilitySummaries.join(QStringLiteral("\n"))
                .contains(QStringLiteral("Prompt mutation: disabled")));
    QVERIFY(service.profileSummaries()
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Personal Assistant")));
}

void SkillProfileServiceTest::reportsMetadataOnlyReadiness() {
    const SkillProfileService service;
    const auto readiness = service.readiness(QStringLiteral("researcher"));

    QCOMPARE(readiness.status, QStringLiteral("Metadata only"));
    QVERIFY(readiness.summary.contains(QStringLiteral("presentation metadata only")));
    QVERIFY(readiness.checks.contains(QStringLiteral("Prompt mutation: disabled")));
    QVERIFY(readiness.checks.contains(QStringLiteral("Hidden system prompt changes: disabled")));
    QVERIFY(
        readiness.checks.contains(QStringLiteral("Tool execution and agent activation: disabled")));
    QVERIFY(
        readiness.developerDiagnostics.contains(QStringLiteral("Runtime authority: unchanged")));
    QVERIFY(readiness.developerDiagnostics.contains(
        QStringLiteral("Data boundary: selected profile id in settings only")));
}

void SkillProfileServiceTest::normalizesUnknownSelection() {
    const SkillProfileService service;

    QCOMPARE(service.normalizedProfileId(QStringLiteral(" STUDENT ")), QStringLiteral("student"));
    QCOMPARE(service.normalizedProfileId(QStringLiteral("unknown")), QStringLiteral("developer"));
    QCOMPARE(service.selectedProfile(QStringLiteral("unknown")).name, QStringLiteral("Developer"));
}

QTEST_MAIN(SkillProfileServiceTest)

#include "test_skill_profile_service.moc"
