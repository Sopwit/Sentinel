// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/voice/UnifiedAudioService.h"
#include <QFile>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QtTest>

using namespace sentinel::core;

// Deliberately returns late success, even after cancellation. The session owns cleanup.
class GatedTts final : public ITextToSpeechRuntime {
public:
    explicit GatedTts(QString path) : path(std::move(path)) {}
    SpeechProviderInfo info() const override {
        SpeechProviderInfo result;
        result.readiness = AudioRuntimeReadiness::Ready;
        return result;
    }
    SpeechAudio synthesize(const SpeechSynthesisRequest&,
                          std::shared_ptr<std::atomic_bool> flag) override {
        cancellation = flag;
        entered.release();
        if (!release.tryAcquire(1, 5000)) return {.failure = AudioFailure::Timeout};
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return {.failure = AudioFailure::SynthesisFailure};
        file.write("late audio artifact");
        file.close();
        return {.filePath = path};
    }
    QString path;
    QSemaphore entered, release;
    std::shared_ptr<std::atomic_bool> cancellation;
};

class GatedStt final : public ISpeechToTextRuntime {
public:
    SpeechProviderInfo info() const override {
        SpeechProviderInfo info; info.readiness = AudioRuntimeReadiness::Ready; return info;
    }
    SpeechTranscript transcribeFile(const QString&, const QString&,
                                    std::shared_ptr<std::atomic_bool> flag) override {
        cancellation = flag;
        entered.release();
        if (!release.tryAcquire(1, 5000)) return {.failure = AudioFailure::Timeout};
        return result;
    }
    SpeechTranscript result{.finalText = "late transcript"};
    QSemaphore entered, release;
    std::shared_ptr<std::atomic_bool> cancellation;
};

