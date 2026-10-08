// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/app/ApplicationController.h"
#include "sentinel/core/runtime/OllamaRuntime.h"
#include <QJsonArray>
#include <QJsonObject>
namespace sentinel::daemon {
class DaemonModelHelpers final : public QObject {
public:
    explicit DaemonModelHelpers(core::ApplicationController* controller, QObject* parent = nullptr);
    QJsonObject state(const QString& component) const;
    bool action(const QString& component, const QString& action, const QString& value,
                const QString& endpoint);
    OllamaModelPuller puller;

private:
    core::ApplicationController* controller_;
    QJsonArray catalog_;
    QString activeOperation_;
    OllamaLibraryFetcher library;
    OllamaModelDetailFetcher detail;
    LMStudioLibraryFetcher lmStudio;
};
} // namespace sentinel::daemon
