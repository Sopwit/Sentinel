// SPDX-License-Identifier: GPL-3.0-or-later
#include "LocalModelRuntime.h"
#include "sentinel/core/app/ApplicationController.h"
#include "sentinel/core/model/ModelLibrary.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrlQuery>
namespace sentinel::daemon {
namespace {
LocalModelRuntime::Dependencies controllerDependencies(core::ApplicationController& controller) {
    LocalModelRuntime::Dependencies dependencies;
    dependencies.selection = [&controller] {
        return LocalModelRuntime::Selection{
            controller.selectedRuntimeProvider(), controller.llamaCppEndpoint(),
            controller.selectedLocalModel(),
            controller.agentLoopActive() || controller.chatModeService()->busy()};
    };
    dependencies.entries = [&controller] { return controller.modelLibrary()->entries(); };
    dependencies.publishStatus = [&controller](const QString& status) {
        controller.setProperty("localLlamaRuntimeStatus", status);
    };
    dependencies.refreshDiscovery = [&controller] { controller.refreshModelDiscovery(); };
    return dependencies;
}
} // namespace
LocalModelRuntime::LocalModelRuntime(core::ApplicationController& controller, QObject* parent)
    : LocalModelRuntime(controllerDependencies(controller), parent) {
    const auto changed = [this] {
        attemptedKey_.clear();
        observeSelection();
        scheduleReconcile();
    };
    connect(&controller, &core::ApplicationController::localModelSelectionChanged, this, changed);
    connect(&controller, &core::ApplicationController::modelLibraryChanged, this, changed);
    // Endpoint changes emit this existing signal, not localModelSelectionChanged.
    // Observe even A -> B -> A changes before a queued health callback runs.
    connect(&controller, &core::ApplicationController::ollamaStatusChanged, this, [this] {
        const auto revision = selectionRevision_;
        observeSelection();
        if (revision != selectionRevision_) {
            scheduleReconcile();
        }
    });
}
LocalModelRuntime::LocalModelRuntime(Dependencies dependencies, QObject* parent)
    : QObject(parent), dependencies_(std::move(dependencies)), processes_(this), network_(this) {
    // Omitted IO adapters preserve the existing production implementation and permissions.
    if (!dependencies_.get) {
        dependencies_.get = [this](const QNetworkRequest& request) {
            return network_.get(request);
        };
    }
    if (!dependencies_.findExecutable) {
        dependencies_.findExecutable = [] {
            auto binary = QStandardPaths::findExecutable("llama-server");
            if (binary.isEmpty()) {
                binary = QStandardPaths::findExecutable(
                    "llama-server",
                    {QCoreApplication::applicationDirPath(), "/opt/homebrew/bin", "/usr/local/bin",
                     QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                         .filePath("runtimes/llama.cpp/build/bin"),
                     QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                         .filePath("runtimes/llama.cpp/build/bin/Release")});
            }
            return binary;
        };
    }
    if (!dependencies_.start) {
        dependencies_.start = [this](const core::ProcessRequest& request,
                                     core::ProcessExecutor::StateCallback callback) {
            return processes_.start(request, std::move(callback));
        };
    }
    if (!dependencies_.record) {
        dependencies_.record = [this](const QString& id) { return processes_.record(id); };
    }
    if (!dependencies_.terminate) {
        dependencies_.terminate = [this](const QString& id) { return processes_.terminate(id); };
    }
    timer_.setInterval(3000);
    connect(&timer_, &QTimer::timeout, this, &LocalModelRuntime::ensureRunning);
    if (dependencies_.automaticPolling) {
        timer_.start();
        scheduleReconcile();
    }
}
void LocalModelRuntime::scheduleReconcile(int delayMs) {
    if (dependencies_.automaticPolling) {
        QTimer::singleShot(delayMs, this, &LocalModelRuntime::ensureRunning);
    }
}
LocalModelRuntime::~LocalModelRuntime() {
    shuttingDown_ = true;
    timer_.stop();
    ++selectionRevision_;
    // Destruction must not publish through a borrowed controller/test callback.
    // Disconnect before abort; reply adapters may emit finished synchronously.
    const auto reply = probeReply_;
    probeReply_.clear();
    if (reply) {
        QObject::disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
    }
    processes_.shutdown();
}
void LocalModelRuntime::beginShutdown() {
    shuttingDown_ = true;
    timer_.stop();
    invalidateProbe();
}
void LocalModelRuntime::setStatus(const QString& status) {
    status_ = status;
    dependencies_.publishStatus(status);
}
void LocalModelRuntime::setReadiness(core::RuntimeReadinessState state, const QString& status) {
    readiness_ = state;
    if (state != core::RuntimeReadinessState::Ready &&
        state != core::RuntimeReadinessState::Unknown) {
        identityConfirmed_ = false;
    }
    setStatus(status);
}
void LocalModelRuntime::invalidateProbe() {
    ++selectionRevision_;
    identityConfirmed_ = false;
    probing_ = false;
    // Clear/fence before abort: Qt may emit finished synchronously from abort().
    const auto reply = probeReply_;
    probeReply_.clear();
    if (reply) {
        reply->abort();
        reply->deleteLater();
    }
    setReadiness(core::RuntimeReadinessState::Unknown, "llama.cpp model readiness is unverified.");
}
void LocalModelRuntime::observeSelection() {
    const auto current = dependencies_.selection();
    const bool changed = selectionObserved_ && (current.provider != observedSelection_.provider ||
                                                current.model != observedSelection_.model ||
                                                current.endpoint != observedSelection_.endpoint);
    observedSelection_ = current;
    selectionObserved_ = true;
    if (changed) {
        invalidateProbe();
    }
}
bool LocalModelRuntime::acceptsObservation(quint64 revision) {
    if (shuttingDown_) {
        return false;
    }
    observeSelection();
    return revision == selectionRevision_;
}
void LocalModelRuntime::verifyIdentity(const QUrl& endpoint, const QString& model,
                                       quint64 revision) {
    // Read-only llama.cpp runtime metadata, never catalog membership or an inferred filename.
    // No model query: routers without a selected runtime remain unverified. Disable autoload.
    QUrl props = endpoint;
    props.setPath("/props");
    props.setFragment({});
    QUrlQuery query;
    query.addQueryItem("autoload", "false");
    props.setQuery(query);
    if (core::NetworkPolicyService::instance().check(props) != core::NetworkDecision::Allowed) {
        probing_ = false;
        setReadiness(core::RuntimeReadinessState::Unauthorized,
                     "llama.cpp identity probe is blocked by network policy.");
        return;
    }
    QNetworkRequest request(props);
    request.setTransferTimeout(1500);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    auto* reply = dependencies_.get(request);
    probeReply_ = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, model, revision] {
        const auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto error = reply->error();
        const auto body = reply->readAll();
        reply->deleteLater();
        if (!acceptsObservation(revision) || probeReply_ != reply) {
            return;
        }
        probeReply_.clear();
        probing_ = false;
        const bool previouslyConfirmed = identityConfirmed_;
        identityConfirmed_ = false;
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(body, &parseError);
        const auto props = document.object();
        const auto alias = props.value("model_alias");
        const auto sleeping = props.value("is_sleeping");
        if (code != 200 || error != QNetworkReply::NoError ||
            parseError.error != QJsonParseError::NoError || !document.isObject() ||
            !alias.isString() || alias.toString().isEmpty() || !sleeping.isBool()) {
            setReadiness(core::RuntimeReadinessState::Unknown,
                         "llama.cpp is reachable; selected model identity is unverified.");
            return;
        }
        if (alias.toString() != model) {
            setReadiness(core::RuntimeReadinessState::MissingModel,
                         "llama.cpp is reachable; the selected model is not loaded.");
            return;
        }
        if (sleeping.toBool()) {
            setReadiness(core::RuntimeReadinessState::Unavailable,
                         "llama.cpp selected model is sleeping; readiness is unavailable.");
            return;
        }
        identityConfirmed_ = true;
        setReadiness(core::RuntimeReadinessState::Ready, "llama.cpp is ready.");
        if (!previouslyConfirmed) {
            dependencies_.refreshDiscovery();
        }
    });
}
void LocalModelRuntime::ensureRunning() {
    if (shuttingDown_) {
        return;
    }
    observeSelection();
    if (probing_) {
        return;
    }
    const auto selected = dependencies_.selection();
    if (selected.provider != "llama-cpp-server") {
        return;
    }
    if (selected.model.trimmed().isEmpty()) {
        setReadiness(core::RuntimeReadinessState::MissingModel,
                     "Select a GGUF model explicitly to start llama.cpp.");
        return;
    }
    const QUrl endpoint(selected.endpoint);
    if (endpoint.scheme() != "http" ||
        (endpoint.host() != "127.0.0.1" && endpoint.host() != "localhost" &&
         endpoint.host() != "::1") ||
        !endpoint.userInfo().isEmpty() ||
        core::NetworkPolicyService::instance().check(endpoint) != core::NetworkDecision::Allowed) {
        setReadiness(core::RuntimeReadinessState::InvalidEndpoint,
                     "llama.cpp endpoint is unavailable or not allowed.");
        return;
    }
    const auto selection = selected.model;
    const auto key = endpoint.toString() + "|" + selection;
    if (!processId_.isEmpty() && modelId_ != selection && !selected.busy) {
        dependencies_.terminate(processId_);
        processId_.clear();
        attemptedKey_.clear();
        scheduleReconcile(1000);
        return;
    }
    QUrl health = endpoint;
    health.setPath("/health");
    health.setQuery(QString{});
    QNetworkRequest request(health);
    request.setTransferTimeout(1500);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    const auto revision = ++selectionRevision_;
    setReadiness(core::RuntimeReadinessState::Unknown, "Checking llama.cpp model readiness…");
    probing_ = true;
    auto* reply = dependencies_.get(request);
    probeReply_ = reply;
    connect(
        reply, &QNetworkReply::finished, this, [this, reply, endpoint, selection, key, revision] {
            const auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const auto error = reply->error();
            reply->deleteLater();
            if (!acceptsObservation(revision) || probeReply_ != reply) {
                return;
            }
            probeReply_.clear();
            if (code == 200 && error == QNetworkReply::NoError) {
                setReadiness(core::RuntimeReadinessState::Unknown,
                             "llama.cpp is reachable; selected model identity is unverified.");
                verifyIdentity(endpoint, selection, revision);
                return;
            }
            probing_ = false;
            setReadiness(core::RuntimeReadinessState::Unavailable,
                         "llama.cpp health probe failed; readiness is unavailable.");
            // A loading/other existing listener owns this port. Do not start a competing server.
            if (code != 0 || error != QNetworkReply::ConnectionRefusedError ||
                !processId_.isEmpty() || attemptedKey_ == key) {
                return;
            }
            attemptedKey_ = key;
            QString file, alias;
            for (const auto& entry : dependencies_.entries()) {
                if (entry.format != "GGUF" || entry.localFile.isEmpty() ||
                    !QFileInfo::exists(entry.localFile)) {
                    continue;
                }
                if (entry.nativeModelId == selection || entry.displayName == selection) {
                    file = entry.localFile;
                    alias = selection;
                    break;
                }
            }
            if (file.isEmpty()) {
                setReadiness(core::RuntimeReadinessState::MissingModel,
                             "Download or import a GGUF model to start llama.cpp.");
                return;
            }
            const auto binary = dependencies_.findExecutable();
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
            // Explicit provider activation authorizes this fixed local runtime command, never a
            // model tool.
            launch.unconfinedPermitted = true;
            launch.environment = QProcessEnvironment::systemEnvironment();
            modelId_ = selection;
            setReadiness(core::RuntimeReadinessState::Unknown,
                         "Starting llama.cpp; loading the selected model…");
            processId_ = dependencies_.start(launch, [this](const core::ProcessRecord& record) {
                if (shuttingDown_) {
                    return;
                }
                if (record.state == core::ProcessState::Failed ||
                    record.state == core::ProcessState::Exited ||
                    record.state == core::ProcessState::Cancelled) {
                    processId_.clear();
                    setReadiness(core::RuntimeReadinessState::Unavailable,
                                 record.error.isEmpty() ? "llama.cpp stopped." : record.error);
                    dependencies_.refreshDiscovery();
                }
            });
            if (dependencies_.record(processId_).state == core::ProcessState::Failed) {
                processId_.clear();
            }
        });
}
} // namespace sentinel::daemon
