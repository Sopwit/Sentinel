// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/voice/Voice.h"
#include "sentinel/core/voice/WhisperTranscription.h"
#include "sentinel/core/voice/PiperTts.h"
#include "sentinel/core/model/HardwareCapabilityService.h"
#include "sentinel/core/runtime/IFileSystemService.h"
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSource>
#include <QByteArray>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QQueue>
#include <functional>
#include <atomic>
#include <memory>
#include <optional>

namespace sentinel::core {

enum class AudioFailure {
    None, MicrophonePermissionDenied, AudioDeviceUnavailable, CaptureFailure,
    ModelUnavailable, RuntimeUnavailable, UnsupportedFormat, TranscriptionFailure,
    SynthesisFailure, PlaybackFailure, Cancelled, Timeout, ProviderFailure,
    PermissionDenied, NoSpeechDetected
};
enum class AudioRuntimeReadiness {
    Installed, ModelAvailable, ModelUnavailable, RuntimeAvailable, DependencyMissing,
    UnsupportedPlatform, Ready
};
enum class SpeechHardwareSuitability { Unknown, CpuFriendly, ResourceConstrained };
enum class VoiceInteractionMode { Dictation, VoiceChat, VoiceAgent, ReadAloud, AudioTranscription };
enum class VoiceInteractionState {
    Idle, Listening, Transcribing, Thinking, WaitingForApproval, ToolActivity,
    Synthesizing, Speaking,
    Completed, Failed, Cancelled
};
QString audioFailureName(AudioFailure failure);
QString audioRuntimeReadinessName(AudioRuntimeReadiness readiness);
QString voiceInteractionStateName(VoiceInteractionState state);
struct AudioPrivacyState {
    bool localOnly{true};
    bool cloudProviderActive{false};
    bool processingRawAudio{false};
    bool retainRawRecordings{false};
};
struct SpeechProviderInfo {
    QString id;
    QString runtimeId;
    QString modelId;
    QStringList languages;
    QStringList voices;
    AudioRuntimeReadiness readiness{AudioRuntimeReadiness::DependencyMissing};
    bool installed{false};
    bool modelAvailable{false};
    bool runtimeAvailable{false};
    bool externalDependencyMissing{true};
    bool unsupportedPlatform{false};
    bool local{true};
    bool streaming{false};
    bool cancellable{false};
    bool cpuFriendly{false};
    SpeechHardwareSuitability suitability{SpeechHardwareSuitability::Unknown};
};
struct SpeechTranscript {
    struct Segment {
        QString text;
        qint64 startMs{-1};
        qint64 endMs{-1};
        std::optional<double> confidence;
    };
    QString providerId;
    QString modelId;
    QString language;
    QString detectedLanguage;
    QString partialText;
    QString finalText;
    QList<Segment> segments; // Empty until a runtime supplies authoritative timing.
    AudioFailure failure{AudioFailure::None};
    QString detail;
};
struct SpeechAudio {
    QString providerId;
    QString runtimeId;
    QString modelId;
    QString artifactId;
    QString voiceId;
    QString language;
    QString filePath;
    qint64 durationMs{-1};
    int sampleRate{0};
    int channels{0};
    AudioFailure failure{AudioFailure::None};
    QString detail;
};
struct SpeechSynthesisRequest {
    QString text;
    QString voiceId;
    QString language;
    double speed{1.0};
};
struct AudioDeviceDescriptor {
    QString id;
    QString name;
    bool isDefault{false};
};

// Adapters own runtime/model execution. No UI or permission decision occurs here.
class ISpeechToTextRuntime {
public:
    virtual ~ISpeechToTextRuntime() = default;
    virtual SpeechProviderInfo info() const = 0;
    virtual SpeechTranscript transcribeFile(const QString& authorizedPath,
                                            const QString& language,
                                            std::shared_ptr<std::atomic_bool> cancellation) = 0;
};
class ITextToSpeechRuntime {
public:
    virtual ~ITextToSpeechRuntime() = default;
    virtual SpeechProviderInfo info() const = 0;
    virtual SpeechAudio synthesize(const SpeechSynthesisRequest& request,
                                   std::shared_ptr<std::atomic_bool> cancellation) = 0;
};

class WhisperSttRuntime final : public ISpeechToTextRuntime {
public:
    explicit WhisperSttRuntime(WhisperTranscriptionConfig config) : config_(std::move(config)) {}
    SpeechProviderInfo info() const override;
    SpeechTranscript transcribeFile(const QString& authorizedPath, const QString& language,
                                    std::shared_ptr<std::atomic_bool> cancellation) override;
private:
    WhisperTranscriptionConfig config_;
};
class PiperTtsRuntime final : public ITextToSpeechRuntime {
public:
    explicit PiperTtsRuntime(PiperTtsConfig config) : config_(std::move(config)) {}
    SpeechProviderInfo info() const override;
    SpeechAudio synthesize(const SpeechSynthesisRequest& request,
                           std::shared_ptr<std::atomic_bool> cancellation) override;
private:
    PiperTtsConfig config_;
};
class KokoroTtsRuntime final : public ITextToSpeechRuntime {
public:
    KokoroTtsRuntime(QString modelPath, QString voice)
        : modelPath_(std::move(modelPath)), voice_(std::move(voice)) {}
    SpeechProviderInfo info() const override;
    SpeechAudio synthesize(const SpeechSynthesisRequest& request,
                           std::shared_ptr<std::atomic_bool> cancellation) override;
private:
    QString modelPath_;
    QString voice_;
};

// Capture is in memory. The only temporary WAV is scoped to STT execution and deleted afterward.
class AudioDeviceService final : public QObject {
    Q_OBJECT
public:
    explicit AudioDeviceService(QObject* parent = nullptr);
    QStringList inputDeviceIds() const;
    QStringList outputDeviceIds() const;
    QList<AudioDeviceDescriptor> inputDevices() const;
    QList<AudioDeviceDescriptor> outputDevices() const;
    bool selectInput(const QString& id);
    bool selectOutput(const QString& id);
    QString selectedInputId() const { return inputId_; }
    QString selectedOutputId() const { return outputId_; }
    bool startCapture();
    void requestMicrophonePermission();
    QByteArray stopCapture(); // normalized mono 16 kHz signed 16-bit PCM
    void cancelCapture();
    bool isCapturing() const { return source_ != nullptr; }
    AudioFailure lastFailure() const { return failure_; }
    qreal inputLevel() const { return inputLevel_; }
    bool speechDetected() const { return hadSpeech_; }
    void setVadEnabled(bool enabled) { vadEnabled_ = enabled; }
    bool vadEnabled() const { return vadEnabled_; }
    void setSilenceLimitMs(int ms) { silenceLimitMs_ = qBound(250, ms, 3000); }

signals:
    void inputDevicesChanged();
    void outputDevicesChanged();
    void inputLevelChanged(qreal level);
    void speechStarted();
    void speechEnded();
    void speechSegmentReady(const QByteArray& pcm);
    void captureFailed(AudioFailure failure);
    void microphonePermissionChanged(Qt::PermissionStatus status);

private:
    void readCapture();
    QByteArray normalizedPcm(qsizetype fromByte = 0) const;
    QMediaDevices devices_;
    QString inputId_;
    QString outputId_;
    std::unique_ptr<QAudioSource> source_;
    QIODevice* stream_{nullptr};
    QAudioFormat captureFormat_;
    QByteArray captured_;
    qreal inputLevel_{0};
    bool speechDetected_{false};
    bool hadSpeech_{false};
    qsizetype segmentStartByte_{0};
    bool vadEnabled_{true};
    int silenceMs_{0};
    int voicedMs_{0};
    int silenceLimitMs_{800};
    AudioFailure failure_{AudioFailure::None};
};

class AudioPlaybackService final : public QObject {
    Q_OBJECT
public:
    explicit AudioPlaybackService(AudioDeviceService* devices, QObject* parent = nullptr);
    bool playFile(const QString& path, bool replace = true);
    void pause();
    void stop();
    bool isPlaying() const;
    bool isPaused() const;
    qint64 positionMs() const;
    AudioFailure lastFailure() const { return failure_; }

signals:
    void playbackChanged();
    void playbackProgress(qint64 positionMs, qint64 durationMs);
    void playbackFailed(AudioFailure failure);

private:
    QMediaPlayer player_;
    QAudioOutput output_;
    AudioDeviceService* devices_;
    AudioFailure failure_{AudioFailure::None};
};

// Chat and Agent bridges accept only final transcripts. The Agent bridge must enter AgentRuntime.
class VoiceSessionService final : public QObject {
    Q_OBJECT
public:
    explicit VoiceSessionService(QObject* parent = nullptr);
    ~VoiceSessionService() override;
    AudioDeviceService* devices() { return &devices_; }
    AudioPlaybackService* playback() { return &playback_; }
    void setSttRuntime(std::shared_ptr<ISpeechToTextRuntime> runtime);
    void setTtsRuntime(std::shared_ptr<ITextToSpeechRuntime> runtime);
    void setHardwareFacts(HardwareFacts facts) { hardwareFacts_ = std::move(facts); emit providersChanged(); }
    void setChatBridge(std::function<void(const QString&, std::function<void(QString)>)> bridge);
    void setAgentBridge(std::function<void(const QString&, std::function<void(QString, bool)>)> bridge);
    void setBridgeCancellation(std::function<void()> cancelChat,
                               std::function<void()> cancelAgent);
    // The product boundary must grant Read and return its identity-bound path.
    void setFileAuthorization(std::function<std::optional<AuthorizedPath>(const QString&)> authorization,
                              QString workingDirectory);
    void setFileSystemService(const IFileSystemService* service) { fileSystemService_ = service; }
    bool startPushToTalk(VoiceInteractionMode mode, bool speakAnswer = false);
    void stopPushToTalk();
    // GUI-owned, permission-checked mono 16 kHz signed 16-bit capture. No file ingress.
    bool transcribeCapturedPcm(const QByteArray& pcm, bool speechDetected);
    void transcribeAudioFile(const QString& path, const QString& language = {});
    void readAloud(const QString& text, const QString& voice = {});
    void updateAgentActivity(VoiceInteractionState state);
    void cancel();
    VoiceInteractionState state() const { return state_; }
    VoiceInteractionMode mode() const { return mode_; }
    SpeechTranscript transcript() const { return transcript_; }
    SpeechProviderInfo sttInfo() const;
    quint64 sttRevision() const { return sttRevision_; }
    SpeechProviderInfo ttsInfo() const;
    AudioPrivacyState privacy() const { return privacy_; }
    AudioFailure failure() const { return failure_; }

signals:
    void providersChanged();
    void stateChanged(VoiceInteractionState state);
    void privacyChanged(AudioPrivacyState state);
    void partialTranscriptChanged(const QString& text);
    void finalTranscriptChanged(const QString& text);
    void answerReady(const QString& text);
    void approvalRequired();
    void failed(AudioFailure failure, const QString& detail);

private:
    void setState(VoiceInteractionState state);
    void processFile(const AuthorizedPath& path, const QString& language);
    void processNextSegment(quint64 generation);
    void deliverTranscript(quint64 generation);
    void speak(const QString& text, const QString& voice, quint64 generation);
    void fail(AudioFailure failure, const QString& detail);
    AudioDeviceService devices_;
    AudioPlaybackService playback_;
    std::shared_ptr<ISpeechToTextRuntime> stt_;
    std::shared_ptr<ITextToSpeechRuntime> tts_;
    HardwareFacts hardwareFacts_;
    std::function<void(const QString&, std::function<void(QString)>)> chatBridge_;
    std::function<void(const QString&, std::function<void(QString, bool)>)> agentBridge_;
    std::function<void()> cancelChatBridge_;
    std::function<void()> cancelAgentBridge_;
    std::function<std::optional<AuthorizedPath>(const QString&)> fileAuthorization_;
    QString fileAuthorizationCwd_;
    const IFileSystemService* fileSystemService_{nullptr};
    VoiceInteractionState state_{VoiceInteractionState::Idle};
    VoiceInteractionMode mode_{VoiceInteractionMode::Dictation};
    SpeechTranscript transcript_;
    QQueue<QByteArray> pendingSegments_;
    QStringList segmentTexts_;
    bool segmentActive_{false};
    bool finalSegmentsRequested_{false};
    AudioPrivacyState privacy_;
    AudioFailure failure_{AudioFailure::None};
    quint64 generation_{0};
    quint64 sttRevision_{0};
    bool speakAnswer_{false};
    QString generatedAudioPath_;
    bool bridgeActive_{false};
    std::shared_ptr<std::atomic_bool> operationCancellation_;
};

} // namespace sentinel::core
