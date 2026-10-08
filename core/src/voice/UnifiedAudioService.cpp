// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/voice/UnifiedAudioService.h"
#include "sentinel/core/app/RecoveryService.h"
#include "sentinel/core/runtime/ProcessExecutor.h"
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QPointer>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUrl>
#include <QUuid>
#include <QtConcurrent>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <cstring>
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#include <QPermissions>
#endif

namespace sentinel::core {
QString audioFailureName(AudioFailure value) {
    switch (value) {
    case AudioFailure::None:
        return QStringLiteral("None");
    case AudioFailure::MicrophonePermissionDenied:
        return QStringLiteral("MicrophonePermissionDenied");
    case AudioFailure::AudioDeviceUnavailable:
        return QStringLiteral("AudioDeviceUnavailable");
    case AudioFailure::CaptureFailure:
        return QStringLiteral("CaptureFailure");
    case AudioFailure::ModelUnavailable:
        return QStringLiteral("ModelUnavailable");
    case AudioFailure::RuntimeUnavailable:
        return QStringLiteral("RuntimeUnavailable");
    case AudioFailure::UnsupportedFormat:
        return QStringLiteral("UnsupportedFormat");
    case AudioFailure::TranscriptionFailure:
        return QStringLiteral("TranscriptionFailure");
    case AudioFailure::SynthesisFailure:
        return QStringLiteral("SynthesisFailure");
    case AudioFailure::PlaybackFailure:
        return QStringLiteral("PlaybackFailure");
    case AudioFailure::Cancelled:
        return QStringLiteral("Cancelled");
    case AudioFailure::Timeout:
        return QStringLiteral("Timeout");
    case AudioFailure::ProviderFailure:
        return QStringLiteral("ProviderFailure");
    case AudioFailure::PermissionDenied:
        return QStringLiteral("PermissionDenied");
    case AudioFailure::NoSpeechDetected:
        return QStringLiteral("NoSpeechDetected");
    }
    return QStringLiteral("ProviderFailure");
}
QString audioRuntimeReadinessName(AudioRuntimeReadiness value) {
    switch (value) {
    case AudioRuntimeReadiness::Installed:
        return QStringLiteral("Installed");
    case AudioRuntimeReadiness::ModelAvailable:
        return QStringLiteral("ModelAvailable");
    case AudioRuntimeReadiness::ModelUnavailable:
        return QStringLiteral("ModelUnavailable");
    case AudioRuntimeReadiness::RuntimeAvailable:
        return QStringLiteral("RuntimeAvailable");
    case AudioRuntimeReadiness::DependencyMissing:
        return QStringLiteral("DependencyMissing");
    case AudioRuntimeReadiness::UnsupportedPlatform:
        return QStringLiteral("UnsupportedPlatform");
    case AudioRuntimeReadiness::Ready:
        return QStringLiteral("Ready");
    }
    return QStringLiteral("DependencyMissing");
}
QString voiceInteractionStateName(VoiceInteractionState value) {
    switch (value) {
    case VoiceInteractionState::Idle:
        return QStringLiteral("Idle");
    case VoiceInteractionState::Listening:
        return QStringLiteral("Listening");
    case VoiceInteractionState::Transcribing:
        return QStringLiteral("Transcribing");
    case VoiceInteractionState::Thinking:
        return QStringLiteral("Thinking");
    case VoiceInteractionState::WaitingForApproval:
        return QStringLiteral("WaitingForApproval");
    case VoiceInteractionState::ToolActivity:
        return QStringLiteral("ToolActivity");
    case VoiceInteractionState::Synthesizing:
        return QStringLiteral("Synthesizing");
    case VoiceInteractionState::Speaking:
        return QStringLiteral("Speaking");
    case VoiceInteractionState::Completed:
        return QStringLiteral("Completed");
    case VoiceInteractionState::Failed:
        return QStringLiteral("Failed");
    case VoiceInteractionState::Cancelled:
        return QStringLiteral("Cancelled");
    }
    return QStringLiteral("Failed");
}
namespace {
QAudioDevice inputDevice(const QString& id) {
    for (const auto& device : QMediaDevices::audioInputs()) {
        if (QString::fromUtf8(device.id()) == id) {
            return device;
        }
    }
    return {};
}
QAudioDevice outputDevice(const QString& id) {
    for (const auto& device : QMediaDevices::audioOutputs()) {
        if (QString::fromUtf8(device.id()) == id) {
            return device;
        }
    }
    return {};
}
float sample(const char* data, QAudioFormat::SampleFormat format) {
    if (format == QAudioFormat::UInt8) {
        return static_cast<float>(static_cast<unsigned char>(*data) - 128) / 128.f;
    }
    if (format == QAudioFormat::Int16) {
        const auto raw = qFromLittleEndian<qint16>(reinterpret_cast<const uchar*>(data));
        return static_cast<float>(raw) / 32768.f;
    }
    if (format == QAudioFormat::Int32) {
        const auto raw = qFromLittleEndian<qint32>(reinterpret_cast<const uchar*>(data));
        return static_cast<float>(raw) / 2147483648.f;
    }
    if (format == QAudioFormat::Float) {
        float value = 0;
        std::memcpy(&value, data, sizeof(value));
        return std::clamp(value, -1.f, 1.f);
    }
    return 0;
}
QByteArray wavFile(const QByteArray& pcm) {
    QByteArray wav(44, '\0');
    std::memcpy(wav.data(), "RIFF", 4);
    qToLittleEndian<quint32>(quint32(pcm.size() + 36), reinterpret_cast<uchar*>(wav.data() + 4));
    std::memcpy(wav.data() + 8, "WAVEfmt ", 8);
    qToLittleEndian<quint32>(16, reinterpret_cast<uchar*>(wav.data() + 16));
    qToLittleEndian<quint16>(1, reinterpret_cast<uchar*>(wav.data() + 20));
    qToLittleEndian<quint16>(1, reinterpret_cast<uchar*>(wav.data() + 22));
    qToLittleEndian<quint32>(16000, reinterpret_cast<uchar*>(wav.data() + 24));
    qToLittleEndian<quint32>(32000, reinterpret_cast<uchar*>(wav.data() + 28));
    qToLittleEndian<quint16>(2, reinterpret_cast<uchar*>(wav.data() + 32));
    qToLittleEndian<quint16>(16, reinterpret_cast<uchar*>(wav.data() + 34));
    std::memcpy(wav.data() + 36, "data", 4);
    qToLittleEndian<quint32>(quint32(pcm.size()), reinterpret_cast<uchar*>(wav.data() + 40));
    wav.append(pcm);
    return wav;
}
SpeechTranscript transcribePcm(const std::shared_ptr<ISpeechToTextRuntime>& runtime,
                               const QByteArray& pcm,
                               const std::shared_ptr<std::atomic_bool>& cancellation) {
    const auto root = RecoveryService::captureDirectory();
    if (QFileInfo(root).isSymLink() || !QDir().mkpath(root) || QFileInfo(root).isSymLink()) {
        return SpeechTranscript{.failure = AudioFailure::CaptureFailure,
                                .detail = QStringLiteral("Private capture directory unavailable")};
    }
    if (!QFile::setPermissions(root, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                         QFileDevice::ExeOwner)) {
        return SpeechTranscript{.failure = AudioFailure::CaptureFailure,
                                .detail = QStringLiteral("Private capture directory unavailable")};
    }
    QTemporaryFile file(QDir(root).filePath(QStringLiteral("sentinel-capture-XXXXXX.wav")));
    file.setAutoRemove(true);
    if (!file.open()) {
        return SpeechTranscript{.failure = AudioFailure::CaptureFailure,
                                .detail = QStringLiteral("Private capture file unavailable")};
    }
    const auto wav = wavFile(pcm);
    if (file.write(wav) != wav.size() || !file.flush()) {
        return SpeechTranscript{.failure = AudioFailure::CaptureFailure,
                                .detail = QStringLiteral("Capture file write failed")};
    }
    const auto path = QFileInfo(file.fileName()).canonicalFilePath();
    file.close();
    return runtime->transcribeFile(path, {}, cancellation);
}
struct SpeechProcessOutcome {
    ProcessRecord record;
    QByteArray output;
    bool outputExceeded{false};
};
SpeechProcessOutcome executeSpeechProcess(const QString& binary, const QStringList& arguments,
                                          const QByteArray& input, const QStringList& readablePaths,
                                          const QStringList& writablePaths, int timeoutMs,
                                          const std::shared_ptr<std::atomic_bool>& cancellation,
                                          const QString& boundedOutputFile = {}) {
    SpeechProcessOutcome outcome;
    QTemporaryDir privateDirectory;
    if (!privateDirectory.isValid()) {
        outcome.record.state = ProcessState::Failed;
        outcome.record.error = QStringLiteral("Private speech sandbox unavailable");
        return outcome;
    }
    SandboxExecutionPlan sandbox;
    sandbox.workingDirectory = QFileInfo(binary).absolutePath();
    sandbox.readablePaths = readablePaths;
    sandbox.writablePaths = writablePaths;
    sandbox.temporaryDirectory = privateDirectory.path();
    sandbox.networkAllowed = false;
    sandbox.requireEnforcement = true;
    sandbox.forbidDetachedChildren = true;
    ProcessRequest request;
    request.program = binary;
    request.arguments = arguments;
    request.workingDirectory = sandbox.workingDirectory;
    request.environment = QProcessEnvironment::systemEnvironment();
    request.timeoutMs = std::clamp(timeoutMs, 1000, 120000);
    request.sandbox = sandbox;
    ProcessExecutor executor;
    QEventLoop loop;
    bool done = false;
    QString processId;
    processId = executor.start(
        request,
        [&](const ProcessRecord& record) {
            if (record.state == ProcessState::Running && !input.isEmpty()) {
                executor.write(record.processId, input);
                executor.closeWriteChannel(record.processId);
            }
            if (record.state == ProcessState::Exited || record.state == ProcessState::Failed ||
                record.state == ProcessState::Cancelled) {
                outcome.record = record;
                done = true;
                loop.quit();
            }
        },
        [&](const QString& id, ProcessStream stream, const QByteArray& bytes) {
            if (stream != ProcessStream::Stdout) {
                return;
            }
            if (outcome.output.size() + bytes.size() > 32768) {
                outcome.outputExceeded = true;
                executor.kill(id);
                return;
            }
            outcome.output.append(bytes);
        });
    QTimer cancellationPoll;
    cancellationPoll.setInterval(50);
    QObject::connect(&cancellationPoll, &QTimer::timeout, &loop, [&] {
        if (cancellation && cancellation->load() && !done) {
            executor.kill(processId);
        }
        if (!boundedOutputFile.isEmpty() &&
            QFileInfo(boundedOutputFile).size() > 32LL * 1024 * 1024 && !done) {
            outcome.outputExceeded = true;
            executor.kill(processId);
        }
    });
    cancellationPoll.start();
    if (!done) {
        loop.exec();
    }
    return outcome;
}
} // namespace

SpeechProviderInfo WhisperSttRuntime::info() const {
    SpeechProviderInfo info;
    info.id = QStringLiteral("whisper-stt");
    info.runtimeId = QStringLiteral("whisper.cpp");
    info.modelId = config_.model.expectedPath;
    info.local = true;
    info.cpuFriendly = true;
    info.runtimeAvailable = config_.binary.status == VoiceBinaryStatus::PresentMetadata &&
                            QFileInfo(config_.binary.expectedPath).isExecutable();
    info.modelAvailable = QFileInfo(config_.model.expectedPath).isFile() &&
                          QFileInfo(config_.model.expectedPath).isReadable();
    info.installed = info.runtimeAvailable && info.modelAvailable;
    info.externalDependencyMissing = !info.runtimeAvailable;
    info.readiness = !info.runtimeAvailable ? AudioRuntimeReadiness::DependencyMissing
                     : !info.modelAvailable ? AudioRuntimeReadiness::RuntimeAvailable
                     : config_.policy.processExecutionAllowed
                         ? AudioRuntimeReadiness::Ready
                         : AudioRuntimeReadiness::ModelAvailable;
    return info;
}
SpeechTranscript WhisperSttRuntime::transcribeFile(const QString& path, const QString& language,
                                                   std::shared_ptr<std::atomic_bool> cancellation) {
    SpeechTranscript output;
    output.providerId = QStringLiteral("whisper-stt");
    output.modelId = config_.model.expectedPath;
    output.language = language;
    const auto readiness = info();
    if (readiness.readiness != AudioRuntimeReadiness::Ready) {
        output.failure = readiness.modelAvailable ? AudioFailure::RuntimeUnavailable
                                                  : AudioFailure::ModelUnavailable;
        output.detail = QStringLiteral("Whisper runtime or model unavailable");
        return output;
    }
    const QFileInfo audioFile(path);
    if (!audioFile.isFile() || !audioFile.isReadable() || audioFile.canonicalFilePath() != path ||
        audioFile.size() <= 0 || audioFile.size() > 50LL * 1024 * 1024) {
        output.failure = AudioFailure::UnsupportedFormat;
        output.detail = QStringLiteral("Audio file unreadable");
        return output;
    }
    QStringList arguments{QStringLiteral("-m"), config_.model.expectedPath, QStringLiteral("-f"),
                          path};
    if (!language.trimmed().isEmpty()) {
        arguments.append({QStringLiteral("-l"), language.trimmed()});
    }
    arguments.append(QStringLiteral("-nt"));
    const auto run =
        executeSpeechProcess(config_.binary.expectedPath, arguments, {},
                             {config_.binary.expectedPath, config_.model.expectedPath, path}, {},
                             config_.budget.timeoutMs, cancellation);
    if (cancellation && cancellation->load()) {
        output.failure = AudioFailure::Cancelled;
    } else if (run.record.timedOut) {
        output.failure = AudioFailure::Timeout;
    } else if (run.outputExceeded) {
        output.failure = AudioFailure::TranscriptionFailure;
    } else if (run.record.state != ProcessState::Exited || run.record.exitCode != 0) {
        output.failure = run.record.systemPid == 0 ? AudioFailure::RuntimeUnavailable
                                                   : AudioFailure::TranscriptionFailure;
    } else {
        output.finalText = QString::fromUtf8(run.output).trimmed();
    }
    if (output.failure == AudioFailure::None && output.finalText.isEmpty()) {
        output.failure = AudioFailure::TranscriptionFailure;
    }
    if (output.failure != AudioFailure::None) {
        output.detail = QStringLiteral(
            "Whisper process failed, timed out, was cancelled, or exceeded output bounds");
    }
    return output;
}
SpeechProviderInfo PiperTtsRuntime::info() const {
    SpeechProviderInfo info;
    info.id = QStringLiteral("piper-tts");
    info.runtimeId = QStringLiteral("piper");
    info.modelId = config_.voiceModel.expectedPath;
    info.voices = config_.voiceModel.speaker.isEmpty() ? QStringList{}
                                                       : QStringList{config_.voiceModel.speaker};
    info.languages = config_.voiceModel.language.isEmpty()
                         ? QStringList{}
                         : QStringList{config_.voiceModel.language};
    info.cpuFriendly = true;
    info.runtimeAvailable = config_.binary.status == VoiceBinaryStatus::PresentMetadata &&
                            QFileInfo(config_.binary.expectedPath).isExecutable();
    info.modelAvailable = QFileInfo(config_.voiceModel.expectedPath).isFile() &&
                          QFileInfo(config_.voiceModel.expectedPath).isReadable();
    info.installed = info.runtimeAvailable && info.modelAvailable;
    info.externalDependencyMissing = !info.runtimeAvailable;
    info.readiness = !info.runtimeAvailable ? AudioRuntimeReadiness::DependencyMissing
                     : !info.modelAvailable ? AudioRuntimeReadiness::RuntimeAvailable
                     : config_.processExecutionAllowed && config_.fileOutputAllowed
                         ? AudioRuntimeReadiness::Ready
                         : AudioRuntimeReadiness::ModelAvailable;
    return info;
}
SpeechAudio PiperTtsRuntime::synthesize(const SpeechSynthesisRequest& request,
                                        std::shared_ptr<std::atomic_bool> cancellation) {
    SpeechAudio output;
    output.providerId = QStringLiteral("piper-tts");
    output.runtimeId = QStringLiteral("piper");
    output.modelId = config_.voiceModel.expectedPath;
    output.voiceId = config_.voiceModel.speaker;
    output.language = request.language;
    const auto readiness = info();
    if (readiness.readiness != AudioRuntimeReadiness::Ready) {
        output.failure = readiness.modelAvailable ? AudioFailure::RuntimeUnavailable
                                                  : AudioFailure::ModelUnavailable;
        output.detail = QStringLiteral("Piper runtime or voice model unavailable");
        return output;
    }
    if (request.speed != 1.0 || (!request.voiceId.isEmpty() && request.voiceId != output.voiceId)) {
        output.failure = AudioFailure::ProviderFailure;
        output.detail =
            QStringLiteral("Requested Piper voice or speed is unsupported by this configuration");
        return output;
    }
    if (!QDir().mkpath(config_.controlledOutputDirectory)) {
        output.failure = AudioFailure::SynthesisFailure;
        output.detail = QStringLiteral("Piper output directory unavailable");
        return output;
    }
    const auto outputDirectory = QFileInfo(config_.controlledOutputDirectory).canonicalFilePath();
    if (outputDirectory.isEmpty()) {
        output.failure = AudioFailure::SynthesisFailure;
        output.detail = QStringLiteral("Piper output directory invalid");
        return output;
    }
    const auto path = QDir(outputDirectory)
                          .filePath(QStringLiteral("speech-%1.wav")
                                        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    if (request.text.trimmed().isEmpty() || request.text.size() > 8000) {
        output.failure = AudioFailure::SynthesisFailure;
        output.detail = QStringLiteral("Piper text is empty or exceeds the limit");
        return output;
    }
    QStringList arguments{QStringLiteral("--model"), config_.voiceModel.expectedPath,
                          QStringLiteral("--output_file"), path};
    if (!request.language.trimmed().isEmpty()) {
        arguments.append({QStringLiteral("--language"), request.language.trimmed()});
    }
    if (!config_.voiceModel.speaker.trimmed().isEmpty()) {
        arguments.append({QStringLiteral("--speaker"), config_.voiceModel.speaker.trimmed()});
    }
    QStringList readable{config_.binary.expectedPath, config_.voiceModel.expectedPath};
    const auto voiceConfig = config_.voiceModel.expectedPath + QStringLiteral(".json");
    if (QFileInfo::exists(voiceConfig)) {
        readable.append(voiceConfig);
    }
    const auto run =
        executeSpeechProcess(config_.binary.expectedPath, arguments, request.text.toUtf8(),
                             readable, {outputDirectory}, config_.timeoutMs, cancellation, path);
    if (cancellation && cancellation->load()) {
        output.failure = AudioFailure::Cancelled;
    } else if (run.record.timedOut) {
        output.failure = AudioFailure::Timeout;
    } else if (run.outputExceeded || run.record.state != ProcessState::Exited ||
               run.record.exitCode != 0 || !QFileInfo(path).isFile() ||
               QFileInfo(path).size() > 32LL * 1024 * 1024) {
        output.failure = run.record.systemPid == 0 ? AudioFailure::RuntimeUnavailable
                                                   : AudioFailure::SynthesisFailure;
    } else {
        output.filePath = path;
        output.artifactId = QFileInfo(path).completeBaseName();
    }
    if (output.failure != AudioFailure::None) {
        QFile::remove(path);
        output.detail = QStringLiteral(
            "Piper process failed, timed out, was cancelled, or produced invalid output");
    } else {
        QFile wav(path);
        if (wav.open(QIODevice::ReadOnly)) {
            const auto header = wav.read(44);
            if (header.size() == 44 && header.startsWith("RIFF") &&
                header.mid(8, 4) == QByteArray("WAVE")) {
                const auto bytes = reinterpret_cast<const uchar*>(header.constData());
                output.channels = qFromLittleEndian<quint16>(bytes + 22);
                output.sampleRate = int(qFromLittleEndian<quint32>(bytes + 24));
                const auto byteRate = qFromLittleEndian<quint32>(bytes + 28);
                const auto dataBytes = qFromLittleEndian<quint32>(bytes + 40);
                if (byteRate > 0) {
                    output.durationMs = qint64(dataBytes) * 1000 / byteRate;
                }
            }
        }
    }
    return output;
}
SpeechProviderInfo KokoroTtsRuntime::info() const {
    SpeechProviderInfo info;
    info.id = QStringLiteral("kokoro-tts");
    info.runtimeId = QStringLiteral("kokoro");
    info.modelId = modelPath_;
    const QFileInfo model(modelPath_);
    const QFileInfo voiceAsset(model.absolutePath() + QStringLiteral("/voices-v1.0.bin"));
    info.modelAvailable = model.isFile() && model.isReadable() &&
                          model.suffix() == QLatin1String("onnx") && voiceAsset.isFile() &&
                          voiceAsset.isReadable();
    const auto executable = QStandardPaths::findExecutable(QStringLiteral("kokoro-tts"));
    const QFileInfo runtime(executable);
    info.runtimeAvailable = runtime.isExecutable() && !runtime.canonicalFilePath().isEmpty();
    info.installed = info.runtimeAvailable && info.modelAvailable;
    info.externalDependencyMissing = !info.runtimeAvailable;
    info.cancellable = info.runtimeAvailable;
    info.cpuFriendly = true;
    if (info.modelAvailable && voiceAsset.fileName() == QLatin1String("voices-v1.0.bin")) {
        info.voices = {QStringLiteral("af_alloy"),   QStringLiteral("af_aoede"),
                       QStringLiteral("af_bella"),   QStringLiteral("af_heart"),
                       QStringLiteral("af_jessica"), QStringLiteral("af_kore"),
                       QStringLiteral("af_nicole"),  QStringLiteral("af_nova"),
                       QStringLiteral("af_river"),   QStringLiteral("af_sarah"),
                       QStringLiteral("af_sky"),     QStringLiteral("am_adam"),
                       QStringLiteral("am_echo"),    QStringLiteral("am_eric"),
                       QStringLiteral("am_fenrir"),  QStringLiteral("am_liam"),
                       QStringLiteral("am_michael"), QStringLiteral("am_onyx"),
                       QStringLiteral("am_puck"),    QStringLiteral("bf_alice"),
                       QStringLiteral("bf_emma"),    QStringLiteral("bf_isabella"),
                       QStringLiteral("bf_lily"),    QStringLiteral("bm_daniel"),
                       QStringLiteral("bm_fable"),   QStringLiteral("bm_george"),
                       QStringLiteral("bm_lewis"),   QStringLiteral("ff_siwis"),
                       QStringLiteral("if_sara"),    QStringLiteral("im_nicola"),
                       QStringLiteral("jf_alpha"),   QStringLiteral("jf_gongitsune"),
                       QStringLiteral("jf_nezumi"),  QStringLiteral("jf_tebukuro"),
                       QStringLiteral("jm_kumo"),    QStringLiteral("zf_xiaobei"),
                       QStringLiteral("zf_xiaoni"),  QStringLiteral("zf_xiaoxiao"),
                       QStringLiteral("zf_xiaoyi"),  QStringLiteral("zm_yunjian"),
                       QStringLiteral("zm_yunxi"),   QStringLiteral("zm_yunxia"),
                       QStringLiteral("zm_yunyang")};
        info.languages = {QStringLiteral("en-us"), QStringLiteral("en-gb"), QStringLiteral("fr-fr"),
                          QStringLiteral("it"),    QStringLiteral("ja"),    QStringLiteral("cmn")};
    }
    info.readiness = !info.runtimeAvailable ? AudioRuntimeReadiness::DependencyMissing
                     : !info.modelAvailable ? AudioRuntimeReadiness::ModelUnavailable
                                            : AudioRuntimeReadiness::Ready;
    return info;
}
SpeechAudio KokoroTtsRuntime::synthesize(const SpeechSynthesisRequest& request,
                                         std::shared_ptr<std::atomic_bool> cancellation) {
    SpeechAudio output;
    output.providerId = QStringLiteral("kokoro-tts");
    output.runtimeId = QStringLiteral("kokoro-tts-cli");
    output.modelId = modelPath_;
    output.voiceId = request.voiceId.isEmpty() ? voice_ : request.voiceId;
    output.language = request.language.isEmpty() ? QStringLiteral("en-us") : request.language;
    const auto readiness = info();
    if (!readiness.runtimeAvailable) {
        output.failure = AudioFailure::RuntimeUnavailable;
        output.detail = QStringLiteral("Kokoro CLI is unavailable");
        return output;
    }
    if (!readiness.modelAvailable) {
        output.failure = AudioFailure::ModelUnavailable;
        output.detail = QStringLiteral("Kokoro ONNX model or voices asset is unavailable");
        return output;
    }
    if (request.text.trimmed().isEmpty() || request.text.size() > 8000 ||
        !readiness.voices.contains(output.voiceId) ||
        !readiness.languages.contains(output.language) || !std::isfinite(request.speed) ||
        request.speed < 0.5 || request.speed > 2.0) {
        output.failure = AudioFailure::ProviderFailure;
        output.detail = QStringLiteral("Kokoro text, voice, language, or speed is unsupported");
        return output;
    }
    const auto cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const auto outputDirectory =
        QDir(cache.isEmpty() ? QDir::tempPath() : cache).filePath(QStringLiteral("speech-output"));
    if (!QDir().mkpath(outputDirectory)) {
        output.failure = AudioFailure::SynthesisFailure;
        return output;
    }
    const auto path = QDir(outputDirectory)
                          .filePath(QStringLiteral("speech-%1.wav")
                                        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    const auto binary =
        QFileInfo(QStandardPaths::findExecutable(QStringLiteral("kokoro-tts"))).canonicalFilePath();
    const auto runtimeRoot =
        QDir(QFileInfo(binary).absolutePath()).absoluteFilePath(QStringLiteral(".."));
    const auto voiceAsset =
        QFileInfo(modelPath_).absolutePath() + QStringLiteral("/voices-v1.0.bin");
    const QStringList arguments{QStringLiteral("-"),        path,
                                QStringLiteral("--model"),  modelPath_,
                                QStringLiteral("--voices"), voiceAsset,
                                QStringLiteral("--voice"),  output.voiceId,
                                QStringLiteral("--lang"),   output.language,
                                QStringLiteral("--speed"),  QString::number(request.speed, 'f', 2),
                                QStringLiteral("--format"), QStringLiteral("wav")};
    QStringList readable{binary, modelPath_, voiceAsset};
    if (!QDir(runtimeRoot).isRoot() && runtimeRoot != QDir::homePath()) {
        readable.append(runtimeRoot);
    }
    const auto run = executeSpeechProcess(binary, arguments, request.text.toUtf8(), readable,
                                          {outputDirectory}, 120000, cancellation, path);
    if (cancellation && cancellation->load()) {
        output.failure = AudioFailure::Cancelled;
    } else if (run.record.timedOut) {
        output.failure = AudioFailure::Timeout;
    } else if (run.outputExceeded || run.record.state != ProcessState::Exited ||
               run.record.exitCode != 0 || !QFileInfo(path).isFile() ||
               QFileInfo(path).size() > 32LL * 1024 * 1024) {
        output.failure = AudioFailure::SynthesisFailure;
    }
    if (output.failure != AudioFailure::None) {
        QFile::remove(path);
        output.detail = QStringLiteral(
            "Kokoro process failed, timed out, was cancelled, or produced invalid output");
        return output;
    }
    QFile wav(path);
    if (!wav.open(QIODevice::ReadOnly)) {
        QFile::remove(path);
        output.failure = AudioFailure::UnsupportedFormat;
        return output;
    }
    const auto header = wav.read(44);
    if (header.size() != 44 || !header.startsWith("RIFF") ||
        header.mid(8, 4) != QByteArray("WAVE")) {
        wav.close();
        QFile::remove(path);
        output.failure = AudioFailure::UnsupportedFormat;
        return output;
    }
    const auto bytes = reinterpret_cast<const uchar*>(header.constData());
    output.channels = qFromLittleEndian<quint16>(bytes + 22);
    output.sampleRate = int(qFromLittleEndian<quint32>(bytes + 24));
    const auto byteRate = qFromLittleEndian<quint32>(bytes + 28);
    const auto dataBytes = qFromLittleEndian<quint32>(bytes + 40);
    if (byteRate > 0) {
        output.durationMs = qint64(dataBytes) * 1000 / byteRate;
    }
    output.filePath = path;
    output.artifactId = QFileInfo(path).completeBaseName();
    return output;
}

AudioDeviceService::AudioDeviceService(QObject* parent) : QObject(parent) {
    connect(&devices_, &QMediaDevices::audioInputsChanged, this,
            &AudioDeviceService::inputDevicesChanged);
    connect(&devices_, &QMediaDevices::audioOutputsChanged, this,
            &AudioDeviceService::outputDevicesChanged);
}
void AudioDeviceService::requestMicrophonePermission() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    if (auto* app = QCoreApplication::instance()) {
        app->requestPermission(QMicrophonePermission{}, this,
                               [this](const QPermission& permission) {
                                   emit microphonePermissionChanged(permission.status());
                               });
    }
#else
    emit microphonePermissionChanged(Qt::PermissionStatus::Undetermined);
#endif
}
QStringList AudioDeviceService::inputDeviceIds() const {
    QStringList ids;
    for (const auto& device : QMediaDevices::audioInputs()) {
        ids.append(QString::fromUtf8(device.id()));
    }
    return ids;
}
QList<AudioDeviceDescriptor> AudioDeviceService::inputDevices() const {
    QList<AudioDeviceDescriptor> devices;
    const auto defaultId = QMediaDevices::defaultAudioInput().id();
    for (const auto& device : QMediaDevices::audioInputs()) {
        devices.append(
            {QString::fromUtf8(device.id()), device.description(), device.id() == defaultId});
    }
    return devices;
}
QStringList AudioDeviceService::outputDeviceIds() const {
    QStringList ids;
    for (const auto& device : QMediaDevices::audioOutputs()) {
        ids.append(QString::fromUtf8(device.id()));
    }
    return ids;
}
QList<AudioDeviceDescriptor> AudioDeviceService::outputDevices() const {
    QList<AudioDeviceDescriptor> devices;
    const auto defaultId = QMediaDevices::defaultAudioOutput().id();
    for (const auto& device : QMediaDevices::audioOutputs()) {
        devices.append(
            {QString::fromUtf8(device.id()), device.description(), device.id() == defaultId});
    }
    return devices;
}
bool AudioDeviceService::selectInput(const QString& id) {
    if (isCapturing() || (!id.isEmpty() && inputDevice(id).isNull())) {
        return false;
    }
    inputId_ = id;
    return true;
}
bool AudioDeviceService::selectOutput(const QString& id) {
    if (!id.isEmpty() && outputDevice(id).isNull()) {
        return false;
    }
    outputId_ = id;
    return true;
}
bool AudioDeviceService::startCapture() {
    if (isCapturing()) {
        return false;
    }
    failure_ = AudioFailure::None;
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    if (QCoreApplication::instance() &&
        QCoreApplication::instance()->checkPermission(QMicrophonePermission{}) ==
            Qt::PermissionStatus::Denied) {
        failure_ = AudioFailure::MicrophonePermissionDenied;
        emit captureFailed(failure_);
        return false;
    }
#endif
    const auto device =
        inputId_.isEmpty() ? QMediaDevices::defaultAudioInput() : inputDevice(inputId_);
    if (device.isNull()) {
        failure_ = AudioFailure::AudioDeviceUnavailable;
        emit captureFailed(failure_);
        return false;
    }
    QAudioFormat preferred;
    preferred.setSampleRate(16000);
    preferred.setChannelCount(1);
    preferred.setSampleFormat(QAudioFormat::Int16);
    captureFormat_ = device.isFormatSupported(preferred) ? preferred : device.preferredFormat();
    if (captureFormat_.sampleRate() <= 0 || captureFormat_.channelCount() <= 0 ||
        captureFormat_.bytesPerSample() <= 0) {
        failure_ = AudioFailure::UnsupportedFormat;
        emit captureFailed(failure_);
        return false;
    }
    captured_.clear();
    speechDetected_ = false;
    hadSpeech_ = false;
    segmentStartByte_ = 0;
    silenceMs_ = 0;
    voicedMs_ = 0;
    source_ = std::make_unique<QAudioSource>(device, captureFormat_);
    stream_ = source_->start();
    if (!stream_ || source_->error() != QtAudio::NoError) {
        source_.reset();
        stream_ = nullptr;
        failure_ = AudioFailure::CaptureFailure;
        emit captureFailed(failure_);
        return false;
    }
    connect(stream_, &QIODevice::readyRead, this, &AudioDeviceService::readCapture);
    connect(source_.get(), &QAudioSource::stateChanged, this, [this](QtAudio::State state) {
        if (state == QtAudio::StoppedState && source_ && source_->error() != QtAudio::NoError) {
            cancelCapture();
            failure_ = AudioFailure::CaptureFailure;
            emit captureFailed(failure_);
        }
    });
    return true;
}
void AudioDeviceService::readCapture() {
    if (!stream_) {
        return;
    }
    const auto chunk = stream_->readAll();
    if (captured_.size() + chunk.size() > 32LL * 1024 * 1024) {
        cancelCapture();
        failure_ = AudioFailure::CaptureFailure;
        emit captureFailed(failure_);
        return;
    }
    captured_.append(chunk);
    const int stride = captureFormat_.bytesPerFrame();
    const int bytes = captureFormat_.bytesPerSample();
    if (stride <= 0 || bytes <= 0) {
        return;
    }
    if (captured_.size() / stride > 120LL * captureFormat_.sampleRate()) {
        cancelCapture();
        failure_ = AudioFailure::Timeout;
        emit captureFailed(failure_);
        return;
    }
    double energy = 0;
    int frameCount = 0;
    for (int offset = 0; offset + stride <= chunk.size(); offset += stride) {
        double mono = 0;
        for (int channel = 0; channel < captureFormat_.channelCount(); ++channel) {
            mono += sample(chunk.constData() + offset + qsizetype(channel) * bytes,
                           captureFormat_.sampleFormat());
        }
        const double normalized = mono / captureFormat_.channelCount();
        energy += normalized * normalized;
        ++frameCount;
    }
    inputLevel_ = std::clamp(std::sqrt(energy / std::max(1, frameCount)), 0.0, 1.0);
    emit inputLevelChanged(inputLevel_);
    const int durationMs = int(1000LL * frameCount / captureFormat_.sampleRate());
    if (inputLevel_ >= 0.02) {
        voicedMs_ += durationMs;
        silenceMs_ = 0;
        if (!speechDetected_ && voicedMs_ >= 180) {
            speechDetected_ = true;
            hadSpeech_ = true;
            const auto preroll =
                qsizetype(captureFormat_.bytesPerFrame()) * captureFormat_.sampleRate() / 5;
            segmentStartByte_ = std::max(segmentStartByte_, captured_.size() - preroll);
            emit speechStarted();
        }
    } else if (speechDetected_ && vadEnabled_) {
        silenceMs_ += durationMs;
        if (silenceMs_ >= silenceLimitMs_) {
            emit speechSegmentReady(normalizedPcm(segmentStartByte_));
            segmentStartByte_ = captured_.size();
            speechDetected_ = false;
            voicedMs_ = 0;
            silenceMs_ = 0;
            emit speechEnded();
        }
    } else {
        voicedMs_ = 0;
    }
    if (speechDetected_ &&
        captured_.size() - segmentStartByte_ >=
            qsizetype(captureFormat_.bytesPerFrame()) * captureFormat_.sampleRate() * 15) {
        emit speechSegmentReady(normalizedPcm(segmentStartByte_));
        segmentStartByte_ = captured_.size();
    }
}
QByteArray AudioDeviceService::normalizedPcm(qsizetype fromByte) const {
    const int stride = captureFormat_.bytesPerFrame();
    const int bytes = captureFormat_.bytesPerSample();
    if (stride <= 0 || bytes <= 0 || captureFormat_.sampleRate() <= 0) {
        return {};
    }
    const qsizetype startFrame =
        std::clamp(fromByte / stride, qsizetype(0), captured_.size() / stride);
    const qsizetype frames = captured_.size() / stride - startFrame;
    const qsizetype outputFrames = frames * 16000 / captureFormat_.sampleRate();
    if (outputFrames <= 0 || outputFrames > 16LL * 1024 * 1024) {
        return {};
    }
    QByteArray output(outputFrames * 2, '\0');
    for (qsizetype i = 0; i < outputFrames; ++i) {
        const double position = double(i) * captureFormat_.sampleRate() / 16000.0;
        const qsizetype first = std::min(qsizetype(position), frames - 1);
        const qsizetype second = std::min(first + 1, frames - 1);
        auto monoAt = [&](qsizetype index) {
            double value = 0;
            const char* frame = captured_.constData() + (startFrame + index) * stride;
            for (int channel = 0; channel < captureFormat_.channelCount(); ++channel) {
                value += sample(frame + qsizetype(channel) * bytes, captureFormat_.sampleFormat());
            }
            return value / captureFormat_.channelCount();
        };
        const double fraction = position - static_cast<double>(first);
        const double mono = monoAt(first) * (1.0 - fraction) + monoAt(second) * fraction;
        const auto value = qint16(std::clamp(mono, -1.0, 1.0) * 32767);
        qToLittleEndian<qint16>(value, reinterpret_cast<uchar*>(output.data() + i * 2));
    }
    return output;
}
QByteArray AudioDeviceService::stopCapture() {
    if (!source_) {
        return {};
    }
    readCapture();
    if (!source_) {
        return {};
    }
    disconnect(source_.get(), nullptr, this, nullptr);
    source_->stop();
    stream_ = nullptr;
    source_.reset();
    inputLevel_ = 0;
    emit inputLevelChanged(0);
    auto pcm = speechDetected_ ? normalizedPcm(segmentStartByte_) : QByteArray{};
    captured_.clear();
    segmentStartByte_ = 0;
    return pcm;
}
void AudioDeviceService::cancelCapture() {
    if (source_) {
        disconnect(source_.get(), nullptr, this, nullptr);
        source_->stop();
    }
    stream_ = nullptr;
    source_.reset();
    captured_.clear();
    segmentStartByte_ = 0;
    hadSpeech_ = false;
    inputLevel_ = 0;
    speechDetected_ = false;
    emit inputLevelChanged(0);
}

AudioPlaybackService::AudioPlaybackService(AudioDeviceService* devices, QObject* parent)
    : QObject(parent), devices_(devices) {
    player_.setAudioOutput(&output_);
    connect(&player_, &QMediaPlayer::playbackStateChanged, this,
            &AudioPlaybackService::playbackChanged);
    connect(&player_, &QMediaPlayer::positionChanged, this,
            [this](qint64 position) { emit playbackProgress(position, player_.duration()); });
    connect(&player_, &QMediaPlayer::errorOccurred, this, [this] {
        failure_ = AudioFailure::PlaybackFailure;
        emit playbackFailed(failure_);
    });
}
bool AudioPlaybackService::playFile(const QString& path, bool replace) {
    if (isPlaying() && !replace) {
        return false;
    }
    if (!QFileInfo(path).isReadable()) {
        failure_ = AudioFailure::PlaybackFailure;
        emit playbackFailed(failure_);
        return false;
    }
    stop();
    const auto device = devices_->selectedOutputId().isEmpty()
                            ? QMediaDevices::defaultAudioOutput()
                            : outputDevice(devices_->selectedOutputId());
    if (device.isNull()) {
        failure_ = AudioFailure::AudioDeviceUnavailable;
        emit playbackFailed(failure_);
        return false;
    }
    output_.setDevice(device);
    failure_ = AudioFailure::None;
    player_.setSource(QUrl::fromLocalFile(path));
    player_.play();
    return true;
}
void AudioPlaybackService::pause() {
    player_.pause();
}
void AudioPlaybackService::stop() {
    player_.stop();
}
bool AudioPlaybackService::isPlaying() const {
    return player_.playbackState() == QMediaPlayer::PlayingState;
}
bool AudioPlaybackService::isPaused() const {
    return player_.playbackState() == QMediaPlayer::PausedState;
}
qint64 AudioPlaybackService::positionMs() const {
    return player_.position();
}

VoiceSessionService::VoiceSessionService(QObject* parent)
    : QObject(parent), devices_(this), playback_(&devices_, this) {
    connect(&devices_, &AudioDeviceService::speechSegmentReady, this,
            [this](const QByteArray& pcm) {
                if (state_ != VoiceInteractionState::Listening || pcm.isEmpty()) {
                    return;
                }
                if (pendingSegments_.size() >= 8) {
                    devices_.cancelCapture();
                    fail(AudioFailure::Timeout, QStringLiteral("Too many pending speech segments"));
                    return;
                }
                pendingSegments_.enqueue(pcm);
                processNextSegment(generation_);
            });
    connect(&devices_, &AudioDeviceService::captureFailed, this, [this](AudioFailure error) {
        fail(error, QStringLiteral("Microphone capture failed"));
    });
    connect(&playback_, &AudioPlaybackService::playbackFailed, this,
            [this](AudioFailure error) { fail(error, QStringLiteral("Audio playback failed")); });
    connect(&playback_, &AudioPlaybackService::playbackChanged, this, [this] {
        if (state_ == VoiceInteractionState::Speaking && !playback_.isPlaying() &&
            !playback_.isPaused()) {
            if (!generatedAudioPath_.isEmpty()) {
                QFile::remove(generatedAudioPath_);
                generatedAudioPath_.clear();
            }
            setState(VoiceInteractionState::Completed);
        }
    });
}
VoiceSessionService::~VoiceSessionService() {
    try {
        cancel();
    } catch (...) {
        // Destruction must not propagate exceptions from external bridge callbacks.
        if (operationCancellation_) {
            operationCancellation_->store(true);
        }
    }
}
void VoiceSessionService::setSttRuntime(std::shared_ptr<ISpeechToTextRuntime> runtime) {
    if (state_ == VoiceInteractionState::Listening ||
        state_ == VoiceInteractionState::Transcribing) {
        cancel();
    }
    ++sttRevision_;
    stt_ = std::move(runtime);
    emit providersChanged();
}
void VoiceSessionService::setTtsRuntime(std::shared_ptr<ITextToSpeechRuntime> runtime) {
    if (state_ == VoiceInteractionState::Speaking ||
        (state_ == VoiceInteractionState::Synthesizing &&
         mode_ == VoiceInteractionMode::ReadAloud)) {
        cancel();
    }
    tts_ = std::move(runtime);
    emit providersChanged();
}
void VoiceSessionService::setChatBridge(
    std::function<void(const QString&, std::function<void(QString)>)> bridge) {
    chatBridge_ = std::move(bridge);
}
void VoiceSessionService::setAgentBridge(
    std::function<void(const QString&, std::function<void(QString, bool)>)> bridge) {
    agentBridge_ = std::move(bridge);
}
void VoiceSessionService::setBridgeCancellation(std::function<void()> cancelChat,
                                                std::function<void()> cancelAgent) {
    cancelChatBridge_ = std::move(cancelChat);
    cancelAgentBridge_ = std::move(cancelAgent);
}
void VoiceSessionService::setFileAuthorization(
    std::function<std::optional<AuthorizedPath>(const QString&)> authorization,
    QString workingDirectory) {
    fileAuthorization_ = std::move(authorization);
    fileAuthorizationCwd_ = std::move(workingDirectory);
}
SpeechProviderInfo VoiceSessionService::sttInfo() const {
    auto info = stt_ ? stt_->info() : SpeechProviderInfo{};
    info.suitability = hardwareFacts_.logicalCpuCount > 0 && hardwareFacts_.logicalCpuCount < 4
                           ? SpeechHardwareSuitability::ResourceConstrained
                       : info.cpuFriendly ? SpeechHardwareSuitability::CpuFriendly
                                          : SpeechHardwareSuitability::Unknown;
    return info;
}
SpeechProviderInfo VoiceSessionService::ttsInfo() const {
    auto info = tts_ ? tts_->info() : SpeechProviderInfo{};
    info.suitability = hardwareFacts_.logicalCpuCount > 0 && hardwareFacts_.logicalCpuCount < 4
                           ? SpeechHardwareSuitability::ResourceConstrained
                       : info.cpuFriendly ? SpeechHardwareSuitability::CpuFriendly
                                          : SpeechHardwareSuitability::Unknown;
    return info;
}
void VoiceSessionService::setState(VoiceInteractionState state) {
    if (state_ == state) {
        return;
    }
    state_ = state;
    emit stateChanged(state);
}
void VoiceSessionService::fail(AudioFailure error, const QString& detail) {
    cancel();
    failure_ = error;
    privacy_.processingRawAudio = false;
    emit privacyChanged(privacy_);
    setState(VoiceInteractionState::Failed);
    emit failed(error, detail.left(300));
}
bool VoiceSessionService::startPushToTalk(VoiceInteractionMode mode, bool speakAnswer) {
    if (mode != VoiceInteractionMode::Dictation && mode != VoiceInteractionMode::VoiceChat &&
        mode != VoiceInteractionMode::VoiceAgent) {
        return false;
    }
    cancel();
    if (!stt_) {
        fail(AudioFailure::RuntimeUnavailable, QStringLiteral("No STT runtime selected"));
        return false;
    }
    const auto readiness = stt_->info();
    if (readiness.readiness != AudioRuntimeReadiness::Ready) {
        fail(readiness.modelAvailable ? AudioFailure::RuntimeUnavailable
                                      : AudioFailure::ModelUnavailable,
             QStringLiteral("STT runtime is not ready"));
        return false;
    }
    mode_ = mode;
    speakAnswer_ = speakAnswer;
    transcript_ = {};
    transcript_.providerId = readiness.id;
    transcript_.modelId = readiness.modelId;
    pendingSegments_.clear();
    segmentTexts_.clear();
    segmentActive_ = false;
    finalSegmentsRequested_ = false;
    failure_ = AudioFailure::None;
    if (!devices_.startCapture()) {
        fail(devices_.lastFailure(), QStringLiteral("Microphone unavailable"));
        return false;
    }
    privacy_.localOnly = stt_->info().local;
    privacy_.cloudProviderActive = !stt_->info().local;
    privacy_.processingRawAudio = true;
    privacy_.retainRawRecordings = false;
    emit privacyChanged(privacy_);
    setState(VoiceInteractionState::Listening);
    return true;
}
void VoiceSessionService::stopPushToTalk() {
    if (state_ != VoiceInteractionState::Listening) {
        return;
    }
    const auto pcm = devices_.stopCapture();
    const bool speech = devices_.speechDetected();
    if (devices_.lastFailure() != AudioFailure::None) {
        fail(devices_.lastFailure(), QStringLiteral("Microphone capture failed"));
        return;
    }
    setState(VoiceInteractionState::Transcribing);
    if (!speech) {
        fail(AudioFailure::NoSpeechDetected, QStringLiteral("No speech detected"));
        return;
    }
    if (!pcm.isEmpty()) {
        pendingSegments_.enqueue(pcm);
    }
    finalSegmentsRequested_ = true;
    processNextSegment(generation_);
}
bool VoiceSessionService::transcribeCapturedPcm(const QByteArray& pcm, bool speechDetected) {
    cancel();
    if (!stt_ || stt_->info().readiness != AudioRuntimeReadiness::Ready) {
        fail(AudioFailure::RuntimeUnavailable, QStringLiteral("STT runtime is not ready"));
        return false;
    }
    if (pcm.isEmpty() || pcm.size() % 2 != 0 || pcm.size() > 16000LL * 2 * 60) {
        fail(AudioFailure::UnsupportedFormat, QStringLiteral("Invalid or oversized captured PCM"));
        return false;
    }
    if (!speechDetected) {
        fail(AudioFailure::NoSpeechDetected, QStringLiteral("No speech detected"));
        return false;
    }
    mode_ = VoiceInteractionMode::Dictation;
    speakAnswer_ = false;
    transcript_ = {};
    transcript_.providerId = stt_->info().id;
    transcript_.modelId = stt_->info().modelId;
    failure_ = AudioFailure::None;
    privacy_.localOnly = stt_->info().local;
    privacy_.cloudProviderActive = !stt_->info().local;
    privacy_.processingRawAudio = true;
    privacy_.retainRawRecordings = false;
    emit privacyChanged(privacy_);
    pendingSegments_.enqueue(pcm);
    finalSegmentsRequested_ = true;
    setState(VoiceInteractionState::Transcribing);
    processNextSegment(generation_);
    return true;
}
void VoiceSessionService::processNextSegment(quint64 generation) {
    if (generation != generation_ || segmentActive_) {
        return;
    }
    if (pendingSegments_.isEmpty()) {
        if (!finalSegmentsRequested_) {
            return;
        }
        transcript_.partialText.clear();
        transcript_.finalText = segmentTexts_.join(QLatin1Char(' ')).trimmed();
        privacy_.processingRawAudio = false;
        emit privacyChanged(privacy_);
        if (transcript_.finalText.isEmpty()) {
            fail(AudioFailure::TranscriptionFailure, QStringLiteral("Empty transcript"));
            return;
        }
        emit finalTranscriptChanged(transcript_.finalText);
        deliverTranscript(generation);
        return;
    }
    segmentActive_ = true;
    const auto pcm = pendingSegments_.dequeue();
    const auto runtime = stt_;
    if (!operationCancellation_) {
        operationCancellation_ = std::make_shared<std::atomic_bool>(false);
    }
    const auto cancellation = operationCancellation_;
    auto* watcher = new QFutureWatcher<SpeechTranscript>(this);
    connect(watcher, &QFutureWatcher<SpeechTranscript>::finished, this,
            [this, watcher, generation] {
                const auto result = watcher->result();
                watcher->deleteLater();
                if (generation != generation_) {
                    return;
                }
                segmentActive_ = false;
                if (result.failure != AudioFailure::None) {
                    devices_.cancelCapture();
                    fail(result.failure, result.detail);
                    return;
                }
                if (!result.finalText.trimmed().isEmpty()) {
                    segmentTexts_.append(result.finalText.trimmed());
                    transcript_.partialText = segmentTexts_.join(QLatin1Char(' '));
                    emit partialTranscriptChanged(transcript_.partialText);
                }
                processNextSegment(generation);
            });
    watcher->setFuture(QtConcurrent::run(
        [runtime, pcm, cancellation] { return transcribePcm(runtime, pcm, cancellation); }));
}
void VoiceSessionService::processFile(const AuthorizedPath& path, const QString& language) {
    if (!stt_) {
        fail(AudioFailure::RuntimeUnavailable, QStringLiteral("No STT runtime selected"));
        return;
    }
    if (stt_->info().readiness != AudioRuntimeReadiness::Ready) {
        fail(stt_->info().modelAvailable ? AudioFailure::RuntimeUnavailable
                                         : AudioFailure::ModelUnavailable,
             QStringLiteral("STT runtime is not ready"));
        return;
    }
    setState(VoiceInteractionState::Transcribing);
    const auto generation = ++generation_;
    const auto runtime = stt_;
    operationCancellation_ = std::make_shared<std::atomic_bool>(false);
    const auto cancellation = operationCancellation_;
    auto* watcher = new QFutureWatcher<SpeechTranscript>(this);
    connect(watcher, &QFutureWatcher<SpeechTranscript>::finished, this,
            [this, watcher, generation] {
                const auto result = watcher->result();
                watcher->deleteLater();
                if (generation != generation_) {
                    return;
                }
                transcript_ = result;
                privacy_.processingRawAudio = false;
                emit privacyChanged(privacy_);
                if (result.failure != AudioFailure::None) {
                    fail(result.failure, result.detail);
                    return;
                }
                if (result.finalText.trimmed().isEmpty()) {
                    fail(AudioFailure::TranscriptionFailure, QStringLiteral("Empty transcript"));
                    return;
                }
                emit finalTranscriptChanged(result.finalText);
                deliverTranscript(generation);
            });
    const auto* fileSystem = fileSystemService_;
    const auto cwd = fileAuthorizationCwd_;
    watcher->setFuture(QtConcurrent::run([runtime, path, language, cancellation, fileSystem, cwd] {
        if (!fileSystem || path.access != FileSystemAccess::Read) {
            return SpeechTranscript{
                .failure = AudioFailure::PermissionDenied,
                .detail = QStringLiteral("Audio file Read authorization unavailable")};
        }
        const auto verified = fileSystem->revalidateAuthorized(path, cwd);
        const auto currentGrant =
            fileSystem->resolve(path.canonicalPath, cwd, FileSystemAccess::Read);
        if (!verified.ok() || !currentGrant.ok() ||
            currentGrant.value->canonicalPath != path.canonicalPath ||
            currentGrant.value->deviceId != path.deviceId ||
            currentGrant.value->fileId != path.fileId) {
            return SpeechTranscript{.failure = AudioFailure::PermissionDenied,
                                    .detail = QStringLiteral("Audio file authorization expired")};
        }
        const auto file = fileSystem->stat(*verified.value);
        if (!file.ok() || !file.value->regularFile || file.value->size <= 0 ||
            file.value->size > 50LL * 1024 * 1024) {
            return SpeechTranscript{.failure = AudioFailure::UnsupportedFormat,
                                    .detail =
                                        QStringLiteral("Authorized audio file is unavailable")};
        }
        return runtime->transcribeFile(verified.value->canonicalPath, language, cancellation);
    }));
}
void VoiceSessionService::transcribeAudioFile(const QString& path, const QString& language) {
    cancel();
    mode_ = VoiceInteractionMode::AudioTranscription;
    const auto authorized = fileAuthorization_ ? fileAuthorization_(path) : std::nullopt;
    if (!authorized || authorized->access != FileSystemAccess::Read ||
        authorized->canonicalPath.isEmpty() || !fileSystemService_) {
        fail(AudioFailure::PermissionDenied,
             QStringLiteral("Audio file access was not authorized"));
        return;
    }
    privacy_.localOnly = stt_ ? stt_->info().local : true;
    privacy_.cloudProviderActive = stt_ && !stt_->info().local;
    privacy_.processingRawAudio = true;
    emit privacyChanged(privacy_);
    processFile(*authorized, language);
}
void VoiceSessionService::deliverTranscript(quint64 generation) {
    if (transcript_.finalText.isEmpty()) {
        fail(AudioFailure::TranscriptionFailure, QStringLiteral("Empty transcript"));
        return;
    }
    if (mode_ == VoiceInteractionMode::Dictation ||
        mode_ == VoiceInteractionMode::AudioTranscription) {
        setState(VoiceInteractionState::Completed);
        return;
    }
    if (mode_ == VoiceInteractionMode::VoiceChat) {
        setState(VoiceInteractionState::Thinking);
        if (!chatBridge_) {
            fail(AudioFailure::RuntimeUnavailable, QStringLiteral("Chat bridge unavailable"));
            return;
        }
        bridgeActive_ = true;
        QPointer<VoiceSessionService> self(this);
        chatBridge_(transcript_.finalText, [self, generation](QString answer) {
            if (!self) {
                return;
            }
            QMetaObject::invokeMethod(self, [self, generation, answer = std::move(answer)] {
                if (!self) {
                    return;
                }
                auto* service = self.data();
                if (generation != service->generation_) {
                    return;
                }
                service->bridgeActive_ = false;
                if (answer.isEmpty()) {
                    service->fail(AudioFailure::ProviderFailure,
                                  QStringLiteral("Chat response unavailable"));
                    return;
                }
                emit service->answerReady(answer);
                if (service->speakAnswer_) {
                    service->speak(answer, {}, generation);
                } else {
                    service->setState(VoiceInteractionState::Completed);
                }
            });
        });
        return;
    }
    if (!agentBridge_) {
        fail(AudioFailure::RuntimeUnavailable, QStringLiteral("AgentRuntime bridge unavailable"));
        return;
    }
    setState(VoiceInteractionState::Thinking);
    bridgeActive_ = true;
    QPointer<VoiceSessionService> self(this);
    agentBridge_(transcript_.finalText, [self, generation](QString answer, bool approval) {
        if (!self) {
            return;
        }
        QMetaObject::invokeMethod(self, [self, generation, answer = std::move(answer), approval] {
            if (!self) {
                return;
            }
            auto* service = self.data();
            if (generation != service->generation_) {
                return;
            }
            if (approval) {
                service->setState(VoiceInteractionState::WaitingForApproval);
                emit service->approvalRequired();
                return;
            }
            service->bridgeActive_ = false;
            if (answer.isEmpty()) {
                service->fail(AudioFailure::ProviderFailure,
                              QStringLiteral("Agent response unavailable"));
                return;
            }
            emit service->answerReady(answer);
            if (service->speakAnswer_) {
                service->speak(answer, {}, generation);
            } else {
                service->setState(VoiceInteractionState::Completed);
            }
        });
    });
}
// QObject parent owns the watcher; finished callback also schedules deleteLater.
// NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
void VoiceSessionService::speak(const QString& text, const QString& voice, quint64 generation) {
    if (!tts_) {
        fail(AudioFailure::RuntimeUnavailable, QStringLiteral("No TTS runtime selected"));
        return;
    }
    if (tts_->info().readiness != AudioRuntimeReadiness::Ready) {
        fail(tts_->info().modelAvailable ? AudioFailure::RuntimeUnavailable
                                         : AudioFailure::ModelUnavailable,
             QStringLiteral("TTS runtime is not ready"));
        return;
    }
    setState(VoiceInteractionState::Synthesizing);
    const auto runtime = tts_;
    operationCancellation_ = std::make_shared<std::atomic_bool>(false);
    const auto cancellation = operationCancellation_;
    auto* watcher = new QFutureWatcher<SpeechAudio>(this);
    // Until the session accepts playback ownership, the worker and completion callback
    // share cleanup ownership. Destroying the session cannot strand a late artifact.
    auto artifact = std::shared_ptr<QString>(new QString, [](QString* path) {
        if (!path->isEmpty()) {
            QFile::remove(*path);
        }
        delete path;
    });
    connect(watcher, &QFutureWatcher<SpeechAudio>::finished, this,
            [this, watcher, generation, artifact] {
                const auto audio = watcher->result();
                watcher->deleteLater();
                if (generation != generation_) {
                    if (!audio.filePath.isEmpty()) {
                        QFile::remove(audio.filePath);
                    }
                    return;
                }
                if (audio.failure != AudioFailure::None) {
                    fail(audio.failure, audio.detail);
                    return;
                }
                generatedAudioPath_ = audio.filePath;
                artifact->clear();
                if (!playback_.playFile(audio.filePath)) {
                    QFile::remove(generatedAudioPath_);
                    generatedAudioPath_.clear();
                    fail(playback_.lastFailure(), QStringLiteral("Playback failed"));
                    return;
                }
                setState(VoiceInteractionState::Speaking);
            });
    watcher->setFuture(QtConcurrent::run([runtime, text, voice, cancellation, artifact] {
        auto audio =
            runtime->synthesize(SpeechSynthesisRequest{text, voice, {}, 1.0}, cancellation);
        *artifact = audio.filePath;
        return audio;
    }));
}
// NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)
void VoiceSessionService::readAloud(const QString& text, const QString& voice) {
    cancel();
    mode_ = VoiceInteractionMode::ReadAloud;
    if (text.trimmed().isEmpty()) {
        fail(AudioFailure::SynthesisFailure, QStringLiteral("Empty text"));
        return;
    }
    privacy_.localOnly = tts_ ? tts_->info().local : true;
    privacy_.cloudProviderActive = tts_ && !tts_->info().local;
    emit privacyChanged(privacy_);
    setState(VoiceInteractionState::Synthesizing);
    speak(text, voice, ++generation_);
}
void VoiceSessionService::updateAgentActivity(VoiceInteractionState state) {
    if (mode_ != VoiceInteractionMode::VoiceAgent || !bridgeActive_ ||
        (state != VoiceInteractionState::Thinking && state != VoiceInteractionState::ToolActivity &&
         state != VoiceInteractionState::WaitingForApproval)) {
        return;
    }
    setState(state);
}
void VoiceSessionService::cancel() {
    if (operationCancellation_) {
        operationCancellation_->store(true);
    }
    operationCancellation_.reset();
    pendingSegments_.clear();
    segmentTexts_.clear();
    segmentActive_ = false;
    finalSegmentsRequested_ = false;
    transcript_.partialText.clear();
    ++generation_;
    if (state_ != VoiceInteractionState::Idle) {
        setState(VoiceInteractionState::Cancelled);
    }
    if (bridgeActive_) {
        bridgeActive_ = false;
        if (mode_ == VoiceInteractionMode::VoiceChat && cancelChatBridge_) {
            cancelChatBridge_();
        }
        if (mode_ == VoiceInteractionMode::VoiceAgent && cancelAgentBridge_) {
            cancelAgentBridge_();
        }
    }
    devices_.cancelCapture();
    playback_.stop();
    if (!generatedAudioPath_.isEmpty()) {
        QFile::remove(generatedAudioPath_);
        generatedAudioPath_.clear();
    }
    privacy_.processingRawAudio = false;
    emit privacyChanged(privacy_);
}
} // namespace sentinel::core
