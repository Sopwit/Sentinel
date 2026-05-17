#include "sentinel/core/PiperTts.h"
#include "sentinel/core/Voice.h"

#include <QtTest>

using sentinel::core::buildVoiceReadinessReport;
using sentinel::core::NullPiperTtsClient;
using sentinel::core::NullSpeechToTextProvider;
using sentinel::core::NullTextToSpeechProvider;
using sentinel::core::NullVoiceRuntimeEnvironment;
using sentinel::core::PiperTextToSpeechProvider;
using sentinel::core::PiperTtsConfig;
using sentinel::core::PiperTtsRequest;
using sentinel::core::PiperTtsStatus;
using sentinel::core::piperTtsStatusName;
using sentinel::core::safePiperTtsResultSummary;
using sentinel::core::safeVoicePipelineSummary;
using sentinel::core::safeVoiceResponseSummary;
using sentinel::core::StaticVoiceRuntimeCoordinator;
using sentinel::core::StaticVoiceRuntimeEnvironment;
using sentinel::core::VoiceBinaryDescriptor;
using sentinel::core::voiceBinaryDescriptorSummaries;
using sentinel::core::VoiceBinaryStatus;
using sentinel::core::VoiceCapability;
using sentinel::core::VoiceModelDescriptor;
using sentinel::core::voiceModelDescriptorSummaries;
using sentinel::core::VoiceModelStatus;
using sentinel::core::VoicePipelineStage;
using sentinel::core::voicePipelineStageName;
using sentinel::core::VoicePipelineStatus;
using sentinel::core::voicePipelineStatusName;
using sentinel::core::voicePipelineTraceSummaries;
using sentinel::core::VoiceProviderStatus;
using sentinel::core::voiceProviderStatusName;
using sentinel::core::VoiceRequest;
using sentinel::core::VoiceRuntimeMode;
using sentinel::core::voiceRuntimeModeName;
using sentinel::core::VoiceSessionState;
using sentinel::core::voiceSessionStateName;

class VoiceTest final : public QObject {
    Q_OBJECT

private slots:
    void nullTextToSpeechRefusesDeterministically();
    void nullSpeechToTextRefusesDeterministically();
    void readinessReportStaysMetadataOnly();
    void staticVoiceRuntimeCoordinatorCompletesDeterministicPipeline();
    void staticVoiceRuntimeCoordinatorReportsBlockedAndErrorMetadata();
    void nullVoiceRuntimeEnvironmentReportsMissingOwnershipMetadata();
    void staticVoiceRuntimeEnvironmentUsesInjectedMetadataOnly();
    void voiceRuntimeSafetyBlocksExecutionByDefault();
    void nullPiperTtsClientRefusesWithoutSideEffects();
    void piperTextToSpeechProviderRefusesMissingBinaryAndModel();
    void piperTextToSpeechProviderReportsSafetyBlockedMetadata();
};

void VoiceTest::nullTextToSpeechRefusesDeterministically() {
    NullTextToSpeechProvider provider;

    const auto descriptor = provider.descriptor();
    QCOMPARE(descriptor.id, QStringLiteral("null-tts"));
    QCOMPARE(descriptor.status, VoiceProviderStatus::Disabled);
    QCOMPARE(descriptor.runtimeMode, VoiceRuntimeMode::Disabled);
    QCOMPARE(descriptor.capabilities, QList<VoiceCapability>{VoiceCapability::TextToSpeech});

    const auto response = provider.synthesize(VoiceRequest{QStringLiteral("hello")});
    QCOMPARE(response.status, VoiceProviderStatus::Refused);
    QCOMPARE(response.capability, VoiceCapability::TextToSpeech);
    QVERIFY(!response.available);
    QVERIFY(response.text.isEmpty());
    QCOMPARE(safeVoiceResponseSummary(response),
             QStringLiteral("Text-to-speech is disabled: Sentinel will not synthesize or play "
                            "audio in this phase."));
}

void VoiceTest::nullSpeechToTextRefusesDeterministically() {
    NullSpeechToTextProvider provider;

    const auto descriptor = provider.descriptor();
    QCOMPARE(descriptor.id, QStringLiteral("null-stt"));
    QCOMPARE(descriptor.status, VoiceProviderStatus::Disabled);
    QCOMPARE(descriptor.runtimeMode, VoiceRuntimeMode::Disabled);
    QCOMPARE(descriptor.capabilities, QList<VoiceCapability>{VoiceCapability::SpeechToText});

    const auto response = provider.transcribe(VoiceRequest{});
    QCOMPARE(response.status, VoiceProviderStatus::Refused);
    QCOMPARE(response.capability, VoiceCapability::SpeechToText);
    QVERIFY(!response.available);
    QVERIFY(response.text.isEmpty());
    QCOMPARE(safeVoiceResponseSummary(response),
             QStringLiteral("Speech-to-text is disabled: Sentinel will not record, read, or "
                            "transcribe audio in this phase."));
}

