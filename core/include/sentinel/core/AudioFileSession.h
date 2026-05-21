#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

struct AudioFileSessionId {
    QString value = QStringLiteral("audio-file-session-1");
};

enum class AudioFileSessionStatus : std::uint8_t {
    Disabled,
    ReadyMetadata,
    Blocked,
    Refused,
    Fallback,
};

QString audioFileSessionStatusName(AudioFileSessionStatus status);

enum class AudioFileValidationStatus : std::uint8_t {
    LocalOnly,
    SupportedExtension,
    UnsupportedExtension,
    EmptyFile,
    OversizedFile,
    RefusedPath,
    SandboxRequired,
    FutureTranscriptionReady,
    DisabledByPolicy,
};

QString audioFileValidationStatusName(AudioFileValidationStatus status);

struct AudioFileRestriction {
    QString id;
    QString summary;
    bool active = true;
};

struct AudioFileBudget {
    qint64 maxMetadataBytes = 50LL * 1024LL * 1024LL;
    int maxValidationCount = 8;
    int maxTraceCount = 8;
    int maxSummaryCharacters = 260;
    QString summary = QStringLiteral(
        "Audio file metadata is bounded to 8 validations, 8 traces, and 50 MB declared size.");
};

struct AudioFileSessionPolicy {
    bool enabled = false;
    bool metadataOnly = true;
    bool localOnly = true;
    bool disabledByDefault = true;
    bool fileLoadingAllowed = false;
    bool waveformDecodingAllowed = false;
    bool transcriptionAllowed = false;
    bool playbackAllowed = false;
    bool microphoneCaptureAllowed = false;
    bool subprocessExecutionAllowed = false;
    bool filesystemScanningAllowed = false;
    bool automaticIngestionAllowed = false;
    bool cloudAllowed = false;
    QString summary = QStringLiteral(
        "Audio file session policy is disabled, local-only, and metadata-only.");
};

struct AudioFileDescriptor {
    QString pathStyleValue;
    QString extensionHint;
    qint64 declaredSizeBytes = -1;
    bool sandboxRequired = true;
    bool localOnly = true;
};

struct AudioFileValidation {
    AudioFileValidationStatus status = AudioFileValidationStatus::DisabledByPolicy;
    bool accepted = false;
    QString summary;
};

struct AudioFileSessionReadiness {
    AudioFileSessionStatus status = AudioFileSessionStatus::Disabled;
    bool ready = false;
    int acceptedCount = 0;
    int refusedCount = 0;
    int fallbackCount = 0;
    bool executionAttempted = false;
    QString summary = QStringLiteral("Audio file session readiness is disabled.");
    QStringList checks;
};

struct AudioFileSessionSafetyReport {
    QString status = QStringLiteral("Blocked");
    QString summary = QStringLiteral("Audio file session execution is blocked by default.");
    bool safe = false;
    bool executionAttempted = false;
    bool fileLoadingAllowed = false;
    bool waveformDecodingAllowed = false;
    bool transcriptionAllowed = false;
    bool playbackAllowed = false;
    bool microphoneCaptureAllowed = false;
    bool subprocessExecutionAllowed = false;
    bool filesystemScanningAllowed = false;
    bool automaticIngestionAllowed = false;
    bool cloudAllowed = false;
    QStringList checks;
};

struct AudioFileSessionFallback {
    AudioFileSessionStatus status = AudioFileSessionStatus::Fallback;
    QString reason = QStringLiteral("disabled-by-policy");
    QString summary = QStringLiteral(
        "Audio file fallback is no file loading, no decoding, no transcription, and no playback.");
};

struct AudioFileSessionSummary {
    AudioFileSessionStatus status = AudioFileSessionStatus::Disabled;
    int validationCount = 0;
    int acceptedCount = 0;
    int refusedCount = 0;
    int fallbackCount = 0;
    int supportedExtensionCount = 0;
    bool executionAttempted = false;
    QString summary = QStringLiteral("Audio file session is disabled metadata only.");
};

struct AudioFileTrace {
    int sequence = 0;
    AudioFileValidationStatus status = AudioFileValidationStatus::DisabledByPolicy;
    bool executionAttempted = false;
    QString summary;
};

struct AudioFileSession {
    AudioFileSessionId id;
    AudioFileSessionStatus status = AudioFileSessionStatus::Disabled;
    AudioFileSessionPolicy policy;
    AudioFileBudget budget;
    QString summary = QStringLiteral("Audio file session is disabled metadata only.");
};

struct AudioFileSessionResult {
    AudioFileSession session;
    AudioFileSessionStatus status = AudioFileSessionStatus::Disabled;
    AudioFileDescriptor descriptor;
    QList<AudioFileValidation> validations;
    QList<AudioFileTrace> traces;
    AudioFileSessionReadiness readiness;
    AudioFileSessionSafetyReport safetyReport;
    AudioFileSessionFallback fallback;
    AudioFileSessionSummary summary;
    bool executionAttempted = false;
};

QStringList supportedAudioFileExtensions();
QStringList supportedAudioFileExtensionSummaries();
AudioFileSessionSafetyReport audioFileSessionSafetyReport(
    const AudioFileSessionPolicy& policy = AudioFileSessionPolicy{});
AudioFileSessionResult buildAudioFileSessionResult(
    const AudioFileDescriptor& descriptor = AudioFileDescriptor{},
    const AudioFileSessionPolicy& policy = AudioFileSessionPolicy{},
    const AudioFileBudget& budget = AudioFileBudget{});
QString audioFileSessionReadinessSummary(const AudioFileSessionReadiness& readiness);
QStringList audioFileValidationSummaries(const QList<AudioFileValidation>& validations);
QStringList audioFileTraceSummaries(const QList<AudioFileTrace>& traces);
QString audioFileSessionFallbackSummary(const AudioFileSessionFallback& fallback);
QString audioFileSessionSafetySummary(const AudioFileSessionSafetyReport& report);
QStringList audioFileSessionSafetyChecks(const AudioFileSessionSafetyReport& report);
QStringList audioFileSessionRefusalSummaries(const AudioFileSessionResult& result);
QString audioFileSessionSummaryText(const AudioFileSessionSummary& summary);

} // namespace sentinel::core
