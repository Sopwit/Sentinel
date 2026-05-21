#include "sentinel/core/Voice.h"

#include <utility>

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

QString voicePipelineSessionStatusName(VoicePipelineSessionStatus status) {
    switch (status) {
    case VoicePipelineSessionStatus::Disabled:
        return QStringLiteral("disabled");
    case VoicePipelineSessionStatus::ReadyMetadata:
        return QStringLiteral("ready-metadata");
    case VoicePipelineSessionStatus::Blocked:
        return QStringLiteral("blocked");
    case VoicePipelineSessionStatus::Refused:
        return QStringLiteral("refused");
    case VoicePipelineSessionStatus::Fallback:
        return QStringLiteral("fallback");
    case VoicePipelineSessionStatus::Completed:
        return QStringLiteral("completed");
    }

    return QStringLiteral("disabled");
}

QString voicePipelineSessionStepName(VoicePipelineSessionStep step) {
    switch (step) {
    case VoicePipelineSessionStep::Prepare:
        return QStringLiteral("prepare");
    case VoicePipelineSessionStep::AwaitAudioInput:
        return QStringLiteral("await-audio-input");
    case VoicePipelineSessionStep::TranscriptionReadiness:
        return QStringLiteral("transcription-readiness");
    case VoicePipelineSessionStep::ChatInferenceReadiness:
        return QStringLiteral("chat-inference-readiness");
    case VoicePipelineSessionStep::SynthesisReadiness:
        return QStringLiteral("synthesis-readiness");
    case VoicePipelineSessionStep::Completion:
        return QStringLiteral("completion");
    case VoicePipelineSessionStep::Refusal:
        return QStringLiteral("refusal");
    case VoicePipelineSessionStep::Fallback:
        return QStringLiteral("fallback");
    }

    return QStringLiteral("prepare");
}

QString voiceBinaryStatusName(VoiceBinaryStatus status) {
    switch (status) {
    case VoiceBinaryStatus::Missing:
        return QStringLiteral("Missing");
    case VoiceBinaryStatus::ExpectedPath:
        return QStringLiteral("Expected Path");
    case VoiceBinaryStatus::PresentMetadata:
        return QStringLiteral("Present Metadata");
    case VoiceBinaryStatus::Unavailable:
        return QStringLiteral("Unavailable");
    }

    return QStringLiteral("Unavailable");
}

QString voiceModelStatusName(VoiceModelStatus status) {
    switch (status) {
    case VoiceModelStatus::Missing:
        return QStringLiteral("Missing");
    case VoiceModelStatus::ExpectedPath:
        return QStringLiteral("Expected Path");
    case VoiceModelStatus::PresentMetadata:
        return QStringLiteral("Present Metadata");
    case VoiceModelStatus::Unavailable:
        return QStringLiteral("Unavailable");
    }

    return QStringLiteral("Unavailable");
}

