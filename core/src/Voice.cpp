#include "sentinel/core/Voice.h"

namespace sentinel::core {

QString voiceCapabilityName(VoiceCapability capability) {
    switch (capability) {
    case VoiceCapability::TextToSpeech:
        return QStringLiteral("Text To Speech");
    case VoiceCapability::SpeechToText:
        return QStringLiteral("Speech To Text");
    }

    return QStringLiteral("Unknown");
}

QString voiceProviderStatusName(VoiceProviderStatus status) {
    switch (status) {
    case VoiceProviderStatus::Disabled:
        return QStringLiteral("Disabled");
    case VoiceProviderStatus::MetadataOnly:
        return QStringLiteral("Metadata Only");
    case VoiceProviderStatus::Unavailable:
        return QStringLiteral("Unavailable");
    case VoiceProviderStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Unavailable");
}

QString voiceRuntimeModeName(VoiceRuntimeMode mode) {
    switch (mode) {
    case VoiceRuntimeMode::Disabled:
        return QStringLiteral("Disabled");
    case VoiceRuntimeMode::MetadataOnly:
        return QStringLiteral("Metadata Only");
    case VoiceRuntimeMode::FutureLocal:
        return QStringLiteral("Future Local");
    }

    return QStringLiteral("Disabled");
}

QString voiceSessionStateName(VoiceSessionState state) {
    switch (state) {
    case VoiceSessionState::Idle:
        return QStringLiteral("idle");
    case VoiceSessionState::Preparing:
        return QStringLiteral("preparing");
    case VoiceSessionState::AwaitingInput:
        return QStringLiteral("awaiting-input");
    case VoiceSessionState::TranscribingPlaceholder:
        return QStringLiteral("transcribing-placeholder");
    case VoiceSessionState::InferencePlaceholder:
        return QStringLiteral("inference-placeholder");
    case VoiceSessionState::SynthesisPlaceholder:
        return QStringLiteral("synthesis-placeholder");
    case VoiceSessionState::Completed:
        return QStringLiteral("completed");
    case VoiceSessionState::Blocked:
        return QStringLiteral("blocked");
    case VoiceSessionState::Error:
        return QStringLiteral("error");
    }

    return QStringLiteral("idle");
}

QString voicePipelineStageName(VoicePipelineStage stage) {
    switch (stage) {
    case VoicePipelineStage::Idle:
        return QStringLiteral("idle");
    case VoicePipelineStage::Preparing:
        return QStringLiteral("preparing");
    case VoicePipelineStage::AwaitingInput:
        return QStringLiteral("awaiting-input");
    case VoicePipelineStage::TranscribingPlaceholder:
        return QStringLiteral("transcribing-placeholder");
    case VoicePipelineStage::InferencePlaceholder:
        return QStringLiteral("inference-placeholder");
    case VoicePipelineStage::SynthesisPlaceholder:
        return QStringLiteral("synthesis-placeholder");
    case VoicePipelineStage::Completed:
        return QStringLiteral("completed");
    case VoicePipelineStage::Blocked:
        return QStringLiteral("blocked");
    case VoicePipelineStage::Error:
        return QStringLiteral("error");
    }

    return QStringLiteral("idle");
}

QString voicePipelineStatusName(VoicePipelineStatus status) {
    switch (status) {
    case VoicePipelineStatus::Pending:
        return QStringLiteral("pending");
    case VoicePipelineStatus::MetadataOnly:
        return QStringLiteral("metadata-only");
    case VoicePipelineStatus::Completed:
        return QStringLiteral("completed");
    case VoicePipelineStatus::Blocked:
        return QStringLiteral("blocked");
    case VoicePipelineStatus::Error:
        return QStringLiteral("error");
    }

    return QStringLiteral("pending");
}

QString voiceProviderDescriptorSummary(const VoiceProviderDescriptor& descriptor) {
    return QStringLiteral("%1: %2 / %3. %4")
        .arg(descriptor.name, voiceProviderStatusName(descriptor.status),
             voiceRuntimeModeName(descriptor.runtimeMode),
             descriptor.summary.trimmed().isEmpty()
                 ? QStringLiteral("No voice provider metadata available.")
                 : descriptor.summary.trimmed());
}

QStringList voiceProviderCapabilitySummaries(const VoiceProviderDescriptor& descriptor) {
    QStringList summaries;
    for (const auto capability : descriptor.capabilities) {
        summaries.append(QStringLiteral("%1: %2 / %3")
                             .arg(voiceCapabilityName(capability),
                                  voiceProviderStatusName(descriptor.status),
                                  voiceRuntimeModeName(descriptor.runtimeMode)));
    }
    return summaries;
}

QString safeVoiceResponseSummary(const VoiceResponse& response) {
    if (!response.summary.trimmed().isEmpty()) {
        return response.summary.trimmed();
    }

    return QStringLiteral("%1 is %2.")
        .arg(voiceCapabilityName(response.capability), voiceProviderStatusName(response.status));
}

VoiceReadinessReport buildVoiceReadinessReport(const VoiceProviderDescriptor& textToSpeech,
                                               const VoiceProviderDescriptor& speechToText) {
    VoiceReadinessReport report;
    report.status = QStringLiteral("Disabled");
    report.summary =
        QStringLiteral("Voice readiness is metadata-only: runtime unavailable, TTS unavailable, "
                       "STT unavailable, no microphone access, playback disabled, local-only "
                       "policy active, Piper and Whisper disabled, and process execution "
                       "disabled.");
    report.checks = {
        QStringLiteral("Runtime: Unavailable. Voice session orchestration is metadata-only."),
        QStringLiteral("Text to speech: %1").arg(voiceProviderDescriptorSummary(textToSpeech)),
        QStringLiteral("Speech to text: %1").arg(voiceProviderDescriptorSummary(speechToText)),
        QStringLiteral("Microphone: Disabled. No microphone access is performed."),
        QStringLiteral("Playback: Disabled. No audio playback is performed."),
        QStringLiteral("Policy: Local Only. Voice metadata cannot call cloud providers or use API "
                       "keys."),
        QStringLiteral("Process execution: Disabled. No Piper, Whisper, subprocess, filesystem, "
                       "download, or system action is performed."),
    };
    return report;
}

QString voiceRuntimeSummaryText(const VoiceRuntimeSummary& summary) {
    if (!summary.summary.trimmed().isEmpty()) {
        return summary.summary.trimmed();
    }

    return QStringLiteral("Voice runtime %1.").arg(summary.status);
}

QStringList voiceRuntimeCheckSummaries(const VoiceRuntimeSummary& summary) {
    if (!summary.checks.isEmpty()) {
        return summary.checks;
    }

    return {
        QStringLiteral("Runtime: %1")
            .arg(summary.runtimeAvailable ? QStringLiteral("Available")
                                          : QStringLiteral("Unavailable")),
        QStringLiteral("TTS: %1").arg(summary.textToSpeechAvailable
                                          ? QStringLiteral("Available")
                                          : QStringLiteral("Unavailable")),
        QStringLiteral("STT: %1").arg(summary.speechToTextAvailable
                                          ? QStringLiteral("Available")
                                          : QStringLiteral("Unavailable")),
        QStringLiteral("Microphone: %1")
            .arg(summary.microphoneEnabled ? QStringLiteral("Enabled")
                                           : QStringLiteral("Disabled")),
        QStringLiteral("Playback: %1")
            .arg(summary.playbackEnabled ? QStringLiteral("Enabled") : QStringLiteral("Disabled")),
        QStringLiteral("Local-only policy: %1")
            .arg(summary.localOnlyPolicy ? QStringLiteral("Active") : QStringLiteral("Inactive")),
        QStringLiteral("Process execution: %1")
            .arg(summary.processExecutionEnabled ? QStringLiteral("Enabled")
                                                 : QStringLiteral("Disabled")),
    };
}

QString voicePipelineTraceSummary(const VoicePipelineTrace& trace) {
    if (!trace.summary.trimmed().isEmpty()) {
        return QStringLiteral("%1 [%2]: %3")
            .arg(voicePipelineStageName(trace.stage), voicePipelineStatusName(trace.status),
                 trace.summary.trimmed());
    }

    return QStringLiteral("%1 [%2]").arg(voicePipelineStageName(trace.stage),
                                         voicePipelineStatusName(trace.status));
}

QStringList voicePipelineTraceSummaries(const QList<VoicePipelineTrace>& traces) {
    QStringList summaries;
    for (const auto& trace : traces) {
        summaries.append(voicePipelineTraceSummary(trace));
    }
    return summaries;
}

QString safeVoicePipelineSummary(const VoicePipelineResult& result) {
    if (!result.summary.trimmed().isEmpty()) {
        return result.summary.trimmed();
    }

    return QStringLiteral("Voice pipeline %1 (%2 traces).")
        .arg(voicePipelineStatusName(result.status))
        .arg(result.traces.size());
}

VoiceProviderDescriptor NullTextToSpeechProvider::descriptor() const {
    return VoiceProviderDescriptor{
        QStringLiteral("null-tts"),
        QStringLiteral("Null Text To Speech"),
        VoiceProviderStatus::Disabled,
        VoiceRuntimeMode::Disabled,
        {VoiceCapability::TextToSpeech},
        QStringLiteral("Text-to-speech is a disabled placeholder; no audio is generated or "
                       "played."),
    };
}

VoiceResponse NullTextToSpeechProvider::synthesize(const VoiceRequest& request) {
    Q_UNUSED(request);
    return VoiceResponse{
        VoiceProviderStatus::Refused,
        VoiceCapability::TextToSpeech,
        {},
        false,
        QStringLiteral("Text-to-speech is disabled: Sentinel will not synthesize or play audio in "
                       "this phase."),
    };
}

QString NullTextToSpeechProvider::statusSummary() const {
    return descriptor().summary;
}

VoiceProviderDescriptor NullSpeechToTextProvider::descriptor() const {
    return VoiceProviderDescriptor{
        QStringLiteral("null-stt"),
        QStringLiteral("Null Speech To Text"),
        VoiceProviderStatus::Disabled,
        VoiceRuntimeMode::Disabled,
        {VoiceCapability::SpeechToText},
        QStringLiteral("Speech-to-text is a disabled placeholder; no microphone or audio input is "
                       "read."),
    };
}

VoiceResponse NullSpeechToTextProvider::transcribe(const VoiceRequest& request) {
    Q_UNUSED(request);
    return VoiceResponse{
        VoiceProviderStatus::Refused,
        VoiceCapability::SpeechToText,
        {},
        false,
        QStringLiteral("Speech-to-text is disabled: Sentinel will not record, read, or transcribe "
                       "audio in this phase."),
    };
}

QString NullSpeechToTextProvider::statusSummary() const {
    return descriptor().summary;
}

VoiceSession StaticVoiceRuntimeCoordinator::currentSession() const {
    return VoiceSession{
        VoiceSessionId{QStringLiteral("voice-session-1")},
        VoiceSessionState::Idle,
        runtimeSummary(),
        QStringLiteral("Voice session is idle; runtime orchestration is metadata-only."),
    };
}

VoiceRuntimeSummary StaticVoiceRuntimeCoordinator::runtimeSummary() const {
    VoiceRuntimeSummary summary;
    summary.status = QStringLiteral("Unavailable");
    summary.summary =
        QStringLiteral("Voice runtime unavailable: TTS unavailable, STT unavailable, microphone "
                       "disabled, playback disabled, local-only policy active, and process "
                       "execution disabled.");
    summary.checks = voiceRuntimeCheckSummaries(summary);
    return summary;
}

VoicePipelineResult
StaticVoiceRuntimeCoordinator::evaluate(VoiceSessionState requestedState) const {
    VoicePipelineResult result;
    result.session = currentSession();
    result.session.state = requestedState;
    result.session.summary = QStringLiteral("Voice session %1 is metadata-only.")
                                 .arg(voiceSessionStateName(requestedState));

    const auto appendTrace = [&result](VoicePipelineStage stage, VoicePipelineStatus status,
                                       const QString& summary) {
        result.traces.append(VoicePipelineTrace{stage, status, summary});
    };

    appendTrace(VoicePipelineStage::Idle, VoicePipelineStatus::MetadataOnly,
                QStringLiteral("Voice runtime starts idle without opening audio devices."));
    appendTrace(VoicePipelineStage::Preparing, VoicePipelineStatus::MetadataOnly,
                QStringLiteral("Preparation records metadata only; no files, processes, or "
                               "devices are touched."));

    if (requestedState == VoiceSessionState::Blocked) {
        appendTrace(VoicePipelineStage::Blocked, VoicePipelineStatus::Blocked,
                    QStringLiteral("Voice pipeline blocked by metadata-only runtime policy."));
        result.status = VoicePipelineStatus::Blocked;
        result.summary =
            QStringLiteral("Voice pipeline blocked before audio input, inference, or playback.");
        return result;
    }

    if (requestedState == VoiceSessionState::Error) {
        appendTrace(VoicePipelineStage::Error, VoicePipelineStatus::Error,
                    QStringLiteral("Voice pipeline recorded error metadata without runtime "
                                   "execution."));
        result.status = VoicePipelineStatus::Error;
        result.summary =
            QStringLiteral("Voice pipeline recorded error metadata; no runtime action was "
                           "performed.");
        return result;
    }

    appendTrace(VoicePipelineStage::AwaitingInput, VoicePipelineStatus::MetadataOnly,
                QStringLiteral("Awaiting-input is placeholder metadata; microphone remains "
                               "disabled."));
    appendTrace(VoicePipelineStage::TranscribingPlaceholder, VoicePipelineStatus::MetadataOnly,
                QStringLiteral("Transcribing placeholder does not invoke Whisper or read audio."));
    appendTrace(VoicePipelineStage::InferencePlaceholder, VoicePipelineStatus::MetadataOnly,
                QStringLiteral("Inference placeholder does not call providers, models, tools, or "
                               "cloud services."));
    appendTrace(VoicePipelineStage::SynthesisPlaceholder, VoicePipelineStatus::MetadataOnly,
                QStringLiteral("Synthesis placeholder does not invoke Piper or play audio."));
    appendTrace(VoicePipelineStage::Completed, VoicePipelineStatus::Completed,
                QStringLiteral("Voice pipeline completed deterministic metadata planning only."));

    result.session.state = VoiceSessionState::Completed;
    result.session.summary =
        QStringLiteral("Voice session completed metadata-only pipeline planning.");
    result.status = VoicePipelineStatus::Completed;
    result.summary =
        QStringLiteral("Voice pipeline completed metadata-only planning; no audio or runtime "
                       "execution occurred.");
    return result;
}

} // namespace sentinel::core
