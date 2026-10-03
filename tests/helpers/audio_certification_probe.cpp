// SPDX-License-Identifier: GPL-3.0-or-later
// Opt-in current-host probe. No fake runtime, downloads, or permission bypass.
#include "sentinel/core/voice/UnifiedAudioService.h"
#include "sentinel/core/platform/StandardPathProvider.h"
#include "sentinel/core/app/AppMetadata.h"
#include "sentinel/core/runtime/ProcessExecutor.h"
#include <QGuiApplication>
#include <QDir>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QPermissions>
#include <QTemporaryDir>
#include <QEventLoop>
#include <QtEndian>
#include <cstring>

using namespace sentinel::core;
static void report(const QJsonObject& result) {
    fprintf(stdout, "%s\n", QJsonDocument(result).toJson(QJsonDocument::Compact).constData());
    fflush(stdout);
}
int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(AppMetadata::displayName());
    QCoreApplication::setOrganizationName(AppMetadata::organizationName());
    const auto args = app.arguments();
    if (args.size() < 2) return 2;
    if (args[1] == "paths") {
        StandardPathProvider paths;
        report({{"settings", paths.settingsFilePath()}, {"memory", paths.memoryDatabasePath()},
                {"conversations", paths.conversationDatabasePath()},
                {"data", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)},
                {"cache", QStandardPaths::writableLocation(QStandardPaths::CacheLocation)},
                {"temp", QDir::tempPath()}});
        return 0;
    }
    if (args[1] == "capture" && args.size() == 3) {
        AudioDeviceService device;
        const auto status = app.checkPermission(QMicrophonePermission{});
        report({{"permission", int(status)}, {"inputDevices", device.inputDeviceIds().size()},
                {"outputDevices", device.outputDeviceIds().size()}});
        auto capture = [&] {
            const bool started = device.startCapture();
            report({{"started", started}, {"capturing", device.isCapturing()},
                    {"failure", audioFailureName(device.lastFailure())}});
            if (!started) { app.exit(3); return; }
            report({{"duplicateStartRejected", !device.startCapture()}});
            QTimer::singleShot(5000, &app, [&] {
                auto pcm = device.stopCapture();
                QByteArray wav(44, '\0');
                std::memcpy(wav.data(), "RIFF", 4);
                qToLittleEndian<quint32>(pcm.size() + 36, reinterpret_cast<uchar*>(wav.data() + 4));
                std::memcpy(wav.data() + 8, "WAVEfmt ", 8);
                qToLittleEndian<quint32>(16, reinterpret_cast<uchar*>(wav.data() + 16));
                qToLittleEndian<quint16>(1, reinterpret_cast<uchar*>(wav.data() + 20));
                qToLittleEndian<quint16>(1, reinterpret_cast<uchar*>(wav.data() + 22));
                qToLittleEndian<quint32>(16000, reinterpret_cast<uchar*>(wav.data() + 24));
                qToLittleEndian<quint32>(32000, reinterpret_cast<uchar*>(wav.data() + 28));
                qToLittleEndian<quint16>(2, reinterpret_cast<uchar*>(wav.data() + 32));
                qToLittleEndian<quint16>(16, reinterpret_cast<uchar*>(wav.data() + 34));
                std::memcpy(wav.data() + 36, "data", 4);
                qToLittleEndian<quint32>(pcm.size(), reinterpret_cast<uchar*>(wav.data() + 40));
                wav.append(pcm);
                QFile file(args[2]);
                const bool saved = !pcm.isEmpty() && file.open(QIODevice::WriteOnly) && file.write(wav) == wav.size();
                file.close();
                report({{"pcmBytes", pcm.size()}, {"saved", saved},
                        {"speechDetected", device.speechDetected()}, {"capturing", device.isCapturing()},
                        {"secondStopEmpty", device.stopCapture().isEmpty()}});
                device.cancelCapture();
                app.exit(saved ? 0 : 4);
            });
        };
        QObject::connect(&device, &AudioDeviceService::microphonePermissionChanged, &app,
                         [&](Qt::PermissionStatus permission) {
            report({{"permissionResult", int(permission)}});
            capture();
        });
        QTimer::singleShot(0, &app, [&] {
            if (status == Qt::PermissionStatus::Undetermined) device.requestMicrophonePermission();
            else capture();
        });
        QTimer::singleShot(45000, &app, [&] { device.cancelCapture(); app.exit(5); });
        return app.exec();
    }
    QElapsedTimer timer;
    timer.start();
    if (args[1] == "diagnose" && args.size() >= 3) {
        QTemporaryDir temporary;
        SandboxExecutionPlan plan;
        plan.workingDirectory = QFileInfo(args[2]).absolutePath();
        plan.temporaryDirectory = temporary.path();
        plan.requireEnforcement = true;
        plan.forbidDetachedChildren = true;
        plan.networkAllowed = false;
        plan.readablePaths.append(args[2]);
        for (const auto& arg : args.mid(3))
            if (QFileInfo(arg).isFile()) plan.readablePaths.append(arg);
        ProcessRequest request;
        request.program = args[2];
        request.arguments = args.mid(3);
        request.workingDirectory = plan.workingDirectory;
        request.environment = QProcessEnvironment::systemEnvironment();
        request.sandbox = plan;
        request.timeoutMs = 10000;
        ProcessExecutor executor;
        QEventLoop loop;
        QByteArray errors;
        bool done = false;
        executor.start(request, [&](const ProcessRecord& record) {
            if (record.state == ProcessState::Exited || record.state == ProcessState::Failed) {
                report({{"exitCode", record.exitCode}, {"error", record.error},
                        {"stderr", QString::fromUtf8(errors)}, {"latencyMs", timer.elapsed()}});
                done = true;
                loop.quit();
            }
        }, [&](const QString&, ProcessStream stream, const QByteArray& bytes) {
            if (stream == ProcessStream::Stderr) errors.append(bytes.left(4096 - errors.size()));
        });
        if (!done) loop.exec();
        return 0;
    }
    const auto cancellation = std::make_shared<std::atomic_bool>(false);
    if (args[1] == "whisper" && args.size() == 5) {
        auto config = configuredWhisperTranscriptionConfig(args[2], args[3], true);
        config.policy.processExecutionAllowed = true;
        WhisperSttRuntime runtime(config);
        const auto result = runtime.transcribeFile(QFileInfo(args[4]).canonicalFilePath(), {}, cancellation);
        report({{"runtime", runtime.info().runtimeId}, {"model", args[3]},
                {"failure", audioFailureName(result.failure)}, {"detail", result.detail},
                {"transcript", result.finalText}, {"latencyMs", timer.elapsed()}});
        return result.failure == AudioFailure::None ? 0 : 3;
    }
    if (args[1] == "piper" && args.size() == 5) {
        auto config = defaultDisabledPiperTtsConfig();
        config.enabled = true;
        config.binary.expectedPath = args[2];
        config.binary.status = VoiceBinaryStatus::PresentMetadata;
        config.voiceModel.expectedPath = args[3];
        config.processExecutionAllowed = true;
        config.fileOutputAllowed = true;
        config.controlledOutputDirectory = args[4];
        PiperTtsRuntime runtime(config);
        const auto result = runtime.synthesize({"Sentinel ses testi başarılı."}, cancellation);
        report({{"runtime", runtime.info().runtimeId}, {"model", args[3]},
                {"failure", audioFailureName(result.failure)}, {"detail", result.detail},
                {"file", result.filePath}, {"durationMs", result.durationMs},
                {"sampleRate", result.sampleRate}, {"latencyMs", timer.elapsed()}});
        return result.failure == AudioFailure::None ? 0 : 3;
    }
    if (args[1] == "kokoro" && args.size() == 3) {
        KokoroTtsRuntime runtime(args[2], "af_heart");
        const auto result = runtime.synthesize({"Sentinel ses testi başarılı.", "af_heart", "en-us", 1.0}, cancellation);
        report({{"runtimeAvailable", runtime.info().runtimeAvailable},
                {"modelAvailable", runtime.info().modelAvailable},
                {"failure", audioFailureName(result.failure)}, {"detail", result.detail},
                {"file", result.filePath}, {"latencyMs", timer.elapsed()}});
        return result.failure == AudioFailure::None ? 0 : 3;
    }
    return 2;
}
