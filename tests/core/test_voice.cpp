#include "sentinel/core/AudioFileSession.h"
#include "sentinel/core/PiperTts.h"
#include "sentinel/core/Voice.h"
#include "sentinel/core/WhisperTranscription.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using sentinel::core::AudioFileDescriptor;
using sentinel::core::AudioFileSessionPolicy;
using sentinel::core::audioFileSessionRefusalSummaries;
using sentinel::core::audioFileSessionSafetyChecks;
using sentinel::core::AudioFileSessionStatus;
using sentinel::core::audioFileSessionStatusName;
using sentinel::core::audioFileTraceSummaries;
using sentinel::core::AudioFileValidationStatus;
using sentinel::core::audioFileValidationSummaries;
using sentinel::core::buildAudioFileSessionResult;
using sentinel::core::buildVoicePipelineSessionResult;
using sentinel::core::buildVoiceReadinessReport;
using sentinel::core::configuredPiperSynthesisConfig;
using sentinel::core::configuredWhisperTranscriptionConfig;
using sentinel::core::defaultDisabledPiperSynthesisConfig;
using sentinel::core::defaultDisabledWhisperTranscriptionConfig;
using sentinel::core::LocalPiperSynthesisClient;
using sentinel::core::LocalWhisperTranscriptionClient;
using sentinel::core::NullPiperSynthesisClient;
using sentinel::core::NullPiperTtsClient;
using sentinel::core::NullSpeechToTextProvider;
using sentinel::core::NullTextToSpeechProvider;
using sentinel::core::NullVoiceRuntimeEnvironment;
using sentinel::core::NullWhisperTranscriptionClient;
using sentinel::core::piperRuntimeDescriptorFromConfiguration;
using sentinel::core::piperRuntimeDescriptorSummary;
using sentinel::core::PiperRuntimeReadiness;
using sentinel::core::PiperRuntimeStatus;
using sentinel::core::piperSynthesisReadiness;
using sentinel::core::PiperSynthesisRequest;
using sentinel::core::piperSynthesisSafetyReport;
using sentinel::core::PiperSynthesisStatus;
using sentinel::core::piperSynthesisStatusName;
using sentinel::core::PiperTextToSpeechProvider;
using sentinel::core::PiperTtsConfig;
using sentinel::core::PiperTtsRequest;
using sentinel::core::PiperTtsResult;
using sentinel::core::PiperTtsStatus;
using sentinel::core::piperTtsStatusName;
using sentinel::core::safePiperSynthesisResultSummary;
using sentinel::core::safePiperTtsResultSummary;
using sentinel::core::safeVoicePipelineSummary;
using sentinel::core::safeVoiceResponseSummary;
using sentinel::core::safeWhisperTranscriptionResultSummary;
using sentinel::core::StaticVoiceRuntimeCoordinator;
using sentinel::core::StaticVoiceRuntimeEnvironment;
using sentinel::core::supportedAudioFileExtensionSummaries;
using sentinel::core::VoiceBinaryDescriptor;
using sentinel::core::voiceBinaryDescriptorSummaries;
using sentinel::core::VoiceBinaryStatus;
using sentinel::core::VoiceCapability;
using sentinel::core::VoiceModelDescriptor;
using sentinel::core::voiceModelDescriptorSummaries;
using sentinel::core::VoiceModelStatus;
using sentinel::core::VoicePipelineSessionReadiness;
using sentinel::core::VoicePipelineSessionStatus;
using sentinel::core::voicePipelineSessionStatusName;
using sentinel::core::VoicePipelineSessionStep;
using sentinel::core::voicePipelineSessionStepName;
using sentinel::core::voicePipelineSessionStepSummaries;
using sentinel::core::voicePipelineSessionTraceSummaries;
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
using sentinel::core::VoiceRuntimeReadiness;
using sentinel::core::voiceRuntimeReadinessChecks;
using sentinel::core::voiceRuntimeReadinessReport;
using sentinel::core::voiceRuntimeReadinessSummaryText;
using sentinel::core::voiceRuntimeSafetyReportForReadiness;
using sentinel::core::VoiceSessionState;
using sentinel::core::voiceSessionStateName;
using sentinel::core::whisperRuntimeDescriptorFromConfiguration;
using sentinel::core::whisperRuntimeDescriptorSummary;
using sentinel::core::WhisperRuntimeReadiness;
using sentinel::core::WhisperRuntimeStatus;
using sentinel::core::whisperTranscriptionReadiness;
using sentinel::core::WhisperTranscriptionRequest;
using sentinel::core::whisperTranscriptionSafetyReport;
using sentinel::core::WhisperTranscriptionStatus;
using sentinel::core::whisperTranscriptionStatusName;

namespace {

class FakePiperTtsClient final : public sentinel::core::IPiperTtsClient {
public:
    enum class Mode : std::uint8_t {
        Success,
        Failure,
        Timeout,
    };

    explicit FakePiperTtsClient(Mode mode) : mode_(mode) {}

    PiperTtsStatus status() const override {
        return PiperTtsStatus::Configured;
    }

    QString statusSummary() const override {
        return QStringLiteral("Fake Piper TTS client is deterministic and local-only.");
    }

