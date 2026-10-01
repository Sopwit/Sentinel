// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/interfaces/IChatProvider.h"
#include "sentinel/core/model/ModelService.h"

#include <QThread>

#include <atomic>
#include <memory>

namespace sentinel::test {

enum class DeterministicChatReply { Final, Streaming, Failure, Delayed, Unavailable };

struct DeterministicChatProviderState {
    DeterministicChatReply reply = DeterministicChatReply::Final;
    int delayMs = 100;
};

class DeterministicChatProvider final : public core::IChatProvider {
public:
    explicit DeterministicChatProvider(std::shared_ptr<DeterministicChatProviderState> state)
        : state_(std::move(state)) {}

    QString name() const override { return QStringLiteral("deterministic-chat"); }
    core::ChatProviderStatus status() const override {
        return state_->reply == DeterministicChatReply::Unavailable
                   ? core::ChatProviderStatus::Unavailable
                   : core::ChatProviderStatus::Ready;
    }
    core::ChatProviderConcurrency concurrency() const override {
        return core::ChatProviderConcurrency::Supported;
    }
    bool supportsStreaming() const override {
        return state_->reply == DeterministicChatReply::Streaming;
    }
    core::ChatProviderReply sendMessage(const QString&) override { return finalReply(); }
    core::ChatProviderReply sendRequest(const QString&, const core::ChatRequestOptions&) override {
        return finalReply();
    }
    core::ChatProviderReply sendMessageStreaming(
        const QString&, const std::function<void(const QString&)>& onDelta,
        const std::shared_ptr<std::atomic_bool>& cancellation) override {
        onDelta(QStringLiteral("SENTINEL_"));
        if (!cancellation || !cancellation->load()) onDelta(QStringLiteral("TEST_RESPONSE"));
        core::ChatProviderReply reply;
        reply.success = !cancellation || !cancellation->load();
        reply.lifecycle = reply.success ? core::ChatRequestLifecycle::Completed
                                        : core::ChatRequestLifecycle::Cancelled;
        reply.category = reply.success ? core::ChatProviderErrorCategory::None
                                       : core::ChatProviderErrorCategory::Cancelled;
        return reply;
    }

private:
    core::ChatProviderReply finalReply() const {
        if (state_->reply == DeterministicChatReply::Delayed)
            QThread::msleep(static_cast<unsigned long>(state_->delayMs));
        core::ChatProviderReply reply;
        if (state_->reply == DeterministicChatReply::Failure) {
            reply.errorMessage = QStringLiteral("Deterministic provider failure.");
            reply.category = core::ChatProviderErrorCategory::ConnectionFailed;
            reply.lifecycle = core::ChatRequestLifecycle::Failed;
            return reply;
        }
        reply.success = true;
        reply.message = QStringLiteral("SENTINEL_TEST_RESPONSE");
        reply.lifecycle = core::ChatRequestLifecycle::Completed;
        return reply;
    }

    std::shared_ptr<DeterministicChatProviderState> state_;
};

// Owns the production ModelService configuration used by Chat tests.  It
// publishes a completed local catalog before selecting the deterministic model,
// so ModelBinding resolution follows the same rules as a live Ollama catalog.
class DeterministicModelServiceFixture {
public:
    explicit DeterministicModelServiceFixture(DeterministicChatReply reply =
                                                  DeterministicChatReply::Final)
        : state(std::make_shared<DeterministicChatProviderState>()) {
        state->reply = reply;
        models = std::make_unique<core::ModelService>();
        models->registerProvider(QStringLiteral("ollama"), [shared = state](const core::ModelBinding&) {
            return std::make_shared<DeterministicChatProvider>(shared);
        });
        core::OllamaModelDiscoveryResult catalog;
        catalog.models = {{QStringLiteral("sentinel-test-model"), {}, 0}};
        catalog.lifecycle = core::ChatRequestLifecycle::Completed;
        catalog.errorCategory = core::ChatProviderErrorCategory::None;
        catalog.safeDetail = QStringLiteral("Deterministic test catalog is ready.");
        models->acceptOllamaDiscovery(catalog, models->beginProviderHealthObservation());
        models->setSelectedModel({QStringLiteral("ollama"), QStringLiteral("sentinel-test-model")});
    }

    std::unique_ptr<core::ModelService> takeModelService() { return std::move(models); }

    std::shared_ptr<DeterministicChatProviderState> state;

private:
    std::unique_ptr<core::ModelService> models;
};

} // namespace sentinel::test
