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

} // namespace sentinel::core
