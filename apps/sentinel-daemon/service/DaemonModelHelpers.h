// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/app/ApplicationController.h"
#include "sentinel/core/runtime/OllamaRuntime.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>
#include <QElapsedTimer>
#include <atomic>
#include <memory>
#include "sentinel/core/runtime/ProcessExecutor.h"
namespace sentinel::daemon {
class DaemonModelHelpers final : public QObject {
public:
    explicit DaemonModelHelpers(core::ApplicationController* controller, QObject* parent = nullptr);
    ~DaemonModelHelpers() override;
    QJsonObject state(const QString& component) const;
    bool action(const QString& component, const QString& action, const QString& value,
                const QString& endpoint);
    OllamaModelPuller puller;

private:
    core::ApplicationController* controller_;
    QJsonArray catalog_;
    mutable QJsonArray modelRows_;
    mutable int installedCount_ = 0;
    mutable QElapsedTimer modelRowsAge_;
    bool modelRowsBuilding_ = false;
    std::shared_ptr<std::atomic_bool> modelRowsCancelled_;
    quint64 modelRowsGeneration_ = 0;
    void rebuildModelRows();
    QString activeOperation_;
    QString searchText_;
    QString queryKey_;
    QString searchTask_;
    QString searchSort_ = "downloads";
    QString ollamaSort_ = "popular";
    bool ggufOnly_ = false;
    int catalogPage_ = 0;
    int catalogPages_ = 1;
    struct ModelRows { QJsonArray models; int installed; int pages; };
    QTimer refreshTimer_;
    void refreshCatalog(bool force);
    QString runtimeBinary() const;
    bool setupRuntime();
    void runSetupStep(int step);
    core::ProcessExecutor setupProcesses_;
    QString setupProcess_, setupStatus_;
    bool setupBusy_ = false;
    bool setupCancelled_ = false;
    OllamaLibraryFetcher library;
    OllamaModelDetailFetcher detail;
    LMStudioLibraryFetcher lmStudio;
};
} // namespace sentinel::daemon
