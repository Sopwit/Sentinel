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
    void exposesOllamaEndpointDefault();
    void normalizesOllamaEndpoint();
    void persistsSelectedLocalModel();
    void persistsLocalChatInferenceOptIn();
    void persistsLocalInferenceStreamingOptIn();
    void persistsPromptContextInjectionOptIn();
    void persistsSemanticPromptInclusionOptIn();
    void persistsContextExplainabilityVisibility();
    void persistsDeveloperModeVisibilityOptIn();
    void persistsVoiceConfigurationPaths();
    void persistsPiperFileOutputExecutionOptIn();
    void persistsLocalAiRuntimeSettingsThroughJsonStore();
};

static std::unique_ptr<AppSettings> makeSettings() {
    return std::make_unique<AppSettings>(std::make_unique<InMemorySettingsStore>());
}

void AppSettingsTest::exposesDefaults() {
    const auto settings = makeSettings();

    QCOMPARE(settings->themeName(), QStringLiteral("Sentinel Dark"));
    QCOMPARE(settings->configurationProfile(), QStringLiteral("Desktop Alpha"));
    QVERIFY(settings->selectedLocalModel().isEmpty());
    QVERIFY(!settings->localChatInferenceEnabled());
    QVERIFY(!settings->localInferenceStreamingEnabled());
    QVERIFY(!settings->promptContextInjectionEnabled());
    QVERIFY(!settings->semanticPromptInclusionEnabled());
    QVERIFY(settings->contextExplainabilityVisible());
    QVERIFY(!settings->developerModeEnabled());
    QVERIFY(settings->piperBinaryPath().isEmpty());
    QVERIFY(settings->piperModelPath().isEmpty());
    QVERIFY(settings->whisperBinaryPath().isEmpty());
    QVERIFY(settings->whisperModelPath().isEmpty());
    QVERIFY(!settings->piperFileOutputExecutionEnabled());
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

void AppSettingsTest::exposesOllamaEndpointDefault() {
    const auto settings = makeSettings();

    QCOMPARE(settings->ollamaEndpoint(), QStringLiteral("http://127.0.0.1:11434"));
}

void AppSettingsTest::normalizesOllamaEndpoint() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::ollamaEndpointChanged);

    settings->setOllamaEndpoint(QStringLiteral(" http://localhost:11434/ "));

    QCOMPARE(settings->ollamaEndpoint(), QStringLiteral("http://localhost:11434"));
    QCOMPARE(spy.count(), 1);

    settings->setOllamaEndpoint(QStringLiteral("https://example.com"));

    QCOMPARE(settings->ollamaEndpoint(), QStringLiteral("http://127.0.0.1:11434"));
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsSelectedLocalModel() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::selectedLocalModelChanged);

    settings->setSelectedLocalModel(QStringLiteral(" llama3.2 "));

    QCOMPARE(settings->selectedLocalModel(), QStringLiteral("llama3.2"));
    QCOMPARE(spy.count(), 1);

    settings->setSelectedLocalModel(QStringLiteral("llama3.2"));
    QCOMPARE(spy.count(), 1);
}