void VoiceTest::readinessReportStaysMetadataOnly() {
    NullTextToSpeechProvider tts;
    NullSpeechToTextProvider stt;

    const auto report = buildVoiceReadinessReport(tts.descriptor(), stt.descriptor());

    QCOMPARE(voiceRuntimeModeName(VoiceRuntimeMode::Disabled), QStringLiteral("Disabled"));
    QCOMPARE(voiceProviderStatusName(VoiceProviderStatus::Disabled), QStringLiteral("Disabled"));
    QCOMPARE(report.status, QStringLiteral("Disabled"));
    QVERIFY(report.summary.contains(QStringLiteral("metadata-only")));
    QVERIFY(report.summary.contains(QStringLiteral("no microphone")));
    QVERIFY(report.summary.contains(QStringLiteral("Piper")));
    QCOMPARE(report.checks.size(), 7);
    QVERIFY(report.checks.at(0).contains(QStringLiteral("Runtime")));
    QVERIFY(report.checks.at(1).contains(QStringLiteral("Text to speech")));
    QVERIFY(report.checks.at(2).contains(QStringLiteral("Speech to text")));
    QVERIFY(report.checks.at(3).contains(QStringLiteral("No microphone access")));
    QVERIFY(report.checks.at(4).contains(QStringLiteral("No audio playback")));
    QVERIFY(report.checks.at(5).contains(QStringLiteral("Local Only")));
    QVERIFY(report.checks.at(6).contains(QStringLiteral("No Piper")));
}

void VoiceTest::staticVoiceRuntimeCoordinatorCompletesDeterministicPipeline() {
    StaticVoiceRuntimeCoordinator coordinator;

    const auto runtime = coordinator.runtimeSummary();
    QVERIFY(!runtime.runtimeAvailable);
    QVERIFY(!runtime.textToSpeechAvailable);
    QVERIFY(!runtime.speechToTextAvailable);
    QVERIFY(!runtime.microphoneEnabled);
    QVERIFY(!runtime.playbackEnabled);
    QVERIFY(runtime.localOnlyPolicy);
    QVERIFY(!runtime.processExecutionEnabled);
    QCOMPARE(runtime.checks.size(), 7);

    const auto result = coordinator.evaluate(VoiceSessionState::Completed);

    QCOMPARE(result.session.id.value, QStringLiteral("voice-session-1"));
    QCOMPARE(result.session.state, VoiceSessionState::Completed);
    QCOMPARE(result.status, VoicePipelineStatus::Completed);
    QCOMPARE(voiceSessionStateName(result.session.state), QStringLiteral("completed"));
    QCOMPARE(voicePipelineStatusName(result.status), QStringLiteral("completed"));
    QCOMPARE(voicePipelineStageName(VoicePipelineStage::AwaitingInput),
             QStringLiteral("awaiting-input"));
    QCOMPARE(result.traces.size(), 7);
    QCOMPARE(result.traces.at(0).stage, VoicePipelineStage::Idle);
    QCOMPARE(result.traces.at(1).stage, VoicePipelineStage::Preparing);
    QCOMPARE(result.traces.at(2).stage, VoicePipelineStage::AwaitingInput);
    QCOMPARE(result.traces.at(3).stage, VoicePipelineStage::TranscribingPlaceholder);
    QCOMPARE(result.traces.at(4).stage, VoicePipelineStage::InferencePlaceholder);
    QCOMPARE(result.traces.at(5).stage, VoicePipelineStage::SynthesisPlaceholder);
    QCOMPARE(result.traces.at(6).stage, VoicePipelineStage::Completed);
    QVERIFY(safeVoicePipelineSummary(result).contains(QStringLiteral("metadata-only")));
    QVERIFY(voicePipelineTraceSummaries(result.traces)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("microphone remains disabled")));
    QVERIFY(voicePipelineTraceSummaries(result.traces)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("does not invoke Piper")));
}