    PiperTtsResult synthesize(const PiperTtsRequest& request,
                              const PiperTtsConfig& config) override {
        if (mode_ == Mode::Failure) {
            return PiperTtsResult{
                PiperTtsStatus::Failed,
                false,
                request.outputPath,
                QStringLiteral("Controlled Piper TTS output path: %1").arg(request.outputPath),
                request.timeoutMs,
                7,
                QStringLiteral("fake failure"),
                QStringLiteral("Fake Piper failed while writing controlled output metadata."),
                {QStringLiteral("Fake Piper failure path.")},
            };
        }

        if (mode_ == Mode::Timeout) {
            return PiperTtsResult{
                PiperTtsStatus::Timeout,
                false,
                request.outputPath,
                QStringLiteral("Controlled Piper TTS output path: %1").arg(request.outputPath),
                request.timeoutMs,
                -1,
                QStringLiteral("fake timeout"),
                QStringLiteral("Fake Piper timed out before writing controlled output."),
                {QStringLiteral("Fake Piper timeout path.")},
            };
        }

        QDir().mkpath(config.controlledOutputDirectory);
        QFile file(request.outputPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return PiperTtsResult{
                PiperTtsStatus::Failed,
                false,
                request.outputPath,
                QStringLiteral("Controlled Piper TTS output path: %1").arg(request.outputPath),
                request.timeoutMs,
                -1,
                file.errorString(),
                QStringLiteral("Fake Piper could not write controlled output."),
                {QStringLiteral("Fake Piper write failed.")},
            };
        }
        file.write("FAKE-WAV");
        file.close();
        return PiperTtsResult{
            PiperTtsStatus::Succeeded,
            true,
            request.outputPath,
            QStringLiteral("Controlled Piper TTS output path: %1").arg(request.outputPath),
            request.timeoutMs,
            0,
            {},
            QStringLiteral("Fake Piper generated controlled local audio output. Playback was not "
                           "started."),
            {QStringLiteral("Fake Piper wrote controlled output without playback.")},
        };
    }

private:
    Mode mode_;
};

bool writeFile(const QString& path, const QByteArray& bytes) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(bytes);
    return true;
}

PiperTtsConfig configuredPiperConfig(QTemporaryDir& dir) {
    auto config = sentinel::core::defaultDisabledPiperTtsConfig();
    const auto binaryPath = QDir(dir.path()).filePath(QStringLiteral("piper"));
    const auto modelPath = QDir(dir.path()).filePath(QStringLiteral("voice.onnx"));
    const auto outputDir = QDir(dir.path()).filePath(QStringLiteral("tts-cache"));
    const bool binaryWritten = writeFile(binaryPath, "#!/bin/sh\nexit 0\n");
    const bool permissionsSet = QFile::setPermissions(binaryPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                                   QFileDevice::ExeOwner);
    const bool modelWritten = writeFile(modelPath, "model");
    if (!binaryWritten) {
        qFatal("Failed to write mock binary to %s", qPrintable(binaryPath));
    }
    if (!permissionsSet) {
        qFatal("Failed to set executable permissions on %s", qPrintable(binaryPath));
    }
    if (!modelWritten) {
        qFatal("Failed to write mock model to %s", qPrintable(modelPath));
    }

    config.enabled = true;
    config.processExecutionAllowed = false;
    config.fileOutputAllowed = false;
    config.audioPlaybackAllowed = false;
    config.binary.status = VoiceBinaryStatus::PresentMetadata;
    config.binary.expectedPath = binaryPath;
    config.binary.executableAllowed = false;
    config.voiceModel.status = VoiceModelStatus::PresentMetadata;
    config.voiceModel.expectedPath = modelPath;
    config.voiceModel.loadAllowed = false;
    config.controlledOutputDirectory = outputDir;
    config.timeoutMs = 1234;
    config.safetyReport.status = QStringLiteral("Blocked");
    config.safetyReport.summary =
        QStringLiteral("Voice runtime safety allows explicit local Piper file output only.");
    config.safetyReport.executionAllowed = false;
    config.safetyReport.processExecutionAllowed = false;
    config.safetyReport.microphoneAllowed = false;
    config.safetyReport.playbackAllowed = false;
    config.safetyReport.filesystemWideScanAllowed = false;
    config.safetyReport.downloadsAllowed = false;
    config.safetyReport.cloudAllowed = false;
    config.safetyReport.checks =
        sentinel::core::voiceRuntimeSafetyCheckSummaries(config.safetyReport);
    return config;
}

} // namespace

