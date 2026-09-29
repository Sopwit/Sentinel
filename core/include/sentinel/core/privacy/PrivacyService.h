// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/model/ModelService.h"
#include "sentinel/core/voice/UnifiedAudioService.h"
#include "sentinel/core/chat/IConversationStore.h"
#include "sentinel/core/interfaces/IMemoryStore.h"
#include "sentinel/core/agent/IAgentRunStore.h"
#include "sentinel/core/privacy/RetentionPolicy.h"
#include "sentinel/core/app/RecoveryService.h"
#include "sentinel/core/security/CredentialStore.h"
#include "sentinel/core/network/NetworkPolicyService.h"

#include <QJsonObject>

namespace sentinel::core {

class PrivacyService final {
public:
    PrivacyService(const AppSettings& settings, const ModelService* models = nullptr,
                   const VoiceSessionService* audio = nullptr,
                   const IConversationStore* chat = nullptr,
                   const IMemoryStore* memory = nullptr,
                   const IAgentRunStore* agentRuns = nullptr)
        : settings_(settings), models_(models), audio_(audio), chat_(chat), memory_(memory),
          agentRuns_(agentRuns) {}

    QJsonObject state() const {
        QString modelProcessing = QStringLiteral("Unknown");
        if (models_) {
            const auto selected = models_->selectedModel();
            if (selected.isValid() && models_->isKnownProvider(selected.providerId)) {
                const auto metadata = models_->currentModelMetadata(selected.providerId,
                                                                    selected.modelId);
                modelProcessing = metadata.providerKind == ProviderKind::Local
                    ? QStringLiteral("Local") : QStringLiteral("Cloud");
            }
        }
        const auto audioState = audio_ ? audio_->privacy() : AudioPrivacyState{};
        const auto stt = audio_ ? audio_->sttInfo() : SpeechProviderInfo{};
        const auto tts = audio_ ? audio_->ttsInfo() : SpeechProviderInfo{};
        const auto secure = defaultCredentialStore().summary();
        const auto effectiveMode = NetworkPolicyService::instance().mode();
        QJsonObject credentialStates;
        for (const auto& id : {QStringLiteral("openai"), QStringLiteral("claude"),
                               QStringLiteral("gemini"), QStringLiteral("deepseek"),
                               QStringLiteral("groq"), QStringLiteral("mistral"),
                               QStringLiteral("web-search"), QStringLiteral("proxy-user"),
                               QStringLiteral("proxy-password")})
            credentialStates.insert(id, settings_.credentialState(id));
        return {{QStringLiteral("modelProcessing"), modelProcessing},
                {QStringLiteral("speechProcessing"), audio_ ?
                     (audioState.processingRawAudio ?
                         (audioState.cloudProviderActive ? QStringLiteral("Cloud") :
                             QStringLiteral("Local")) : QStringLiteral("Idle")) :
                     QStringLiteral("Unknown")},
                {QStringLiteral("sttProviderLocation"), stt.id.isEmpty() ? QStringLiteral("Unknown") :
                    stt.local ? QStringLiteral("Local") : QStringLiteral("Cloud")},
                {QStringLiteral("ttsProviderLocation"), tts.id.isEmpty() ? QStringLiteral("Unknown") :
                    tts.local ? QStringLiteral("Local") : QStringLiteral("Cloud")},
                {QStringLiteral("rawAudioRetained"), audio_ ?
                     QJsonValue(audioState.retainRawRecordings) : QJsonValue()},
                {QStringLiteral("chatPersistence"), chat_ ?
                     (chat_->status() == ConversationStoreStatus::Ready ?
                      QStringLiteral("Available") : QStringLiteral("Unavailable")) :
                     QStringLiteral("Unknown")},
                {QStringLiteral("memoryPersistence"), memory_ ?
                     (memory_->isAvailable() ? QStringLiteral("Available") :
                      QStringLiteral("Unavailable")) : QStringLiteral("Unknown")},
                {QStringLiteral("agentRunPersistence"), agentRuns_ ?
                     QStringLiteral("Configured") : QStringLiteral("Unknown")},
                {QStringLiteral("networkMode"), settings_.networkMode()},
                {QStringLiteral("effectiveNetworkMode"), effectiveMode == NetworkMode::Offline
                    ? QStringLiteral("Offline") : effectiveMode == NetworkMode::LocalOnly
                        ? QStringLiteral("LocalOnly") : QStringLiteral("Online")},
                {QStringLiteral("secureStoreAvailable"),
                    secure.status == CredentialStoreStatus::Ready},
                {QStringLiteral("credentialStates"), credentialStates},
                {QStringLiteral("retention"), RetentionPolicy::effective(settings_)},
                {QStringLiteral("recoveryHealth"), RecoveryService(chat_, nullptr, agentRuns_)
                    .state().value(QStringLiteral("health"))}};
    }

private:
    const AppSettings& settings_;
    const ModelService* models_;
    const VoiceSessionService* audio_;
    const IConversationStore* chat_;
    const IMemoryStore* memory_;
    const IAgentRunStore* agentRuns_;
};

} // namespace sentinel::core