void AppSettingsTest::persistsLocalChatInferenceOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::localChatInferenceEnabledChanged);

    QVERIFY(!settings->localChatInferenceEnabled());

    settings->setLocalChatInferenceEnabled(true);

    QVERIFY(settings->localChatInferenceEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setLocalChatInferenceEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setLocalChatInferenceEnabled(false);
    QVERIFY(!settings->localChatInferenceEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsLocalInferenceStreamingOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::localInferenceStreamingEnabledChanged);

    QVERIFY(!settings->localInferenceStreamingEnabled());

    settings->setLocalInferenceStreamingEnabled(true);

    QVERIFY(settings->localInferenceStreamingEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setLocalInferenceStreamingEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setLocalInferenceStreamingEnabled(false);
    QVERIFY(!settings->localInferenceStreamingEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsPromptContextInjectionOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::promptContextInjectionEnabledChanged);

    QVERIFY(!settings->promptContextInjectionEnabled());

    settings->setPromptContextInjectionEnabled(true);

    QVERIFY(settings->promptContextInjectionEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setPromptContextInjectionEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setPromptContextInjectionEnabled(false);
    QVERIFY(!settings->promptContextInjectionEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsSemanticPromptInclusionOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::semanticPromptInclusionEnabledChanged);

    QVERIFY(!settings->semanticPromptInclusionEnabled());

    settings->setSemanticPromptInclusionEnabled(true);

    QVERIFY(settings->semanticPromptInclusionEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setSemanticPromptInclusionEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setSemanticPromptInclusionEnabled(false);
    QVERIFY(!settings->semanticPromptInclusionEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsContextExplainabilityVisibility() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::contextExplainabilityVisibleChanged);

    QVERIFY(settings->contextExplainabilityVisible());

    settings->setContextExplainabilityVisible(false);

    QVERIFY(!settings->contextExplainabilityVisible());
    QCOMPARE(spy.count(), 1);

    settings->setContextExplainabilityVisible(false);
    QCOMPARE(spy.count(), 1);

    settings->setContextExplainabilityVisible(true);
    QVERIFY(settings->contextExplainabilityVisible());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsDeveloperModeVisibilityOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::developerModeEnabledChanged);

    QVERIFY(!settings->developerModeEnabled());

    settings->setDeveloperModeEnabled(true);

    QVERIFY(settings->developerModeEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setDeveloperModeEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setDeveloperModeEnabled(false);
    QVERIFY(!settings->developerModeEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsVoiceConfigurationPaths() {
    const auto settings = makeSettings();
    QSignalSpy piperBinarySpy(settings.get(), &AppSettings::piperBinaryPathChanged);
    QSignalSpy piperModelSpy(settings.get(), &AppSettings::piperModelPathChanged);
    QSignalSpy whisperBinarySpy(settings.get(), &AppSettings::whisperBinaryPathChanged);
    QSignalSpy whisperModelSpy(settings.get(), &AppSettings::whisperModelPathChanged);

    settings->setPiperBinaryPath(QStringLiteral(" /tmp/piper "));
    settings->setPiperModelPath(QStringLiteral(" /tmp/piper.onnx "));
    settings->setWhisperBinaryPath(QStringLiteral(" /tmp/whisper "));
    settings->setWhisperModelPath(QStringLiteral(" /tmp/whisper-models "));

    QCOMPARE(settings->piperBinaryPath(), QStringLiteral("/tmp/piper"));
    QCOMPARE(settings->piperModelPath(), QStringLiteral("/tmp/piper.onnx"));
    QCOMPARE(settings->whisperBinaryPath(), QStringLiteral("/tmp/whisper"));
    QCOMPARE(settings->whisperModelPath(), QStringLiteral("/tmp/whisper-models"));
    QCOMPARE(piperBinarySpy.count(), 1);
    QCOMPARE(piperModelSpy.count(), 1);
    QCOMPARE(whisperBinarySpy.count(), 1);
    QCOMPARE(whisperModelSpy.count(), 1);
}

void AppSettingsTest::persistsPiperFileOutputExecutionOptIn() {
    const auto settings = makeSettings();
    QSignalSpy spy(settings.get(), &AppSettings::piperFileOutputExecutionEnabledChanged);

    QVERIFY(!settings->piperFileOutputExecutionEnabled());

    settings->setPiperFileOutputExecutionEnabled(true);

    QVERIFY(settings->piperFileOutputExecutionEnabled());
    QCOMPARE(spy.count(), 1);

    settings->setPiperFileOutputExecutionEnabled(true);
    QCOMPARE(spy.count(), 1);

    settings->setPiperFileOutputExecutionEnabled(false);
    QVERIFY(!settings->piperFileOutputExecutionEnabled());
    QCOMPARE(spy.count(), 2);
}

void AppSettingsTest::persistsLocalAiRuntimeSettingsThroughJsonStore() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("settings.json"));

    {
        AppSettings settings{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};
        settings.setSelectedLocalModel(QStringLiteral(" llama3.2 "));
        settings.setLocalChatInferenceEnabled(true);
        settings.setLocalInferenceStreamingEnabled(true);
        settings.setPromptContextInjectionEnabled(true);
        settings.setSemanticPromptInclusionEnabled(true);
        settings.setContextExplainabilityVisible(false);
        settings.setDeveloperModeEnabled(true);
        settings.setPiperBinaryPath(QStringLiteral("/opt/piper/piper"));
        settings.setPiperModelPath(QStringLiteral("/opt/piper/model.onnx"));
        settings.setWhisperBinaryPath(QStringLiteral("/opt/whisper/whisper"));
        settings.setWhisperModelPath(QStringLiteral("/opt/whisper/models"));
        settings.setPiperFileOutputExecutionEnabled(true);
    }

    AppSettings reloaded{std::make_unique<sentinel::core::JsonSettingsStore>(filePath)};

    QCOMPARE(reloaded.selectedLocalModel(), QStringLiteral("llama3.2"));
    QVERIFY(reloaded.localChatInferenceEnabled());
    QVERIFY(reloaded.localInferenceStreamingEnabled());
    QVERIFY(reloaded.promptContextInjectionEnabled());
    QVERIFY(reloaded.semanticPromptInclusionEnabled());
    QVERIFY(!reloaded.contextExplainabilityVisible());
    QVERIFY(reloaded.developerModeEnabled());
    QCOMPARE(reloaded.piperBinaryPath(), QStringLiteral("/opt/piper/piper"));
    QCOMPARE(reloaded.piperModelPath(), QStringLiteral("/opt/piper/model.onnx"));
    QCOMPARE(reloaded.whisperBinaryPath(), QStringLiteral("/opt/whisper/whisper"));
    QCOMPARE(reloaded.whisperModelPath(), QStringLiteral("/opt/whisper/models"));
    QVERIFY(reloaded.piperFileOutputExecutionEnabled());
}

QTEST_MAIN(AppSettingsTest)

#include "test_app_settings.moc"