class VoiceTest final : public QObject {
    Q_OBJECT

private slots:
    void nullTextToSpeechRefusesDeterministically();
    void nullSpeechToTextRefusesDeterministically();
    void readinessReportStaysMetadataOnly();
    void staticVoiceRuntimeCoordinatorCompletesDeterministicPipeline();
    void staticVoiceRuntimeCoordinatorReportsBlockedAndErrorMetadata();
    void voicePipelineSessionOrchestratesReadinessDeterministically();
    void voicePipelineSessionBlocksMissingStagesAndFallsBackWithoutExecution();
    void audioFileSessionOrdersValidationMetadataDeterministically();
    void audioFileSessionReportsSupportedAndUnsupportedExtensionMetadata();
    void audioFileSessionRefusesUnsafePathWithoutExposingRawPath();
    void audioFileSessionFallsBackForEmptyAndOversizedMetadataWithoutExecution();
    void nullVoiceRuntimeEnvironmentReportsMissingOwnershipMetadata();
    void staticVoiceRuntimeEnvironmentUsesInjectedMetadataOnly();
    void voiceRuntimeSafetyBlocksExecutionByDefault();
    void voiceRuntimeConfigurationReportsMissingAndReadyMetadata();
    void voiceRuntimeConfigurationRefusesUnsafeNonLocalPaths();
    void nullWhisperTranscriptionClientRefusesWithoutSideEffects();
    void localWhisperTranscriptionRefusesMissingUnsafeAndNonLocalInput();
    void localWhisperTranscriptionReportsTimeoutFallbackWithoutExecution();
    void nullPiperSynthesisClientRefusesWithoutSideEffects();
    void localPiperSynthesisRefusesMissingUnsafeAndNonLocalInput();
    void localPiperSynthesisReportsTimeoutFallbackWithoutExecution();
    void nullPiperTtsClientRefusesWithoutSideEffects();
    void piperTextToSpeechProviderRefusesMissingBinaryAndModel();
    void piperTextToSpeechProviderReportsSafetyBlockedMetadata();
    void piperFileOutputRefusesPolicyBlockedAndInvalidRequests();
    void piperLegacyFileOutputRefusesWithoutSideEffects();
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

void VoiceTest::voicePipelineSessionOrchestratesReadinessDeterministically() {
    const auto ready = [](VoicePipelineSessionStep step, const QString& summary) {
        return VoicePipelineSessionReadiness{step, VoicePipelineSessionStatus::ReadyMetadata, true,
                                             summary};
    };

    const auto result =
        buildVoicePipelineSessionResult(ready(VoicePipelineSessionStep::TranscriptionReadiness,
                                              QStringLiteral("Whisper ready metadata only.")),
                                        ready(VoicePipelineSessionStep::ChatInferenceReadiness,
                                              QStringLiteral("Local chat ready metadata only.")),
                                        ready(VoicePipelineSessionStep::SynthesisReadiness,
                                              QStringLiteral("Piper ready metadata only.")));

    QCOMPARE(result.status, VoicePipelineSessionStatus::Completed);
    QCOMPARE(voicePipelineSessionStatusName(result.status), QStringLiteral("completed"));
    QCOMPARE(result.executionAttempted, false);
    QVERIFY(!result.safetyReport.executionAttempted);
    QVERIFY(!result.safetyReport.microphoneCaptureAllowed);
    QVERIFY(!result.safetyReport.audioPlaybackAllowed);
    QVERIFY(!result.safetyReport.whisperExecutionAllowed);
    QVERIFY(!result.safetyReport.piperExecutionAllowed);
    QVERIFY(!result.safetyReport.subprocessExecutionAllowed);
    QCOMPARE(result.traces.size(), 6);
    QCOMPARE(result.traces.at(0).step, VoicePipelineSessionStep::Prepare);
    QCOMPARE(result.traces.at(1).step, VoicePipelineSessionStep::AwaitAudioInput);
    QCOMPARE(result.traces.at(2).step, VoicePipelineSessionStep::TranscriptionReadiness);
    QCOMPARE(result.traces.at(3).step, VoicePipelineSessionStep::ChatInferenceReadiness);
    QCOMPARE(result.traces.at(4).step, VoicePipelineSessionStep::SynthesisReadiness);
    QCOMPARE(result.traces.at(5).step, VoicePipelineSessionStep::Completion);
    QCOMPARE(voicePipelineSessionStepName(result.traces.at(2).step),
             QStringLiteral("transcription-readiness"));
    QVERIFY(voicePipelineSessionTraceSummaries(result.traces)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("no STT")));
    QVERIFY(result.summary.summary.contains(QStringLiteral("execution attempted: no")));
}

