// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/runtime/ProcessExecutor.h"
#include <QNetworkAccessManager>
#include <QTimer>
namespace sentinel::core {
class ApplicationController;
}
namespace sentinel::daemon {
// Owns only processes started by Sentinel. Existing servers are never terminated.
class LocalModelRuntime final : public QObject {
public:
    explicit LocalModelRuntime(core::ApplicationController& controller, QObject* parent = nullptr);
    ~LocalModelRuntime() override;
    void ensureRunning();
    QString status() const {
        return status_;
    }

private:
    void setStatus(const QString& status);
    core::ApplicationController& controller_;
    core::ProcessExecutor processes_;
    QNetworkAccessManager network_;
    QTimer timer_;
    QString processId_, modelId_, attemptedKey_, status_;
    bool probing_ = false;
    bool shuttingDown_ = false;
};
} // namespace sentinel::daemon
