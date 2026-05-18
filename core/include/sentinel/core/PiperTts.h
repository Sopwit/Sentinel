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

class ProcessPiperTtsClient final : public IPiperTtsClient {
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