void VoiceTest::staticVoiceRuntimeCoordinatorReportsBlockedAndErrorMetadata() {
    StaticVoiceRuntimeCoordinator coordinator;

    const auto blocked = coordinator.evaluate(VoiceSessionState::Blocked);
    QCOMPARE(blocked.status, VoicePipelineStatus::Blocked);
    QCOMPARE(blocked.session.state, VoiceSessionState::Blocked);
    QCOMPARE(blocked.traces.size(), 3);
    QCOMPARE(blocked.traces.last().stage, VoicePipelineStage::Blocked);
    QVERIFY(safeVoicePipelineSummary(blocked).contains(QStringLiteral("blocked")));
    QVERIFY(voicePipelineTraceSummaries(blocked.traces)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("metadata-only runtime policy")));

    const auto error = coordinator.evaluate(VoiceSessionState::Error);
    QCOMPARE(error.status, VoicePipelineStatus::Error);
    QCOMPARE(error.session.state, VoiceSessionState::Error);
    QCOMPARE(error.traces.size(), 3);
    QCOMPARE(error.traces.last().stage, VoicePipelineStage::Error);
    QVERIFY(safeVoicePipelineSummary(error).contains(QStringLiteral("error metadata")));
    QVERIFY(voicePipelineTraceSummaries(error.traces)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("without runtime execution")));
}

void VoiceTest::nullVoiceRuntimeEnvironmentReportsMissingOwnershipMetadata() {
    NullVoiceRuntimeEnvironment environment;

    QCOMPARE(environment.status(), QStringLiteral("Blocked"));
    QVERIFY(environment.summary().contains(QStringLiteral("metadata-only")));
    QVERIFY(environment.summary().contains(QStringLiteral("no filesystem scan")));
    QCOMPARE(environment.binaries().size(), 2);
    QCOMPARE(environment.models().size(), 2);
    QCOMPARE(environment.binaries().first().status, VoiceBinaryStatus::Missing);
    QCOMPARE(environment.models().last().status, VoiceModelStatus::Missing);
    QVERIFY(voiceBinaryDescriptorSummaries(environment.binaries())
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper Binary: Missing")));
    QVERIFY(voiceModelDescriptorSummaries(environment.models())
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Whisper Model: Missing")));
    QVERIFY(!environment.binaries().first().executableAllowed);
    QVERIFY(!environment.models().first().loadAllowed);
}

void VoiceTest::staticVoiceRuntimeEnvironmentUsesInjectedMetadataOnly() {
    StaticVoiceRuntimeEnvironment environment{
        QList<VoiceBinaryDescriptor>{
            VoiceBinaryDescriptor{QStringLiteral("piper-binary"), QStringLiteral("Piper Binary"),
                                  VoiceCapability::TextToSpeech, VoiceBinaryStatus::ExpectedPath,
                                  QStringLiteral("/opt/sentinel/voice/piper"), false,
                                  QStringLiteral("Expected explicit Piper path only.")},
            VoiceBinaryDescriptor{QStringLiteral("whisper-binary"),
                                  QStringLiteral("Whisper Binary"), VoiceCapability::SpeechToText,
                                  VoiceBinaryStatus::ExpectedPath,
                                  QStringLiteral("/opt/sentinel/voice/whisper"), false,
                                  QStringLiteral("Expected explicit Whisper path only.")},
        },
        QList<VoiceModelDescriptor>{
            VoiceModelDescriptor{QStringLiteral("piper-model"), QStringLiteral("Piper Voice Model"),
                                 VoiceCapability::TextToSpeech, VoiceModelStatus::ExpectedPath,
                                 QStringLiteral("/opt/sentinel/voice/models/piper.onnx"), false,
                                 QStringLiteral("Expected explicit Piper model path only.")},
            VoiceModelDescriptor{QStringLiteral("whisper-model"), QStringLiteral("Whisper Model"),
                                 VoiceCapability::SpeechToText, VoiceModelStatus::ExpectedPath,
                                 QStringLiteral("/opt/sentinel/voice/models/whisper.bin"), false,
                                 QStringLiteral("Expected explicit Whisper model path only.")},
        }};

    QCOMPARE(environment.status(), QStringLiteral("Blocked"));
    QVERIFY(environment.summary().contains(QStringLiteral("static metadata only")));
    QVERIFY(voiceBinaryDescriptorSummaries(environment.binaries())
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("/opt/sentinel/voice/piper")));
    QVERIFY(voiceModelDescriptorSummaries(environment.models())
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("/opt/sentinel/voice/models/whisper.bin")));
    QCOMPARE(environment.permissions().size(), 4);
}

