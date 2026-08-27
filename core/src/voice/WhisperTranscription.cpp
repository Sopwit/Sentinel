// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/voice/WhisperTranscription.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include <memory>

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
    if (info.exists() && info.isFile()) {
#if defined(Q_OS_WIN)
        return info.isExecutable() || QFileInfo(path + QStringLiteral(".exe")).exists();
#else
        return info.isExecutable();
#endif
    }
    return false;
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

[[maybe_unused]] WhisperTranscriptionTrace
trace(const QString& stage, WhisperTranscriptionStatus status, const QString& summary) {
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

WhisperTranscriptionResult completedTranscriptionResult(WhisperTranscriptionStatus status,
                                                        const QString& reason,
                                                        const WhisperTranscriptionRequest& request,
                                                        const WhisperTranscriptionConfig& config,
                                                        const QStringList& traces, bool success,
                                                        const QString& transcript) {
    auto safety = whisperTranscriptionSafetyReport(config.policy);
    safety.executionAttempted = true;
    WhisperTranscriptionResult result;
    result.status = status;
    result.success = success;
    result.transcript = transcript;
    result.transcriptSummary = success ? QStringLiteral("Local transcript produced.")
                                       : QStringLiteral("No transcript produced.");
    result.audioPathSummary = safeAudioPathSummary(request.audioPath);
    result.timeoutMs = request.timeoutMs > 0 ? request.timeoutMs : config.budget.timeoutMs;
    result.executionAttempted = true;
    result.session = WhisperTranscriptionSession{
        QStringLiteral("whisper-transcription-session-1"),
        status,
        true,
        true,
        success ? QStringLiteral("Whisper transcription session completed a controlled local "
                                 "subprocess without microphone access.")
                : QStringLiteral("Whisper transcription session attempted a controlled local "
                                 "subprocess and did not complete successfully."),
    };
    result.fallback = WhisperTranscriptionFallback{
        status,
        reason,
        QStringLiteral("Whisper transcription %1: %2; no transcript was produced or sent.")
            .arg(whisperTranscriptionStatusName(status), reason),
    };
    result.safetyReport = safety;
    result.summary = QStringLiteral("Whisper transcription %1: %2. A controlled local subprocess "
                                    "was attempted; no microphone, playback, streaming, prompt "
                                    "injection, or chat send occurred.")
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
    case WhisperTranscriptionStatus::Succeeded:
        return QStringLiteral("Succeeded");
    case WhisperTranscriptionStatus::Failed:
        return QStringLiteral("Failed");
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
    report.safe = policy.localOnly && !policy.microphoneCaptureAllowed &&
                  !policy.audioPlaybackAllowed && !policy.streamingAllowed &&
                  !policy.cloudAllowed && !policy.downloadsAllowed &&
                  !policy.filesystemScanAllowed && !policy.promptInjectionAllowed &&
                  !policy.automaticChatSendAllowed;
    if (policy.processExecutionAllowed) {
        report.status = QStringLiteral("Allowed");
        report.summary =
            QStringLiteral("Whisper STT safety allows explicit controlled local subprocess "
                           "transcription only; microphone capture, playback, streaming, cloud "
                           "calls, downloads, filesystem scanning, prompt injection, and automatic "
                           "chat send remain blocked; execution attempted: no.");
        report.checks = {
            QStringLiteral("Execution attempted: no"),
            QStringLiteral("Controlled subprocess transcription: allowed"),
            QStringLiteral("Microphone capture: blocked"),
            QStringLiteral("Audio playback: blocked"),
            QStringLiteral("Streaming STT: blocked"),
            QStringLiteral("Cloud/download/filesystem scan: blocked"),
            QStringLiteral("Prompt injection and automatic chat send: blocked"),
        };
        return report;
    }

    report.status = QStringLiteral("Blocked");
    report.summary =
        QStringLiteral("Whisper STT safety blocks subprocess execution, microphone capture, "
                       "playback, streaming, cloud calls, downloads, filesystem scanning, prompt "
                       "injection, and automatic chat send; execution attempted: no.");
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
                                                                const QString& modelPath,
                                                                bool processExecutionAllowed) {
    auto config = defaultDisabledWhisperTranscriptionConfig();
    config.policy.enabled = configuredPath(binaryPath) || configuredPath(modelPath);
    config.policy.processExecutionAllowed = processExecutionAllowed;
    config.binary.status = existingExecutableFile(binaryPath) ? VoiceBinaryStatus::PresentMetadata
                                                              : VoiceBinaryStatus::Missing;
    config.binary.expectedPath =
        configuredPath(binaryPath) ? binaryPath.trimmed() : QStringLiteral("not configured");
    config.binary.executableAllowed = processExecutionAllowed;
    config.binary.summary =
        processExecutionAllowed
            ? QStringLiteral("Whisper binary is configured for explicit local STT; subprocess "
                             "execution is allowed.")
            : QStringLiteral("Whisper binary metadata is checked for future local STT only.");
    config.model.status = existingReadableModelPath(modelPath) ? VoiceModelStatus::PresentMetadata
                                                               : VoiceModelStatus::Missing;
    config.model.expectedPath =
        configuredPath(modelPath) ? modelPath.trimmed() : QStringLiteral("not configured");
    config.model.loadAllowed = processExecutionAllowed;
    config.model.summary =
        processExecutionAllowed
            ? QStringLiteral("Whisper model is configured for explicit local STT; model load is "
                             "allowed.")
            : QStringLiteral("Whisper model metadata is checked for future local STT only.");
    config.summary =
        processExecutionAllowed
            ? QStringLiteral("Whisper transcription configuration is local-only; subprocess "
                             "execution is explicitly enabled for controlled audio files.")
            : QStringLiteral("Whisper transcription configuration is local-only readiness "
                             "metadata; execution remains disabled.");
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
    return QStringLiteral("Local Whisper transcription client executes a controlled local "
                          "subprocess only when readiness and the process-execution safety gate "
                          "pass.");
}

WhisperTranscriptionResult
LocalWhisperTranscriptionClient::transcribe(const WhisperTranscriptionRequest& request,
                                            const WhisperTranscriptionConfig& config) {
    const auto readiness = whisperTranscriptionReadiness(config, request);
    QStringList traces = {
        QStringLiteral("Request metadata accepted for validation."),
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
        request.allowPromptInjection || request.allowAutomaticChatSend || !request.localOnly) {
        traces.append(QStringLiteral("Safety policy refused runtime privileges."));
        return refusedResult(WhisperTranscriptionStatus::SafetyBlocked,
                             QStringLiteral("unsafe runtime privileges requested"), request, config,
                             traces);
    }
    if (!request.allowProcessExecution || !config.policy.processExecutionAllowed) {
        traces.append(QStringLiteral("Process execution is not enabled for this transcription."));
        return refusedResult(WhisperTranscriptionStatus::Refused,
                             QStringLiteral("Whisper execution phase not enabled"), request, config,
                             traces);
    }

    QProcess process;
    QStringList arguments = {QStringLiteral("-m"), config.model.expectedPath, QStringLiteral("-f"),
                             request.audioPath};
    if (!request.languageHint.trimmed().isEmpty()) {
        arguments << QStringLiteral("-l") << request.languageHint.trimmed();
    }
    arguments << QStringLiteral("-nt");

    traces.append(QStringLiteral("Local Whisper client started a controlled subprocess."));
    process.start(config.binary.expectedPath, arguments);
    if (!process.waitForStarted(request.timeoutMs)) {
        const auto errorText = process.errorString();
        traces.append(QStringLiteral("Whisper subprocess failed to start: %1").arg(errorText));
        return completedTranscriptionResult(WhisperTranscriptionStatus::Failed,
                                            QStringLiteral("Whisper subprocess failed to start"),
                                            request, config, traces, false, {});
    }
    if (!process.waitForFinished(request.timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        traces.append(QStringLiteral("Whisper subprocess timed out and was terminated."));
        return completedTranscriptionResult(WhisperTranscriptionStatus::Timeout,
                                            QStringLiteral("Whisper subprocess timed out"), request,
                                            config, traces, false, {});
    }

    const auto exitCode = process.exitCode();
    const auto transcript = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    if (exitCode != 0) {
        traces.append(QStringLiteral("Whisper subprocess exited with code %1.").arg(exitCode));
        return completedTranscriptionResult(WhisperTranscriptionStatus::Failed,
                                            QStringLiteral("Whisper subprocess exited with code "
                                                           "%1")
                                                .arg(exitCode),
                                            request, config, traces, false, {});
    }
    if (transcript.isEmpty()) {
        traces.append(QStringLiteral("Whisper subprocess produced an empty transcript."));
        return completedTranscriptionResult(WhisperTranscriptionStatus::Failed,
                                            QStringLiteral("Whisper produced no transcript"),
                                            request, config, traces, false, {});
    }

    traces.append(QStringLiteral("Whisper subprocess completed successfully with code 0."));
    return completedTranscriptionResult(WhisperTranscriptionStatus::Succeeded,
                                        QStringLiteral("local transcription completed"), request,
                                        config, traces, true, transcript);
}

WhisperSpeechToTextProvider::WhisperSpeechToTextProvider()
    : WhisperSpeechToTextProvider(defaultDisabledWhisperTranscriptionConfig(),
                                  std::make_unique<NullWhisperTranscriptionClient>()) {}

WhisperSpeechToTextProvider::WhisperSpeechToTextProvider(
    WhisperTranscriptionConfig config, std::unique_ptr<IWhisperTranscriptionClient> client)
    : config_(std::move(config)), client_(std::move(client)) {}

VoiceProviderDescriptor WhisperSpeechToTextProvider::descriptor() const {
    return VoiceProviderDescriptor{
        QStringLiteral("whisper-stt"),
        QStringLiteral("Whisper Speech To Text"),
        status() == WhisperTranscriptionStatus::ReadyMetadata ? VoiceProviderStatus::MetadataOnly
                                                              : VoiceProviderStatus::Disabled,
        VoiceRuntimeMode::MetadataOnly,
        {VoiceCapability::SpeechToText},
        whisperStatusSummary(),
    };
}

VoiceResponse WhisperSpeechToTextProvider::transcribe(const VoiceRequest& request) {
    const auto result = transcribeWhisper(WhisperTranscriptionRequest{
        request.text,
        request.languageHint,
        true,
        true,
        false,
        false,
        false,
        false,
        config_.budget.timeoutMs,
    });
    return VoiceResponse{
        result.success ? VoiceProviderStatus::MetadataOnly : VoiceProviderStatus::Refused,
        VoiceCapability::SpeechToText,
        result.transcript,
        result.success,
        safeWhisperTranscriptionResultSummary(result),
    };
}

QString WhisperSpeechToTextProvider::statusSummary() const {
    return whisperStatusSummary();
}

WhisperTranscriptionStatus WhisperSpeechToTextProvider::status() const {
    return evaluateStatus();
}

QString WhisperSpeechToTextProvider::whisperStatusSummary() const {
    switch (evaluateStatus()) {
    case WhisperTranscriptionStatus::Disabled:
        return QStringLiteral("Whisper STT is disabled by default; no transcription, subprocess, "
                              "microphone, or audio input is available.");
    case WhisperTranscriptionStatus::NotConfigured:
        return QStringLiteral("Whisper STT is not configured; binary and model metadata must be "
                              "explicit before any future execution phase.");
    case WhisperTranscriptionStatus::MissingBinary:
        return QStringLiteral("Whisper STT refused readiness: Whisper binary metadata is missing "
                              "or not configured.");
    case WhisperTranscriptionStatus::MissingModel:
        return QStringLiteral("Whisper STT refused readiness: Whisper model metadata is missing "
                              "or not configured.");
    case WhisperTranscriptionStatus::SafetyBlocked:
        return QStringLiteral("Whisper STT is blocked by voice runtime safety policy; process "
                              "execution and microphone access are not currently allowed.");
    case WhisperTranscriptionStatus::ReadyMetadata:
        return QStringLiteral("Whisper STT is configured for explicit controlled local "
                              "transcription; subprocess execution is enabled.");
    case WhisperTranscriptionStatus::Refused:
        return QStringLiteral("Whisper STT refused transcription before execution.");
    case WhisperTranscriptionStatus::Succeeded:
        return QStringLiteral("Whisper STT completed a controlled local transcription.");
    case WhisperTranscriptionStatus::Failed:
        return QStringLiteral("Whisper STT transcription failed.");
    case WhisperTranscriptionStatus::Timeout:
        return QStringLiteral("Whisper STT transcription timed out.");
    case WhisperTranscriptionStatus::MissingAudio:
        return QStringLiteral("Whisper STT refused readiness: audio input is missing.");
    case WhisperTranscriptionStatus::UnsafePath:
        return QStringLiteral("Whisper STT refused an unsafe or non-local audio path.");
    }

    return QStringLiteral("Whisper STT is disabled.");
}

QStringList WhisperSpeechToTextProvider::readinessChecks() const {
    auto checks = whisperTranscriptionReadiness(config_, WhisperTranscriptionRequest{}).checks;
    checks.append(QStringLiteral("Whisper client: %1").arg(client_->statusSummary()));
    return checks;
}

WhisperTranscriptionResult
WhisperSpeechToTextProvider::transcribeWhisper(const WhisperTranscriptionRequest& request) {
    const auto currentStatus = evaluateStatus();
    if (currentStatus != WhisperTranscriptionStatus::ReadyMetadata) {
        return WhisperTranscriptionResult{
            currentStatus,
            false,
            {},
            whisperTranscriptionStatusName(currentStatus),
            safeAudioPathSummary(request.audioPath),
            request.timeoutMs,
            false,
            WhisperTranscriptionSession{},
            WhisperTranscriptionFallback{},
            whisperTranscriptionSafetyReport(config_.policy),
            whisperStatusSummary(),
            {QStringLiteral("Whisper provider refused before reaching the client boundary.")},
        };
    }

    if (!request.localOnly || request.allowMicrophoneCapture || request.allowAudioPlayback ||
        request.allowPromptInjection || request.allowAutomaticChatSend ||
        !request.allowProcessExecution || !config_.policy.processExecutionAllowed) {
        return WhisperTranscriptionResult{
            WhisperTranscriptionStatus::Refused,
            false,
            {},
            {},
            safeAudioPathSummary(request.audioPath),
            request.timeoutMs,
            false,
            WhisperTranscriptionSession{},
            WhisperTranscriptionFallback{},
            whisperTranscriptionSafetyReport(config_.policy),
            QStringLiteral("Whisper STT refused request policy: transcription is local-only "
                           "controlled subprocess execution; microphone, playback, prompt "
                           "injection, and chat send must be explicitly disabled."),
            {QStringLiteral("Whisper request policy gate refused transcription.")},
        };
    }

    return client_->transcribe(request, config_);
}

const WhisperTranscriptionConfig& WhisperSpeechToTextProvider::config() const {
    return config_;
}

void WhisperSpeechToTextProvider::setConfig(WhisperTranscriptionConfig config) {
    config_ = std::move(config);
}

WhisperTranscriptionStatus WhisperSpeechToTextProvider::evaluateStatus() const {
    if (!config_.policy.enabled) {
        return WhisperTranscriptionStatus::Disabled;
    }

    if (!configuredPath(config_.binary.expectedPath) &&
        !configuredPath(config_.model.expectedPath)) {
        return WhisperTranscriptionStatus::NotConfigured;
    }

    if (config_.binary.status == VoiceBinaryStatus::Missing ||
        !configuredPath(config_.binary.expectedPath) ||
        !existingExecutableFile(config_.binary.expectedPath)) {
        return WhisperTranscriptionStatus::MissingBinary;
    }

    if (config_.model.status == VoiceModelStatus::Missing ||
        !configuredPath(config_.model.expectedPath) ||
        !existingReadableModelPath(config_.model.expectedPath)) {
        return WhisperTranscriptionStatus::MissingModel;
    }

    if (!config_.policy.processExecutionAllowed) {
        return WhisperTranscriptionStatus::SafetyBlocked;
    }

    return WhisperTranscriptionStatus::ReadyMetadata;
}

} // namespace sentinel::core
