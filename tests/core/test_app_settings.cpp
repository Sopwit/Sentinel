#include "sentinel/core/AppSettings.h"
#include "sentinel/core/InMemorySettingsStore.h"

#include <QSignalSpy>
#include <QtTest>

#include <memory>

using sentinel::core::AppSettings;
using sentinel::core::InMemorySettingsStore;

class AppSettingsTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesDefaults();
    void updatesThemeName();
    void ignoresBlankThemeName();
    void updatesConfigurationProfile();
};

static std::unique_ptr<AppSettings> makeSettings() {
    return std::make_unique<AppSettings>(std::make_unique<InMemorySettingsStore>());
}

void AppSettingsTest::exposesDefaults() {
    const auto settings = makeSettings();

    QCOMPARE(settings->themeName(), QStringLiteral("Sentinel Dark"));
    QCOMPARE(settings->configurationProfile(), QStringLiteral("Desktop Alpha"));
}

void AppSettingsTest::updatesThemeName() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::themeNameChanged);

    settings->setThemeName(QStringLiteral(" Sentinel Light "));

    QCOMPARE(settings->themeName(), QStringLiteral("Sentinel Light"));
    QCOMPARE(spy.count(), 1);

    settings->setThemeName(QStringLiteral("Sentinel Light"));
    QCOMPARE(spy.count(), 1);
}

void AppSettingsTest::ignoresBlankThemeName() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::themeNameChanged);

    settings->setThemeName(QStringLiteral("   "));

    QCOMPARE(settings->themeName(), QStringLiteral("Sentinel Dark"));
    QCOMPARE(spy.count(), 0);
}

void AppSettingsTest::updatesConfigurationProfile() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::configurationProfileChanged);

    settings->setConfigurationProfile(QStringLiteral("Phase 2 Shell"));

    QCOMPARE(settings->configurationProfile(), QStringLiteral("Phase 2 Shell"));
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(AppSettingsTest)

#include "test_app_settings.moc"
