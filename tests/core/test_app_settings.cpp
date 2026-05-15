#include "sentinel/core/AppSettings.h"
#include "sentinel/core/InMemorySettingsStore.h"
#include "sentinel/core/JsonSettingsStore.h"

#include <QSignalSpy>
#include <QTemporaryDir>
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
    void exposesRoutingModeDefault();
    void updatesRoutingMode();
    void fallsBackForInvalidRoutingMode();
    void persistsRoutingModeThroughJsonStore();
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

void AppSettingsTest::exposesRoutingModeDefault() {
    const auto settings = makeSettings();

    QCOMPARE(settings->routingModeName(), QStringLiteral("Local Only"));
    QCOMPARE(
        settings->availableRoutingModes(),
        QStringList({QStringLiteral("Auto"), QStringLiteral("Fast"), QStringLiteral("Balanced"),
                     QStringLiteral("Quality"), QStringLiteral("Local Only"),
                     QStringLiteral("Cloud Allowed"), QStringLiteral("Battery Saver")}));
}

void AppSettingsTest::updatesRoutingMode() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::routingModeNameChanged);

    settings->setRoutingModeName(QStringLiteral(" Balanced "));

    QCOMPARE(settings->routingModeName(), QStringLiteral("Balanced"));
    QCOMPARE(spy.count(), 1);

    settings->setRoutingModeName(QStringLiteral("Balanced"));
    QCOMPARE(spy.count(), 1);
}

void AppSettingsTest::fallsBackForInvalidRoutingMode() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::routingModeNameChanged);

    settings->setRoutingModeName(QStringLiteral("Balanced"));
    settings->setRoutingModeName(QStringLiteral("invalid-mode"));

    QCOMPARE(settings->routingModeName(), QStringLiteral("Local Only"));
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsRoutingModeThroughJsonStore() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("settings.json"));

    {
        AppSettings settings{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};
        settings.setRoutingModeName(QStringLiteral("Cloud Allowed"));
    }

    AppSettings reloaded{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};

    QCOMPARE(reloaded.routingModeName(), QStringLiteral("Cloud Allowed"));
}

QTEST_MAIN(AppSettingsTest)

#include "test_app_settings.moc"
