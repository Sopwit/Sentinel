// SPDX-License-Identifier: GPL-3.0-or-later
#include "LocalModelRuntime.h"
#include "sentinel/core/app/ApplicationController.h"
#include "sentinel/core/model/ModelLibrary.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
namespace sentinel::daemon {
LocalModelRuntime::LocalModelRuntime(core::ApplicationController& controller, QObject* parent)
    : QObject(parent), controller_(controller), processes_(this), network_(this) {
    timer_.setInterval(3000);
    connect(&timer_, &QTimer::timeout, this, &LocalModelRuntime::ensureRunning);
    connect(&controller_, &core::ApplicationController::localModelSelectionChanged, this, [this] {
        attemptedKey_.clear();
        QTimer::singleShot(0, this, &LocalModelRuntime::ensureRunning);
    });
    connect(&controller_, &core::ApplicationController::modelLibraryChanged, this, [this] {
        attemptedKey_.clear();
        QTimer::singleShot(0, this, &LocalModelRuntime::ensureRunning);
    });
    timer_.start();
    QTimer::singleShot(0, this, &LocalModelRuntime::ensureRunning);
}
LocalModelRuntime::~LocalModelRuntime() {
    shuttingDown_ = true;
    timer_.stop();
    processes_.shutdown();
}
void LocalModelRuntime::setStatus(const QString& status) {
    status_ = status;
    controller_.setProperty("localLlamaRuntimeStatus", status);
}
void LocalModelRuntime::ensureRunning() {
    if (shuttingDown_ || probing_ || controller_.selectedRuntimeProvider() != "llama-cpp-server")
        return;
    const QUrl endpoint(controller_.llamaCppEndpoint());
    if (endpoint.scheme() != "http" ||
        (endpoint.host() != "127.0.0.1" && endpoint.host() != "localhost" &&
         endpoint.host() != "::1") ||
        !endpoint.userInfo().isEmpty() ||
        core::NetworkPolicyService::instance().check(endpoint) != core::NetworkDecision::Allowed)
        return;
    const auto selection = controller_.selectedLocalModel();
    const auto key = endpoint.toString() + "|" + selection;
    if (!processId_.isEmpty() && modelId_ != selection && !controller_.agentLoopActive() &&
        !controller_.chatModeService()->busy()) {
        processes_.terminate(processId_);
        processId_.clear();
        attemptedKey_.clear();
        QTimer::singleShot(1000, this, &LocalModelRuntime::ensureRunning);
        return;
    }
    QUrl health = endpoint;
    health.setPath("/health");
    health.setQuery({});
    QNetworkRequest request(health);
    request.setTransferTimeout(1500);
    probing_ = true;
    auto* reply = network_.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, endpoint, selection, key] {
        probing_ = false;
        const auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto error = reply->error();
        reply->deleteLater();
        if (shuttingDown_)
            return;
        if (controller_.selectedRuntimeProvider() != "llama-cpp-server" ||
            selection != controller_.selectedLocalModel())
            return;
        if (code == 200) {
            if (status_ != "llama.cpp is ready.") {
                setStatus("llama.cpp is ready.");
                controller_.refreshModelDiscovery();
            }
            return;
        }
        // A loading/other existing listener owns this port. Do not start a competing server.
        if (code != 0 || error != QNetworkReply::ConnectionRefusedError || !processId_.isEmpty() ||
            attemptedKey_ == key)
            return;
        attemptedKey_ = key;
        QString file, alias;
        for (const auto& entry : controller_.modelLibrary()->entries()) {
            if (entry.format != "GGUF" || entry.localFile.isEmpty() ||
                !QFileInfo::exists(entry.localFile))
                continue;
            if (selection.isEmpty() || entry.nativeModelId == selection ||
                entry.displayName == selection) {
                file = entry.localFile;
                alias = selection.isEmpty() ? entry.nativeModelId : selection;
                break;
            }
        }
        if (file.isEmpty()) {
            setStatus("Download or import a GGUF model to start llama.cpp.");
            return;
        }
        auto binary = QStandardPaths::findExecutable("llama-server");
        if (binary.isEmpty())
            binary = QStandardPaths::findExecutable(
                "llama-server",
                {QCoreApplication::applicationDirPath(), "/opt/homebrew/bin", "/usr/local/bin", QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("runtimes/llama.cpp/build/bin"), QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("runtimes/llama.cpp/build/bin/Release")});
        if (binary.isEmpty()) {
            setStatus("llama-server is not installed or not on PATH.");
            return;
        }
        core::ProcessRequest launch;
        launch.program = binary;
        launch.arguments = {"--model",    file,
                            "--alias",    alias,
                            "--host",     endpoint.host(),
                            "--port",     QString::number(endpoint.port(8080)),
                            "--ctx-size", "8192",
                            "--parallel", "1",
                            "--jinja"};
        launch.timeoutMs = 0;
        // Explicit provider activation authorizes this fixed local runtime command, never a model
        // tool.
        launch.unconfinedPermitted = true;
        launch.environment = QProcessEnvironment::systemEnvironment();
        modelId_ = selection;
        setStatus("Starting llama.cpp; loading the selected model…");
        processId_ = processes_.start(launch, [this](const core::ProcessRecord& record) {
            if (shuttingDown_)
                return;
            if (record.state == core::ProcessState::Failed ||
                record.state == core::ProcessState::Exited ||
                record.state == core::ProcessState::Cancelled) {
                processId_.clear();
                setStatus(record.error.isEmpty() ? "llama.cpp stopped." : record.error);
                controller_.refreshModelDiscovery();
            }
        });
        if (processes_.record(processId_).state == core::ProcessState::Failed)
            processId_.clear();
    });
}
} // namespace sentinel::daemon
