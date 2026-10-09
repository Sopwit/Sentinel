// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/desktop/SystemDictationService.h"
#include <QPointer>
#include <QTimer>
#import <AVFoundation/AVFoundation.h>
#import <Speech/Speech.h>
namespace sentinel::desktop {
struct SystemDictationService::Impl {
    AVAudioEngine* engine = nil;
    SFSpeechRecognizer* recognizer = nil;
    SFSpeechAudioBufferRecognitionRequest* request = nil;
    SFSpeechRecognitionTask* task = nil;
    quint64 generation = 0;
    bool active = false;
    bool tapped = false;
    QString partial;
};
SystemDictationService::SystemDictationService(QObject* parent) : QObject(parent), impl_(std::make_unique<Impl>()) {}
SystemDictationService::~SystemDictationService() { cancel(); }
bool SystemDictationService::active() const { return impl_->active; }
bool SystemDictationService::busy() const { return impl_->active || impl_->task != nil; }
void SystemDictationService::cancel() {
    ++impl_->generation;
    [impl_->engine stop];
    if (impl_->tapped) [impl_->engine.inputNode removeTapOnBus:0];
    impl_->tapped = false;
    [impl_->request endAudio];
    [impl_->task cancel];
    impl_->task = nil;
    impl_->request = nil;
    impl_->engine = nil;
    impl_->partial.clear();
    if (impl_->active) { impl_->active = false; emit activeChanged(); }
}
void SystemDictationService::stop() {
    if (!impl_->engine) { cancel(); return; }
    [impl_->engine stop];
    if (impl_->tapped) [impl_->engine.inputNode removeTapOnBus:0];
    impl_->tapped = false;
    [impl_->request endAudio];
    if (impl_->active) { impl_->active = false; emit activeChanged(); }
    const auto generation = impl_->generation;
    QTimer::singleShot(3000, this, [this, generation] {
        if (generation != impl_->generation || !impl_->task) return;
        const auto text = impl_->partial;
        cancel();
        if (!text.isEmpty()) emit completed(text);
        else emit failed(tr("No speech was recognized. Try Local Whisper or another input language."));
    });
}
void SystemDictationService::start(const QString& language) {
    if (impl_->active || impl_->task) return;
    impl_->active = true;
    emit activeChanged();
    const auto generation = ++impl_->generation;
    QPointer<SystemDictationService> guard(this);
    const auto begin = [guard, generation, language] {
        if (!guard || generation != guard->impl_->generation) return;
        if ([SFSpeechRecognizer authorizationStatus] != SFSpeechRecognizerAuthorizationStatusAuthorized ||
            [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio] != AVAuthorizationStatusAuthorized) {
            guard->cancel();
            emit guard->failed(guard->tr("Allow microphone and speech recognition access in System Settings."));
            return;
        }
        auto& state = *guard->impl_;
        NSString* identifier = language.isEmpty() || language == "system" ? NSLocale.currentLocale.localeIdentifier : [NSString stringWithUTF8String:language.toUtf8().constData()];
        state.recognizer = [[SFSpeechRecognizer alloc] initWithLocale:[[NSLocale alloc] initWithLocaleIdentifier:identifier]];
        if (!state.recognizer || !state.recognizer.supportsOnDeviceRecognition) {
            guard->cancel();
            emit guard->failed(guard->tr("On-device system dictation is unavailable for this language. Select Local Whisper."));
            return;
        }
        state.engine = [[AVAudioEngine alloc] init];
        state.request = [[SFSpeechAudioBufferRecognitionRequest alloc] init];
        state.request.requiresOnDeviceRecognition = YES;
        state.request.shouldReportPartialResults = YES;
        auto* node = state.engine.inputNode;
        auto* format = [node outputFormatForBus:0];
        if (format.sampleRate <= 0 || format.channelCount == 0) {
            guard->cancel(); emit guard->failed(guard->tr("No microphone input is available.")); return;
        }
        SFSpeechAudioBufferRecognitionRequest* request = state.request;
        AVAudioNodeTapBlock tap = ^(AVAudioPCMBuffer* buffer, AVAudioTime*) {
            [request appendAudioPCMBuffer:buffer];
        };
        // The error-reporting API accepts buffers covering 100–400 ms.
        const auto bufferSize = static_cast<AVAudioFrameCount>(format.sampleRate * 0.1 + 0.5);
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 270000
        if (@available(macOS 27.0, *)) {
            NSError* tapError = nil;
            if (![node installTapOnBus:0 bufferSize:bufferSize format:format error:&tapError block:tap]) {
                const QString message = tapError
                    ? QString::fromUtf8(tapError.localizedDescription.UTF8String)
                    : guard->tr("No microphone input is available.");
                guard->cancel();
                emit guard->failed(message);
                return;
            }
        } else
#endif
        {
            // Required only on systems predating the error-reporting API.
            // Clang still warns about this runtime-guarded compatibility call.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            [node installTapOnBus:0 bufferSize:bufferSize format:format block:tap];
#pragma clang diagnostic pop
        }
        state.tapped = true;
        state.task = [state.recognizer recognitionTaskWithRequest:state.request resultHandler:^(SFSpeechRecognitionResult* result, NSError* error) {
            const QString text = result ? QString::fromUtf8(result.bestTranscription.formattedString.UTF8String) : QString{};
            const bool final = result.isFinal;
            const QString failure = error ? QString::fromUtf8(error.localizedDescription.UTF8String) : QString{};
            if (!guard) return;
            QMetaObject::invokeMethod(guard.data(), [guard, generation, text, final, failure] {
                if (!guard || generation != guard->impl_->generation) return;
                if (!text.isEmpty()) guard->impl_->partial = text;
                if (final || !failure.isEmpty()) {
                    const auto transcript = guard->impl_->partial;
                    guard->cancel();
                    if (!transcript.isEmpty()) emit guard->completed(transcript);
                    else emit guard->failed(failure);
                }
            }, Qt::QueuedConnection);
        }];
        NSError* error = nil;
        [state.engine prepare];
        if (![state.engine startAndReturnError:&error]) {
            const QString message = QString::fromUtf8(error.localizedDescription.UTF8String);
            guard->cancel(); emit guard->failed(message); return;
        }
        QTimer::singleShot(60000, guard.data(), [guard, generation] { if (guard && generation == guard->impl_->generation) guard->stop(); });
    };
    [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus) {
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL) {
            if (guard) QMetaObject::invokeMethod(guard.data(), begin, Qt::QueuedConnection);
        }];
    }];
}
}
