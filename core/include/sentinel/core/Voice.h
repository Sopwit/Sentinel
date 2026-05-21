#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class VoiceCapability : std::uint8_t {
    TextToSpeech,
    SpeechToText,
};

QString voiceCapabilityName(VoiceCapability capability);

enum class VoiceProviderStatus : std::uint8_t {
    Disabled,
    MetadataOnly,
    Unavailable,
    Refused,
};

QString voiceProviderStatusName(VoiceProviderStatus status);

enum class VoiceRuntimeMode : std::uint8_t {
    Disabled,
    MetadataOnly,
    FutureLocal,
};

QString voiceRuntimeModeName(VoiceRuntimeMode mode);

struct VoiceSessionId {
    QString value = QStringLiteral("voice-session-1");
};

enum class VoiceSessionState : std::uint8_t {
    Idle,
    Preparing,
    AwaitingInput,
    TranscribingPlaceholder,
    InferencePlaceholder,
    SynthesisPlaceholder,
    Completed,
    Blocked,
    Error,
};

QString voiceSessionStateName(VoiceSessionState state);

enum class VoicePipelineStage : std::uint8_t {
    Idle,
    Preparing,
    AwaitingInput,
    TranscribingPlaceholder,
    InferencePlaceholder,
    SynthesisPlaceholder,
    Completed,
    Blocked,
    Error,
};

QString voicePipelineStageName(VoicePipelineStage stage);

enum class VoicePipelineStatus : std::uint8_t {
    Pending,
    MetadataOnly,
    Completed,
    Blocked,
    Error,
};

QString voicePipelineStatusName(VoicePipelineStatus status);

enum class VoiceBinaryStatus : std::uint8_t {
    Missing,
    ExpectedPath,
    PresentMetadata,
    Unavailable,
};

QString voiceBinaryStatusName(VoiceBinaryStatus status);

enum class VoiceModelStatus : std::uint8_t {
    Missing,
    ExpectedPath,
    PresentMetadata,
    Unavailable,
};

QString voiceModelStatusName(VoiceModelStatus status);

enum class VoiceRuntimeStatus : std::uint8_t {
    Disabled,
    ReadyMetadata,
    MissingConfiguration,
    Refused,
};

QString voiceRuntimeStatusName(VoiceRuntimeStatus status);

enum class VoiceRuntimeReadiness : std::uint8_t {
    Disabled,
    ReadyMetadata,
    MissingConfiguration,
    Refused,
};

QString voiceRuntimeReadinessName(VoiceRuntimeReadiness readiness);

enum class VoiceRuntimeHealth : std::uint8_t {
    HealthyMetadata,
    DegradedMetadata,
    Blocked,
};

QString voiceRuntimeHealthName(VoiceRuntimeHealth health);

enum class VoiceRuntimeSandbox : std::uint8_t {
    NotRequired,
    RequiredMetadata,
    Refused,
};

QString voiceRuntimeSandboxName(VoiceRuntimeSandbox sandbox);

enum class WhisperRuntimeStatus : std::uint8_t {
    Disabled,
    ReadyMetadata,
    MissingConfiguration,
    Refused,
};

QString whisperRuntimeStatusName(WhisperRuntimeStatus status);

enum class WhisperRuntimeReadiness : std::uint8_t {
    Disabled,
    ReadyMetadata,
    MissingConfiguration,
    Refused,
};

QString whisperRuntimeReadinessName(WhisperRuntimeReadiness readiness);

enum class PiperRuntimeStatus : std::uint8_t {
    Disabled,
    ReadyMetadata,
    MissingConfiguration,
    Refused,
};

QString piperRuntimeStatusName(PiperRuntimeStatus status);

enum class PiperRuntimeReadiness : std::uint8_t {
    Disabled,
    ReadyMetadata,
    MissingConfiguration,
    Refused,
};

QString piperRuntimeReadinessName(PiperRuntimeReadiness readiness);

