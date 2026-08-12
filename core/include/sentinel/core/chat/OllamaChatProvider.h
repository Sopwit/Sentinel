// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/interfaces/IChatProvider.h"
#include "sentinel/core/runtime/OllamaRuntime.h"

namespace sentinel::core {

// Real IChatProvider backed by a local Ollama runtime. sendMessage() performs a
// real /api/generate request through OllamaLocalInferenceClient; status() reflects
// live Ollama health and installed-model discovery instead of a hardcoded state.
class OllamaChatProvider final : public IChatProvider {
public:
    explicit OllamaChatProvider(OllamaConfig config = OllamaConfig{}, int timeoutMs = 30000);

    QString name() const override;
    ChatProviderStatus status() const override;
    ChatProviderReply sendMessage(const QString& message) override;

    QString endpoint() const;
    QString selectedModel() const;
    void setSelectedModel(const QString& model);

private:
    OllamaConfig config_;
    int timeoutMs_ = 30000;
    QString selectedModel_;
};

} // namespace sentinel::core