void VoiceTest::voicePipelineSessionBlocksMissingStagesAndFallsBackWithoutExecution() {
    const auto ready = [](VoicePipelineSessionStep step) {
        return VoicePipelineSessionReadiness{
            step, VoicePipelineSessionStatus::ReadyMetadata, true,
            QStringLiteral("Ready metadata only; execution remains disabled.")};
    };
    const auto blocked = [](VoicePipelineSessionStep step, const QString& summary) {
        return VoicePipelineSessionReadiness{step, VoicePipelineSessionStatus::Blocked, false,
                                             summary};
    };

    const auto missingWhisper = buildVoicePipelineSessionResult(
        blocked(VoicePipelineSessionStep::TranscriptionReadiness,
                QStringLiteral("Missing Whisper readiness blocks transcription.")),
        ready(VoicePipelineSessionStep::ChatInferenceReadiness),
        ready(VoicePipelineSessionStep::SynthesisReadiness));
    QCOMPARE(missingWhisper.status, VoicePipelineSessionStatus::Fallback);
    QCOMPARE(missingWhisper.traces.at(2).step, VoicePipelineSessionStep::TranscriptionReadiness);
    QCOMPARE(missingWhisper.traces.at(3).step, VoicePipelineSessionStep::Refusal);
    QVERIFY(missingWhisper.fallback.summary.contains(QStringLiteral("no transcript")));
    QVERIFY(!missingWhisper.executionAttempted);

    const auto missingChat = buildVoicePipelineSessionResult(
        ready(VoicePipelineSessionStep::TranscriptionReadiness),
        blocked(VoicePipelineSessionStep::ChatInferenceReadiness,
                QStringLiteral("Missing local chat/model readiness blocks inference.")),
        ready(VoicePipelineSessionStep::SynthesisReadiness));
    QCOMPARE(missingChat.status, VoicePipelineSessionStatus::Fallback);
    QCOMPARE(missingChat.traces.at(3).step, VoicePipelineSessionStep::ChatInferenceReadiness);
    QVERIFY(missingChat.fallback.summary.contains(QStringLiteral("no chat send")));

    const auto missingPiper = buildVoicePipelineSessionResult(
        ready(VoicePipelineSessionStep::TranscriptionReadiness),
        ready(VoicePipelineSessionStep::ChatInferenceReadiness),
        blocked(VoicePipelineSessionStep::SynthesisReadiness,
                QStringLiteral("Missing Piper readiness blocks synthesis.")));
    QCOMPARE(missingPiper.status, VoicePipelineSessionStatus::Fallback);
    QCOMPARE(missingPiper.traces.at(4).step, VoicePipelineSessionStep::SynthesisReadiness);
    QVERIFY(missingPiper.fallback.summary.contains(QStringLiteral("no synthesis")));
    QVERIFY(voicePipelineSessionStepSummaries(missingPiper.steps)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Missing Piper readiness")));
    QVERIFY(!missingPiper.safetyReport.voiceChatAutoSendAllowed);
    QVERIFY(!missingPiper.safetyReport.transcriptAutoInjectionAllowed);
}

void VoiceTest::audioFileSessionOrdersValidationMetadataDeterministically() {
    AudioFileSessionPolicy policy;
    policy.enabled = true;
    const auto result = buildAudioFileSessionResult(
        AudioFileDescriptor{QStringLiteral("/local/audio/sample.wav"), {}, 1024, true, true},
        policy);

    QCOMPARE(result.status, AudioFileSessionStatus::Fallback);
    QCOMPARE(result.executionAttempted, false);
    QVERIFY(!result.safetyReport.executionAttempted);
    QCOMPARE(result.validations.at(0).status, AudioFileValidationStatus::LocalOnly);
    QCOMPARE(result.validations.at(1).status, AudioFileValidationStatus::SandboxRequired);
    QCOMPARE(result.validations.at(2).status, AudioFileValidationStatus::SupportedExtension);
    QCOMPARE(result.traces.at(0).sequence, 1);
    QVERIFY(audioFileTraceSummaries(result.traces)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("sandbox-required")));
    QVERIFY(result.summary.summary.contains(QStringLiteral("execution attempted: no")));
}

void VoiceTest::audioFileSessionReportsSupportedAndUnsupportedExtensionMetadata() {
    AudioFileSessionPolicy policy;
    policy.enabled = true;

    const auto ready = buildAudioFileSessionResult(
        AudioFileDescriptor{QStringLiteral("/local/audio/sample.flac"), {}, 4096, false, true},
        policy);
    QCOMPARE(ready.status, AudioFileSessionStatus::ReadyMetadata);
    QCOMPARE(audioFileSessionStatusName(ready.status), QStringLiteral("ready-metadata"));
    QVERIFY(audioFileValidationSummaries(ready.validations)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("supported-extension")));
    QVERIFY(audioFileValidationSummaries(ready.validations)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("future-transcription-ready")));
    QCOMPARE(supportedAudioFileExtensionSummaries().size(), 4);
    QVERIFY(supportedAudioFileExtensionSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("ogg")));

    const auto unsupported = buildAudioFileSessionResult(
        AudioFileDescriptor{QStringLiteral("/local/audio/sample.aac"), {}, 4096, false, true},
        policy);
    QCOMPARE(unsupported.status, AudioFileSessionStatus::Refused);
    QVERIFY(audioFileSessionRefusalSummaries(unsupported)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("unsupported-extension")));
}

void VoiceTest::audioFileSessionRefusesUnsafePathWithoutExposingRawPath() {
    AudioFileSessionPolicy policy;
    policy.enabled = true;
    const auto result = buildAudioFileSessionResult(
        AudioFileDescriptor{
            QStringLiteral("https://example.invalid/audio.wav"), {}, 4096, false, true},
        policy);

    QCOMPARE(result.status, AudioFileSessionStatus::Refused);
    const auto refusals = audioFileSessionRefusalSummaries(result).join(QStringLiteral(" "));
    QVERIFY(refusals.contains(QStringLiteral("refused-path")));
    QVERIFY(refusals.contains(QStringLiteral("raw path value is not exposed")));
    QVERIFY(!refusals.contains(QStringLiteral("https://example.invalid")));
    QVERIFY(!audioFileValidationSummaries(result.validations)
                 .join(QStringLiteral(" "))
                 .contains(QStringLiteral("https://example.invalid")));
    QVERIFY(!result.executionAttempted);
}

void VoiceTest::audioFileSessionFallsBackForEmptyAndOversizedMetadataWithoutExecution() {
    AudioFileSessionPolicy policy;
    policy.enabled = true;

    const auto empty = buildAudioFileSessionResult(
        AudioFileDescriptor{QStringLiteral("/local/audio/empty.wav"), {}, 0, false, true}, policy);
    QCOMPARE(empty.status, AudioFileSessionStatus::Fallback);
    QVERIFY(audioFileSessionRefusalSummaries(empty)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("empty-file")));
    QVERIFY(!empty.safetyReport.fileLoadingAllowed);
    QVERIFY(!empty.safetyReport.transcriptionAllowed);
    QVERIFY(!empty.safetyReport.playbackAllowed);

    const auto oversized = buildAudioFileSessionResult(
        AudioFileDescriptor{
            QStringLiteral("/local/audio/large.mp3"), {}, 51LL * 1024LL * 1024LL, false, true},
        policy);
    QCOMPARE(oversized.status, AudioFileSessionStatus::Fallback);
    QVERIFY(audioFileSessionRefusalSummaries(oversized)
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("oversized-file")));
    QCOMPARE(audioFileSessionSafetyChecks(oversized.safetyReport).size(), 8);
    QVERIFY(!oversized.executionAttempted);
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
    QVERIFY(!report.executionAttempted);
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