struct VoiceProviderDescriptor {
    QString id;
    QString name;
    VoiceProviderStatus status = VoiceProviderStatus::Disabled;
    VoiceRuntimeMode runtimeMode = VoiceRuntimeMode::Disabled;
    QList<VoiceCapability> capabilities;
    QString summary;
};

struct VoiceRequest {
    QString text;
    QString languageHint;
    VoiceRuntimeMode runtimeMode = VoiceRuntimeMode::Disabled;
};

struct VoiceResponse {
    VoiceProviderStatus status = VoiceProviderStatus::Refused;
    VoiceCapability capability = VoiceCapability::TextToSpeech;
    QString text;
    bool available = false;
    QString summary;
};

struct VoiceReadinessReport {
    QString status = QStringLiteral("Disabled");
    QString summary = QStringLiteral("Voice readiness is disabled by default.");
    QStringList checks;
};

struct VoiceRuntimeSummary {
    bool runtimeAvailable = false;
    bool textToSpeechAvailable = false;
    bool speechToTextAvailable = false;
    bool microphoneEnabled = false;
    bool playbackEnabled = false;
    bool localOnlyPolicy = true;
    bool processExecutionEnabled = false;
    QString status = QStringLiteral("Unavailable");
    QString summary = QStringLiteral("Voice runtime is unavailable in metadata-only mode.");
    QStringList checks;
};

struct VoiceSession {
    VoiceSessionId id;
    VoiceSessionState state = VoiceSessionState::Idle;
    VoiceRuntimeSummary runtime;
    QString summary = QStringLiteral("Voice session is idle and metadata-only.");
};

struct VoicePipelineTrace {
    VoicePipelineStage stage = VoicePipelineStage::Idle;
    VoicePipelineStatus status = VoicePipelineStatus::Pending;
    QString summary;
};

struct VoicePipelineResult {
    VoiceSession session;
    VoicePipelineStatus status = VoicePipelineStatus::Pending;
    QString summary;
    QList<VoicePipelineTrace> traces;
};

struct VoicePipelineSessionId {
    QString value = QStringLiteral("voice-pipeline-session-1");
};

enum class VoicePipelineSessionStatus : std::uint8_t {
    Disabled,
    ReadyMetadata,
    Blocked,
    Refused,
    Fallback,
    Completed,
};

QString voicePipelineSessionStatusName(VoicePipelineSessionStatus status);

enum class VoicePipelineSessionStep : std::uint8_t {
    Prepare,
    AwaitAudioInput,
    TranscriptionReadiness,
    ChatInferenceReadiness,
    SynthesisReadiness,
    Completion,
    Refusal,
    Fallback,
};

QString voicePipelineSessionStepName(VoicePipelineSessionStep step);

struct VoicePipelineSessionPolicy {
    bool enabled = false;
    bool metadataOnly = true;
    bool localOnly = true;
    bool disabledByDefault = true;
    bool microphoneCaptureAllowed = false;
    bool audioPlaybackAllowed = false;
    bool whisperExecutionAllowed = false;
    bool piperExecutionAllowed = false;
    bool subprocessExecutionAllowed = false;
    bool voiceChatAutoSendAllowed = false;
    bool transcriptAutoInjectionAllowed = false;
    bool backgroundWorkersAllowed = false;
    bool autonomousLoopsAllowed = false;
    QString summary = QStringLiteral(
        "Voice pipeline session policy is disabled, local-only, and metadata-only.");
};

struct VoicePipelineSessionBudget {
    int maxStepCount = 8;
    int maxTraceCount = 8;
    int maxSummaryCharacters = 260;
    QString summary = QStringLiteral(
        "Voice pipeline session metadata is bounded to 8 steps, 8 traces, and 260 summary "
        "characters.");
};

struct VoicePipelineSessionReadiness {
    VoicePipelineSessionStep step = VoicePipelineSessionStep::Prepare;
    VoicePipelineSessionStatus status = VoicePipelineSessionStatus::Disabled;
    bool ready = false;
    QString summary;
};