QString voiceRuntimeStatusName(VoiceRuntimeStatus status) {
    switch (status) {
    case VoiceRuntimeStatus::Disabled:
        return QStringLiteral("Disabled");
    case VoiceRuntimeStatus::ReadyMetadata:
        return QStringLiteral("Ready Metadata");
    case VoiceRuntimeStatus::MissingConfiguration:
        return QStringLiteral("Missing Configuration");
    case VoiceRuntimeStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString voiceRuntimeReadinessName(VoiceRuntimeReadiness readiness) {
    switch (readiness) {
    case VoiceRuntimeReadiness::Disabled:
        return QStringLiteral("Disabled");
    case VoiceRuntimeReadiness::ReadyMetadata:
        return QStringLiteral("Ready Metadata");
    case VoiceRuntimeReadiness::MissingConfiguration:
        return QStringLiteral("Missing Configuration");
    case VoiceRuntimeReadiness::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString voiceRuntimeHealthName(VoiceRuntimeHealth health) {
    switch (health) {
    case VoiceRuntimeHealth::HealthyMetadata:
        return QStringLiteral("Healthy Metadata");
    case VoiceRuntimeHealth::DegradedMetadata:
        return QStringLiteral("Degraded Metadata");
    case VoiceRuntimeHealth::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Blocked");
}

QString voiceRuntimeSandboxName(VoiceRuntimeSandbox sandbox) {
    switch (sandbox) {
    case VoiceRuntimeSandbox::NotRequired:
        return QStringLiteral("Not Required");
    case VoiceRuntimeSandbox::RequiredMetadata:
        return QStringLiteral("Required Metadata");
    case VoiceRuntimeSandbox::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Required Metadata");
}

QString whisperRuntimeStatusName(WhisperRuntimeStatus status) {
    switch (status) {
    case WhisperRuntimeStatus::Disabled:
        return QStringLiteral("Disabled");
    case WhisperRuntimeStatus::ReadyMetadata:
        return QStringLiteral("Ready Metadata");
    case WhisperRuntimeStatus::MissingConfiguration:
        return QStringLiteral("Missing Configuration");
    case WhisperRuntimeStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString whisperRuntimeReadinessName(WhisperRuntimeReadiness readiness) {
    switch (readiness) {
    case WhisperRuntimeReadiness::Disabled:
        return QStringLiteral("Disabled");
    case WhisperRuntimeReadiness::ReadyMetadata:
        return QStringLiteral("Ready Metadata");
    case WhisperRuntimeReadiness::MissingConfiguration:
        return QStringLiteral("Missing Configuration");
    case WhisperRuntimeReadiness::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString piperRuntimeStatusName(PiperRuntimeStatus status) {
    switch (status) {
    case PiperRuntimeStatus::Disabled:
        return QStringLiteral("Disabled");
    case PiperRuntimeStatus::ReadyMetadata:
        return QStringLiteral("Ready Metadata");
    case PiperRuntimeStatus::MissingConfiguration:
        return QStringLiteral("Missing Configuration");
    case PiperRuntimeStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString piperRuntimeReadinessName(PiperRuntimeReadiness readiness) {
    switch (readiness) {
    case PiperRuntimeReadiness::Disabled:
        return QStringLiteral("Disabled");
    case PiperRuntimeReadiness::ReadyMetadata:
        return QStringLiteral("Ready Metadata");
    case PiperRuntimeReadiness::MissingConfiguration:
        return QStringLiteral("Missing Configuration");
    case PiperRuntimeReadiness::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

namespace {

bool configured(const QString& path) {
    return !path.trimmed().isEmpty();
}

bool unsafeVoiceRuntimePath(const QString& path) {
    const auto trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    const auto lower = trimmed.toLower();
    return lower.contains(QStringLiteral("://")) || lower.startsWith(QStringLiteral("\\\\"));
}

QString configuredMissingSummary(bool binaryConfigured, bool modelConfigured) {
    return QStringLiteral("%1 configured, %2 missing")
        .arg((binaryConfigured ? 1 : 0) + (modelConfigured ? 1 : 0))
        .arg((binaryConfigured ? 0 : 1) + (modelConfigured ? 0 : 1));
}

} // namespace

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

QString voicePipelineSessionReadinessSummary(
    const VoicePipelineSessionReadiness& readiness) {
    const auto summary = readiness.summary.trimmed().isEmpty()
                             ? QStringLiteral("No voice pipeline readiness metadata available.")
                             : readiness.summary.trimmed();
    return QStringLiteral("%1 [%2]: %3")
        .arg(voicePipelineSessionStepName(readiness.step),
             voicePipelineSessionStatusName(readiness.status), summary);
}

QString voicePipelineSessionStepSummary(const VoicePipelineSessionStepRecord& step) {
    const auto summary = step.summary.trimmed().isEmpty()
                             ? QStringLiteral("No voice pipeline step metadata available.")
                             : step.summary.trimmed();
    return QStringLiteral("%1 [%2]: %3")
        .arg(voicePipelineSessionStepName(step.step),
             voicePipelineSessionStatusName(step.status), summary);
}

QStringList voicePipelineSessionStepSummaries(
    const QList<VoicePipelineSessionStepRecord>& steps) {
    QStringList summaries;
    for (const auto& step : steps) {
        summaries.append(voicePipelineSessionStepSummary(step));
    }
    return summaries;
}

QString voicePipelineSessionTraceSummary(const VoicePipelineSessionTrace& trace) {
    const auto summary = trace.summary.trimmed().isEmpty()
                             ? QStringLiteral("No voice pipeline trace metadata available.")
                             : trace.summary.trimmed();
    return QStringLiteral("%1. %2 [%3]: %4")
        .arg(trace.sequence)
        .arg(voicePipelineSessionStepName(trace.step),
             voicePipelineSessionStatusName(trace.status), summary);
}

QStringList voicePipelineSessionTraceSummaries(
    const QList<VoicePipelineSessionTrace>& traces) {
    QStringList summaries;
    for (const auto& trace : traces) {
        summaries.append(voicePipelineSessionTraceSummary(trace));
    }
    return summaries;
}

QString voicePipelineSessionSafetySummary(const VoicePipelineSessionSafetyReport& report) {
    if (!report.summary.trimmed().isEmpty()) {
        return report.summary.trimmed();
    }

    return QStringLiteral("Voice pipeline session safety status: %1.").arg(report.status);
}

QStringList voicePipelineSessionSafetyChecks(const VoicePipelineSessionSafetyReport& report) {
    if (!report.checks.isEmpty()) {
        return report.checks;
    }

    return {
        QStringLiteral("Execution attempted: no"),
        QStringLiteral("Microphone capture: blocked"),
        QStringLiteral("Audio playback: blocked"),
        QStringLiteral("Whisper execution: blocked"),
        QStringLiteral("Piper execution: blocked"),
        QStringLiteral("Subprocess execution: blocked"),
        QStringLiteral("Voice chat auto-send and transcript auto-injection: blocked"),
        QStringLiteral("Background workers and autonomous loops: blocked"),
    };
}

QString voicePipelineSessionFallbackSummary(const VoicePipelineSessionFallback& fallback) {
    if (!fallback.summary.trimmed().isEmpty()) {
        return fallback.summary.trimmed();
    }

    return QStringLiteral("Voice pipeline session fallback is metadata only.");
}

QString voicePipelineSessionSummaryText(const VoicePipelineSessionSummary& summary) {
    if (!summary.summary.trimmed().isEmpty()) {
        return summary.summary.trimmed();
    }

    return QStringLiteral("%1 voice pipeline session: %2 ready, %3 blocked, %4 refused, %5 "
                          "traces.")
        .arg(voicePipelineSessionStatusName(summary.status))
        .arg(summary.readyStageCount)
        .arg(summary.blockedStageCount)
        .arg(summary.refusedStageCount)
        .arg(summary.traceCount);
}

VoicePipelineSessionSafetyReport voicePipelineSessionSafetyReport(
    const VoicePipelineSessionPolicy& policy) {
    VoicePipelineSessionSafetyReport report;
    const auto blocked = !policy.enabled || !policy.metadataOnly || !policy.localOnly ||
                         policy.microphoneCaptureAllowed || policy.audioPlaybackAllowed ||
                         policy.whisperExecutionAllowed || policy.piperExecutionAllowed ||
                         policy.subprocessExecutionAllowed || policy.voiceChatAutoSendAllowed ||
                         policy.transcriptAutoInjectionAllowed || policy.backgroundWorkersAllowed ||
                         policy.autonomousLoopsAllowed;
    report.status = blocked ? QStringLiteral("Blocked") : QStringLiteral("Metadata Only");
    report.summary =
        QStringLiteral("Voice pipeline session safety preserves no-execution guarantees: "
                       "execution attempted: no; microphone capture, playback, Whisper, Piper, "
                       "subprocesses, voice chat auto-send, transcript injection, background "
                       "workers, and autonomous loops are blocked.");
    report.executionAllowed = false;
    report.executionAttempted = false;
    report.microphoneCaptureAllowed = false;
    report.audioPlaybackAllowed = false;
    report.whisperExecutionAllowed = false;
    report.piperExecutionAllowed = false;
    report.subprocessExecutionAllowed = false;
    report.voiceChatAutoSendAllowed = false;
    report.transcriptAutoInjectionAllowed = false;
    report.backgroundWorkersAllowed = false;
    report.autonomousLoopsAllowed = false;
    report.checks = voicePipelineSessionSafetyChecks(report);
    return report;
}

VoicePipelineSessionResult buildVoicePipelineSessionResult(
    const VoicePipelineSessionReadiness& transcriptionReadiness,
    const VoicePipelineSessionReadiness& chatInferenceReadiness,
    const VoicePipelineSessionReadiness& synthesisReadiness,
    const VoicePipelineSessionPolicy& policy,
    const VoicePipelineSessionBudget& budget) {
    VoicePipelineSessionResult result;
    result.session.policy = policy;
    result.session.budget = budget;
    result.safetyReport = voicePipelineSessionSafetyReport(policy);
    result.executionAttempted = false;

    const auto appendStep = [&result](VoicePipelineSessionStep step,
                                      VoicePipelineSessionStatus status, bool ready,
                                      const QString& summary) {
        result.steps.append(VoicePipelineSessionStepRecord{step, status, ready, summary});
        result.traces.append(VoicePipelineSessionTrace{
            static_cast<int>(result.traces.size() + 1), step, status, summary});
    };
    const auto appendReadiness = [&appendStep](const VoicePipelineSessionReadiness& readiness) {
        appendStep(readiness.step, readiness.status, readiness.ready, readiness.summary);
    };

    appendStep(VoicePipelineSessionStep::Prepare, VoicePipelineSessionStatus::ReadyMetadata, true,
               QStringLiteral("Prepare records deterministic local metadata only; no devices, "
                              "files, providers, models, workers, or subprocesses are started."));
    appendStep(VoicePipelineSessionStep::AwaitAudioInput,
               VoicePipelineSessionStatus::ReadyMetadata, true,
               QStringLiteral("Await audio input is readiness metadata only; microphone capture "
                              "and live voice activation remain disabled."));

    auto terminalStatus = VoicePipelineSessionStatus::Completed;
    QString terminalSummary;
    appendReadiness(transcriptionReadiness);
    if (!transcriptionReadiness.ready) {
        terminalStatus = transcriptionReadiness.status == VoicePipelineSessionStatus::Refused
                             ? VoicePipelineSessionStatus::Refused
                             : VoicePipelineSessionStatus::Fallback;
        terminalSummary = QStringLiteral("Voice pipeline stopped at transcription readiness; "
                                         "Whisper is not executed and no transcript is injected.");
    } else {
        appendReadiness(chatInferenceReadiness);
        if (!chatInferenceReadiness.ready) {
            terminalStatus = chatInferenceReadiness.status == VoicePipelineSessionStatus::Refused
                                 ? VoicePipelineSessionStatus::Refused
                                 : VoicePipelineSessionStatus::Fallback;
            terminalSummary =
                QStringLiteral("Voice pipeline stopped at chat inference readiness; no voice "
                               "transcript is sent to chat and no model request is started.");
        } else {
            appendReadiness(synthesisReadiness);
            if (!synthesisReadiness.ready) {
                terminalStatus = synthesisReadiness.status == VoicePipelineSessionStatus::Refused
                                     ? VoicePipelineSessionStatus::Refused
                                     : VoicePipelineSessionStatus::Fallback;
                terminalSummary =
                    QStringLiteral("Voice pipeline stopped at synthesis readiness; Piper is not "
                                   "executed and no audio playback is performed.");
            }
        }
    }

    if (terminalStatus == VoicePipelineSessionStatus::Completed) {
        appendStep(VoicePipelineSessionStep::Completion, VoicePipelineSessionStatus::Completed,
                   true,
                   QStringLiteral("Voice pipeline completed readiness orchestration metadata "
                                  "only; no STT, chat auto-send, TTS, playback, or subprocess "
                                  "execution occurred."));
        result.status = VoicePipelineSessionStatus::Completed;
    } else {
        appendStep(VoicePipelineSessionStep::Refusal, VoicePipelineSessionStatus::Refused, false,
                   terminalSummary);
        appendStep(VoicePipelineSessionStep::Fallback, VoicePipelineSessionStatus::Fallback, false,
                   QStringLiteral("Fallback metadata selected: no audio input, no transcript, no "
                                  "chat mutation, no synthesis, and no playback."));
        result.status = terminalStatus;
        result.fallback.status = VoicePipelineSessionStatus::Fallback;
        result.fallback.summary =
            QStringLiteral("%1 Fallback is no transcript, no chat send, no synthesis, and no "
                           "playback.")
                .arg(terminalSummary);
    }

    int readyCount = 0;
    int blockedCount = 0;
    int refusedCount = 0;
    for (const auto& step : result.steps) {
        if (step.ready) {
            ++readyCount;
        }
        if (step.status == VoicePipelineSessionStatus::Blocked ||
            step.status == VoicePipelineSessionStatus::Fallback) {
            ++blockedCount;
        }
        if (step.status == VoicePipelineSessionStatus::Refused) {
            ++refusedCount;
        }
    }

    result.session.status = result.status;
    result.session.summary =
        QStringLiteral("Voice pipeline session %1: local-only, disabled by default, metadata-only, "
                       "execution attempted: no.")
            .arg(voicePipelineSessionStatusName(result.status));
    result.summary.status = result.status;
    result.summary.readyStageCount = readyCount;
    result.summary.blockedStageCount = blockedCount;
    result.summary.refusedStageCount = refusedCount;
    result.summary.traceCount = result.traces.size();
    result.summary.summary =
        QStringLiteral("%1 voice pipeline session: %2 ready stages, %3 blocked/fallback stages, "
                       "%4 refused stages, %5 traces; execution attempted: no; microphone, "
                       "playback, Whisper, Piper, subprocesses, chat auto-send, transcript "
                       "injection, background workers, and autonomous loops are disabled.")
            .arg(voicePipelineSessionStatusName(result.status))
            .arg(readyCount)
            .arg(blockedCount)
            .arg(refusedCount)
            .arg(result.traces.size());
    return result;
}

QString voiceBinaryDescriptorSummary(const VoiceBinaryDescriptor& descriptor) {
    const auto path = descriptor.expectedPath.trimmed().isEmpty()
                          ? QStringLiteral("not configured")
                          : descriptor.expectedPath.trimmed();
    const auto summary = descriptor.summary.trimmed().isEmpty()
                             ? QStringLiteral("No voice binary metadata available.")
                             : descriptor.summary.trimmed();
    return QStringLiteral("%1: %2 for %3 at %4. Executable allowed: %5. %6")
        .arg(descriptor.name, voiceBinaryStatusName(descriptor.status),
             voiceCapabilityName(descriptor.capability), path,
             descriptor.executableAllowed ? QStringLiteral("yes") : QStringLiteral("no"), summary);
}

QStringList voiceBinaryDescriptorSummaries(const QList<VoiceBinaryDescriptor>& descriptors) {
    QStringList summaries;
    for (const auto& descriptor : descriptors) {
        summaries.append(voiceBinaryDescriptorSummary(descriptor));
    }
    return summaries;
}

QString voiceModelDescriptorSummary(const VoiceModelDescriptor& descriptor) {
    const auto path = descriptor.expectedPath.trimmed().isEmpty()
                          ? QStringLiteral("not configured")
                          : descriptor.expectedPath.trimmed();
    const auto summary = descriptor.summary.trimmed().isEmpty()
                             ? QStringLiteral("No voice model metadata available.")
                             : descriptor.summary.trimmed();
    return QStringLiteral("%1: %2 for %3 at %4. Load allowed: %5. %6")
        .arg(descriptor.name, voiceModelStatusName(descriptor.status),
             voiceCapabilityName(descriptor.capability), path,
             descriptor.loadAllowed ? QStringLiteral("yes") : QStringLiteral("no"), summary);
}

QStringList voiceModelDescriptorSummaries(const QList<VoiceModelDescriptor>& descriptors) {
    QStringList summaries;
    for (const auto& descriptor : descriptors) {
        summaries.append(voiceModelDescriptorSummary(descriptor));
    }
    return summaries;
}

QString voiceRuntimePermissionSummary(const VoiceRuntimePermission& permission) {
    const auto summary = permission.summary.trimmed().isEmpty()
                             ? QStringLiteral("No permission metadata available.")
                             : permission.summary.trimmed();
    return QStringLiteral("%1: %2. %3")
        .arg(permission.name,
             permission.granted ? QStringLiteral("Granted") : QStringLiteral("Denied"), summary);
}

QStringList voiceRuntimePermissionSummaries(const QList<VoiceRuntimePermission>& permissions) {
    QStringList summaries;
    for (const auto& permission : permissions) {
        summaries.append(voiceRuntimePermissionSummary(permission));
    }
    return summaries;
}

QString voiceRuntimeSafetySummaryText(const VoiceRuntimeSafetyReport& report) {
    if (!report.summary.trimmed().isEmpty()) {
        return report.summary.trimmed();
    }

    return QStringLiteral("Voice runtime safety status: %1.").arg(report.status);
}

QStringList voiceRuntimeSafetyCheckSummaries(const VoiceRuntimeSafetyReport& report) {
    if (!report.checks.isEmpty()) {
        return report.checks;
    }

    return {
        QStringLiteral("Execution: %1")
            .arg(report.executionAllowed ? QStringLiteral("Allowed") : QStringLiteral("Blocked")),
        QStringLiteral("Microphone: %1")
            .arg(report.microphoneAllowed ? QStringLiteral("Allowed") : QStringLiteral("Blocked")),
        QStringLiteral("Playback: %1")
            .arg(report.playbackAllowed ? QStringLiteral("Allowed") : QStringLiteral("Blocked")),
        QStringLiteral("Process execution: %1")
            .arg(report.processExecutionAllowed ? QStringLiteral("Allowed")
                                                : QStringLiteral("Blocked")),
        QStringLiteral("Filesystem-wide scan: %1")
            .arg(report.filesystemWideScanAllowed ? QStringLiteral("Allowed")
                                                  : QStringLiteral("Blocked")),
        QStringLiteral("Downloads: %1")
            .arg(report.downloadsAllowed ? QStringLiteral("Allowed") : QStringLiteral("Blocked")),
        QStringLiteral("Cloud: %1")
            .arg(report.cloudAllowed ? QStringLiteral("Allowed") : QStringLiteral("Blocked")),
    };
}

WhisperRuntimeDescriptor whisperRuntimeDescriptorFromConfiguration(const QString& binaryPath,
                                                                   const QString& modelPath) {
    const auto binaryConfigured = configured(binaryPath);
    const auto modelConfigured = configured(modelPath);
    const auto refused = unsafeVoiceRuntimePath(binaryPath) || unsafeVoiceRuntimePath(modelPath);
    const auto complete = binaryConfigured && modelConfigured && !refused;

    WhisperRuntimePathSummary pathSummary;
    pathSummary.binaryConfigured = binaryConfigured;
    pathSummary.modelConfigured = modelConfigured;
    pathSummary.localOnlyPathConfigured =
        (binaryConfigured || modelConfigured) && !refused;
    pathSummary.refused = refused;
    pathSummary.configuredCount = (binaryConfigured ? 1 : 0) + (modelConfigured ? 1 : 0);
    pathSummary.missingCount = 2 - pathSummary.configuredCount;
    pathSummary.summary = refused
        ? QStringLiteral("Whisper path summary: refused unsafe/non-local configuration; raw paths "
                         "are not exposed.")
        : QStringLiteral("Whisper path summary: %1; local-only path configured: %2.")
              .arg(configuredMissingSummary(binaryConfigured, modelConfigured),
                   pathSummary.localOnlyPathConfigured ? QStringLiteral("yes")
                                                       : QStringLiteral("no"));

    WhisperRuntimeConfiguration configuration;
    configuration.binaryConfigured = binaryConfigured;
    configuration.modelConfigured = modelConfigured;
    configuration.localOnlyPathConfigured = pathSummary.localOnlyPathConfigured;
    configuration.disabled = true;
    configuration.refused = refused;
    configuration.sandboxRequired = true;
    configuration.futureMicrophoneAccess = true;
    configuration.futureTranscriptionRuntime = true;
    configuration.executionAttempted = false;
    configuration.summary = refused
        ? QStringLiteral("Whisper configuration refused unsafe/non-local paths before runtime "
                         "activation.")
        : QStringLiteral("Whisper configuration is local-only metadata, disabled by default, and "
                         "prepared for future transcription activation only.");

    const auto status = refused ? WhisperRuntimeStatus::Refused
                                : (complete ? WhisperRuntimeStatus::ReadyMetadata
                                            : WhisperRuntimeStatus::MissingConfiguration);
    const auto readiness = refused ? WhisperRuntimeReadiness::Refused
                                   : (complete ? WhisperRuntimeReadiness::ReadyMetadata
                                               : WhisperRuntimeReadiness::MissingConfiguration);

    WhisperRuntimeDescriptor descriptor;
    descriptor.status = status;
    descriptor.readiness = readiness;
    descriptor.configuration = configuration;
    descriptor.pathSummary = pathSummary;
    descriptor.summary =
        QStringLiteral("Whisper runtime [%1/%2]: local-only, disabled by default, not actively "
                       "running, readiness-only; execution attempted: no.")
            .arg(whisperRuntimeStatusName(status), whisperRuntimeReadinessName(readiness));
    return descriptor;
}

PiperRuntimeDescriptor piperRuntimeDescriptorFromConfiguration(const QString& binaryPath,
                                                              const QString& modelPath) {
    const auto binaryConfigured = configured(binaryPath);
    const auto modelConfigured = configured(modelPath);
    const auto refused = unsafeVoiceRuntimePath(binaryPath) || unsafeVoiceRuntimePath(modelPath);
    const auto complete = binaryConfigured && modelConfigured && !refused;

    PiperRuntimePathSummary pathSummary;
    pathSummary.binaryConfigured = binaryConfigured;
    pathSummary.modelConfigured = modelConfigured;
    pathSummary.localOnlyPathConfigured =
        (binaryConfigured || modelConfigured) && !refused;
    pathSummary.refused = refused;
    pathSummary.configuredCount = (binaryConfigured ? 1 : 0) + (modelConfigured ? 1 : 0);
    pathSummary.missingCount = 2 - pathSummary.configuredCount;
    pathSummary.summary = refused
        ? QStringLiteral("Piper path summary: refused unsafe/non-local configuration; raw paths "
                         "are not exposed.")
        : QStringLiteral("Piper path summary: %1; local-only path configured: %2.")
              .arg(configuredMissingSummary(binaryConfigured, modelConfigured),
                   pathSummary.localOnlyPathConfigured ? QStringLiteral("yes")
                                                       : QStringLiteral("no"));

    PiperRuntimeConfiguration configuration;
    configuration.binaryConfigured = binaryConfigured;
    configuration.modelConfigured = modelConfigured;
    configuration.localOnlyPathConfigured = pathSummary.localOnlyPathConfigured;
    configuration.disabled = true;
    configuration.refused = refused;
    configuration.sandboxRequired = true;
    configuration.futureAudioPlayback = true;
    configuration.futureSynthesisRuntime = true;
    configuration.executionAttempted = false;
    configuration.summary = refused
        ? QStringLiteral("Piper configuration refused unsafe/non-local paths before runtime "
                         "activation.")
        : QStringLiteral("Piper configuration is local-only metadata, disabled by default, and "
                         "prepared for future synthesis activation only.");

    const auto status = refused ? PiperRuntimeStatus::Refused
                                : (complete ? PiperRuntimeStatus::ReadyMetadata
                                            : PiperRuntimeStatus::MissingConfiguration);
    const auto readiness = refused ? PiperRuntimeReadiness::Refused
                                   : (complete ? PiperRuntimeReadiness::ReadyMetadata
                                               : PiperRuntimeReadiness::MissingConfiguration);

    PiperRuntimeDescriptor descriptor;
    descriptor.status = status;
    descriptor.readiness = readiness;
    descriptor.configuration = configuration;
    descriptor.pathSummary = pathSummary;
    descriptor.summary =
        QStringLiteral("Piper runtime [%1/%2]: local-only, disabled by default, not actively "
                       "running, readiness-only; execution attempted: no.")
            .arg(piperRuntimeStatusName(status), piperRuntimeReadinessName(readiness));
    return descriptor;
}

VoiceRuntimeReadinessReport voiceRuntimeReadinessReport(
    const WhisperRuntimeDescriptor& whisper, const PiperRuntimeDescriptor& piper) {
    const auto configuredCount = whisper.pathSummary.configuredCount + piper.pathSummary.configuredCount;
    const auto missingCount = whisper.pathSummary.missingCount + piper.pathSummary.missingCount;
    const auto refusedCount = (whisper.configuration.refused ? 1 : 0) +
                              (piper.configuration.refused ? 1 : 0);

    VoiceRuntimeReadinessReport report;
    report.configuredCount = configuredCount;
    report.missingCount = missingCount;
    report.refusedCount = refusedCount;
    report.localOnly = true;
    report.disabledByDefault = true;
    report.executionAttempted = false;
    report.sandbox = refusedCount > 0 ? VoiceRuntimeSandbox::Refused
                                      : VoiceRuntimeSandbox::RequiredMetadata;
    report.readiness = refusedCount > 0
        ? VoiceRuntimeReadiness::Refused
        : (missingCount == 0 ? VoiceRuntimeReadiness::ReadyMetadata
                             : VoiceRuntimeReadiness::MissingConfiguration);
    report.health = refusedCount > 0
        ? VoiceRuntimeHealth::Blocked
        : (missingCount == 0 ? VoiceRuntimeHealth::HealthyMetadata
                             : VoiceRuntimeHealth::DegradedMetadata);
    report.summary =
        QStringLiteral("%1 voice runtime readiness: %2 configured, %3 missing, %4 refused; "
                       "local-only, disabled by default, not actively running, readiness-only, "
                       "execution attempted: no.")
            .arg(voiceRuntimeReadinessName(report.readiness))
            .arg(configuredCount)
            .arg(missingCount)
            .arg(refusedCount);
    report.checks = {
        whisperRuntimeDescriptorSummary(whisper),
        piperRuntimeDescriptorSummary(piper),
        voiceRuntimePermissionFoundationSummary(),
        voiceRuntimeSandboxSummary(report),
        QStringLiteral("No subprocess execution, inference, microphone capture, playback, "
                       "streaming, downloads, cloud/API calls, filesystem scanning, or "
                       "background workers are started."),
    };
    return report;
}

QString whisperRuntimeDescriptorSummary(const WhisperRuntimeDescriptor& descriptor) {
    return QStringLiteral("%1: %2 / %3. %4 %5 %6")
        .arg(descriptor.name, whisperRuntimeStatusName(descriptor.status),
             whisperRuntimeReadinessName(descriptor.readiness), descriptor.pathSummary.summary,
             descriptor.configuration.summary, descriptor.summary);
}

QString whisperRuntimePathSummaryText(const WhisperRuntimePathSummary& summary) {
    return summary.summary;
}

QString piperRuntimeDescriptorSummary(const PiperRuntimeDescriptor& descriptor) {
    return QStringLiteral("%1: %2 / %3. %4 %5 %6")
        .arg(descriptor.name, piperRuntimeStatusName(descriptor.status),
             piperRuntimeReadinessName(descriptor.readiness), descriptor.pathSummary.summary,
             descriptor.configuration.summary, descriptor.summary);
}

QString piperRuntimePathSummaryText(const PiperRuntimePathSummary& summary) {
    return summary.summary;
}

QString voiceRuntimeReadinessSummaryText(const VoiceRuntimeReadinessReport& report) {
    return report.summary;
}

QStringList voiceRuntimeReadinessChecks(const VoiceRuntimeReadinessReport& report) {
    return report.checks;
}

QString voiceRuntimePermissionFoundationSummary() {
    return QStringLiteral("Voice runtime permissions: local-only metadata, disabled, sandbox-"
                          "required; future microphone access, future audio playback, future "
                          "transcription runtime, and future synthesis runtime are not grants.");
}

QString voiceRuntimeSandboxSummary(const VoiceRuntimeReadinessReport& report) {
    return QStringLiteral("Voice runtime sandbox: %1; sandbox requirements are summarized only "
                          "and grant no filesystem, subprocess, microphone, or playback access.")
        .arg(voiceRuntimeSandboxName(report.sandbox));
}

VoiceRuntimeSafetyReport voiceRuntimeSafetyReportForReadiness(
    const VoiceRuntimeReadinessReport& report) {
    VoiceRuntimeSafetyReport safety;
    safety.status = report.readiness == VoiceRuntimeReadiness::Refused
        ? QStringLiteral("Refused")
        : QStringLiteral("Blocked");
    safety.summary =
        QStringLiteral("Voice runtime safety preserves no-runtime-execution guarantees: "
                       "execution attempted: no; configured/missing/refused metadata only.");
    safety.executionAllowed = false;
    safety.executionAttempted = false;
    safety.microphoneAllowed = false;
    safety.playbackAllowed = false;
    safety.processExecutionAllowed = false;
    safety.filesystemWideScanAllowed = false;
    safety.downloadsAllowed = false;
    safety.cloudAllowed = false;
    safety.checks = {
        QStringLiteral("Execution attempted: no"),
        QStringLiteral("Subprocess execution: blocked"),
        QStringLiteral("Inference: blocked"),
        QStringLiteral("Microphone capture: blocked"),
        QStringLiteral("Audio playback: blocked"),
        QStringLiteral("Filesystem scanning: blocked"),
        QStringLiteral("Downloads/cloud/API calls/background workers: blocked"),
    };
    return safety;
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

QString NullVoiceRuntimeEnvironment::status() const {
    return QStringLiteral("Blocked");
}

QString NullVoiceRuntimeEnvironment::summary() const {
    return QStringLiteral("Voice runtime environment is metadata-only: Piper and Whisper binaries "
                          "and models are not configured, execution is blocked, and no filesystem "
                          "scan is performed.");
}

QList<VoiceBinaryDescriptor> NullVoiceRuntimeEnvironment::binaries() const {
    return {
        VoiceBinaryDescriptor{QStringLiteral("piper-binary"), QStringLiteral("Piper Binary"),
                              VoiceCapability::TextToSpeech, VoiceBinaryStatus::Missing,
                              QStringLiteral("not configured"), false,
                              QStringLiteral("Expected future Piper binary path is not configured; "
                                             "Sentinel will not execute Piper.")},
        VoiceBinaryDescriptor{QStringLiteral("whisper-binary"), QStringLiteral("Whisper Binary"),
                              VoiceCapability::SpeechToText, VoiceBinaryStatus::Missing,
                              QStringLiteral("not configured"), false,
                              QStringLiteral("Expected future Whisper binary path is not "
                                             "configured; Sentinel will not execute Whisper.")},
    };
}

QList<VoiceModelDescriptor> NullVoiceRuntimeEnvironment::models() const {
    return {
        VoiceModelDescriptor{QStringLiteral("piper-voice-model"),
                             QStringLiteral("Piper Voice Model"), VoiceCapability::TextToSpeech,
                             VoiceModelStatus::Missing, QStringLiteral("not configured"), false,
                             QStringLiteral("Expected future Piper voice model path is not "
                                            "configured or loaded.")},
        VoiceModelDescriptor{QStringLiteral("whisper-model"), QStringLiteral("Whisper Model"),
                             VoiceCapability::SpeechToText, VoiceModelStatus::Missing,
                             QStringLiteral("not configured"), false,
                             QStringLiteral("Expected future Whisper model path is not configured "
                                            "or loaded.")},
    };
}

QList<VoiceRuntimePermission> NullVoiceRuntimeEnvironment::permissions() const {
    return {
        VoiceRuntimePermission{QStringLiteral("voice.microphone"), QStringLiteral("Microphone"),
                               false, QStringLiteral("Microphone access is not requested.")},
        VoiceRuntimePermission{QStringLiteral("voice.playback"), QStringLiteral("Playback"), false,
                               QStringLiteral("Audio playback is not enabled.")},
        VoiceRuntimePermission{QStringLiteral("voice.process"), QStringLiteral("Process Execution"),
                               false,
                               QStringLiteral("Piper and Whisper subprocess execution is "
                                              "blocked by default.")},
        VoiceRuntimePermission{QStringLiteral("voice.model-read"),
                               QStringLiteral("Voice Model Read"), false,
                               QStringLiteral("Voice model files are not opened or scanned.")},
    };
}

VoiceRuntimeSafetyReport NullVoiceRuntimeEnvironment::safetyReport() const {
    VoiceRuntimeSafetyReport report;
    report.status = QStringLiteral("Blocked");
    report.summary =
        QStringLiteral("Voice runtime safety blocks execution by default: no microphone, playback, "
                       "Piper, Whisper, subprocess, filesystem-wide scan, download, cloud call, or "
                       "API key behavior is allowed.");
    report.checks = voiceRuntimeSafetyCheckSummaries(report);
    return report;
}

StaticVoiceRuntimeEnvironment::StaticVoiceRuntimeEnvironment()
    : StaticVoiceRuntimeEnvironment(NullVoiceRuntimeEnvironment{}.binaries(),
                                    NullVoiceRuntimeEnvironment{}.models()) {}

StaticVoiceRuntimeEnvironment::StaticVoiceRuntimeEnvironment(QList<VoiceBinaryDescriptor> binaries,
                                                             QList<VoiceModelDescriptor> models)
    : binaries_(std::move(binaries)), models_(std::move(models)) {}

QString StaticVoiceRuntimeEnvironment::status() const {
    return safetyReport().status;
}

QString StaticVoiceRuntimeEnvironment::summary() const {
    return QStringLiteral("Voice runtime environment readiness is static metadata only: expected "
                          "Piper/Whisper binary and model paths are described without execution, "
                          "downloads, audio I/O, or filesystem-wide scanning.");
}

QList<VoiceBinaryDescriptor> StaticVoiceRuntimeEnvironment::binaries() const {
    return binaries_;
}

QList<VoiceModelDescriptor> StaticVoiceRuntimeEnvironment::models() const {
    return models_;
}

QList<VoiceRuntimePermission> StaticVoiceRuntimeEnvironment::permissions() const {
    return NullVoiceRuntimeEnvironment{}.permissions();
}

VoiceRuntimeSafetyReport StaticVoiceRuntimeEnvironment::safetyReport() const {
    return NullVoiceRuntimeEnvironment{}.safetyReport();
}

} // namespace sentinel::core