void VoiceTest::voiceRuntimeConfigurationReportsMissingAndReadyMetadata() {
    const auto missingWhisper = whisperRuntimeDescriptorFromConfiguration({}, {});
    const auto missingPiper = piperRuntimeDescriptorFromConfiguration({}, {});
    const auto missingReport = voiceRuntimeReadinessReport(missingWhisper, missingPiper);

    QCOMPARE(missingWhisper.status, WhisperRuntimeStatus::MissingConfiguration);
    QCOMPARE(missingPiper.status, PiperRuntimeStatus::MissingConfiguration);
    QCOMPARE(missingReport.readiness, VoiceRuntimeReadiness::MissingConfiguration);
    QCOMPARE(missingReport.configuredCount, 0);
    QCOMPARE(missingReport.missingCount, 4);
    QCOMPARE(missingReport.refusedCount, 0);
    QVERIFY(!missingReport.executionAttempted);
    QVERIFY(voiceRuntimeReadinessSummaryText(missingReport)
                .contains(QStringLiteral("execution attempted: no")));

    const auto readyWhisper = whisperRuntimeDescriptorFromConfiguration(
        QStringLiteral("/local/bin/whisper"), QStringLiteral("/local/models/whisper.bin"));
    const auto readyPiper = piperRuntimeDescriptorFromConfiguration(
        QStringLiteral("/local/bin/piper"), QStringLiteral("/local/models/voice.onnx"));
    const auto readyReport = voiceRuntimeReadinessReport(readyWhisper, readyPiper);

    QCOMPARE(readyWhisper.readiness, WhisperRuntimeReadiness::ReadyMetadata);
    QCOMPARE(readyPiper.readiness, PiperRuntimeReadiness::ReadyMetadata);
    QCOMPARE(readyReport.readiness, VoiceRuntimeReadiness::ReadyMetadata);
    QCOMPARE(readyReport.configuredCount, 4);
    QCOMPARE(readyReport.missingCount, 0);
    QCOMPARE(readyReport.refusedCount, 0);
    QVERIFY(whisperRuntimeDescriptorSummary(readyWhisper).contains(QStringLiteral("local-only")));
    QVERIFY(piperRuntimeDescriptorSummary(readyPiper).contains(QStringLiteral("local-only")));
    QVERIFY(!voiceRuntimeSafetyReportForReadiness(readyReport).executionAllowed);
    QVERIFY(!voiceRuntimeSafetyReportForReadiness(readyReport).executionAttempted);
}

void VoiceTest::voiceRuntimeConfigurationRefusesUnsafeNonLocalPaths() {
    const auto whisper = whisperRuntimeDescriptorFromConfiguration(
        QStringLiteral("https://example.invalid/whisper"), QStringLiteral("/local/model.bin"));
    const auto piper = piperRuntimeDescriptorFromConfiguration(QStringLiteral("/local/bin/piper"),
                                                               QStringLiteral("file://voice.onnx"));
    const auto report = voiceRuntimeReadinessReport(whisper, piper);
    const auto checks = voiceRuntimeReadinessChecks(report).join(QStringLiteral(" "));

    QCOMPARE(whisper.status, WhisperRuntimeStatus::Refused);
    QCOMPARE(piper.status, PiperRuntimeStatus::Refused);
    QCOMPARE(report.readiness, VoiceRuntimeReadiness::Refused);
    QCOMPARE(report.refusedCount, 2);
    QVERIFY(checks.contains(QStringLiteral("unsafe/non-local")));
    QVERIFY(!checks.contains(QStringLiteral("https://example.invalid")));
    QVERIFY(!checks.contains(QStringLiteral("file://voice.onnx")));
    QVERIFY(!voiceRuntimeSafetyReportForReadiness(report).executionAttempted);
}

void VoiceTest::nullWhisperTranscriptionClientRefusesWithoutSideEffects() {
    NullWhisperTranscriptionClient client;
    const auto result = client.transcribe(WhisperTranscriptionRequest{},
                                          defaultDisabledWhisperTranscriptionConfig());

    QCOMPARE(client.status(), WhisperTranscriptionStatus::Disabled);
    QVERIFY(client.statusSummary().contains(QStringLiteral("never launches Whisper")));
    QCOMPARE(result.status, WhisperTranscriptionStatus::Disabled);
    QVERIFY(!result.success);
    QVERIFY(result.transcript.isEmpty());
    QVERIFY(!result.executionAttempted);
    QVERIFY(!result.safetyReport.executionAttempted);
    QVERIFY(safeWhisperTranscriptionResultSummary(result).contains(QStringLiteral("disabled")));
    QCOMPARE(result.traces.size(), 1);
}

