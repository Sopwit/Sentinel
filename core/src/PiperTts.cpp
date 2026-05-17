#include "sentinel/core/PiperTts.h"

#include <utility>

namespace sentinel::core {

namespace {

bool hasConfiguredPath(const QString& path) {
    const auto trimmed = path.trimmed();
    return !trimmed.isEmpty() && trimmed != QStringLiteral("not configured");
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
    case PiperTtsStatus::ReadyMetadata:
        return QStringLiteral("Ready Metadata");
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
        QStringLiteral("Piper TTS is disabled: no Piper subprocess, audio file, or playback was "
                       "created."),
        {QStringLiteral("Null Piper client refused synthesis without side effects.")},
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
        false,
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
                              "execution and audio playback remain disabled.");
    case PiperTtsStatus::Refused:
        return QStringLiteral("Piper TTS refused synthesis before execution.");
    case PiperTtsStatus::ReadyMetadata:
        return QStringLiteral("Piper TTS metadata is present, but execution and playback remain "
                              "unavailable until a later explicit phase.");
    }

    return QStringLiteral("Piper TTS is disabled.");
}

QStringList PiperTextToSpeechProvider::readinessChecks() const {
    auto checks = piperTtsConfigCheckSummaries(config_);
    checks.append(QStringLiteral("Piper client: %1").arg(client_->statusSummary()));
    return checks;
}

PiperTtsResult PiperTextToSpeechProvider::synthesizePiper(const PiperTtsRequest& request) {
    const auto currentStatus = evaluateStatus();
    if (currentStatus != PiperTtsStatus::ReadyMetadata) {
        return PiperTtsResult{
            currentStatus,
            false,
            {},
            piperStatusSummary(),
            {QStringLiteral("Piper provider refused before reaching the client boundary.")},
        };
    }

    Q_UNUSED(request);
    return PiperTtsResult{
        PiperTtsStatus::SafetyBlocked,
        false,
        {},
        QStringLiteral("Piper TTS execution path is intentionally non-callable in this phase; "
                       "controlled file-output synthesis is reserved for a later explicit phase."),
        {QStringLiteral("Piper subprocess launch is disabled by provider policy.")},
    };
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
        !hasConfiguredPath(config_.binary.expectedPath)) {
        return PiperTtsStatus::MissingBinary;
    }

    if (config_.voiceModel.status == VoiceModelStatus::Missing ||
        !hasConfiguredPath(config_.voiceModel.expectedPath)) {
        return PiperTtsStatus::MissingModel;
    }

    if (!config_.safetyReport.executionAllowed || !config_.processExecutionAllowed ||
        config_.audioPlaybackAllowed || config_.safetyReport.playbackAllowed ||
        config_.safetyReport.microphoneAllowed || config_.safetyReport.cloudAllowed ||
        config_.safetyReport.downloadsAllowed || config_.safetyReport.filesystemWideScanAllowed) {
        return PiperTtsStatus::SafetyBlocked;
    }

    return PiperTtsStatus::ReadyMetadata;
}

PiperTtsConfig defaultDisabledPiperTtsConfig() {
    PiperTtsConfig config;
    config.enabled = false;
    config.processExecutionAllowed = false;
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
    config.summary =
        QStringLiteral("Piper TTS adapter is disabled and not configured; it exposes readiness "
                       "metadata only.");
    return config;
}

} // namespace sentinel::core
