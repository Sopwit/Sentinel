#include "sentinel/core/WhisperTranscription.h"

#include <QDir>
#include <QFileInfo>

namespace sentinel::core {

namespace {

bool configuredPath(const QString& path) {
    const auto trimmed = path.trimmed();
    return !trimmed.isEmpty() && trimmed != QStringLiteral("not configured");
}

bool unsafeLocalPath(const QString& path) {
    const auto trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    const auto lower = trimmed.toLower();
    return lower.contains(QStringLiteral("://")) || lower.startsWith(QStringLiteral("\\\\"));
}

bool existingExecutableFile(const QString& path) {
    const QFileInfo info(path);
    return info.exists() && info.isFile() && info.isExecutable();
}

bool existingReadableModelPath(const QString& path) {
    const QFileInfo info(path);
    return info.exists() && info.isReadable() && (info.isFile() || info.isDir());
}

bool existingReadableAudioFile(const QString& path) {
    const QFileInfo info(path);
    return info.exists() && info.isFile() && info.isReadable();
}

QString safeAudioPathSummary(const QString& path) {
    if (!configuredPath(path)) {
        return QStringLiteral("No audio file path configured.");
    }
    if (unsafeLocalPath(path)) {
        return QStringLiteral("Audio file path refused because it is unsafe or non-local.");
    }
    return QStringLiteral("Local audio file metadata accepted without exposing raw path.");
}

WhisperTranscriptionTrace trace(const QString& stage, WhisperTranscriptionStatus status,
                                const QString& summary) {
    return WhisperTranscriptionTrace{stage, status, false, summary};
}

WhisperTranscriptionResult refusedResult(WhisperTranscriptionStatus status, const QString& reason,
                                         const WhisperTranscriptionRequest& request,
                                         const WhisperTranscriptionConfig& config,
                                         const QStringList& traces) {
    const auto safety = whisperTranscriptionSafetyReport(config.policy);
    WhisperTranscriptionResult result;
    result.status = status;
    result.success = false;
    result.transcript.clear();
    result.transcriptSummary = QStringLiteral("No transcript produced.");
    result.audioPathSummary = safeAudioPathSummary(request.audioPath);
    result.timeoutMs = request.timeoutMs > 0 ? request.timeoutMs : config.budget.timeoutMs;
    result.executionAttempted = false;
    result.session = WhisperTranscriptionSession{
        QStringLiteral("whisper-transcription-session-1"),
        status,
        true,
        false,
        QStringLiteral("Whisper transcription session refused before execution."),
    };
    result.fallback = WhisperTranscriptionFallback{
        status,
        reason,
        QStringLiteral("Whisper transcription fallback: %1; no transcript was produced or sent.")
            .arg(reason),
    };
    result.safetyReport = safety;
    result.summary = QStringLiteral("Whisper transcription %1: %2. No subprocess, microphone, "
                                    "playback, prompt injection, or chat send occurred.")
                         .arg(whisperTranscriptionStatusName(status), reason);
    result.traces = traces;
    return result;
}

} // namespace

QString whisperTranscriptionStatusName(WhisperTranscriptionStatus status) {
    switch (status) {
    case WhisperTranscriptionStatus::Disabled:
        return QStringLiteral("Disabled");
    case WhisperTranscriptionStatus::NotConfigured:
        return QStringLiteral("Not Configured");
    case WhisperTranscriptionStatus::MissingBinary:
        return QStringLiteral("Missing Binary");
    case WhisperTranscriptionStatus::MissingModel:
        return QStringLiteral("Missing Model");
    case WhisperTranscriptionStatus::MissingAudio:
        return QStringLiteral("Missing Audio");
    case WhisperTranscriptionStatus::UnsafePath:
        return QStringLiteral("Unsafe Path");
    case WhisperTranscriptionStatus::SafetyBlocked:
        return QStringLiteral("Safety Blocked");
    case WhisperTranscriptionStatus::Refused:
        return QStringLiteral("Refused");
    case WhisperTranscriptionStatus::ReadyMetadata:
        return QStringLiteral("Ready Metadata");
    case WhisperTranscriptionStatus::Timeout:
        return QStringLiteral("Timeout");
    }
    return QStringLiteral("Disabled");
}

QString whisperTranscriptionReadinessSummary(const WhisperTranscriptionReadiness& readiness) {
    return readiness.summary;
}

QString whisperTranscriptionSafetySummary(const WhisperTranscriptionSafetyReport& report) {
    return report.summary;
}

QString safeWhisperTranscriptionResultSummary(const WhisperTranscriptionResult& result) {
    return result.summary.trimmed().isEmpty()
               ? QStringLiteral("Whisper transcription %1.")
                     .arg(whisperTranscriptionStatusName(result.status))
               : result.summary.trimmed();
}

QStringList whisperTranscriptionTraceSummaries(const QList<WhisperTranscriptionTrace>& traces) {
    QStringList summaries;
    for (const auto& item : traces) {
        summaries.append(
            QStringLiteral("%1 [%2]: %3 Execution attempted: %4")
                .arg(item.stage, whisperTranscriptionStatusName(item.status), item.summary,
                     item.executionAttempted ? QStringLiteral("yes") : QStringLiteral("no")));
    }
    return summaries;
}

WhisperTranscriptionSafetyReport
whisperTranscriptionSafetyReport(const WhisperTranscriptionPolicy& policy) {
    WhisperTranscriptionSafetyReport report;
    report.status = QStringLiteral("Blocked");
    report.summary =
        QStringLiteral("Whisper STT safety blocks subprocess execution, microphone capture, "
                       "playback, streaming, cloud calls, downloads, filesystem scanning, prompt "
                       "injection, and automatic chat send; execution attempted: no.");
    report.safe = policy.localOnly && !policy.processExecutionAllowed &&
                  !policy.microphoneCaptureAllowed && !policy.audioPlaybackAllowed &&
                  !policy.streamingAllowed && !policy.cloudAllowed && !policy.downloadsAllowed &&
                  !policy.filesystemScanAllowed && !policy.promptInjectionAllowed &&
                  !policy.automaticChatSendAllowed;
    report.executionAttempted = false;
    report.processExecutionAllowed = policy.processExecutionAllowed;
    report.microphoneCaptureAllowed = policy.microphoneCaptureAllowed;
    report.audioPlaybackAllowed = policy.audioPlaybackAllowed;
    report.streamingAllowed = policy.streamingAllowed;
    report.cloudAllowed = policy.cloudAllowed;
    report.downloadsAllowed = policy.downloadsAllowed;
    report.filesystemScanAllowed = policy.filesystemScanAllowed;
    report.promptInjectionAllowed = policy.promptInjectionAllowed;
    report.automaticChatSendAllowed = policy.automaticChatSendAllowed;
    report.checks = {
        QStringLiteral("Execution attempted: no"),
        QStringLiteral("Subprocess execution: blocked"),
        QStringLiteral("Microphone capture: blocked"),
        QStringLiteral("Audio playback: blocked"),
        QStringLiteral("Streaming STT: blocked"),
        QStringLiteral("Cloud/download/filesystem scan: blocked"),
        QStringLiteral("Prompt injection and automatic chat send: blocked"),
    };
    return report;
}

WhisperTranscriptionConfig defaultDisabledWhisperTranscriptionConfig() {
    WhisperTranscriptionConfig config;
    config.policy = WhisperTranscriptionPolicy{};
    config.budget = WhisperTranscriptionBudget{};
    config.binary = VoiceBinaryDescriptor{
        QStringLiteral("whisper-binary"),
        QStringLiteral("Whisper Binary"),
        VoiceCapability::SpeechToText,
        VoiceBinaryStatus::Missing,
        QStringLiteral("not configured"),
        false,
        QStringLiteral("Whisper binary path is not configured; Sentinel will not execute Whisper."),
    };
    config.model = VoiceModelDescriptor{
        QStringLiteral("whisper-model"),
        QStringLiteral("Whisper Model"),
        VoiceCapability::SpeechToText,
        VoiceModelStatus::Missing,
        QStringLiteral("not configured"),
        false,
        QStringLiteral("Whisper model path is not configured or loaded."),
    };
    config.summary =
        QStringLiteral("Whisper transcription is disabled and exposes readiness metadata only.");
    return config;
}

WhisperTranscriptionConfig configuredWhisperTranscriptionConfig(const QString& binaryPath,
                                                                const QString& modelPath) {
    auto config = defaultDisabledWhisperTranscriptionConfig();
    config.policy.enabled = configuredPath(binaryPath) || configuredPath(modelPath);
    config.binary.status = existingExecutableFile(binaryPath) ? VoiceBinaryStatus::PresentMetadata
                                                              : VoiceBinaryStatus::Missing;
    config.binary.expectedPath =
        configuredPath(binaryPath) ? binaryPath.trimmed() : QStringLiteral("not configured");
    config.binary.executableAllowed = false;
    config.binary.summary =
        QStringLiteral("Whisper binary metadata is checked for future local STT only.");
    config.model.status = existingReadableModelPath(modelPath) ? VoiceModelStatus::PresentMetadata
                                                               : VoiceModelStatus::Missing;
    config.model.expectedPath =
        configuredPath(modelPath) ? modelPath.trimmed() : QStringLiteral("not configured");
    config.model.loadAllowed = false;
    config.model.summary =
        QStringLiteral("Whisper model metadata is checked for future local STT only.");
    config.summary =
        QStringLiteral("Whisper transcription configuration is local-only readiness metadata; "
                       "execution remains disabled.");
    return config;
}

WhisperTranscriptionReadiness
whisperTranscriptionReadiness(const WhisperTranscriptionConfig& config,
                              const WhisperTranscriptionRequest& request) {
    const auto binaryConfigured = configuredPath(config.binary.expectedPath);
    const auto modelConfigured = configuredPath(config.model.expectedPath);
    const auto audioConfigured = configuredPath(request.audioPath);
    const auto unsafe = unsafeLocalPath(config.binary.expectedPath) ||
                        unsafeLocalPath(config.model.expectedPath) ||
                        unsafeLocalPath(request.audioPath) || !request.localOnly;

    WhisperTranscriptionReadiness readiness;
    readiness.configuredCount =
        (binaryConfigured ? 1 : 0) + (modelConfigured ? 1 : 0) + (audioConfigured ? 1 : 0);
    readiness.missingCount = 3 - readiness.configuredCount;
    readiness.refusedCount = unsafe ? 1 : 0;
    readiness.localOnly = request.localOnly && config.policy.localOnly;
    readiness.executionAttempted = false;

    if (!config.policy.enabled) {
        readiness.status = WhisperTranscriptionStatus::Disabled;
    } else if (unsafe) {
        readiness.status = WhisperTranscriptionStatus::UnsafePath;
    } else if (!binaryConfigured || config.binary.status != VoiceBinaryStatus::PresentMetadata) {
        readiness.status = WhisperTranscriptionStatus::MissingBinary;
    } else if (!modelConfigured || config.model.status != VoiceModelStatus::PresentMetadata) {
        readiness.status = WhisperTranscriptionStatus::MissingModel;
    } else if (!audioConfigured || !existingReadableAudioFile(request.audioPath)) {
        readiness.status = WhisperTranscriptionStatus::MissingAudio;
    } else {
        readiness.status = WhisperTranscriptionStatus::ReadyMetadata;
        readiness.ready = true;
    }

    readiness.summary =
        QStringLiteral("Whisper STT transcription readiness: %1; %2 configured, %3 missing, %4 "
                       "refused; local-only metadata; execution attempted: no.")
            .arg(whisperTranscriptionStatusName(readiness.status))
            .arg(readiness.configuredCount)
            .arg(readiness.missingCount)
            .arg(readiness.refusedCount);
    readiness.checks = {
        QStringLiteral("Whisper binary: %1")
            .arg(binaryConfigured ? QStringLiteral("configured metadata")
                                  : QStringLiteral("missing")),
        QStringLiteral("Whisper model: %1")
            .arg(modelConfigured ? QStringLiteral("configured metadata")
                                 : QStringLiteral("missing")),
        QStringLiteral("Audio file: %1")
            .arg(audioConfigured ? safeAudioPathSummary(request.audioPath)
                                 : QStringLiteral("missing")),
        QStringLiteral("No microphone capture, live recording, playback, streaming STT, cloud "
                       "call, download, prompt injection, or automatic chat send."),
    };
    return readiness;
}

WhisperTranscriptionStatus NullWhisperTranscriptionClient::status() const {
    return WhisperTranscriptionStatus::Disabled;
}

QString NullWhisperTranscriptionClient::statusSummary() const {
    return QStringLiteral("Null Whisper transcription client is disabled and never launches "
                          "Whisper, reads audio, opens microphones, or sends transcripts.");
}

WhisperTranscriptionResult
NullWhisperTranscriptionClient::transcribe(const WhisperTranscriptionRequest& request,
                                           const WhisperTranscriptionConfig& config) {
    Q_UNUSED(config);
    return refusedResult(WhisperTranscriptionStatus::Disabled,
                         QStringLiteral("disabled by default"), request,
                         defaultDisabledWhisperTranscriptionConfig(),
                         {QStringLiteral("Null Whisper client refused transcription without side "
                                         "effects.")});
}

WhisperTranscriptionStatus LocalWhisperTranscriptionClient::status() const {
    return WhisperTranscriptionStatus::ReadyMetadata;
}

QString LocalWhisperTranscriptionClient::statusSummary() const {
    return QStringLiteral("Local Whisper transcription client is a bounded non-executing "
                          "skeleton; it validates metadata and refuses before subprocess start.");
}

WhisperTranscriptionResult
LocalWhisperTranscriptionClient::transcribe(const WhisperTranscriptionRequest& request,
                                            const WhisperTranscriptionConfig& config) {
    const auto readiness = whisperTranscriptionReadiness(config, request);
    QStringList traces = {
        QStringLiteral("Request metadata accepted for validation only."),
        QStringLiteral("Readiness: %1").arg(whisperTranscriptionReadinessSummary(readiness)),
    };

    if (request.timeoutMs <= 0) {
        traces.append(QStringLiteral("Timeout metadata fallback selected before execution."));
        return refusedResult(WhisperTranscriptionStatus::Timeout,
                             QStringLiteral("timeout budget invalid"), request, config, traces);
    }
    if (readiness.status != WhisperTranscriptionStatus::ReadyMetadata) {
        return refusedResult(readiness.status, whisperTranscriptionStatusName(readiness.status),
                             request, config, traces);
    }
    if (request.allowMicrophoneCapture || request.allowAudioPlayback ||
        request.allowPromptInjection || request.allowAutomaticChatSend ||
        !request.allowProcessExecution || config.policy.processExecutionAllowed) {
        traces.append(QStringLiteral("Safety policy refused runtime privileges."));
        return refusedResult(WhisperTranscriptionStatus::SafetyBlocked,
                             QStringLiteral("runtime execution remains out of scope"), request,
                             config, traces);
    }

    traces.append(QStringLiteral("Local Whisper skeleton reached the execution boundary and "
                                 "refused without launching a subprocess."));
    return refusedResult(WhisperTranscriptionStatus::Refused,
                         QStringLiteral("Whisper execution phase not enabled"), request, config,
                         traces);
}

} // namespace sentinel::core
