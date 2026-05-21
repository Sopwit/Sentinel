#pragma once

#include "sentinel/core/Voice.h"

#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class WhisperTranscriptionStatus : std::uint8_t {
    Disabled,
    NotConfigured,
    MissingBinary,
    MissingModel,
    MissingAudio,
    UnsafePath,
    SafetyBlocked,
    Refused,
    ReadyMetadata,
    Timeout,
};

QString whisperTranscriptionStatusName(WhisperTranscriptionStatus status);

struct WhisperTranscriptionPolicy {
    bool enabled = false;
    bool localOnly = true;
    bool processExecutionAllowed = false;
    bool microphoneCaptureAllowed = false;
    bool audioPlaybackAllowed = false;
    bool streamingAllowed = false;
    bool cloudAllowed = false;
    bool downloadsAllowed = false;
    bool filesystemScanAllowed = false;
    bool promptInjectionAllowed = false;
    bool automaticChatSendAllowed = false;
    QString summary = QStringLiteral(
        "Whisper transcription policy is disabled, local-only, and metadata-only.");
};

struct WhisperTranscriptionBudget {
    int timeoutMs = 5000;
    int maxTraceCount = 8;
    int maxSummaryCharacters = 240;
    QString summary = QStringLiteral(
        "Whisper transcription metadata is bounded to 8 traces and 240 summary characters.");
};

struct WhisperTranscriptionRequest {
    QString audioPath;
    QString languageHint;
    bool localOnly = true;
    bool allowProcessExecution = false;
    bool allowMicrophoneCapture = false;
    bool allowAudioPlayback = false;
    bool allowPromptInjection = false;
    bool allowAutomaticChatSend = false;
    int timeoutMs = 5000;
};

struct WhisperTranscriptionSession {
    QString id = QStringLiteral("whisper-transcription-session-1");
    WhisperTranscriptionStatus status = WhisperTranscriptionStatus::Disabled;
    bool bounded = true;
    bool executionAttempted = false;
    QString summary = QStringLiteral("Whisper transcription session is disabled metadata only.");
};

struct WhisperTranscriptionReadiness {
    WhisperTranscriptionStatus status = WhisperTranscriptionStatus::Disabled;
    bool ready = false;
    int configuredCount = 0;
    int missingCount = 3;
    int refusedCount = 0;
    bool localOnly = true;
    bool executionAttempted = false;
    QString summary = QStringLiteral("Whisper transcription readiness is disabled.");
    QStringList checks;
};

struct WhisperTranscriptionSafetyReport {
    QString status = QStringLiteral("Blocked");
    QString summary = QStringLiteral("Whisper transcription execution is blocked by default.");
    bool safe = false;
    bool executionAttempted = false;
    bool processExecutionAllowed = false;
    bool microphoneCaptureAllowed = false;
    bool audioPlaybackAllowed = false;
    bool streamingAllowed = false;
    bool cloudAllowed = false;
    bool downloadsAllowed = false;
    bool filesystemScanAllowed = false;
    bool promptInjectionAllowed = false;
    bool automaticChatSendAllowed = false;
    QStringList checks;
};

struct WhisperTranscriptionFallback {
    WhisperTranscriptionStatus status = WhisperTranscriptionStatus::Disabled;
    QString reason = QStringLiteral("disabled");
    QString summary = QStringLiteral("Whisper transcription fell back to no transcript.");
};

struct WhisperTranscriptionTrace {
    QString stage;
    WhisperTranscriptionStatus status = WhisperTranscriptionStatus::Disabled;
    bool executionAttempted = false;
    QString summary;
};

struct WhisperTranscriptionResult {
    WhisperTranscriptionStatus status = WhisperTranscriptionStatus::Disabled;
    bool success = false;
    QString transcript;
    QString transcriptSummary = QStringLiteral("No transcript produced.");
    QString audioPathSummary = QStringLiteral("No audio file accepted.");
    int timeoutMs = 0;
    bool executionAttempted = false;
    WhisperTranscriptionSession session;
    WhisperTranscriptionFallback fallback;
    WhisperTranscriptionSafetyReport safetyReport;
    QString summary = QStringLiteral("Whisper transcription is disabled.");
    QStringList traces;
};

struct WhisperTranscriptionConfig {
    WhisperTranscriptionPolicy policy;
    WhisperTranscriptionBudget budget;
    VoiceBinaryDescriptor binary;
    VoiceModelDescriptor model;
    QString summary = QStringLiteral("Whisper transcription config is disabled.");
};

QString whisperTranscriptionReadinessSummary(
    const WhisperTranscriptionReadiness& readiness);
QString whisperTranscriptionSafetySummary(
    const WhisperTranscriptionSafetyReport& report);
QString safeWhisperTranscriptionResultSummary(
    const WhisperTranscriptionResult& result);
QStringList whisperTranscriptionTraceSummaries(
    const QList<WhisperTranscriptionTrace>& traces);
WhisperTranscriptionConfig defaultDisabledWhisperTranscriptionConfig();
WhisperTranscriptionConfig configuredWhisperTranscriptionConfig(const QString& binaryPath,
                                                                const QString& modelPath);
WhisperTranscriptionReadiness whisperTranscriptionReadiness(
    const WhisperTranscriptionConfig& config, const WhisperTranscriptionRequest& request);
WhisperTranscriptionSafetyReport whisperTranscriptionSafetyReport(
    const WhisperTranscriptionPolicy& policy);

class IWhisperTranscriptionClient {
public:
    virtual ~IWhisperTranscriptionClient() = default;

    virtual WhisperTranscriptionStatus status() const = 0;
    virtual QString statusSummary() const = 0;
    virtual WhisperTranscriptionResult transcribe(
        const WhisperTranscriptionRequest& request,
        const WhisperTranscriptionConfig& config) = 0;
};

class NullWhisperTranscriptionClient final : public IWhisperTranscriptionClient {
public:
    WhisperTranscriptionStatus status() const override;
    QString statusSummary() const override;
    WhisperTranscriptionResult transcribe(const WhisperTranscriptionRequest& request,
                                          const WhisperTranscriptionConfig& config) override;
};

class LocalWhisperTranscriptionClient final : public IWhisperTranscriptionClient {
public:
    WhisperTranscriptionStatus status() const override;
    QString statusSummary() const override;
    WhisperTranscriptionResult transcribe(const WhisperTranscriptionRequest& request,
                                          const WhisperTranscriptionConfig& config) override;
};

} // namespace sentinel::core