void VoiceTest::localWhisperTranscriptionRefusesMissingUnsafeAndNonLocalInput() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    LocalWhisperTranscriptionClient client;
    auto config = configuredWhisperTranscriptionConfig({}, {});

    auto result = client.transcribe(WhisperTranscriptionRequest{}, config);
    QCOMPARE(result.status, WhisperTranscriptionStatus::Disabled);
    QVERIFY(!result.executionAttempted);

    config = configuredWhisperTranscriptionConfig(QStringLiteral("https://example.invalid/whisper"),
                                                  QStringLiteral("/local/model.bin"));
    result = client.transcribe(WhisperTranscriptionRequest{dir.filePath(QStringLiteral("a.wav"))},
                               config);
    QCOMPARE(result.status, WhisperTranscriptionStatus::UnsafePath);
    QVERIFY(!result.summary.contains(QStringLiteral("https://example.invalid")));

    const auto binaryPath = dir.filePath(QStringLiteral("whisper"));
    const auto modelPath = dir.filePath(QStringLiteral("model.bin"));
    const auto audioPath = dir.filePath(QStringLiteral("audio.wav"));
    QVERIFY(writeFile(binaryPath, "#!/bin/sh\nexit 0\n"));
    QVERIFY(QFile::setPermissions(binaryPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                                  QFileDevice::ExeOwner));
    QVERIFY(writeFile(modelPath, "model"));
    config = configuredWhisperTranscriptionConfig(binaryPath, modelPath);

    result = client.transcribe(WhisperTranscriptionRequest{audioPath}, config);
    QCOMPARE(result.status, WhisperTranscriptionStatus::MissingAudio);
    QVERIFY(!result.executionAttempted);

    QVERIFY(writeFile(audioPath, "audio"));
    result = client.transcribe(
        WhisperTranscriptionRequest{audioPath, {}, false, true, false, false, false, false, 100},
        config);
    QCOMPARE(result.status, WhisperTranscriptionStatus::UnsafePath);
    QVERIFY(!result.executionAttempted);

    result = client.transcribe(
        WhisperTranscriptionRequest{audioPath, {}, true, true, false, false, false, false, 100},
        config);
    QCOMPARE(result.status, WhisperTranscriptionStatus::Refused);
    QVERIFY(result.summary.contains(QStringLiteral("No subprocess")));
    QVERIFY(!result.executionAttempted);
    QVERIFY(result.transcript.isEmpty());

    const auto readiness =
        whisperTranscriptionReadiness(config, WhisperTranscriptionRequest{audioPath});
    QCOMPARE(readiness.status, WhisperTranscriptionStatus::ReadyMetadata);
    QVERIFY(!whisperTranscriptionSafetyReport(config.policy).executionAttempted);
    QCOMPARE(whisperTranscriptionStatusName(readiness.status), QStringLiteral("Ready Metadata"));
}

void VoiceTest::localWhisperTranscriptionReportsTimeoutFallbackWithoutExecution() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto binaryPath = dir.filePath(QStringLiteral("whisper"));
    const auto modelPath = dir.filePath(QStringLiteral("model.bin"));
    const auto audioPath = dir.filePath(QStringLiteral("audio.wav"));
    QVERIFY(writeFile(binaryPath, "#!/bin/sh\nexit 0\n"));
    QVERIFY(QFile::setPermissions(binaryPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                                  QFileDevice::ExeOwner));
    QVERIFY(writeFile(modelPath, "model"));
    QVERIFY(writeFile(audioPath, "audio"));

    LocalWhisperTranscriptionClient client;
    const auto result = client.transcribe(
        WhisperTranscriptionRequest{audioPath, {}, true, true, false, false, false, false, 0},
        configuredWhisperTranscriptionConfig(binaryPath, modelPath));

    QCOMPARE(result.status, WhisperTranscriptionStatus::Timeout);
    QVERIFY(result.fallback.summary.contains(QStringLiteral("timeout")));
    QVERIFY(result.traces.join(QStringLiteral(" ")).contains(QStringLiteral("Timeout")));
    QVERIFY(!result.executionAttempted);
    QVERIFY(!result.safetyReport.executionAttempted);
    QVERIFY(result.transcriptSummary.contains(QStringLiteral("No transcript")));
}

void VoiceTest::nullPiperSynthesisClientRefusesWithoutSideEffects() {
    NullPiperSynthesisClient client;
    const auto result = client.synthesize(PiperSynthesisRequest{QStringLiteral("hello")},
                                          defaultDisabledPiperSynthesisConfig());

    QCOMPARE(client.status(), PiperSynthesisStatus::Disabled);
    QVERIFY(client.statusSummary().contains(QStringLiteral("never launches Piper")));
    QCOMPARE(result.status, PiperSynthesisStatus::Disabled);
    QVERIFY(!result.success);
    QVERIFY(!result.executionAttempted);
    QVERIFY(!result.safetyReport.executionAttempted);
    QVERIFY(safePiperSynthesisResultSummary(result).contains(QStringLiteral("disabled")));
    QCOMPARE(result.traces.size(), 1);
}

