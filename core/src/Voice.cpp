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
    report.summary = QStringLiteral("Voice readiness is metadata-only: TTS and STT are disabled; "
                                    "no microphone, playback, Whisper, Piper, subprocess, "
                                    "download, cloud, or API-key path is active.");
    report.checks = {
        QStringLiteral("Text to speech: %1").arg(voiceProviderDescriptorSummary(textToSpeech)),
        QStringLiteral("Speech to text: %1").arg(voiceProviderDescriptorSummary(speechToText)),
        QStringLiteral("Audio input/output: Disabled. No microphone access or audio playback is "
                       "performed."),
        QStringLiteral("Runtime execution: Disabled. No Piper, Whisper, subprocess, filesystem, "
                       "download, cloud, or API-key action is performed."),
    };
    return report;
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

} // namespace sentinel::core
