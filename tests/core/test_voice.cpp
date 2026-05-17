#include "sentinel/core/Voice.h"

#include <QtTest>

using sentinel::core::buildVoiceReadinessReport;
using sentinel::core::NullSpeechToTextProvider;
using sentinel::core::NullTextToSpeechProvider;
using sentinel::core::safeVoiceResponseSummary;
using sentinel::core::VoiceCapability;
using sentinel::core::VoiceProviderStatus;
using sentinel::core::voiceProviderStatusName;
using sentinel::core::VoiceRequest;
using sentinel::core::VoiceRuntimeMode;
using sentinel::core::voiceRuntimeModeName;

class VoiceTest final : public QObject {
    Q_OBJECT

private slots:
    void nullTextToSpeechRefusesDeterministically();
    void nullSpeechToTextRefusesDeterministically();
    void readinessReportStaysMetadataOnly();
};

void VoiceTest::nullTextToSpeechRefusesDeterministically() {
    NullTextToSpeechProvider provider;

    const auto descriptor = provider.descriptor();
    QCOMPARE(descriptor.id, QStringLiteral("null-tts"));
    QCOMPARE(descriptor.status, VoiceProviderStatus::Disabled);
    QCOMPARE(descriptor.runtimeMode, VoiceRuntimeMode::Disabled);
    QCOMPARE(descriptor.capabilities, QList<VoiceCapability>{VoiceCapability::TextToSpeech});

    const auto response = provider.synthesize(VoiceRequest{QStringLiteral("hello")});
    QCOMPARE(response.status, VoiceProviderStatus::Refused);
    QCOMPARE(response.capability, VoiceCapability::TextToSpeech);
    QVERIFY(!response.available);
    QVERIFY(response.text.isEmpty());
    QCOMPARE(safeVoiceResponseSummary(response),
             QStringLiteral("Text-to-speech is disabled: Sentinel will not synthesize or play "
                            "audio in this phase."));
}

void VoiceTest::nullSpeechToTextRefusesDeterministically() {
    NullSpeechToTextProvider provider;

    const auto descriptor = provider.descriptor();
    QCOMPARE(descriptor.id, QStringLiteral("null-stt"));
    QCOMPARE(descriptor.status, VoiceProviderStatus::Disabled);
    QCOMPARE(descriptor.runtimeMode, VoiceRuntimeMode::Disabled);
    QCOMPARE(descriptor.capabilities, QList<VoiceCapability>{VoiceCapability::SpeechToText});

    const auto response = provider.transcribe(VoiceRequest{});
    QCOMPARE(response.status, VoiceProviderStatus::Refused);
    QCOMPARE(response.capability, VoiceCapability::SpeechToText);
    QVERIFY(!response.available);
    QVERIFY(response.text.isEmpty());
    QCOMPARE(safeVoiceResponseSummary(response),
             QStringLiteral("Speech-to-text is disabled: Sentinel will not record, read, or "
                            "transcribe audio in this phase."));
}

void VoiceTest::readinessReportStaysMetadataOnly() {
    NullTextToSpeechProvider tts;
    NullSpeechToTextProvider stt;

    const auto report = buildVoiceReadinessReport(tts.descriptor(), stt.descriptor());

    QCOMPARE(voiceRuntimeModeName(VoiceRuntimeMode::Disabled), QStringLiteral("Disabled"));
    QCOMPARE(voiceProviderStatusName(VoiceProviderStatus::Disabled), QStringLiteral("Disabled"));
    QCOMPARE(report.status, QStringLiteral("Disabled"));
    QVERIFY(report.summary.contains(QStringLiteral("metadata-only")));
    QVERIFY(report.summary.contains(QStringLiteral("no microphone")));
    QVERIFY(report.summary.contains(QStringLiteral("Piper")));
    QCOMPARE(report.checks.size(), 4);
    QVERIFY(report.checks.at(0).contains(QStringLiteral("Text to speech")));
    QVERIFY(report.checks.at(1).contains(QStringLiteral("Speech to text")));
    QVERIFY(report.checks.at(2).contains(QStringLiteral("No microphone access")));
    QVERIFY(report.checks.at(3).contains(QStringLiteral("No Piper")));
}

QTEST_MAIN(VoiceTest)

#include "test_voice.moc"