void VoiceTest::localPiperSynthesisRefusesMissingUnsafeAndNonLocalInput() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    LocalPiperSynthesisClient client;
    auto config = configuredPiperSynthesisConfig({}, {});

    auto result = client.synthesize(PiperSynthesisRequest{}, config);
    QCOMPARE(result.status, PiperSynthesisStatus::Disabled);
    QVERIFY(!result.executionAttempted);

    config = configuredPiperSynthesisConfig(QStringLiteral("https://example.invalid/piper"),
                                            QStringLiteral("/local/voice.onnx"));
    result = client.synthesize(PiperSynthesisRequest{QStringLiteral("hello")}, config);
    QCOMPARE(result.status, PiperSynthesisStatus::UnsafePath);
    QVERIFY(!result.summary.contains(QStringLiteral("https://example.invalid")));

    const auto binaryPath = dir.filePath(QStringLiteral("piper"));
    const auto modelPath = dir.filePath(QStringLiteral("voice.onnx"));
    QVERIFY(writeFile(binaryPath, "#!/bin/sh\nexit 0\n"));
    QVERIFY(QFile::setPermissions(binaryPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                                  QFileDevice::ExeOwner));
    config = configuredPiperSynthesisConfig(binaryPath, modelPath);
    result = client.synthesize(PiperSynthesisRequest{QStringLiteral("hello")}, config);
    QCOMPARE(result.status, PiperSynthesisStatus::MissingModel);
    QVERIFY(!result.executionAttempted);

    QVERIFY(writeFile(modelPath, "model"));
    config = configuredPiperSynthesisConfig(binaryPath, modelPath);
    result = client.synthesize(
        PiperSynthesisRequest{
            QStringLiteral("hello"), {}, {}, false, true, false, false, false, false, false, 100},
        config);
    QCOMPARE(result.status, PiperSynthesisStatus::UnsafePath);
    QVERIFY(!result.executionAttempted);

    result = client.synthesize(
        PiperSynthesisRequest{
            QStringLiteral("hello"), {}, {}, true, true, false, false, false, false, false, 100},
        config);
    QCOMPARE(result.status, PiperSynthesisStatus::Refused);
    QVERIFY(result.summary.contains(QStringLiteral("No subprocess")));
    QVERIFY(!result.executionAttempted);

    const auto readiness =
        piperSynthesisReadiness(config, PiperSynthesisRequest{QStringLiteral("hello")});
    QCOMPARE(readiness.status, PiperSynthesisStatus::ReadyMetadata);
    QVERIFY(!piperSynthesisSafetyReport(config.policy).executionAttempted);
    QCOMPARE(piperSynthesisStatusName(readiness.status), QStringLiteral("Ready Metadata"));
}

void VoiceTest::localPiperSynthesisReportsTimeoutFallbackWithoutExecution() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto binaryPath = dir.filePath(QStringLiteral("piper"));
    const auto modelPath = dir.filePath(QStringLiteral("voice.onnx"));
    QVERIFY(writeFile(binaryPath, "#!/bin/sh\nexit 0\n"));
    QVERIFY(QFile::setPermissions(binaryPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                                  QFileDevice::ExeOwner));
    QVERIFY(writeFile(modelPath, "model"));

    LocalPiperSynthesisClient client;
    const auto result = client.synthesize(
        PiperSynthesisRequest{
            QStringLiteral("hello"), {}, {}, true, true, false, false, false, false, false, 0},
        configuredPiperSynthesisConfig(binaryPath, modelPath));

    QCOMPARE(result.status, PiperSynthesisStatus::Timeout);
    QVERIFY(result.fallback.summary.contains(QStringLiteral("timeout")));
    QVERIFY(result.traces.join(QStringLiteral(" ")).contains(QStringLiteral("Timeout")));
    QVERIFY(!result.executionAttempted);
    QVERIFY(!result.safetyReport.executionAttempted);
    QVERIFY(result.audioSummary.contains(QStringLiteral("No audio")));
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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    auto config = sentinel::core::defaultDisabledPiperTtsConfig();
    config.enabled = true;
    PiperTextToSpeechProvider missingAll{config, std::make_unique<NullPiperTtsClient>()};

    QCOMPARE(missingAll.status(), PiperTtsStatus::NotConfigured);
    QCOMPARE(piperTtsStatusName(missingAll.status()), QStringLiteral("Not Configured"));
    QVERIFY(missingAll.piperStatusSummary().contains(QStringLiteral("not configured")));

    config.binary.expectedPath = QDir(dir.path()).filePath(QStringLiteral("missing-piper"));
    PiperTextToSpeechProvider missingBinary{config, std::make_unique<NullPiperTtsClient>()};
    QCOMPARE(missingBinary.status(), PiperTtsStatus::MissingBinary);
    auto result = missingBinary.synthesizePiper(PiperTtsRequest{QStringLiteral("hello")});
    QCOMPARE(result.status, PiperTtsStatus::MissingBinary);
    QVERIFY(!result.success);
    QVERIFY(result.audioPath.isEmpty());

    const auto binaryPath = QDir(dir.path()).filePath(QStringLiteral("piper"));
    QVERIFY(writeFile(binaryPath, "#!/bin/sh\nexit 0\n"));
    QVERIFY(QFile::setPermissions(binaryPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                                  QFileDevice::ExeOwner));
    config.binary.status = VoiceBinaryStatus::PresentMetadata;
    config.binary.expectedPath = binaryPath;
    config.voiceModel.expectedPath = QDir(dir.path()).filePath(QStringLiteral("missing.onnx"));
    PiperTextToSpeechProvider missingModel{config, std::make_unique<NullPiperTtsClient>()};
    QCOMPARE(missingModel.status(), PiperTtsStatus::MissingModel);
    result = missingModel.synthesizePiper(PiperTtsRequest{QStringLiteral("hello")});
    QCOMPARE(result.status, PiperTtsStatus::MissingModel);
    QVERIFY(!result.success);
    QVERIFY(result.audioPath.isEmpty());
}

