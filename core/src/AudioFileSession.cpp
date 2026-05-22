#include "sentinel/core/AudioFileSession.h"

#include <QFileInfo>

#include <algorithm>

namespace sentinel::core {

QString audioFileSessionStatusName(AudioFileSessionStatus status) {
    switch (status) {
    case AudioFileSessionStatus::Disabled:
        return QStringLiteral("disabled");
    case AudioFileSessionStatus::ReadyMetadata:
        return QStringLiteral("ready-metadata");
    case AudioFileSessionStatus::Blocked:
        return QStringLiteral("blocked");
    case AudioFileSessionStatus::Refused:
        return QStringLiteral("refused");
    case AudioFileSessionStatus::Fallback:
        return QStringLiteral("fallback");
    }

    return QStringLiteral("disabled");
}

QString audioFileValidationStatusName(AudioFileValidationStatus status) {
    switch (status) {
    case AudioFileValidationStatus::LocalOnly:
        return QStringLiteral("local-only");
    case AudioFileValidationStatus::SupportedExtension:
        return QStringLiteral("supported-extension");
    case AudioFileValidationStatus::UnsupportedExtension:
        return QStringLiteral("unsupported-extension");
    case AudioFileValidationStatus::EmptyFile:
        return QStringLiteral("empty-file");
    case AudioFileValidationStatus::OversizedFile:
        return QStringLiteral("oversized-file");
    case AudioFileValidationStatus::RefusedPath:
        return QStringLiteral("refused-path");
    case AudioFileValidationStatus::SandboxRequired:
        return QStringLiteral("sandbox-required");
    case AudioFileValidationStatus::FutureTranscriptionReady:
        return QStringLiteral("future-transcription-ready");
    case AudioFileValidationStatus::DisabledByPolicy:
        return QStringLiteral("disabled-by-policy");
    }

    return QStringLiteral("disabled-by-policy");
}

namespace {

QString extensionFromDescriptor(const AudioFileDescriptor& descriptor) {
    const auto hint = descriptor.extensionHint.trimmed().toLower();
    if (!hint.isEmpty()) {
        return hint.startsWith(QLatin1Char('.')) ? hint.mid(1) : hint;
    }

    const auto value = descriptor.pathStyleValue.trimmed();
    if (value.isEmpty()) {
        return {};
    }

    const auto suffix = QFileInfo(value).suffix().trimmed().toLower();
    return suffix;
}

bool unsafeAudioPathStyle(const QString& value) {
    const auto trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    const auto lower = trimmed.toLower();
    return lower.contains(QStringLiteral("://")) || lower.startsWith(QStringLiteral("\\\\"));
}

bool supportedExtension(const QString& extension) {
    return supportedAudioFileExtensions().contains(extension.trimmed().toLower());
}

AudioFileValidation validation(AudioFileValidationStatus status, bool accepted,
                               const QString& summary) {
    return AudioFileValidation{status, accepted, summary};
}

} // namespace

QStringList supportedAudioFileExtensions() {
    return {
        QStringLiteral("wav"),
        QStringLiteral("mp3"),
        QStringLiteral("flac"),
        QStringLiteral("ogg"),
    };
}

QStringList supportedAudioFileExtensionSummaries() {
    QStringList summaries;
    for (const auto& extension : supportedAudioFileExtensions()) {
        summaries.append(
            QStringLiteral("%1: supported future offline STT metadata").arg(extension));
    }
    return summaries;
}

QStringList audioFileSessionSafetyChecks(const AudioFileSessionSafetyReport& report) {
    if (!report.checks.isEmpty()) {
        return report.checks;
    }

    return {
        QStringLiteral("Execution attempted: no"),
        QStringLiteral("File loading: blocked"),
        QStringLiteral("Waveform decoding: blocked"),
        QStringLiteral("Transcription: blocked"),
        QStringLiteral("Playback: blocked"),
        QStringLiteral("Microphone capture: blocked"),
        QStringLiteral("Subprocess execution and filesystem scanning: blocked"),
        QStringLiteral("Automatic ingestion, cloud/API calls, and autonomous loops: blocked"),
    };
}

AudioFileSessionSafetyReport audioFileSessionSafetyReport(const AudioFileSessionPolicy& policy) {
    AudioFileSessionSafetyReport report;
    const auto metadataSafe =
        policy.metadataOnly && policy.localOnly && !policy.fileLoadingAllowed &&
        !policy.waveformDecodingAllowed && !policy.transcriptionAllowed &&
        !policy.playbackAllowed && !policy.microphoneCaptureAllowed &&
        !policy.subprocessExecutionAllowed && !policy.filesystemScanningAllowed &&
        !policy.automaticIngestionAllowed && !policy.cloudAllowed;
    report.status = metadataSafe ? QStringLiteral("Metadata Only") : QStringLiteral("Blocked");
    report.summary =
        QStringLiteral("Audio file session safety preserves no-execution guarantees: execution "
                       "attempted: no; file loading, waveform decoding, transcription, playback, "
                       "microphone capture, subprocess execution, filesystem scanning, automatic "
                       "ingestion, and cloud/API calls are blocked.");
    report.safe = metadataSafe;
    report.executionAttempted = false;
    report.fileLoadingAllowed = false;
    report.waveformDecodingAllowed = false;
    report.transcriptionAllowed = false;
    report.playbackAllowed = false;
    report.microphoneCaptureAllowed = false;
    report.subprocessExecutionAllowed = false;
    report.filesystemScanningAllowed = false;
    report.automaticIngestionAllowed = false;
    report.cloudAllowed = false;
    report.checks = audioFileSessionSafetyChecks(report);
    return report;
}

AudioFileSessionResult buildAudioFileSessionResult(const AudioFileDescriptor& descriptor,
                                                   const AudioFileSessionPolicy& policy,
                                                   const AudioFileBudget& budget) {
    AudioFileSessionResult result;
    result.session.policy = policy;
    result.session.budget = budget;
    result.descriptor = descriptor;
    result.safetyReport = audioFileSessionSafetyReport(policy);
    result.executionAttempted = false;

    const auto appendValidation = [&result](AudioFileValidationStatus status, bool accepted,
                                            const QString& summary) {
        result.validations.append(validation(status, accepted, summary));
        result.traces.append(
            AudioFileTrace{static_cast<int>(result.traces.size() + 1), status, false, summary});
    };

    const auto extension = extensionFromDescriptor(descriptor);
    const auto hasDescriptor = !descriptor.pathStyleValue.trimmed().isEmpty() ||
                               !descriptor.extensionHint.trimmed().isEmpty() ||
                               descriptor.declaredSizeBytes >= 0;

    if (!policy.enabled) {
        appendValidation(AudioFileValidationStatus::DisabledByPolicy, false,
                         QStringLiteral("Audio file sessions are disabled by policy; no file is "
                                        "loaded, decoded, transcribed, played, scanned, or "
                                        "ingested."));
    }

    appendValidation(AudioFileValidationStatus::LocalOnly, descriptor.localOnly,
                     descriptor.localOnly
                         ? QStringLiteral("Audio file session is local-only metadata; no cloud or "
                                          "API call is permitted.")
                         : QStringLiteral("Audio file session refused non-local metadata before "
                                          "any file access."));

    if (descriptor.sandboxRequired) {
        appendValidation(AudioFileValidationStatus::SandboxRequired, false,
                         QStringLiteral("Audio file session requires a future sandbox gate; no "
                                        "filesystem access is attempted in this phase."));
    }

    if (unsafeAudioPathStyle(descriptor.pathStyleValue)) {
        appendValidation(AudioFileValidationStatus::RefusedPath, false,
                         QStringLiteral("Audio file path-style metadata was refused as unsafe or "
                                        "non-local; raw path value is not exposed."));
    }

    if (!extension.isEmpty() && supportedExtension(extension)) {
        appendValidation(AudioFileValidationStatus::SupportedExtension, true,
                         QStringLiteral("%1 metadata is supported for a future controlled offline "
                                        "STT phase.")
                             .arg(extension));
    } else if (!extension.isEmpty()) {
        appendValidation(AudioFileValidationStatus::UnsupportedExtension, false,
                         QStringLiteral("%1 metadata is unsupported; supported future extensions "
                                        "are wav, mp3, flac, and ogg.")
                             .arg(extension));
    }

    if (descriptor.declaredSizeBytes == 0) {
        appendValidation(AudioFileValidationStatus::EmptyFile, false,
                         QStringLiteral("Declared audio file size is empty; fallback metadata is "
                                        "selected without reading the file."));
    } else if (descriptor.declaredSizeBytes > budget.maxMetadataBytes) {
        appendValidation(AudioFileValidationStatus::OversizedFile, false,
                         QStringLiteral("Declared audio file size exceeds the metadata budget; "
                                        "fallback metadata is selected without reading the file."));
    }

    const auto refused = std::any_of(
        result.validations.cbegin(), result.validations.cend(),
        [](const AudioFileValidation& item) {
            return item.status == AudioFileValidationStatus::UnsupportedExtension ||
                   item.status == AudioFileValidationStatus::RefusedPath ||
                   (!item.accepted && item.status == AudioFileValidationStatus::LocalOnly);
        });
    const auto fallback =
        std::any_of(result.validations.cbegin(), result.validations.cend(),
                    [](const AudioFileValidation& item) {
                        return item.status == AudioFileValidationStatus::EmptyFile ||
                               item.status == AudioFileValidationStatus::OversizedFile ||
                               item.status == AudioFileValidationStatus::SandboxRequired ||
                               item.status == AudioFileValidationStatus::DisabledByPolicy;
                    });
    const auto extensionAccepted = std::any_of(
        result.validations.cbegin(), result.validations.cend(),
        [](const AudioFileValidation& item) {
            return item.status == AudioFileValidationStatus::SupportedExtension && item.accepted;
        });

    if (!refused && hasDescriptor && extensionAccepted) {
        appendValidation(AudioFileValidationStatus::FutureTranscriptionReady, true,
                         QStringLiteral("Audio file metadata is ready for a future controlled "
                                        "offline transcription phase; no transcription runs now."));
    }

    int acceptedCount = 0;
    int refusedCount = 0;
    int fallbackCount = 0;
    int supportedCount = 0;
    QStringList checks;
    for (const auto& item : result.validations) {
        if (item.accepted) {
            ++acceptedCount;
        } else if (item.status == AudioFileValidationStatus::UnsupportedExtension ||
                   item.status == AudioFileValidationStatus::RefusedPath ||
                   item.status == AudioFileValidationStatus::LocalOnly ||
                   item.status == AudioFileValidationStatus::DisabledByPolicy) {
            ++refusedCount;
        }
        if (item.status == AudioFileValidationStatus::EmptyFile ||
            item.status == AudioFileValidationStatus::OversizedFile ||
            item.status == AudioFileValidationStatus::SandboxRequired ||
            item.status == AudioFileValidationStatus::DisabledByPolicy) {
            ++fallbackCount;
        }
        if (item.status == AudioFileValidationStatus::SupportedExtension) {
            ++supportedCount;
        }
        checks.append(
            QStringLiteral("%1: %2").arg(audioFileValidationStatusName(item.status), item.summary));
    }

    result.status = !policy.enabled
                        ? AudioFileSessionStatus::Disabled
                        : (refused ? AudioFileSessionStatus::Refused
                                   : (fallback ? AudioFileSessionStatus::Fallback
                                               : AudioFileSessionStatus::ReadyMetadata));
    result.session.status = result.status;
    result.session.summary =
        QStringLiteral("Audio file session %1: local-only, disabled by default, metadata-only, "
                       "execution attempted: no.")
            .arg(audioFileSessionStatusName(result.status));
    result.readiness.status = result.status;
    result.readiness.ready = result.status == AudioFileSessionStatus::ReadyMetadata;
    result.readiness.acceptedCount = acceptedCount;
    result.readiness.refusedCount = refusedCount;
    result.readiness.fallbackCount = fallbackCount;
    result.readiness.executionAttempted = false;
    result.readiness.checks = checks;
    result.readiness.summary =
        QStringLiteral("Audio file readiness %1: %2 accepted, %3 refused, %4 fallback checks; "
                       "supported extensions wav/mp3/flac/ogg; execution attempted: no.")
            .arg(audioFileSessionStatusName(result.status))
            .arg(acceptedCount)
            .arg(refusedCount)
            .arg(fallbackCount);
    result.fallback.status = AudioFileSessionStatus::Fallback;
    result.fallback.reason =
        refused ? QStringLiteral("refused-validation") : QStringLiteral("metadata-fallback");
    result.fallback.summary =
        QStringLiteral("Audio file fallback: no file loading, no waveform decoding, no "
                       "transcription, no playback, no microphone capture, and no subprocess "
                       "execution.");
    result.summary.status = result.status;
    result.summary.validationCount = result.validations.size();
    result.summary.acceptedCount = acceptedCount;
    result.summary.refusedCount = refusedCount;
    result.summary.fallbackCount = fallbackCount;
    result.summary.supportedExtensionCount = supportedCount;
    result.summary.executionAttempted = false;
    result.summary.summary =
        QStringLiteral("%1 audio file session: %2 validations, %3 accepted, %4 refused, %5 "
                       "fallback checks, %6 supported-extension records; local-only metadata; "
                       "execution attempted: no; no file loading, decoding, transcription, "
                       "playback, microphone capture, subprocesses, scanning, or ingestion.")
            .arg(audioFileSessionStatusName(result.status))
            .arg(result.validations.size())
            .arg(acceptedCount)
            .arg(refusedCount)
            .arg(fallbackCount)
            .arg(supportedCount);
    return result;
}

QString audioFileSessionReadinessSummary(const AudioFileSessionReadiness& readiness) {
    if (!readiness.summary.trimmed().isEmpty()) {
        return readiness.summary.trimmed();
    }
    return QStringLiteral("Audio file readiness %1.")
        .arg(audioFileSessionStatusName(readiness.status));
}

QStringList audioFileValidationSummaries(const QList<AudioFileValidation>& validations) {
    QStringList summaries;
    for (const auto& item : validations) {
        summaries.append(
            QStringLiteral("%1: %2").arg(audioFileValidationStatusName(item.status), item.summary));
    }
    return summaries;
}

QStringList audioFileTraceSummaries(const QList<AudioFileTrace>& traces) {
    QStringList summaries;
    for (const auto& trace : traces) {
        summaries.append(QStringLiteral("%1. %2: %3")
                             .arg(trace.sequence)
                             .arg(audioFileValidationStatusName(trace.status), trace.summary));
    }
    return summaries;
}

QString audioFileSessionFallbackSummary(const AudioFileSessionFallback& fallback) {
    return fallback.summary.trimmed().isEmpty()
               ? QStringLiteral("Audio file fallback is metadata only.")
               : fallback.summary.trimmed();
}

QString audioFileSessionSafetySummary(const AudioFileSessionSafetyReport& report) {
    return report.summary.trimmed().isEmpty()
               ? QStringLiteral("Audio file session safety status: %1.").arg(report.status)
               : report.summary.trimmed();
}

QStringList audioFileSessionRefusalSummaries(const AudioFileSessionResult& result) {
    QStringList summaries;
    for (const auto& item : result.validations) {
        if (!item.accepted) {
            summaries.append(QStringLiteral("%1: %2").arg(
                audioFileValidationStatusName(item.status), item.summary));
        }
    }
    return summaries;
}

QString audioFileSessionSummaryText(const AudioFileSessionSummary& summary) {
    if (!summary.summary.trimmed().isEmpty()) {
        return summary.summary.trimmed();
    }
    return QStringLiteral("%1 audio file session: %2 validations.")
        .arg(audioFileSessionStatusName(summary.status))
        .arg(summary.validationCount);
}

} // namespace sentinel::core
