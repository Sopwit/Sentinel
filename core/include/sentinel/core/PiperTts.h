#pragma once

#include "sentinel/core/Voice.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <memory>

namespace sentinel::core {

enum class PiperTtsStatus : std::uint8_t {
    Disabled,
    NotConfigured,
    MissingBinary,
    MissingModel,
    SafetyBlocked,
    Refused,
    Running,
    Configured,
    ReadyMetadata,
    Succeeded,
    Failed,
    Timeout,
};

QString piperTtsStatusName(PiperTtsStatus status);

enum class PiperSynthesisStatus : std::uint8_t {
    Disabled,
    NotConfigured,
    MissingBinary,
    MissingModel,
    UnsafePath,
    SafetyBlocked,
    Refused,
    ReadyMetadata,
    Timeout,
};

QString piperSynthesisStatusName(PiperSynthesisStatus status);

struct PiperSynthesisPolicy {
    bool enabled = false;
    bool localOnly = true;
    bool processExecutionAllowed = false;
    bool audioPlaybackAllowed = false;
    bool liveStreamingAllowed = false;
    bool microphoneCaptureAllowed = false;
    bool cloudAllowed = false;
    bool downloadsAllowed = false;
    bool filesystemScanAllowed = false;
    bool automaticChatInjectionAllowed = false;
    QString summary = QStringLiteral(
        "Piper synthesis policy is disabled, local-only, and metadata-only.");
};

struct PiperSynthesisBudget {
    int timeoutMs = 5000;
    int maxTraceCount = 8;
    int maxSummaryCharacters = 240;
    QString summary = QStringLiteral(
        "Piper synthesis metadata is bounded to 8 traces and 240 summary characters.");
};

struct PiperSynthesisRequest {
    QString text;
    QString languageHint;
    QString voiceHint;
    bool localOnly = true;
    bool allowProcessExecution = false;
    bool allowAudioPlayback = false;
    bool allowLiveStreaming = false;
    bool allowMicrophoneCapture = false;
    bool allowCloud = false;
    bool allowAutomaticChatInjection = false;
    int timeoutMs = 5000;
};

struct PiperSynthesisSession {
    QString id = QStringLiteral("piper-synthesis-session-1");
    PiperSynthesisStatus status = PiperSynthesisStatus::Disabled;
    bool bounded = true;
    bool executionAttempted = false;
    QString summary = QStringLiteral("Piper synthesis session is disabled metadata only.");
};

struct PiperSynthesisReadiness {
    PiperSynthesisStatus status = PiperSynthesisStatus::Disabled;
    bool ready = false;
    int configuredCount = 0;
    int missingCount = 2;
    int refusedCount = 0;
    bool localOnly = true;
    bool executionAttempted = false;
    QString summary = QStringLiteral("Piper synthesis readiness is disabled.");
    QStringList checks;
};

struct PiperSynthesisSafetyReport {
    QString status = QStringLiteral("Blocked");
    QString summary = QStringLiteral("Piper synthesis execution is blocked by default.");
    bool safe = false;
    bool executionAttempted = false;
    bool processExecutionAllowed = false;
    bool audioPlaybackAllowed = false;
    bool liveStreamingAllowed = false;
    bool microphoneCaptureAllowed = false;
    bool cloudAllowed = false;
    bool downloadsAllowed = false;
    bool filesystemScanAllowed = false;
    bool automaticChatInjectionAllowed = false;
    QStringList checks;
};

struct PiperSynthesisFallback {
    PiperSynthesisStatus status = PiperSynthesisStatus::Disabled;
    QString reason = QStringLiteral("disabled");
    QString summary = QStringLiteral("Piper synthesis fell back to no audio.");
};

struct PiperSynthesisTrace {
    QString stage;
    PiperSynthesisStatus status = PiperSynthesisStatus::Disabled;
    bool executionAttempted = false;
    QString summary;
};

struct PiperSynthesisResult {
    PiperSynthesisStatus status = PiperSynthesisStatus::Disabled;
    bool success = false;
    QString audioSummary = QStringLiteral("No audio produced.");
    int timeoutMs = 0;
    bool executionAttempted = false;
    PiperSynthesisSession session;
    PiperSynthesisFallback fallback;
    PiperSynthesisSafetyReport safetyReport;
    QString summary = QStringLiteral("Piper synthesis is disabled.");
    QStringList traces;
};

struct PiperSynthesisConfig {
    PiperSynthesisPolicy policy;
    PiperSynthesisBudget budget;
    VoiceBinaryDescriptor binary;
    VoiceModelDescriptor model;
    QString summary = QStringLiteral("Piper synthesis config is disabled.");
};

QString piperSynthesisReadinessSummary(const PiperSynthesisReadiness& readiness);
QString piperSynthesisSafetySummary(const PiperSynthesisSafetyReport& report);
QString safePiperSynthesisResultSummary(const PiperSynthesisResult& result);
QStringList piperSynthesisTraceSummaries(const QList<PiperSynthesisTrace>& traces);
PiperSynthesisConfig defaultDisabledPiperSynthesisConfig();
PiperSynthesisConfig configuredPiperSynthesisConfig(const QString& binaryPath,
                                                    const QString& modelPath);
PiperSynthesisReadiness piperSynthesisReadiness(const PiperSynthesisConfig& config,
                                                const PiperSynthesisRequest& request);
PiperSynthesisSafetyReport piperSynthesisSafetyReport(const PiperSynthesisPolicy& policy);

class IPiperSynthesisClient {
public:
    virtual ~IPiperSynthesisClient() = default;