struct VoicePipelineSessionStepRecord {
    VoicePipelineSessionStep step = VoicePipelineSessionStep::Prepare;
    VoicePipelineSessionStatus status = VoicePipelineSessionStatus::Disabled;
    bool ready = false;
    QString summary;
};

struct VoicePipelineSessionTrace {
    int sequence = 0;
    VoicePipelineSessionStep step = VoicePipelineSessionStep::Prepare;
    VoicePipelineSessionStatus status = VoicePipelineSessionStatus::Disabled;
    QString summary;
};

struct VoicePipelineSessionSafetyReport {
    QString status = QStringLiteral("Blocked");
    QString summary = QStringLiteral(
        "Voice pipeline session execution is blocked by default.");
    bool executionAllowed = false;
    bool executionAttempted = false;
    bool microphoneCaptureAllowed = false;
    bool audioPlaybackAllowed = false;
    bool whisperExecutionAllowed = false;
    bool piperExecutionAllowed = false;
    bool subprocessExecutionAllowed = false;
    bool voiceChatAutoSendAllowed = false;
    bool transcriptAutoInjectionAllowed = false;
    bool backgroundWorkersAllowed = false;
    bool autonomousLoopsAllowed = false;
    QStringList checks;
};

struct VoicePipelineSessionFallback {
    VoicePipelineSessionStatus status = VoicePipelineSessionStatus::Fallback;
    QString summary = QStringLiteral(
        "Voice pipeline fallback is no audio input, no transcript, no chat send, and no "
        "playback.");
};

struct VoicePipelineSessionSummary {
    VoicePipelineSessionStatus status = VoicePipelineSessionStatus::Disabled;
    int readyStageCount = 0;
    int blockedStageCount = 0;
    int refusedStageCount = 0;
    int traceCount = 0;
    QString summary = QStringLiteral("Voice pipeline session is disabled metadata only.");
};

struct VoicePipelineSession {
    VoicePipelineSessionId id;
    VoicePipelineSessionStatus status = VoicePipelineSessionStatus::Disabled;
    VoicePipelineSessionPolicy policy;
    VoicePipelineSessionBudget budget;
    QString summary = QStringLiteral("Voice pipeline session is disabled metadata only.");
};

struct VoicePipelineSessionResult {
    VoicePipelineSession session;
    VoicePipelineSessionStatus status = VoicePipelineSessionStatus::Disabled;
    QList<VoicePipelineSessionStepRecord> steps;
    QList<VoicePipelineSessionTrace> traces;
    VoicePipelineSessionSafetyReport safetyReport;
    VoicePipelineSessionFallback fallback;
    VoicePipelineSessionSummary summary;
    bool executionAttempted = false;
};

struct VoiceBinaryDescriptor {
    QString id;
    QString name;
    VoiceCapability capability = VoiceCapability::TextToSpeech;
    VoiceBinaryStatus status = VoiceBinaryStatus::Missing;
    QString expectedPath;
    bool executableAllowed = false;
    QString summary;
};

struct VoiceModelDescriptor {
    QString id;
    QString name;
    VoiceCapability capability = VoiceCapability::TextToSpeech;
    VoiceModelStatus status = VoiceModelStatus::Missing;
    QString expectedPath;
    bool loadAllowed = false;
    QString summary;
};

struct VoiceRuntimePermission {
    QString id;
    QString name;
    bool granted = false;
    QString summary;
};

struct VoiceRuntimePolicy {
    bool metadataOnly = true;
    bool localOnly = true;
    bool disabledByDefault = true;
    bool executionAllowed = false;
    bool microphoneAllowed = false;
    bool playbackAllowed = false;
    bool transcriptionRuntimeAllowed = false;
    bool synthesisRuntimeAllowed = false;
    QString summary = QStringLiteral(
        "Voice runtime policy is local-only metadata; STT/TTS execution is disabled by default.");
};