void VoiceTest::voiceRuntimeSafetyBlocksExecutionByDefault() {
    StaticVoiceRuntimeEnvironment environment;
    const auto report = environment.safetyReport();

    QCOMPARE(report.status, QStringLiteral("Blocked"));
    QVERIFY(!report.executionAllowed);
    QVERIFY(!report.microphoneAllowed);
    QVERIFY(!report.playbackAllowed);
    QVERIFY(!report.processExecutionAllowed);
    QVERIFY(!report.filesystemWideScanAllowed);
    QVERIFY(!report.downloadsAllowed);
    QVERIFY(!report.cloudAllowed);
    QCOMPARE(report.checks.size(), 7);
    QVERIFY(report.summary.contains(QStringLiteral("Piper")));
    QVERIFY(report.summary.contains(QStringLiteral("Whisper")));
}

void VoiceTest::nullPiperTtsClientRefusesWithoutSideEffects() {
    NullPiperTtsClient client;
    const auto result = client.synthesize(PiperTtsRequest{QStringLiteral("hello")},
                                          sentinel::core::defaultDisabledPiperTtsConfig());

    QCOMPARE(client.status(), PiperTtsStatus::Disabled);
    QVERIFY(client.statusSummary().contains(QStringLiteral("never launches Piper")));
    QCOMPARE(result.status, PiperTtsStatus::Disabled);
    QVERIFY(!result.success);
    QVERIFY(result.audioPath.isEmpty());
    QVERIFY(safePiperTtsResultSummary(result).contains(QStringLiteral("disabled")));
    QCOMPARE(result.traces.size(), 1);
}

void VoiceTest::piperTextToSpeechProviderRefusesMissingBinaryAndModel() {
    auto config = sentinel::core::defaultDisabledPiperTtsConfig();
    config.enabled = true;
    PiperTextToSpeechProvider missingAll{config, std::make_unique<NullPiperTtsClient>()};

    QCOMPARE(missingAll.status(), PiperTtsStatus::NotConfigured);
    QCOMPARE(piperTtsStatusName(missingAll.status()), QStringLiteral("Not Configured"));
    QVERIFY(missingAll.piperStatusSummary().contains(QStringLiteral("not configured")));

    config.binary.expectedPath = QStringLiteral("/opt/sentinel/voice/piper");
    PiperTextToSpeechProvider missingBinary{config, std::make_unique<NullPiperTtsClient>()};
    QCOMPARE(missingBinary.status(), PiperTtsStatus::MissingBinary);
    auto result = missingBinary.synthesizePiper(PiperTtsRequest{QStringLiteral("hello")});
    QCOMPARE(result.status, PiperTtsStatus::MissingBinary);
    QVERIFY(!result.success);
    QVERIFY(result.audioPath.isEmpty());

    config.binary.status = VoiceBinaryStatus::ExpectedPath;
    config.voiceModel.expectedPath = QStringLiteral("/opt/sentinel/voice/models/en.onnx");
    PiperTextToSpeechProvider missingModel{config, std::make_unique<NullPiperTtsClient>()};
    QCOMPARE(missingModel.status(), PiperTtsStatus::MissingModel);
    result = missingModel.synthesizePiper(PiperTtsRequest{QStringLiteral("hello")});
    QCOMPARE(result.status, PiperTtsStatus::MissingModel);
    QVERIFY(!result.success);
    QVERIFY(result.audioPath.isEmpty());
}

void VoiceTest::piperTextToSpeechProviderReportsSafetyBlockedMetadata() {
    PiperTtsConfig config = sentinel::core::defaultDisabledPiperTtsConfig();
    config.enabled = true;
    config.binary.status = VoiceBinaryStatus::PresentMetadata;
    config.binary.expectedPath = QStringLiteral("/opt/sentinel/voice/piper");
    config.voiceModel.status = VoiceModelStatus::PresentMetadata;
    config.voiceModel.expectedPath = QStringLiteral("/opt/sentinel/voice/models/en.onnx");

    PiperTextToSpeechProvider provider{config, std::make_unique<NullPiperTtsClient>()};

    QCOMPARE(provider.status(), PiperTtsStatus::SafetyBlocked);
    QCOMPARE(provider.descriptor().id, QStringLiteral("piper-tts"));
    QCOMPARE(provider.descriptor().status, VoiceProviderStatus::Disabled);
    QVERIFY(provider.piperStatusSummary().contains(QStringLiteral("safety policy")));
    QVERIFY(provider.readinessChecks()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper binary")));

    const auto response = provider.synthesize(VoiceRequest{QStringLiteral("hello")});
    QCOMPARE(response.status, VoiceProviderStatus::Refused);
    QVERIFY(!response.available);
    QVERIFY(response.summary.contains(QStringLiteral("blocked")));
}

QTEST_MAIN(VoiceTest)

#include "test_voice.moc"
