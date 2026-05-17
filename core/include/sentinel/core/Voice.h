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

QString voiceProviderDescriptorSummary(const VoiceProviderDescriptor& descriptor);
QStringList voiceProviderCapabilitySummaries(const VoiceProviderDescriptor& descriptor);
QString safeVoiceResponseSummary(const VoiceResponse& response);
VoiceReadinessReport buildVoiceReadinessReport(const VoiceProviderDescriptor& textToSpeech,
                                               const VoiceProviderDescriptor& speechToText);

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

} // namespace sentinel::core
