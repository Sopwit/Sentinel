// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/desktop/SystemDictationService.h"
namespace sentinel::desktop {
struct SystemDictationService::Impl {};
SystemDictationService::SystemDictationService(QObject* parent) : QObject(parent), impl_(std::make_unique<Impl>()) {}
SystemDictationService::~SystemDictationService() = default;
bool SystemDictationService::active() const { return false; }
bool SystemDictationService::busy() const { return false; }
void SystemDictationService::start(const QString&) {
#ifdef Q_OS_WIN
    emit failed(tr("Focus the message field and use Windows dictation (Win+H), or select Local Whisper."));
#else
    emit failed(tr("Use your desktop's dictation input method in the message field, or select Local Whisper."));
#endif
}
void SystemDictationService::stop() {}
void SystemDictationService::cancel() {}
}
