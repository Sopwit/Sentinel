// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/model/ModelLibrary.h"
#include "sentinel/core/runtime/ProcessExecutor.h"
#include "sentinel/core/runtime/RuntimeProvider.h"
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTimer>
namespace sentinel::core {
class ApplicationController;
}
namespace sentinel::daemon {
// Owns only processes started by Sentinel. Existing servers are never terminated.
class LocalModelRuntime final : public QObject {
public:
    // In-process adapter seam for deterministic fixtures. Production composition uses
    // ApplicationController and the private Qt network/ProcessExecutor instances.
    // No PID adoption, IPC/settings hook, or additional process authority is exposed.
    struct Selection {
        QString provider, endpoint, model;
        bool busy = false;
    };
    struct Dependencies {
        std::function<Selection()> selection;
        std::function<QList<core::ModelLibraryEntry>()> entries;
        std::function<void(const QString&)> publishStatus;
        std::function<void()> refreshDiscovery;
        std::function<QNetworkReply*(const QNetworkRequest&)> get;
        std::function<QString()> findExecutable;
        std::function<QString(const core::ProcessRequest&, core::ProcessExecutor::StateCallback)>
            start;
        std::function<core::ProcessRecord(const QString&)> record;
        std::function<bool(const QString&)> terminate;
        bool automaticPolling = true;
    };
    explicit LocalModelRuntime(core::ApplicationController& controller, QObject* parent = nullptr);
    explicit LocalModelRuntime(Dependencies dependencies, QObject* parent = nullptr);
    ~LocalModelRuntime() override;
    void ensureRunning();
    void beginShutdown();
    core::RuntimeReadinessState readiness() const {
        return readiness_;
    }
    QString status() const {
        return status_;
    }

private:
    void setStatus(const QString& status);
    void setReadiness(core::RuntimeReadinessState state, const QString& status);
    void invalidateProbe();
    void observeSelection();
    bool acceptsObservation(quint64 revision);
    void verifyIdentity(const QUrl& endpoint, const QString& model, quint64 revision);
    void scheduleReconcile(int delayMs = 0);
    Dependencies dependencies_;
    core::ProcessExecutor processes_;
    QNetworkAccessManager network_;
    QTimer timer_;
    QString processId_, modelId_, attemptedKey_, status_;
    Selection observedSelection_;
    bool selectionObserved_ = false;
    quint64 selectionRevision_ = 0;
    QPointer<QNetworkReply> probeReply_;
    core::RuntimeReadinessState readiness_ = core::RuntimeReadinessState::Unknown;
    bool identityConfirmed_ = false;
    bool probing_ = false;
    bool shuttingDown_ = false;
};
} // namespace sentinel::daemon