    virtual PiperSynthesisStatus status() const = 0;
    virtual QString statusSummary() const = 0;
    virtual PiperSynthesisResult synthesize(const PiperSynthesisRequest& request,
                                            const PiperSynthesisConfig& config) = 0;
};

class NullPiperSynthesisClient final : public IPiperSynthesisClient {
public:
    PiperSynthesisStatus status() const override;
    QString statusSummary() const override;
    PiperSynthesisResult synthesize(const PiperSynthesisRequest& request,
                                    const PiperSynthesisConfig& config) override;
};

class LocalPiperSynthesisClient final : public IPiperSynthesisClient {
public:
    PiperSynthesisStatus status() const override;
    QString statusSummary() const override;
    PiperSynthesisResult synthesize(const PiperSynthesisRequest& request,
                                    const PiperSynthesisConfig& config) override;
};

struct PiperVoiceModelDescriptor {
    QString id = QStringLiteral("piper-voice-model");
    QString name = QStringLiteral("Piper Voice Model");
    VoiceModelStatus status = VoiceModelStatus::Missing;
    QString expectedPath;
    QString language;
    QString speaker;
    bool loadAllowed = false;
    QString summary;
};

struct PiperTtsConfig {
    bool enabled = false;
    bool processExecutionAllowed = false;
    bool fileOutputAllowed = false;
    bool audioPlaybackAllowed = false;
    VoiceBinaryDescriptor binary;
    PiperVoiceModelDescriptor voiceModel;
    VoiceRuntimeSafetyReport safetyReport;
    QString controlledOutputDirectory;
    int timeoutMs = 5000;
    QString summary;
};

struct PiperTtsRequest {
    QString text;
    QString languageHint;
    QString outputPath;
    bool allowProcessExecution = false;
    bool localOnly = true;
    bool allowAudioPlayback = false;
    int timeoutMs = 5000;
};

struct PiperTtsResult {
    PiperTtsStatus status = PiperTtsStatus::Disabled;
    bool success = false;
    QString audioPath;
    QString outputPathSummary;
    int timeoutMs = 0;
    int exitCode = -1;
    QString error;
    QString summary;
    QStringList traces;
};

QString piperVoiceModelDescriptorSummary(const PiperVoiceModelDescriptor& descriptor);
QString piperTtsConfigSummary(const PiperTtsConfig& config);
QString safePiperTtsResultSummary(const PiperTtsResult& result);
QStringList piperTtsConfigCheckSummaries(const PiperTtsConfig& config);

class IPiperTtsClient {
public:
    virtual ~IPiperTtsClient() = default;

    virtual PiperTtsStatus status() const = 0;
    virtual QString statusSummary() const = 0;
    virtual PiperTtsResult synthesize(const PiperTtsRequest& request,
                                      const PiperTtsConfig& config) = 0;
};

class NullPiperTtsClient final : public IPiperTtsClient {
public:
    PiperTtsStatus status() const override;
    QString statusSummary() const override;
    PiperTtsResult synthesize(const PiperTtsRequest& request,
                              const PiperTtsConfig& config) override;
};

class PiperTextToSpeechProvider final : public ITextToSpeechProvider {
public:
    PiperTextToSpeechProvider();
    PiperTextToSpeechProvider(PiperTtsConfig config, std::unique_ptr<IPiperTtsClient> client);

    VoiceProviderDescriptor descriptor() const override;
    VoiceResponse synthesize(const VoiceRequest& request) override;
    QString statusSummary() const override;

    PiperTtsStatus status() const;
    QString piperStatusSummary() const;
    QStringList readinessChecks() const;
    QString fileOutputStatus() const;
    QString fileOutputSummary() const;
    PiperTtsResult synthesizePiper(const PiperTtsRequest& request);
    const PiperTtsConfig& config() const;
    void setConfig(PiperTtsConfig config);

private:
    PiperTtsStatus evaluateStatus() const;

    PiperTtsConfig config_;
    std::unique_ptr<IPiperTtsClient> client_;
};

PiperTtsConfig defaultDisabledPiperTtsConfig();

} // namespace sentinel::core