void VoiceTest::piperTextToSpeechProviderReportsSafetyBlockedMetadata() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    PiperTtsConfig config = configuredPiperConfig(dir);
    config.safetyReport.executionAllowed = false;

    PiperTextToSpeechProvider provider{config, std::make_unique<NullPiperTtsClient>()};

    QCOMPARE(provider.status(), PiperTtsStatus::ReadyMetadata);
    QCOMPARE(provider.descriptor().id, QStringLiteral("piper-tts"));
    QCOMPARE(provider.descriptor().status, VoiceProviderStatus::MetadataOnly);
    QVERIFY(provider.piperStatusSummary().contains(QStringLiteral("metadata")));
    QVERIFY(provider.readinessChecks()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper binary")));

    const auto response = provider.synthesize(VoiceRequest{QStringLiteral("hello")});
    QCOMPARE(response.status, VoiceProviderStatus::Refused);
    QVERIFY(!response.available);
    QVERIFY(response.summary.contains(QStringLiteral("refused")));
}

void VoiceTest::piperFileOutputRefusesPolicyBlockedAndInvalidRequests() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    auto disabled = configuredPiperConfig(dir);
    disabled.enabled = false;
    PiperTextToSpeechProvider disabledProvider{
        disabled, std::make_unique<FakePiperTtsClient>(FakePiperTtsClient::Mode::Success)};
    auto result = disabledProvider.synthesizePiper(
        PiperTtsRequest{QStringLiteral("hello"), {}, {}, true, true, false, 100});
    QCOMPARE(result.status, PiperTtsStatus::Disabled);
    QVERIFY(!result.success);

    auto missingBinary = configuredPiperConfig(dir);
    missingBinary.binary.expectedPath = QDir(dir.path()).filePath(QStringLiteral("missing-piper"));
    PiperTextToSpeechProvider missingBinaryProvider{
        missingBinary, std::make_unique<FakePiperTtsClient>(FakePiperTtsClient::Mode::Success)};
    result = missingBinaryProvider.synthesizePiper(
        PiperTtsRequest{QStringLiteral("hello"), {}, {}, true, true, false, 100});
    QCOMPARE(result.status, PiperTtsStatus::MissingBinary);

    auto missingModel = configuredPiperConfig(dir);
    missingModel.voiceModel.expectedPath =
        QDir(dir.path()).filePath(QStringLiteral("missing.onnx"));
    PiperTextToSpeechProvider missingModelProvider{
        missingModel, std::make_unique<FakePiperTtsClient>(FakePiperTtsClient::Mode::Success)};
    result = missingModelProvider.synthesizePiper(
        PiperTtsRequest{QStringLiteral("hello"), {}, {}, true, true, false, 100});
    QCOMPARE(result.status, PiperTtsStatus::MissingModel);

    auto blocked = configuredPiperConfig(dir);
    blocked.safetyReport.executionAllowed = false;
    PiperTextToSpeechProvider blockedProvider{
        blocked, std::make_unique<FakePiperTtsClient>(FakePiperTtsClient::Mode::Success)};
    result = blockedProvider.synthesizePiper(
        PiperTtsRequest{QStringLiteral("hello"), {}, {}, true, true, false, 100});
    QCOMPARE(result.status, PiperTtsStatus::Refused);

    auto configured = configuredPiperConfig(dir);
    PiperTextToSpeechProvider provider{
        configured, std::make_unique<FakePiperTtsClient>(FakePiperTtsClient::Mode::Success)};
    result = provider.synthesizePiper(
        PiperTtsRequest{QStringLiteral("hello"), {}, {}, false, true, false, 100});
    QCOMPARE(result.status, PiperTtsStatus::Refused);
    result = provider.synthesizePiper(
        PiperTtsRequest{QStringLiteral("hello"), {}, {}, true, false, false, 100});
    QCOMPARE(result.status, PiperTtsStatus::Refused);
    result = provider.synthesizePiper(
        PiperTtsRequest{QStringLiteral("hello"),
                        {},
                        QDir(dir.path()).filePath(QStringLiteral("outside.wav")),
                        true,
                        true,
                        false,
                        100});
    QCOMPARE(result.status, PiperTtsStatus::Refused);
    QVERIFY(!result.outputPathSummary.contains(QStringLiteral("outside.wav")));
}

void VoiceTest::piperLegacyFileOutputRefusesWithoutSideEffects() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    auto config = configuredPiperConfig(dir);
    PiperTextToSpeechProvider provider{
        config, std::make_unique<FakePiperTtsClient>(FakePiperTtsClient::Mode::Success)};

    QCOMPARE(provider.status(), PiperTtsStatus::ReadyMetadata);
    QCOMPARE(provider.fileOutputStatus(), QStringLiteral("Ready Metadata"));
    QVERIFY(provider.fileOutputSummary().contains(QStringLiteral("Controlled Piper TTS output")));

    const auto result = provider.synthesizePiper(
        PiperTtsRequest{QStringLiteral("hello"), {}, {}, false, true, false, 100});

    QCOMPARE(result.status, PiperTtsStatus::Refused);
    QVERIFY(!result.success);
    QVERIFY(result.audioPath.isEmpty());
    QVERIFY(!QFile::exists(
        QDir(config.controlledOutputDirectory).filePath(QStringLiteral("sentinel-piper-tts.wav"))));
    QCOMPARE(result.timeoutMs, 100);
    QCOMPARE(result.exitCode, -1);
    QVERIFY(result.summary.contains(QStringLiteral("refused before execution")));
}

QTEST_MAIN(VoiceTest)

#include "test_voice.moc"
