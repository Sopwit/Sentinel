// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/runtime/OllamaRuntime.h"
#include <QJsonObject>
namespace sentinel::daemon {
class DaemonModelHelpers final : public QObject {
public:
    explicit DaemonModelHelpers(QObject* parent = nullptr)
        : QObject(parent), puller(this), library(this), detail(this), lmStudio(this) {}
    QJsonObject state(const QString& component) const;
    bool action(const QString& component, const QString& action, const QString& value,
                const QString& endpoint);
    OllamaModelPuller puller;

private:
    OllamaLibraryFetcher library;
    OllamaModelDetailFetcher detail;
    LMStudioLibraryFetcher lmStudio;
};
} // namespace sentinel::daemon
