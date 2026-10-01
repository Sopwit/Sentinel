// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/LocalInference.h"
#include <QSemaphore>
#include <atomic>
#include <memory>

namespace sentinel::test {
struct WorkerClientState {
    QSemaphore release;
    std::atomic_int calls{0};
    std::atomic_bool entered{false};
    std::atomic_bool exited{false};
    std::atomic_bool cancelled{false};
    core::LocalInferenceStatus outcome = core::LocalInferenceStatus::Succeeded;
};

class GatedInferenceClient final : public core::ILocalInferenceClient {
public:
    explicit GatedInferenceClient(std::shared_ptr<WorkerClientState> state) : state_(state) {}
    core::LocalInferenceResponse infer(const core::LocalInferenceRequest& request) override {
        ++state_->calls;
        state_->entered = true;
        const bool released = state_->release.tryAcquire(1, 3000);
        state_->cancelled = request.options.cancellationToken->load();
        core::LocalInferenceResponse result;
        result.model = request.options.model;
        result.status = released ? state_->outcome : core::LocalInferenceStatus::Error;
        result.error = result.status == core::LocalInferenceStatus::Succeeded
                           ? core::LocalInferenceError::None
                           : core::LocalInferenceError::ClientUnavailable;
        if (result.status == core::LocalInferenceStatus::Succeeded)
            result.text = QStringLiteral("worker result");
        state_->exited = true;
        return result;
    }
    QString statusSummary() const override {
        return QStringLiteral("Gated test client");
    }

private:
    std::shared_ptr<WorkerClientState> state_;
};

class LocalInferenceWorkerFixture {
public:
    std::shared_ptr<WorkerClientState> state = std::make_shared<WorkerClientState>();
    std::unique_ptr<QObject> context = std::make_unique<QObject>();
    std::unique_ptr<core::LocalInferenceWorker> worker =
        std::make_unique<core::LocalInferenceWorker>(std::make_unique<GatedInferenceClient>(state),
                                                     nullptr, context.get(), true, false);
    int callbacks = 0;
    QString completedId;
    core::LocalInferenceResponse response;

    bool start(const QString& id = QStringLiteral("worker-1")) {
        core::LocalInferenceRequest request;
        request.id = id;
        request.prompt = QStringLiteral("test prompt");
        request.options.model = QStringLiteral("test-model");
        return worker->startInference(request, [this](const QString& id, const auto& result) {
            ++callbacks;
            completedId = id;
            response = result;
        });
    }
    void shutdown() {
        context.reset(); // Drop queued deliveries before joining the worker.
        state->release.release();
        worker.reset();
    }
    ~LocalInferenceWorkerFixture() {
        shutdown();
    }
};
} // namespace sentinel::test