struct VoiceRuntimeRestriction {
    QString summary;
    bool restricted = true;
};

struct VoiceRuntimeBudget {
    int maxRuntimeDescriptors = 2;
    int maxPathSummaries = 4;
    int maxPermissionSummaries = 8;
    QString summary = QStringLiteral(
        "Voice runtime readiness is bounded to 2 runtime descriptors, 4 path summaries, and 8 "
        "permission summaries.");
};

struct VoiceRuntimeSafetyReport {
    QString status = QStringLiteral("Blocked");
    QString summary = QStringLiteral("Voice runtime execution is blocked by default.");
    bool executionAllowed = false;
    bool executionAttempted = false;
    bool microphoneAllowed = false;
    bool playbackAllowed = false;
    bool processExecutionAllowed = false;
    bool filesystemWideScanAllowed = false;
    bool downloadsAllowed = false;
    bool cloudAllowed = false;
    QStringList checks;
};

struct VoiceRuntimeReadinessReport {
    VoiceRuntimeReadiness readiness = VoiceRuntimeReadiness::Disabled;
    VoiceRuntimeHealth health = VoiceRuntimeHealth::Blocked;
    VoiceRuntimeSandbox sandbox = VoiceRuntimeSandbox::RequiredMetadata;
    int configuredCount = 0;
    int missingCount = 4;
    int refusedCount = 0;
    bool localOnly = true;
    bool disabledByDefault = true;
    bool executionAttempted = false;
    QString summary = QStringLiteral("Voice runtime readiness is disabled metadata only.");
    QStringList checks;
};

struct WhisperRuntimePathSummary {
    bool binaryConfigured = false;
    bool modelConfigured = false;
    bool localOnlyPathConfigured = false;
    bool refused = false;
    int configuredCount = 0;
    int missingCount = 2;
    QString summary = QStringLiteral("Whisper paths are missing.");
};

struct WhisperRuntimeConfiguration {
    bool binaryConfigured = false;
    bool modelConfigured = false;
    bool localOnlyPathConfigured = false;
    bool disabled = true;
    bool refused = false;
    bool sandboxRequired = true;
    bool futureMicrophoneAccess = true;
    bool futureTranscriptionRuntime = true;
    bool executionAttempted = false;
    QString summary;
};

struct WhisperRuntimeDescriptor {
    QString id = QStringLiteral("whisper-runtime");
    QString name = QStringLiteral("Whisper Runtime");
    WhisperRuntimeStatus status = WhisperRuntimeStatus::Disabled;
    WhisperRuntimeReadiness readiness = WhisperRuntimeReadiness::Disabled;
    WhisperRuntimeConfiguration configuration;
    WhisperRuntimePathSummary pathSummary;
    QString summary;
};

struct PiperRuntimePathSummary {
    bool binaryConfigured = false;
    bool modelConfigured = false;
    bool localOnlyPathConfigured = false;
    bool refused = false;
    int configuredCount = 0;
    int missingCount = 2;
    QString summary = QStringLiteral("Piper paths are missing.");
};

struct PiperRuntimeConfiguration {
    bool binaryConfigured = false;
    bool modelConfigured = false;
    bool localOnlyPathConfigured = false;
    bool disabled = true;
    bool refused = false;
    bool sandboxRequired = true;
    bool futureAudioPlayback = true;
    bool futureSynthesisRuntime = true;
    bool executionAttempted = false;
    QString summary;
};

struct PiperRuntimeDescriptor {
    QString id = QStringLiteral("piper-runtime");
    QString name = QStringLiteral("Piper Runtime");
    PiperRuntimeStatus status = PiperRuntimeStatus::Disabled;
    PiperRuntimeReadiness readiness = PiperRuntimeReadiness::Disabled;
    PiperRuntimeConfiguration configuration;
    PiperRuntimePathSummary pathSummary;
    QString summary;
};

