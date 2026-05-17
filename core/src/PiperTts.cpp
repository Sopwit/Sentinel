#include "sentinel/core/PiperTts.h"

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

QString normalizedAbsolutePath(const QString& path) {
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool isExistingExecutableFile(const QString& path) {
    const QFileInfo info(path);
    return info.exists() && info.isFile() && info.isExecutable();
}

bool isExistingReadableFile(const QString& path) {
    const QFileInfo info(path);
    return info.exists() && info.isFile() && info.isReadable();
}

bool isControlledOutputPath(const QString& outputPath, const QString& outputDirectory) {
    if (!hasConfiguredPath(outputPath) || !hasConfiguredPath(outputDirectory)) {
        return false;
    }

    const auto output = normalizedAbsolutePath(outputPath);
    const auto directory = normalizedAbsolutePath(outputDirectory);
    return output == directory + QStringLiteral("/sentinel-piper-tts.wav") ||
           output.startsWith(directory + QStringLiteral("/"));
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
    return QStringLiteral("Controlled Piper TTS output path: %1").arg(path);
}

} // namespace

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

QString piperVoiceModelDescriptorSummary(const PiperVoiceModelDescriptor& descriptor) {
    const auto path = hasConfiguredPath(descriptor.expectedPath) ? descriptor.expectedPath.trimmed()
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
            .arg(config.fileOutputAllowed ? QStringLiteral("Allowed to controlled directory")
                                          : QStringLiteral("Blocked")),
        QStringLiteral("Output directory: %1")
            .arg(hasConfiguredPath(config.controlledOutputDirectory)
                     ? normalizedAbsolutePath(config.controlledOutputDirectory)
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

PiperTtsStatus ProcessPiperTtsClient::status() const {
    return PiperTtsStatus::Configured;
}

QString ProcessPiperTtsClient::statusSummary() const {
    return QStringLiteral("Process Piper TTS client can write a local file only when the provider "
                          "has already accepted all safety gates.");
}

PiperTtsResult ProcessPiperTtsClient::synthesize(const PiperTtsRequest& request,
                                                 const PiperTtsConfig& config) {
    const auto outputPath = controlledOutputPath(request, config);
    QDir outputDir(config.controlledOutputDirectory);
    if (!outputDir.exists() && !outputDir.mkpath(QStringLiteral("."))) {
        return PiperTtsResult{
            PiperTtsStatus::Failed,
            false,
            outputPath,
            outputPathSummary(outputPath, config),
            request.timeoutMs,
            -1,
            QStringLiteral("Unable to create controlled Piper TTS output directory."),
            QStringLiteral("Piper TTS failed before process start because the controlled output "
                           "directory could not be created."),
            {QStringLiteral("Controlled output directory creation failed.")},
        };
    }

    QProcess process;
    process.setProgram(config.binary.expectedPath);
    process.setArguments({QStringLiteral("--model"), config.voiceModel.expectedPath,
                          QStringLiteral("--output_file"), outputPath});
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start();
    if (!process.waitForStarted(request.timeoutMs)) {
        return PiperTtsResult{
            PiperTtsStatus::Failed,
            false,
            outputPath,
            outputPathSummary(outputPath, config),
            request.timeoutMs,
            -1,
            process.errorString(),
            QStringLiteral("Piper TTS failed to start within the configured boundary."),
            {QStringLiteral("Piper process start failed.")},
        };
    }

    process.write(request.text.toUtf8());
    process.closeWriteChannel();
    if (!process.waitForFinished(request.timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        return PiperTtsResult{
            PiperTtsStatus::Timeout,
            false,
            outputPath,
            outputPathSummary(outputPath, config),
            request.timeoutMs,
            -1,
            process.errorString(),
            QStringLiteral("Piper TTS timed out before file-output synthesis completed."),
            {QStringLiteral("Piper process timed out and was terminated.")},
        };
    }

    const auto exitCode = process.exitCode();
    if (process.exitStatus() != QProcess::NormalExit || exitCode != 0) {
        const auto stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
        return PiperTtsResult{
            PiperTtsStatus::Failed,
            false,
            outputPath,
            outputPathSummary(outputPath, config),
            request.timeoutMs,
            exitCode,
            stderrText.isEmpty() ? process.errorString() : stderrText.left(240),
            QStringLiteral("Piper TTS failed while generating the controlled output file."),
            {QStringLiteral("Piper process exited with failure metadata.")},
        };
    }

    return PiperTtsResult{
        PiperTtsStatus::Succeeded,
        true,
        outputPath,
        outputPathSummary(outputPath, config),
        request.timeoutMs,
        exitCode,
        {},
        QStringLiteral("Piper TTS generated a local controlled audio file. Playback was not "
                       "started."),
        {QStringLiteral("Piper process completed local file-output synthesis only.")},
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
        false,
        true,
        false,
        config_.timeoutMs,
    });
    return VoiceResponse{
        VoiceProviderStatus::Refused,      VoiceCapability::TextToSpeech, request.text, false,
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
                              "execution or controlled file output is not currently allowed.");
    case PiperTtsStatus::Refused:
        return QStringLiteral("Piper TTS refused synthesis before execution.");
    case PiperTtsStatus::Configured:
        return QStringLiteral("Piper TTS is configured for explicit local file output only. "
                              "Playback and microphone access remain disabled.");
    case PiperTtsStatus::ReadyMetadata:
        return QStringLiteral("Piper TTS metadata is present for controlled file output.");
    case PiperTtsStatus::Succeeded:
        return QStringLiteral("Piper TTS file output succeeded. Playback was not started.");
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

    if (!request.localOnly || request.allowAudioPlayback || !request.allowProcessExecution) {
        return PiperTtsResult{
            PiperTtsStatus::Refused,
            false,
            {},
            {},
            request.timeoutMs,
            -1,
            {},
            QStringLiteral("Piper TTS refused request policy: file output must be local-only, "
                           "explicitly process-allowed, and playback-disabled."),
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

    PiperTtsRequest acceptedRequest = request;
    acceptedRequest.outputPath = outputPath;
    acceptedRequest.timeoutMs = request.timeoutMs > 0 ? request.timeoutMs : config_.timeoutMs;
    return client_->synthesize(acceptedRequest, config_);
}

const PiperTtsConfig& PiperTextToSpeechProvider::config() const {
    return config_;
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

    if (!hasConfiguredPath(config_.controlledOutputDirectory)) {
        return PiperTtsStatus::NotConfigured;
    }

    if (!config_.safetyReport.executionAllowed || !config_.safetyReport.processExecutionAllowed ||
        !config_.processExecutionAllowed || !config_.fileOutputAllowed ||
        config_.audioPlaybackAllowed || config_.safetyReport.playbackAllowed ||
        config_.safetyReport.microphoneAllowed || config_.safetyReport.cloudAllowed ||
        config_.safetyReport.downloadsAllowed || config_.safetyReport.filesystemWideScanAllowed) {
        return PiperTtsStatus::SafetyBlocked;
    }

    return PiperTtsStatus::Configured;
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
