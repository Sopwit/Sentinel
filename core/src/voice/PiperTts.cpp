// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/voice/PiperTts.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#include <utility>

namespace sentinel::core {

namespace {

bool hasConfiguredPath(const QString& path) {
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

QString normalizedAbsolutePath(const QString& path) {
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool isExistingExecutableFile(const QString& path) {
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

bool isExistingReadableFile(const QString& path) {
    const QFileInfo info(path);
    return info.exists() && info.isFile() && info.isReadable();
}

QString safeConfiguredPathSummary(const QString& label, const QString& path) {
    if (!hasConfiguredPath(path)) {
        return QStringLiteral("%1: missing").arg(label);
    }
    if (unsafeLocalPath(path)) {
        return QStringLiteral("%1: refused unsafe/non-local path metadata").arg(label);
    }
    return QStringLiteral("%1: configured local metadata").arg(label);
}

bool isControlledOutputPath(const QString& outputPath, const QString& outputDirectory) {
    if (!hasConfiguredPath(outputPath) || !hasConfiguredPath(outputDirectory)) {
        return false;
    }

    const auto output = normalizedAbsolutePath(outputPath);
    const auto directory = normalizedAbsolutePath(outputDirectory);
    const auto expectedFile = QDir(directory).filePath(QStringLiteral("sentinel-piper-tts.wav"));
    return output == expectedFile || output.startsWith(directory + QStringLiteral("/"));
}

QString defaultControlledOutputDirectory() {
    const auto cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const auto base = cache.isEmpty() ? QDir::tempPath() : cache;
    return QDir(base).filePath(QStringLiteral("piper-tts"));
}

QString controlledOutputPath(const PiperTtsRequest& request, const PiperTtsConfig& config) {
    if (hasConfiguredPath(request.outputPath)) {
        return normalizedAbsolutePath(request.outputPath);
    }
    return normalizedAbsolutePath(
        QDir(config.controlledOutputDirectory).filePath(QStringLiteral("sentinel-piper-tts.wav")));
}

QString outputPathSummary(const QString& path, const PiperTtsConfig& config) {
    if (!hasConfiguredPath(path)) {
        return QStringLiteral("No output path.");
    }
    if (!isControlledOutputPath(path, config.controlledOutputDirectory)) {
        return QStringLiteral("Output path refused because it is outside the controlled Piper TTS "
                              "output directory.");
    }
    return QStringLiteral("Controlled Piper TTS output path metadata accepted.");
}

PiperSynthesisResult refusedSynthesisResult(PiperSynthesisStatus status, const QString& reason,
                                            const PiperSynthesisRequest& request,
                                            const PiperSynthesisConfig& config,
                                            const QStringList& traces) {
    const auto safety = piperSynthesisSafetyReport(config.policy);
    PiperSynthesisResult result;
    result.status = status;
    result.success = false;
    result.audioSummary = QStringLiteral("No audio produced or played.");
    result.timeoutMs = request.timeoutMs > 0 ? request.timeoutMs : config.budget.timeoutMs;
    result.executionAttempted = false;
    result.session = PiperSynthesisSession{
        QStringLiteral("piper-synthesis-session-1"),
        status,
        true,
        false,
        QStringLiteral("Piper synthesis session refused before execution."),
    };
    result.fallback = PiperSynthesisFallback{
        status,
        reason,
        QStringLiteral("Piper synthesis fallback: %1; no audio was produced, played, streamed, "
                       "or injected.")
            .arg(reason),
    };
    result.safetyReport = safety;
    result.summary = QStringLiteral("Piper synthesis %1: %2. No subprocess, playback, live "
                                    "streaming, microphone capture, cloud call, or chat/audio "
                                    "injection occurred.")
                         .arg(piperSynthesisStatusName(status), reason);
    result.traces = traces;
    return result;
}

PiperSynthesisResult completedSynthesisResult(PiperSynthesisStatus status, const QString& reason,
                                              const PiperSynthesisRequest& request,
                                              const PiperSynthesisConfig& config,
                                              const QStringList& traces, bool success,
                                              const QString& audioSummary) {
    auto safety = piperSynthesisSafetyReport(config.policy);
    safety.executionAttempted = true;
    PiperSynthesisResult result;
    result.status = status;
    result.success = success;
    result.audioSummary = audioSummary;
    result.timeoutMs = request.timeoutMs > 0 ? request.timeoutMs : config.budget.timeoutMs;
    result.executionAttempted = true;
    result.session = PiperSynthesisSession{
        QStringLiteral("piper-synthesis-session-1"),
        status,
        true,
        true,
        success ? QStringLiteral("Piper synthesis session completed a controlled local "
                                 "subprocess without playback or streaming.")
                : QStringLiteral("Piper synthesis session attempted a controlled local subprocess "
                                 "and did not complete successfully."),
    };
    result.fallback = PiperSynthesisFallback{
        status,
        reason,
        QStringLiteral("Piper synthesis %1: %2; no audio was played, streamed, or injected.")
            .arg(piperSynthesisStatusName(status), reason),
    };
    result.safetyReport = safety;
    result.summary =
        QStringLiteral("Piper synthesis %1: %2. A controlled local subprocess was attempted; no "
                       "playback, live streaming, microphone capture, cloud call, or chat/audio "
                       "injection occurred.")
            .arg(piperSynthesisStatusName(status), reason);
    result.traces = traces;
    return result;
}

QString controlledSynthesisOutputPath() {
    return normalizedAbsolutePath(
        QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation).isEmpty()
                 ? QDir::tempPath()
                 : QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
            .filePath(QStringLiteral("piper-tts/sentinel-piper-tts.wav")));
}

} // namespace

QString piperSynthesisStatusName(PiperSynthesisStatus status) {
    switch (status) {
    case PiperSynthesisStatus::Disabled:
        return QStringLiteral("Disabled");
    case PiperSynthesisStatus::NotConfigured:
        return QStringLiteral("Not Configured");
    case PiperSynthesisStatus::MissingBinary:
        return QStringLiteral("Missing Binary");
    case PiperSynthesisStatus::MissingModel:
        return QStringLiteral("Missing Model");
    case PiperSynthesisStatus::UnsafePath:
        return QStringLiteral("Unsafe Path");
    case PiperSynthesisStatus::SafetyBlocked:
        return QStringLiteral("Safety Blocked");
    case PiperSynthesisStatus::Refused:
        return QStringLiteral("Refused");
    case PiperSynthesisStatus::ReadyMetadata:
        return QStringLiteral("Ready Metadata");
    case PiperSynthesisStatus::Succeeded:
        return QStringLiteral("Succeeded");
    case PiperSynthesisStatus::Failed:
        return QStringLiteral("Failed");
    case PiperSynthesisStatus::Timeout:
        return QStringLiteral("Timeout");
    }
    return QStringLiteral("Disabled");
}

QString piperTtsStatusName(PiperTtsStatus status) {
    switch (status) {
    case PiperTtsStatus::Disabled:
        return QStringLiteral("Disabled");
    case PiperTtsStatus::NotConfigured:
        return QStringLiteral("Not Configured");
    case PiperTtsStatus::MissingBinary:
        return QStringLiteral("Missing Binary");
    case PiperTtsStatus::MissingModel:
        return QStringLiteral("Missing Model");
    case PiperTtsStatus::SafetyBlocked:
        return QStringLiteral("Safety Blocked");
    case PiperTtsStatus::Refused:
        return QStringLiteral("Refused");
    case PiperTtsStatus::Running:
        return QStringLiteral("Running");
    case PiperTtsStatus::Configured:
        return QStringLiteral("Configured");
    case PiperTtsStatus::ReadyMetadata:
        return QStringLiteral("Ready Metadata");
    case PiperTtsStatus::Succeeded:
        return QStringLiteral("Succeeded");
    case PiperTtsStatus::Failed:
        return QStringLiteral("Failed");
    case PiperTtsStatus::Timeout:
        return QStringLiteral("Timeout");
    }

    return QStringLiteral("Disabled");
}

QString piperSynthesisReadinessSummary(const PiperSynthesisReadiness& readiness) {
    return readiness.summary;
}

QString piperSynthesisSafetySummary(const PiperSynthesisSafetyReport& report) {
    return report.summary;
}

QString safePiperSynthesisResultSummary(const PiperSynthesisResult& result) {
    return result.summary.trimmed().isEmpty()
               ? QStringLiteral("Piper synthesis %1.").arg(piperSynthesisStatusName(result.status))
               : result.summary.trimmed();
}

QStringList piperSynthesisTraceSummaries(const QList<PiperSynthesisTrace>& traces) {
    QStringList summaries;
    for (const auto& item : traces) {
        summaries.append(
            QStringLiteral("%1 [%2]: %3 Execution attempted: %4")
                .arg(item.stage, piperSynthesisStatusName(item.status), item.summary,
                     item.executionAttempted ? QStringLiteral("yes") : QStringLiteral("no")));
    }
    return summaries;
}

PiperSynthesisSafetyReport piperSynthesisSafetyReport(const PiperSynthesisPolicy& policy) {
    PiperSynthesisSafetyReport report;
    report.executionAttempted = false;
    report.processExecutionAllowed = policy.processExecutionAllowed;
    report.audioPlaybackAllowed = policy.audioPlaybackAllowed;
    report.liveStreamingAllowed = policy.liveStreamingAllowed;
    report.microphoneCaptureAllowed = policy.microphoneCaptureAllowed;
    report.cloudAllowed = policy.cloudAllowed;
    report.downloadsAllowed = policy.downloadsAllowed;
    report.filesystemScanAllowed = policy.filesystemScanAllowed;
    report.automaticChatInjectionAllowed = policy.automaticChatInjectionAllowed;
    report.safe = policy.localOnly && !policy.audioPlaybackAllowed &&
                  !policy.liveStreamingAllowed && !policy.microphoneCaptureAllowed &&
                  !policy.cloudAllowed && !policy.downloadsAllowed &&
                  !policy.filesystemScanAllowed && !policy.automaticChatInjectionAllowed;
    if (policy.processExecutionAllowed) {
        report.status = QStringLiteral("Allowed");
        report.summary =
            QStringLiteral("Piper TTS safety allows explicit controlled local subprocess synthesis "
                           "only; audio playback, live voice streaming, microphone capture, cloud "
                           "calls, downloads, filesystem scanning, and automatic chat/audio "
                           "injection remain blocked; execution attempted: no.");
        report.checks = {
            QStringLiteral("Execution attempted: no"),
            QStringLiteral("Controlled subprocess synthesis: allowed"),
            QStringLiteral("Audio playback: blocked"),
            QStringLiteral("Live voice streaming: blocked"),
            QStringLiteral("Microphone capture: blocked"),
            QStringLiteral("Cloud/download/filesystem scan: blocked"),
            QStringLiteral("Automatic chat/audio injection: blocked"),
        };
        return report;
    }

    report.status = QStringLiteral("Blocked");
    report.summary =
        QStringLiteral("Piper TTS safety blocks subprocess execution, audio playback, live voice "
                       "streaming, microphone capture, cloud calls, downloads, filesystem "
                       "scanning, and automatic chat/audio injection; execution attempted: no.");
    report.checks = {
        QStringLiteral("Execution attempted: no"),
        QStringLiteral("Subprocess execution: blocked"),
        QStringLiteral("Audio playback: blocked"),
        QStringLiteral("Live voice streaming: blocked"),
        QStringLiteral("Microphone capture: blocked"),
        QStringLiteral("Cloud/download/filesystem scan: blocked"),
        QStringLiteral("Automatic chat/audio injection: blocked"),
    };
    return report;
}

PiperSynthesisConfig defaultDisabledPiperSynthesisConfig() {
    PiperSynthesisConfig config;
    config.policy = PiperSynthesisPolicy{};
    config.budget = PiperSynthesisBudget{};
    config.binary = VoiceBinaryDescriptor{
        QStringLiteral("piper-binary"),
        QStringLiteral("Piper Binary"),
        VoiceCapability::TextToSpeech,
        VoiceBinaryStatus::Missing,
        QStringLiteral("not configured"),
        false,
        QStringLiteral("Piper binary path is not configured; Sentinel will not execute Piper."),
    };
    config.model = VoiceModelDescriptor{
        QStringLiteral("piper-model"),
        QStringLiteral("Piper Voice Model"),
        VoiceCapability::TextToSpeech,
        VoiceModelStatus::Missing,
        QStringLiteral("not configured"),
        false,
        QStringLiteral("Piper voice model path is not configured or loaded."),
    };
    config.summary =
        QStringLiteral("Piper synthesis is disabled and exposes readiness metadata only.");
    return config;
}

PiperSynthesisConfig configuredPiperSynthesisConfig(const QString& binaryPath,
                                                    const QString& modelPath,
                                                    bool processExecutionAllowed) {
    auto config = defaultDisabledPiperSynthesisConfig();
    config.policy.enabled = hasConfiguredPath(binaryPath) || hasConfiguredPath(modelPath);
    config.policy.processExecutionAllowed = processExecutionAllowed;
    config.binary.status = isExistingExecutableFile(binaryPath) ? VoiceBinaryStatus::PresentMetadata
                                                                : VoiceBinaryStatus::Missing;
    config.binary.expectedPath =
        hasConfiguredPath(binaryPath) ? binaryPath.trimmed() : QStringLiteral("not configured");
    config.binary.executableAllowed = processExecutionAllowed;
    config.binary.summary =
        processExecutionAllowed
            ? QStringLiteral("Piper binary is configured for explicit local synthesis; subprocess "
                             "execution is allowed.")
            : QStringLiteral("Piper binary metadata is checked for future local TTS only.");
    config.model.status = isExistingReadableFile(modelPath) ? VoiceModelStatus::PresentMetadata
                                                            : VoiceModelStatus::Missing;
    config.model.expectedPath =
        hasConfiguredPath(modelPath) ? modelPath.trimmed() : QStringLiteral("not configured");
    config.model.loadAllowed = processExecutionAllowed;
    config.model.summary =
        processExecutionAllowed
            ? QStringLiteral("Piper voice model is configured for explicit local synthesis; model "
                             "load is allowed.")
            : QStringLiteral("Piper voice model metadata is checked for future local TTS only.");
    config.summary =
        processExecutionAllowed
            ? QStringLiteral("Piper synthesis configuration is local-only; subprocess execution is "
                             "explicitly enabled for controlled file output.")
            : QStringLiteral("Piper synthesis configuration is local-only readiness metadata; "
                             "execution and playback remain disabled.");
    return config;
}

PiperSynthesisReadiness piperSynthesisReadiness(const PiperSynthesisConfig& config,
                                                const PiperSynthesisRequest& request) {
    const auto binaryConfigured = hasConfiguredPath(config.binary.expectedPath);
    const auto modelConfigured = hasConfiguredPath(config.model.expectedPath);
    const auto unsafe = unsafeLocalPath(config.binary.expectedPath) ||
                        unsafeLocalPath(config.model.expectedPath) || !request.localOnly;

    PiperSynthesisReadiness readiness;
    readiness.configuredCount = (binaryConfigured ? 1 : 0) + (modelConfigured ? 1 : 0);
    readiness.missingCount = 2 - readiness.configuredCount;
    readiness.refusedCount = unsafe ? 1 : 0;
    readiness.localOnly = request.localOnly && config.policy.localOnly;
    readiness.executionAttempted = false;

    if (!config.policy.enabled) {
        readiness.status = PiperSynthesisStatus::Disabled;
    } else if (unsafe) {
        readiness.status = PiperSynthesisStatus::UnsafePath;
    } else if (!binaryConfigured || config.binary.status != VoiceBinaryStatus::PresentMetadata) {
        readiness.status = PiperSynthesisStatus::MissingBinary;
    } else if (!modelConfigured || config.model.status != VoiceModelStatus::PresentMetadata) {
        readiness.status = PiperSynthesisStatus::MissingModel;
    } else {
        readiness.status = PiperSynthesisStatus::ReadyMetadata;
        readiness.ready = true;
    }

    readiness.summary =
        QStringLiteral("Piper TTS synthesis readiness: %1; %2 configured, %3 missing, %4 refused; "
                       "local-only metadata; execution attempted: no.")
            .arg(piperSynthesisStatusName(readiness.status))
            .arg(readiness.configuredCount)
            .arg(readiness.missingCount)
            .arg(readiness.refusedCount);
    readiness.checks = {
        safeConfiguredPathSummary(QStringLiteral("Piper binary"), config.binary.expectedPath),
        safeConfiguredPathSummary(QStringLiteral("Piper voice model"), config.model.expectedPath),
        QStringLiteral("No subprocess execution, audio playback, live voice streaming, "
                       "microphone capture, cloud call, download, filesystem scan, or automatic "
                       "chat/audio injection."),
    };
    return readiness;
}

PiperSynthesisStatus NullPiperSynthesisClient::status() const {
    return PiperSynthesisStatus::Disabled;
}

QString NullPiperSynthesisClient::statusSummary() const {
    return QStringLiteral("Null Piper synthesis client is disabled and never launches Piper, "
                          "writes audio, plays audio, streams voice, or injects chat.");
}

PiperSynthesisResult NullPiperSynthesisClient::synthesize(const PiperSynthesisRequest& request,
                                                          const PiperSynthesisConfig& config) {
    Q_UNUSED(config);
    return refusedSynthesisResult(
        PiperSynthesisStatus::Disabled, QStringLiteral("disabled by default"), request,
        defaultDisabledPiperSynthesisConfig(),
        {QStringLiteral("Null Piper synthesis client refused without side effects.")});
}

PiperSynthesisStatus LocalPiperSynthesisClient::status() const {
    return PiperSynthesisStatus::ReadyMetadata;
}

QString LocalPiperSynthesisClient::statusSummary() const {
    return QStringLiteral("Local Piper synthesis client executes a controlled local subprocess "
                          "only when readiness and the process-execution safety gate pass.");
}

PiperSynthesisResult LocalPiperSynthesisClient::synthesize(const PiperSynthesisRequest& request,
                                                           const PiperSynthesisConfig& config) {
    const auto readiness = piperSynthesisReadiness(config, request);
    QStringList traces = {
        QStringLiteral("Request metadata accepted for validation."),
        QStringLiteral("Readiness: %1").arg(piperSynthesisReadinessSummary(readiness)),
    };

    if (request.timeoutMs <= 0) {
        traces.append(QStringLiteral("Timeout metadata fallback selected before execution."));
        return refusedSynthesisResult(PiperSynthesisStatus::Timeout,
                                      QStringLiteral("timeout budget invalid"), request, config,
                                      traces);
    }
    if (readiness.status != PiperSynthesisStatus::ReadyMetadata) {
        return refusedSynthesisResult(readiness.status, piperSynthesisStatusName(readiness.status),
                                      request, config, traces);
    }
    if (request.text.trimmed().isEmpty()) {
        return refusedSynthesisResult(PiperSynthesisStatus::Refused,
                                      QStringLiteral("empty synthesis text"), request, config,
                                      traces);
    }
    if (request.allowAudioPlayback || request.allowLiveStreaming ||
        request.allowMicrophoneCapture || request.allowCloud ||
        request.allowAutomaticChatInjection || !request.localOnly) {
        traces.append(QStringLiteral("Safety policy refused runtime privileges."));
        return refusedSynthesisResult(PiperSynthesisStatus::SafetyBlocked,
                                      QStringLiteral("unsafe runtime privileges requested"),
                                      request, config, traces);
    }
    if (!request.allowProcessExecution || !config.policy.processExecutionAllowed) {
        traces.append(QStringLiteral("Process execution is not enabled for this synthesis."));
        return refusedSynthesisResult(PiperSynthesisStatus::Refused,
                                      QStringLiteral("Piper synthesis execution phase not enabled"),
                                      request, config, traces);
    }

    const auto outputPath = controlledSynthesisOutputPath();
    QDir().mkpath(QFileInfo(outputPath).absolutePath());

    QProcess process;
    process.setInputChannelMode(QProcess::ManagedInputChannel);
    QStringList arguments = {QStringLiteral("--model"), config.model.expectedPath};
    if (!request.languageHint.trimmed().isEmpty()) {
        arguments << QStringLiteral("--language") << request.languageHint.trimmed();
    }
    if (!request.voiceHint.trimmed().isEmpty()) {
        arguments << QStringLiteral("--speaker") << request.voiceHint.trimmed();
    }
    arguments << QStringLiteral("--output_file") << outputPath;

    traces.append(QStringLiteral("Local Piper client started a controlled subprocess."));
    process.start(config.binary.expectedPath, arguments);
    if (!process.waitForStarted(request.timeoutMs)) {
        const auto errorText = process.errorString();
        traces.append(QStringLiteral("Piper subprocess failed to start: %1").arg(errorText));
        return completedSynthesisResult(
            PiperSynthesisStatus::Failed, QStringLiteral("Piper subprocess failed to start"),
            request, config, traces, false, QStringLiteral("No audio produced or played."));
    }

    process.write(request.text.toUtf8());
    process.closeWriteChannel();
    if (!process.waitForFinished(request.timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        traces.append(QStringLiteral("Piper subprocess timed out and was terminated."));
        return completedSynthesisResult(
            PiperSynthesisStatus::Timeout, QStringLiteral("Piper subprocess timed out"), request,
            config, traces, false, QStringLiteral("No audio produced or played."));
    }

    const auto exitCode = process.exitCode();
    if (exitCode != 0 || !QFileInfo::exists(outputPath)) {
        traces.append(QStringLiteral("Piper subprocess exited with code %1.").arg(exitCode));
        return completedSynthesisResult(
            PiperSynthesisStatus::Failed,
            QStringLiteral("Piper subprocess exited with code %1").arg(exitCode), request, config,
            traces, false, QStringLiteral("No audio produced or played."));
    }

    traces.append(QStringLiteral("Piper subprocess completed successfully with code 0."));
    return completedSynthesisResult(
        PiperSynthesisStatus::Succeeded, QStringLiteral("local synthesis completed"), request,
        config, traces, true, QStringLiteral("Controlled local audio file: %1").arg(outputPath));
}

QString piperVoiceModelDescriptorSummary(const PiperVoiceModelDescriptor& descriptor) {
    const auto path = hasConfiguredPath(descriptor.expectedPath)
                          ? QStringLiteral("configured local metadata")
                          : QStringLiteral("not configured");
    const auto summary = descriptor.summary.trimmed().isEmpty()
                             ? QStringLiteral("No Piper voice model metadata available.")
                             : descriptor.summary.trimmed();
    const auto language = descriptor.language.trimmed().isEmpty()
                              ? QStringLiteral("unspecified language")
                              : descriptor.language.trimmed();
    const auto speaker = descriptor.speaker.trimmed().isEmpty()
                             ? QStringLiteral("unspecified speaker")
                             : descriptor.speaker.trimmed();
    return QStringLiteral("%1: %2 at %3. Language: %4. Speaker: %5. Load allowed: %6. %7")
        .arg(descriptor.name, voiceModelStatusName(descriptor.status), path, language, speaker,
             descriptor.loadAllowed ? QStringLiteral("yes") : QStringLiteral("no"), summary);
}

QString piperTtsConfigSummary(const PiperTtsConfig& config) {
    if (!config.summary.trimmed().isEmpty()) {
        return config.summary.trimmed();
    }

    return QStringLiteral("Piper TTS is disabled by default and not configured for execution, "
                          "audio playback, downloads, cloud calls, or filesystem-wide scans.");
}

QString safePiperTtsResultSummary(const PiperTtsResult& result) {
    if (!result.summary.trimmed().isEmpty()) {
        return result.summary.trimmed();
    }

    return QStringLiteral("Piper TTS %1.").arg(piperTtsStatusName(result.status));
}

QStringList piperTtsConfigCheckSummaries(const PiperTtsConfig& config) {
    return {
        QStringLiteral("Piper provider: %1")
            .arg(config.enabled ? QStringLiteral("Enabled Metadata") : QStringLiteral("Disabled")),
        QStringLiteral("Piper binary: %1").arg(voiceBinaryDescriptorSummary(config.binary)),
        QStringLiteral("Piper voice model: %1")
            .arg(piperVoiceModelDescriptorSummary(config.voiceModel)),
        QStringLiteral("Process execution: %1")
            .arg(config.processExecutionAllowed ? QStringLiteral("Allowed")
                                                : QStringLiteral("Blocked")),
        QStringLiteral("File output: %1")
            .arg(config.fileOutputAllowed ? QStringLiteral("Refused in readiness-only phase")
                                          : QStringLiteral("Blocked")),
        QStringLiteral("Output directory: %1")
            .arg(hasConfiguredPath(config.controlledOutputDirectory)
                     ? QStringLiteral("configured app-controlled metadata")
                     : QStringLiteral("not configured")),
        QStringLiteral("Audio playback: %1")
            .arg(config.audioPlaybackAllowed ? QStringLiteral("Allowed")
                                             : QStringLiteral("Blocked")),
        QStringLiteral("Runtime safety: %1 / %2")
            .arg(config.safetyReport.status, voiceRuntimeSafetySummaryText(config.safetyReport)),
    };
}

PiperTtsStatus NullPiperTtsClient::status() const {
    return PiperTtsStatus::Disabled;
}

QString NullPiperTtsClient::statusSummary() const {
    return QStringLiteral("Null Piper TTS client is disabled and never launches Piper, writes "
                          "audio, plays audio, downloads models, or scans the filesystem.");
}

PiperTtsResult NullPiperTtsClient::synthesize(const PiperTtsRequest& request,
                                              const PiperTtsConfig& config) {
    Q_UNUSED(request);
    Q_UNUSED(config);
    return PiperTtsResult{
        PiperTtsStatus::Disabled,
        false,
        {},
        {},
        0,
        -1,
        {},
        QStringLiteral("Piper TTS is disabled: no Piper subprocess, audio file, or playback was "
                       "created."),
        {QStringLiteral("Null Piper client refused synthesis without side effects.")},
    };
}

PiperTtsStatus LocalPiperTtsClient::status() const {
    return PiperTtsStatus::ReadyMetadata;
}

QString LocalPiperTtsClient::statusSummary() const {
    return QStringLiteral("Local Piper TTS client executes a controlled local subprocess only "
                          "when readiness and the process-execution safety gate pass.");
}

PiperTtsResult LocalPiperTtsClient::synthesize(const PiperTtsRequest& request,
                                               const PiperTtsConfig& config) {
    const auto trimmedText = request.text.trimmed();
    if (trimmedText.isEmpty()) {
        return PiperTtsResult{
            PiperTtsStatus::Refused,
            false,
            {},
            {},
            request.timeoutMs,
            -1,
            {},
            QStringLiteral("Piper TTS refused an empty synthesis request."),
            {QStringLiteral("Piper request text was empty.")},
        };
    }
    if (!request.localOnly || request.allowAudioPlayback || !request.allowProcessExecution ||
        !config.processExecutionAllowed || !config.fileOutputAllowed) {
        return PiperTtsResult{
            PiperTtsStatus::Refused,
            false,
            {},
            {},
            request.timeoutMs,
            -1,
            {},
            QStringLiteral("Piper TTS refused request policy: synthesis is local-only controlled "
                           "file output; process execution must be explicitly enabled."),
            {QStringLiteral("Piper request policy gate refused synthesis.")},
        };
    }
    if (request.outputPath.trimmed().isEmpty() ||
        !isControlledOutputPath(request.outputPath, config.controlledOutputDirectory)) {
        return PiperTtsResult{
            PiperTtsStatus::Refused,
            false,
            request.outputPath,
            {},
            request.timeoutMs,
            -1,
            {},
            QStringLiteral("Piper TTS refused output outside the controlled app output directory."),
            {QStringLiteral("Piper output path gate refused synthesis.")},
        };
    }

    QDir().mkpath(QFileInfo(request.outputPath).absolutePath());
    QProcess process;
    process.setInputChannelMode(QProcess::ManagedInputChannel);
    QStringList arguments = {QStringLiteral("--model"), config.voiceModel.expectedPath};
    if (!request.languageHint.trimmed().isEmpty()) {
        arguments << QStringLiteral("--language") << request.languageHint.trimmed();
    }
    if (!config.voiceModel.speaker.trimmed().isEmpty()) {
        arguments << QStringLiteral("--speaker") << config.voiceModel.speaker.trimmed();
    }
    arguments << QStringLiteral("--output_file") << request.outputPath;

    process.start(config.binary.expectedPath, arguments);
    if (!process.waitForStarted(request.timeoutMs)) {
        const auto errorText = process.errorString();
        return PiperTtsResult{
            PiperTtsStatus::Failed,
            false,
            request.outputPath,
            outputPathSummary(request.outputPath, config),
            request.timeoutMs,
            -1,
            errorText,
            QStringLiteral("Piper subprocess failed to start: %1").arg(errorText),
            {QStringLiteral("Piper subprocess failed to start.")},
        };
    }

    process.write(trimmedText.toUtf8());
    process.closeWriteChannel();
    if (!process.waitForFinished(request.timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        return PiperTtsResult{
            PiperTtsStatus::Timeout,
            false,
            request.outputPath,
            outputPathSummary(request.outputPath, config),
            request.timeoutMs,
            -1,
            {},
            QStringLiteral("Piper subprocess timed out and was terminated."),
            {QStringLiteral("Piper subprocess timed out.")},
        };
    }

    const auto exitCode = process.exitCode();
    if (exitCode != 0 || !QFileInfo::exists(request.outputPath)) {
        return PiperTtsResult{
            PiperTtsStatus::Failed,
            false,
            request.outputPath,
            outputPathSummary(request.outputPath, config),
            request.timeoutMs,
            exitCode,
            {},
            QStringLiteral("Piper subprocess exited with code %1.").arg(exitCode),
            {QStringLiteral("Piper subprocess exited with code %1.").arg(exitCode)},
        };
    }

    return PiperTtsResult{
        PiperTtsStatus::Succeeded,
        true,
        request.outputPath,
        outputPathSummary(request.outputPath, config),
        request.timeoutMs,
        0,
        {},
        QStringLiteral("Piper TTS completed a controlled local file-output synthesis."),
        {QStringLiteral("Piper subprocess completed successfully with code 0.")},
    };
}

PiperTextToSpeechProvider::PiperTextToSpeechProvider()
    : PiperTextToSpeechProvider(defaultDisabledPiperTtsConfig(),
                                std::make_unique<NullPiperTtsClient>()) {}

PiperTextToSpeechProvider::PiperTextToSpeechProvider(PiperTtsConfig config,
                                                     std::unique_ptr<IPiperTtsClient> client)
    : config_(std::move(config)),
      client_(client ? std::move(client) : std::make_unique<NullPiperTtsClient>()) {}

VoiceProviderDescriptor PiperTextToSpeechProvider::descriptor() const {
    return VoiceProviderDescriptor{
        QStringLiteral("piper-tts"),
        QStringLiteral("Piper Text To Speech"),
        status() == PiperTtsStatus::ReadyMetadata ? VoiceProviderStatus::MetadataOnly
                                                  : VoiceProviderStatus::Disabled,
        VoiceRuntimeMode::MetadataOnly,
        {VoiceCapability::TextToSpeech},
        piperStatusSummary(),
    };
}

VoiceResponse PiperTextToSpeechProvider::synthesize(const VoiceRequest& request) {
    const auto result = synthesizePiper(PiperTtsRequest{
        request.text,
        request.languageHint,
        {},
        true,
        true,
        false,
        config_.timeoutMs,
    });
    return VoiceResponse{
        VoiceProviderStatus::Refused,      VoiceCapability::TextToSpeech, {}, false,
        safePiperTtsResultSummary(result),
    };
}

QString PiperTextToSpeechProvider::statusSummary() const {
    return piperStatusSummary();
}

PiperTtsStatus PiperTextToSpeechProvider::status() const {
    return evaluateStatus();
}

QString PiperTextToSpeechProvider::piperStatusSummary() const {
    switch (evaluateStatus()) {
    case PiperTtsStatus::Disabled:
        return QStringLiteral("Piper TTS is disabled by default; no synthesis, subprocess, file "
                              "output, or playback is available.");
    case PiperTtsStatus::NotConfigured:
        return QStringLiteral("Piper TTS is not configured; binary and voice model metadata must "
                              "be explicit before any future execution phase.");
    case PiperTtsStatus::MissingBinary:
        return QStringLiteral("Piper TTS refused readiness: Piper binary metadata is missing or "
                              "not configured.");
    case PiperTtsStatus::MissingModel:
        return QStringLiteral("Piper TTS refused readiness: Piper voice model metadata is missing "
                              "or not configured.");
    case PiperTtsStatus::SafetyBlocked:
        return QStringLiteral("Piper TTS is blocked by voice runtime safety policy; process "
                              "execution, file output, and playback are not currently allowed.");
    case PiperTtsStatus::Refused:
        return QStringLiteral("Piper TTS refused synthesis before execution.");
    case PiperTtsStatus::Running:
        return QStringLiteral("Piper TTS file output is running.");
    case PiperTtsStatus::Configured:
        return QStringLiteral("Piper TTS is configured for explicit controlled local file output; "
                              "subprocess execution and file output are enabled.");
    case PiperTtsStatus::ReadyMetadata:
        return QStringLiteral("Piper TTS metadata is present for controlled file output.");
    case PiperTtsStatus::Succeeded:
        return QStringLiteral("Piper TTS completed a controlled local file-output synthesis.");
    case PiperTtsStatus::Failed:
        return QStringLiteral("Piper TTS file output failed.");
    case PiperTtsStatus::Timeout:
        return QStringLiteral("Piper TTS file output timed out.");
    }

    return QStringLiteral("Piper TTS is disabled.");
}

QStringList PiperTextToSpeechProvider::readinessChecks() const {
    auto checks = piperTtsConfigCheckSummaries(config_);
    checks.append(QStringLiteral("Piper client: %1").arg(client_->statusSummary()));
    return checks;
}

QString PiperTextToSpeechProvider::fileOutputStatus() const {
    return piperTtsStatusName(evaluateStatus());
}

QString PiperTextToSpeechProvider::fileOutputSummary() const {
    const auto outputPath =
        hasConfiguredPath(config_.controlledOutputDirectory)
            ? controlledOutputPath(
                  PiperTtsRequest{{}, {}, {}, false, true, false, config_.timeoutMs}, config_)
            : QString{};
    const auto pathText = hasConfiguredPath(outputPath)
                              ? outputPathSummary(outputPath, config_)
                              : QStringLiteral("Controlled Piper TTS output directory is not "
                                               "configured.");
    return QStringLiteral("%1 %2").arg(
        piperStatusSummary(), QStringLiteral("%1 Timeout: %2 ms. No playback or microphone access.")
                                  .arg(pathText)
                                  .arg(config_.timeoutMs));
}

PiperTtsResult PiperTextToSpeechProvider::synthesizePiper(const PiperTtsRequest& request) {
    const auto currentStatus = evaluateStatus();
    if (currentStatus != PiperTtsStatus::Configured &&
        currentStatus != PiperTtsStatus::ReadyMetadata) {
        return PiperTtsResult{
            currentStatus,
            false,
            {},
            {},
            request.timeoutMs,
            -1,
            {},
            piperStatusSummary(),
            {QStringLiteral("Piper provider refused before reaching the client boundary.")},
        };
    }

    const auto trimmedText = request.text.trimmed();
    if (trimmedText.isEmpty()) {
        return PiperTtsResult{
            PiperTtsStatus::Refused,
            false,
            {},
            {},
            request.timeoutMs,
            -1,
            {},
            QStringLiteral("Piper TTS refused an empty synthesis request."),
            {QStringLiteral("Piper request text was empty.")},
        };
    }

    if (!request.localOnly || request.allowAudioPlayback || !request.allowProcessExecution ||
        !config_.processExecutionAllowed || !config_.fileOutputAllowed) {
        return PiperTtsResult{
            PiperTtsStatus::Refused,
            false,
            {},
            {},
            request.timeoutMs,
            -1,
            {},
            QStringLiteral("Piper TTS refused request policy: synthesis is local-only controlled "
                           "file output; process execution and playback must be explicitly "
                           "enabled."),
            {QStringLiteral("Piper request policy gate refused synthesis.")},
        };
    }

    const auto outputPath = controlledOutputPath(request, config_);
    if (!isControlledOutputPath(outputPath, config_.controlledOutputDirectory)) {
        return PiperTtsResult{
            PiperTtsStatus::Refused,
            false,
            outputPath,
            outputPathSummary(outputPath, config_),
            request.timeoutMs,
            -1,
            {},
            QStringLiteral("Piper TTS refused output outside the controlled app output directory."),
            {QStringLiteral("Piper output path gate refused synthesis.")},
        };
    }

    auto effectiveRequest = request;
    effectiveRequest.outputPath = outputPath;
    return client_->synthesize(effectiveRequest, config_);
}

const PiperTtsConfig& PiperTextToSpeechProvider::config() const {
    return config_;
}

void PiperTextToSpeechProvider::setConfig(PiperTtsConfig config) {
    config_ = std::move(config);
}

PiperTtsStatus PiperTextToSpeechProvider::evaluateStatus() const {
    if (!config_.enabled) {
        return PiperTtsStatus::Disabled;
    }

    if (!hasConfiguredPath(config_.binary.expectedPath) &&
        !hasConfiguredPath(config_.voiceModel.expectedPath)) {
        return PiperTtsStatus::NotConfigured;
    }

    if (config_.binary.status == VoiceBinaryStatus::Missing ||
        !hasConfiguredPath(config_.binary.expectedPath) ||
        !isExistingExecutableFile(config_.binary.expectedPath)) {
        return PiperTtsStatus::MissingBinary;
    }

    if (config_.voiceModel.status == VoiceModelStatus::Missing ||
        !hasConfiguredPath(config_.voiceModel.expectedPath) ||
        !isExistingReadableFile(config_.voiceModel.expectedPath)) {
        return PiperTtsStatus::MissingModel;
    }

    if (config_.safetyReport.executionAllowed || config_.safetyReport.processExecutionAllowed ||
        config_.audioPlaybackAllowed || config_.safetyReport.playbackAllowed ||
        config_.safetyReport.microphoneAllowed || config_.safetyReport.cloudAllowed ||
        config_.safetyReport.downloadsAllowed || config_.safetyReport.filesystemWideScanAllowed) {
        return PiperTtsStatus::SafetyBlocked;
    }

    if (config_.processExecutionAllowed && config_.fileOutputAllowed) {
        return PiperTtsStatus::Configured;
    }

    return PiperTtsStatus::ReadyMetadata;
}

PiperTtsConfig defaultDisabledPiperTtsConfig() {
    PiperTtsConfig config;
    config.enabled = false;
    config.processExecutionAllowed = false;
    config.fileOutputAllowed = false;
    config.audioPlaybackAllowed = false;
    config.binary = VoiceBinaryDescriptor{
        QStringLiteral("piper-binary"),
        QStringLiteral("Piper Binary"),
        VoiceCapability::TextToSpeech,
        VoiceBinaryStatus::Missing,
        QStringLiteral("not configured"),
        false,
        QStringLiteral("Piper binary path is not configured; Sentinel will not execute Piper."),
    };
    config.voiceModel = PiperVoiceModelDescriptor{
        QStringLiteral("piper-voice-model"),
        QStringLiteral("Piper Voice Model"),
        VoiceModelStatus::Missing,
        QStringLiteral("not configured"),
        {},
        {},
        false,
        QStringLiteral("Piper voice model path is not configured or loaded."),
    };
    config.safetyReport = NullVoiceRuntimeEnvironment{}.safetyReport();
    config.controlledOutputDirectory = defaultControlledOutputDirectory();
    config.timeoutMs = 5000;
    config.summary =
        QStringLiteral("Piper TTS adapter is disabled and not configured; it exposes readiness "
                       "metadata only.");
    return config;
}

} // namespace sentinel::core