QString voiceProviderDescriptorSummary(const VoiceProviderDescriptor& descriptor);
QStringList voiceProviderCapabilitySummaries(const VoiceProviderDescriptor& descriptor);
QString safeVoiceResponseSummary(const VoiceResponse& response);
VoiceReadinessReport buildVoiceReadinessReport(const VoiceProviderDescriptor& textToSpeech,
                                               const VoiceProviderDescriptor& speechToText);
QString voiceRuntimeSummaryText(const VoiceRuntimeSummary& summary);
QStringList voiceRuntimeCheckSummaries(const VoiceRuntimeSummary& summary);
QString voicePipelineTraceSummary(const VoicePipelineTrace& trace);
QStringList voicePipelineTraceSummaries(const QList<VoicePipelineTrace>& traces);
QString safeVoicePipelineSummary(const VoicePipelineResult& result);
QString voicePipelineSessionReadinessSummary(const VoicePipelineSessionReadiness& readiness);
QString voicePipelineSessionStepSummary(const VoicePipelineSessionStepRecord& step);
QStringList voicePipelineSessionStepSummaries(
    const QList<VoicePipelineSessionStepRecord>& steps);
QString voicePipelineSessionTraceSummary(const VoicePipelineSessionTrace& trace);
QStringList voicePipelineSessionTraceSummaries(
    const QList<VoicePipelineSessionTrace>& traces);
QString voicePipelineSessionSafetySummary(const VoicePipelineSessionSafetyReport& report);
QStringList voicePipelineSessionSafetyChecks(const VoicePipelineSessionSafetyReport& report);
QString voicePipelineSessionFallbackSummary(const VoicePipelineSessionFallback& fallback);
QString voicePipelineSessionSummaryText(const VoicePipelineSessionSummary& summary);
VoicePipelineSessionSafetyReport voicePipelineSessionSafetyReport(
    const VoicePipelineSessionPolicy& policy = VoicePipelineSessionPolicy{});
VoicePipelineSessionResult buildVoicePipelineSessionResult(
    const VoicePipelineSessionReadiness& transcriptionReadiness,
    const VoicePipelineSessionReadiness& chatInferenceReadiness,
    const VoicePipelineSessionReadiness& synthesisReadiness,
    const VoicePipelineSessionPolicy& policy = VoicePipelineSessionPolicy{},
    const VoicePipelineSessionBudget& budget = VoicePipelineSessionBudget{});
QString voiceBinaryDescriptorSummary(const VoiceBinaryDescriptor& descriptor);
QStringList voiceBinaryDescriptorSummaries(const QList<VoiceBinaryDescriptor>& descriptors);
QString voiceModelDescriptorSummary(const VoiceModelDescriptor& descriptor);
QStringList voiceModelDescriptorSummaries(const QList<VoiceModelDescriptor>& descriptors);
QString voiceRuntimePermissionSummary(const VoiceRuntimePermission& permission);
QStringList voiceRuntimePermissionSummaries(const QList<VoiceRuntimePermission>& permissions);
QString voiceRuntimeSafetySummaryText(const VoiceRuntimeSafetyReport& report);
QStringList voiceRuntimeSafetyCheckSummaries(const VoiceRuntimeSafetyReport& report);
WhisperRuntimeDescriptor whisperRuntimeDescriptorFromConfiguration(const QString& binaryPath,
                                                                   const QString& modelPath);
PiperRuntimeDescriptor piperRuntimeDescriptorFromConfiguration(const QString& binaryPath,
                                                              const QString& modelPath);
VoiceRuntimeReadinessReport voiceRuntimeReadinessReport(
    const WhisperRuntimeDescriptor& whisper, const PiperRuntimeDescriptor& piper);