class UnifiedAudioLifecycleTest final : public QObject {
    Q_OBJECT
private slots:
    void capturedPcmRejectsUnavailableMalformedAndSilentInput() {
        VoiceSessionService session;
        QVERIFY(!session.transcribeCapturedPcm(QByteArray(2, '\0'), true));
        QCOMPARE(session.failure(), AudioFailure::RuntimeUnavailable);
        auto runtime = std::make_shared<GatedStt>();
        session.setSttRuntime(runtime);
        QVERIFY(!session.transcribeCapturedPcm(QByteArray(3, '\0'), true));
        QCOMPARE(session.failure(), AudioFailure::UnsupportedFormat);
        QVERIFY(!session.transcribeCapturedPcm(QByteArray(16000 * 2 * 60 + 2, '\0'), true));
        QCOMPARE(session.failure(), AudioFailure::UnsupportedFormat);
        QVERIFY(!session.transcribeCapturedPcm(QByteArray(2, '\0'), false));
        QCOMPARE(session.failure(), AudioFailure::NoSpeechDetected);
        QCOMPARE(runtime->entered.available(), 0);
        QVERIFY(!session.devices()->isCapturing());
        QVERIFY(!session.privacy().processingRawAudio);
    }
    void capturedPcmDictationDoesNotExecuteChatOrAgent() {
        VoiceSessionService session;
        auto runtime = std::make_shared<GatedStt>();
        runtime->result.finalText = "Reviewed dictation";
        session.setSttRuntime(runtime);
        int bridges = 0;
        session.setChatBridge([&](const QString&, auto) { ++bridges; });
        session.setAgentBridge([&](const QString&, auto) { ++bridges; });
        QVERIFY(session.transcribeCapturedPcm(QByteArray(32000, '\0'), true));
        QVERIFY(runtime->entered.tryAcquire(1, 2000));
        QVERIFY(!session.devices()->isCapturing());
        QVERIFY(session.privacy().processingRawAudio);
        runtime->release.release();
        QTRY_COMPARE(session.state(), VoiceInteractionState::Completed);
        QCOMPARE(session.transcript().finalText, QString("Reviewed dictation"));
        QCOMPARE(bridges, 0);
        QVERIFY(!session.privacy().retainRawRecordings);
        QVERIFY(!session.privacy().processingRawAudio);
    }
    void capturedPcmCancellationDiscardsLateTranscript() {
        VoiceSessionService session;
        auto runtime = std::make_shared<GatedStt>();
        session.setSttRuntime(runtime);
        QSignalSpy transcripts(&session, &VoiceSessionService::finalTranscriptChanged);
        QVERIFY(session.transcribeCapturedPcm(QByteArray(32000, '\0'), true));
        QVERIFY(runtime->entered.tryAcquire(1, 2000));
        session.cancel();
        QVERIFY(runtime->cancellation->load());
        runtime->release.release();
        QTest::qWait(100);
        QCOMPARE(session.state(), VoiceInteractionState::Cancelled);
        QCOMPARE(transcripts.size(), 0);
    }
    void unavailableAndEmptyRequests() {
        VoiceSessionService session;
        QVERIFY(!session.startPushToTalk(VoiceInteractionMode::VoiceChat));
        QCOMPARE(session.failure(), AudioFailure::RuntimeUnavailable);
        QVERIFY(!session.devices()->isCapturing());
        session.readAloud(" ");
        QCOMPARE(session.failure(), AudioFailure::SynthesisFailure);
        session.transcribeAudioFile("/unapproved.wav");
        QCOMPARE(session.failure(), AudioFailure::PermissionDenied);
        QVERIFY(!session.privacy().processingRawAudio);
        session.cancel();
        QCOMPARE(session.state(), VoiceInteractionState::Cancelled);
        session.cancel();
        QCOMPARE(session.state(), VoiceInteractionState::Cancelled);
    }
    void deviceSelectionAndIdleStop() {
        AudioDeviceService device;
        QVERIFY(!device.selectInput("sentinel-nonexistent-device"));
        QVERIFY(!device.selectOutput("sentinel-nonexistent-device"));
        QVERIFY(device.stopCapture().isEmpty());
        device.cancelCapture();
        device.cancelCapture();
        QVERIFY(!device.isCapturing());
        QCOMPARE(device.inputLevel(), 0.0);
    }
    void missingRuntimeAndModelFailures() {
        QTemporaryDir dir;
        auto whisperConfig = configuredWhisperTranscriptionConfig(
            dir.filePath("missing-whisper"), dir.filePath("missing-model"), true);
        WhisperSttRuntime whisper(whisperConfig);
        QVERIFY(!whisper.info().runtimeAvailable);
        QVERIFY(!whisper.info().modelAvailable);
        QVERIFY(whisper.transcribeFile(dir.filePath("empty.wav"), {}, {}).failure != AudioFailure::None);
        PiperTtsRuntime piper(defaultDisabledPiperTtsConfig());
        QVERIFY(piper.info().readiness != AudioRuntimeReadiness::Ready);
        QVERIFY(piper.synthesize({"test"}, {}).failure != AudioFailure::None);
        KokoroTtsRuntime kokoro(dir.filePath("missing.onnx"), "af_heart");
        QVERIFY(kokoro.info().readiness != AudioRuntimeReadiness::Ready);
        QVERIFY(kokoro.synthesize({"test", "af_heart", "en-us", 1.0}, {}).failure != AudioFailure::None);
    }
    void authorizedEmptyFileIsRejectedBeforeTranscription() {
        QTemporaryDir dir;
        const auto path = dir.filePath("empty.wav");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        QtFileSystemService filesystem;
        const auto cwd = QFileInfo(dir.path()).canonicalFilePath();
        VoiceSessionService session;
        // GatedTts is not used: this ready STT must never execute for empty data.
        class RejectStt final : public ISpeechToTextRuntime {
        public:
            SpeechProviderInfo info() const override {
                SpeechProviderInfo info; info.readiness = AudioRuntimeReadiness::Ready; return info;
            }
            SpeechTranscript transcribeFile(const QString&, const QString&,
                                            std::shared_ptr<std::atomic_bool>) override {
                called.store(true); return {.finalText = "unexpected success"};
            }
            std::atomic_bool called{false};
        };
        auto runtime = std::make_shared<RejectStt>();
        session.setSttRuntime(runtime);
        session.setFileSystemService(&filesystem);
        session.setFileAuthorization([&](const QString& raw) {
            return filesystem.resolve(raw, cwd, FileSystemAccess::Read).value;
        }, cwd);
        session.transcribeAudioFile(path);
        QTRY_COMPARE(session.state(), VoiceInteractionState::Failed);
        QCOMPARE(session.failure(), AudioFailure::UnsupportedFormat);
        QVERIFY(!runtime->called.load());
        QVERIFY(!session.privacy().processingRawAudio);
    }
    void cancellationSuppressesLateSynthesisAndRemovesArtifact() {
        QTemporaryDir dir;
        auto runtime = std::make_shared<GatedTts>(dir.filePath("late.wav"));
        VoiceSessionService session;
        session.setTtsRuntime(runtime);
        session.readAloud("Sentinel ses testi başarılı.");
        QVERIFY(runtime->entered.tryAcquire(1, 5000));
        session.cancel();
        QVERIFY(runtime->cancellation->load());
        runtime->release.release();
        QVERIFY(QThreadPool::globalInstance()->waitForDone(5000));
        QTRY_VERIFY(!QFile::exists(runtime->path));
        QCOMPARE(session.state(), VoiceInteractionState::Cancelled);
        QVERIFY(!session.playback()->isPlaying());
    }
    void transcriptionCancellationAndEmptySuccess_data() {
        QTest::addColumn<bool>("cancelled");
        QTest::newRow("cancelled-late-success") << true;
        QTest::newRow("empty-success-is-failure") << false;
    }
    void transcriptionCancellationAndEmptySuccess() {
        QFETCH(bool, cancelled);
        QTemporaryDir dir;
        const auto cwd = QFileInfo(dir.path()).canonicalFilePath();
        const auto path = cwd + "/fixture.wav";
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write("fixture"); file.close();
        QtFileSystemService filesystem;
        auto runtime = std::make_shared<GatedStt>();
        if (!cancelled) runtime->result.finalText = " ";
        VoiceSessionService session;
        session.setSttRuntime(runtime);
        session.setFileSystemService(&filesystem);
        session.setFileAuthorization([&](const QString& raw) {
            return filesystem.resolve(raw, cwd, FileSystemAccess::Read).value;
        }, cwd);
        QSignalSpy transcripts(&session, &VoiceSessionService::finalTranscriptChanged);
        session.transcribeAudioFile(path);
        QVERIFY(runtime->entered.tryAcquire(1, 5000));
        if (cancelled) session.cancel();
        runtime->release.release();
        QVERIFY(QThreadPool::globalInstance()->waitForDone(5000));
        if (cancelled) {
            QCoreApplication::processEvents();
            QVERIFY(runtime->cancellation->load());
            QCOMPARE(session.state(), VoiceInteractionState::Cancelled);
        } else {
            QTRY_COMPARE(session.state(), VoiceInteractionState::Failed);
            QCOMPARE(session.failure(), AudioFailure::TranscriptionFailure);
        }
        QCOMPARE(transcripts.size(), 0);
        QVERIFY(!session.privacy().processingRawAudio);
    }
    void failureCancelsOutstandingSynthesis() {
        QTemporaryDir dir;
        auto runtime = std::make_shared<GatedTts>(dir.filePath("late.wav"));
        VoiceSessionService session;
        session.setTtsRuntime(runtime);
        session.readAloud("test");
        QVERIFY(runtime->entered.tryAcquire(1, 5000));
        session.playback()->playbackFailed(AudioFailure::AudioDeviceUnavailable);
        const bool cancelled = runtime->cancellation->load();
        runtime->release.release();
        QVERIFY(QThreadPool::globalInstance()->waitForDone(5000));
        QVERIFY(cancelled);
        QTRY_VERIFY(!QFile::exists(runtime->path));
        QCOMPARE(session.state(), VoiceInteractionState::Failed);
        QCOMPARE(session.failure(), AudioFailure::AudioDeviceUnavailable);
    }
    void destructionCancelsAndCleansLateSynthesis() {
        QTemporaryDir dir;
        auto runtime = std::make_shared<GatedTts>(dir.filePath("late.wav"));
        auto session = std::make_unique<VoiceSessionService>();
        session->setTtsRuntime(runtime);
        session->readAloud("test");
        QVERIFY(runtime->entered.tryAcquire(1, 5000));
        session.reset();
        const bool cancelled = runtime->cancellation->load();
        runtime->release.release();
        QVERIFY(QThreadPool::globalInstance()->waitForDone(5000));
        QVERIFY(cancelled);
        QTRY_VERIFY(!QFile::exists(runtime->path));
    }
};
QTEST_MAIN(UnifiedAudioLifecycleTest)
#include "test_unified_audio_lifecycle.moc"