QString whisperRuntimeDescriptorSummary(const WhisperRuntimeDescriptor& descriptor);
QString whisperRuntimePathSummaryText(const WhisperRuntimePathSummary& summary);
QString piperRuntimeDescriptorSummary(const PiperRuntimeDescriptor& descriptor);
QString piperRuntimePathSummaryText(const PiperRuntimePathSummary& summary);
QString voiceRuntimeReadinessSummaryText(const VoiceRuntimeReadinessReport& report);
QStringList voiceRuntimeReadinessChecks(const VoiceRuntimeReadinessReport& report);
QString voiceRuntimePermissionFoundationSummary();
QString voiceRuntimeSandboxSummary(const VoiceRuntimeReadinessReport& report);
VoiceRuntimeSafetyReport voiceRuntimeSafetyReportForReadiness(
    const VoiceRuntimeReadinessReport& report);

class ITextToSpeechProvider {
public:
    virtual ~ITextToSpeechProvider() = default;

    virtual VoiceProviderDescriptor descriptor() const = 0;
    virtual VoiceResponse synthesize(const VoiceRequest& request) = 0;
    virtual QString statusSummary() const = 0;
};

class ISpeechToTextProvider {
public:
    virtual ~ISpeechToTextProvider() = default;

    virtual VoiceProviderDescriptor descriptor() const = 0;
    virtual VoiceResponse transcribe(const VoiceRequest& request) = 0;
    virtual QString statusSummary() const = 0;
};

class NullTextToSpeechProvider final : public ITextToSpeechProvider {
public:
    VoiceProviderDescriptor descriptor() const override;
    VoiceResponse synthesize(const VoiceRequest& request) override;
    QString statusSummary() const override;
};

class NullSpeechToTextProvider final : public ISpeechToTextProvider {
public:
    VoiceProviderDescriptor descriptor() const override;
    VoiceResponse transcribe(const VoiceRequest& request) override;
    QString statusSummary() const override;
};

class IVoiceRuntimeCoordinator {
public:
    virtual ~IVoiceRuntimeCoordinator() = default;

    virtual VoiceSession currentSession() const = 0;
    virtual VoiceRuntimeSummary runtimeSummary() const = 0;
    virtual VoicePipelineResult evaluate(VoiceSessionState requestedState) const = 0;
};

class StaticVoiceRuntimeCoordinator final : public IVoiceRuntimeCoordinator {
public:
    VoiceSession currentSession() const override;
    VoiceRuntimeSummary runtimeSummary() const override;
    VoicePipelineResult evaluate(VoiceSessionState requestedState) const override;
};

class IVoiceRuntimeEnvironment {
public:
    virtual ~IVoiceRuntimeEnvironment() = default;

    virtual QString status() const = 0;
    virtual QString summary() const = 0;
    virtual QList<VoiceBinaryDescriptor> binaries() const = 0;
    virtual QList<VoiceModelDescriptor> models() const = 0;
    virtual QList<VoiceRuntimePermission> permissions() const = 0;
    virtual VoiceRuntimeSafetyReport safetyReport() const = 0;
};

class NullVoiceRuntimeEnvironment final : public IVoiceRuntimeEnvironment {
public:
    QString status() const override;
    QString summary() const override;
    QList<VoiceBinaryDescriptor> binaries() const override;
    QList<VoiceModelDescriptor> models() const override;
    QList<VoiceRuntimePermission> permissions() const override;
    VoiceRuntimeSafetyReport safetyReport() const override;
};

class StaticVoiceRuntimeEnvironment final : public IVoiceRuntimeEnvironment {
public:
    StaticVoiceRuntimeEnvironment();
    StaticVoiceRuntimeEnvironment(QList<VoiceBinaryDescriptor> binaries,
                                  QList<VoiceModelDescriptor> models);

    QString status() const override;
    QString summary() const override;
    QList<VoiceBinaryDescriptor> binaries() const override;
    QList<VoiceModelDescriptor> models() const override;
    QList<VoiceRuntimePermission> permissions() const override;
    VoiceRuntimeSafetyReport safetyReport() const override;

private:
    QList<VoiceBinaryDescriptor> binaries_;
    QList<VoiceModelDescriptor> models_;
};

} // namespace sentinel::core
