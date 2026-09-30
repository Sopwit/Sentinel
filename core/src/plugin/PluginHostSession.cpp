// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/plugin/PluginHostSession.h"
#include "sentinel/core/plugin/PluginHostProtocol.h"
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcessEnvironment>
#include <QTimer>
#include <QUuid>
#include <utility>

namespace sentinel::core::plugin {
PluginHostSession::PluginHostSession(QObject* parent) : QObject(parent) {}
PluginHostSession::~PluginHostSession() { shutdown(); }

void PluginHostSession::setPluginIdentity(QString pluginId) {
    if (!processId_.isEmpty()) return;
    pluginId_ = std::move(pluginId);
    generationId_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void PluginHostSession::setHostRequestHandler(HostRequestHandler handler) {
    hostRequestHandler_ = std::move(handler);
}

bool PluginHostSession::start(const QString& pluginBinary, const QString& dataDirectory) {
    auto executable = QCoreApplication::applicationDirPath() + QDir::separator() +
        QStringLiteral("sentinel-plugin-host")
#if defined(Q_OS_WIN)
        + QStringLiteral(".exe")
#endif
        ;
    if (!QFileInfo(executable).isExecutable()) {
        const auto sibling = QDir(QCoreApplication::applicationDirPath()).filePath(
            QStringLiteral("../apps/sentinel-plugin-host/sentinel-plugin-host")
#if defined(Q_OS_WIN)
            + QStringLiteral(".exe")
#endif
        );
        if (QFileInfo(sibling).isExecutable()) executable = QFileInfo(sibling).absoluteFilePath();
    }
    if (!QFileInfo(executable).isExecutable()) {
        const auto sibling = QDir(QCoreApplication::applicationDirPath()).filePath(
            QStringLiteral("../sentinel-plugin-host/sentinel-plugin-host")
#if defined(Q_OS_WIN)
            + QStringLiteral(".exe")
#endif
        );
        if (QFileInfo(sibling).isExecutable()) executable = QFileInfo(sibling).absoluteFilePath();
    }
    if (!QFileInfo(executable).isExecutable()) {
        fail(QStringLiteral("PluginHostUnavailable")); return false;
    }
    if (!QDir().mkpath(dataDirectory)) {
        fail(QStringLiteral("PluginSandboxUnavailable")); return false;
    }
    const auto canonicalParent = QFileInfo(QFileInfo(dataDirectory).absolutePath()).canonicalFilePath();
    const auto canonicalData = QFileInfo(dataDirectory).canonicalFilePath();
    if (canonicalParent.isEmpty() || canonicalData.isEmpty() ||
        canonicalData != QDir(canonicalParent).filePath(QFileInfo(dataDirectory).fileName())) {
        fail(QStringLiteral("PluginSandboxUnavailable")); return false;
    }
    ProcessRequest request;
    request.program = executable;
    request.workingDirectory = canonicalData;
    request.timeoutMs = 0;
    request.environment = QProcessEnvironment();
    request.environment.insert(QStringLiteral("PATH"), qEnvironmentVariable("PATH"));
    SandboxExecutionPlan sandbox;
    sandbox.workingDirectory = canonicalData;
    sandbox.readablePaths = {QFileInfo(pluginBinary).absolutePath(), QFileInfo(executable).absolutePath()};
    sandbox.writablePaths = {canonicalData};
    sandbox.networkAllowed = false;
    sandbox.requireEnforcement = true;
    sandbox.forbidDetachedChildren = true;
    request.sandbox = sandbox;
    stopping_ = false;
    processId_ = process_.start(request,
        [this](const ProcessRecord& record) {
            if (record.state == ProcessState::Failed || record.state == ProcessState::Exited ||
                record.state == ProcessState::Cancelled) {
                if (!stopping_) fail(record.sandbox.enforcement != SandboxEnforcement::Enforced
                    ? QStringLiteral("PluginSandboxUnavailable")
                    : record.state == ProcessState::Failed
                        ? QStringLiteral("PluginHostStartFailure") : QStringLiteral("PluginCrashed"));
            }
        }, [this](const QString&, ProcessStream stream, const QByteArray& bytes) {
            if (stream == ProcessStream::Stdout) receive(bytes);
        });
    if (processId_.isEmpty() || process_.record(processId_).state == ProcessState::Failed)
        return false;
    const auto response = call(QStringLiteral("health"), {}, 5000);
    if (!response.value(QStringLiteral("ok")).toBool() ||
        response.value(QStringLiteral("abi")).toInt(-1) != NativePluginAbiVersion ||
        response.value(QStringLiteral("protocolVersion")).toInt(-1) != PluginHostProtocolVersion) {
        fail(QStringLiteral("PluginIncompatible"));
        return false;
    }
    return true;
}

QJsonObject PluginHostSession::call(const QString& operation, QJsonObject fields, int timeoutMs) {
    QEventLoop loop;
    QJsonObject response;
    const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    fields.insert(QStringLiteral("protocol"), PluginHostProtocolVersion);
    fields.insert(QStringLiteral("id"), id);
    fields.insert(QStringLiteral("op"), operation);
    pending_.insert(id, [&response, &loop](QJsonObject value) { response = value; loop.quit(); });
    if (!process_.write(processId_, pluginHostFrame(fields))) {
        pending_.remove(id);
        fail(QStringLiteral("PluginHostUnavailable"));
        return {{QStringLiteral("category"), failureCategory_}};
    }
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    if (response.isEmpty() && failureCategory_.isEmpty()) loop.exec();
    pending_.remove(id);
    if (response.isEmpty()) {
        fail(QStringLiteral("PluginTimeout"));
        return {{QStringLiteral("category"), failureCategory_}};
    }
    return response;
}

QString PluginHostSession::invoke(const QString& toolId, const QJsonObject& arguments,
                                  const PlannedToolInvocation& authorization,
                                  std::function<void(QJsonObject)> completion) {
    if (!isRunning()) {
        completion({{QStringLiteral("ok"), false}, {QStringLiteral("category"), QStringLiteral("PluginHostUnavailable")}});
        return {};
    }
    const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    invocations_.insert(id, toolId);
    authorizations_.insert(id, authorization);
    pending_.insert(id, std::move(completion));
    if (!process_.write(processId_, pluginHostFrame({{QStringLiteral("protocol"), PluginHostProtocolVersion},
        {QStringLiteral("id"), id}, {QStringLiteral("op"), QStringLiteral("invoke")},
        {QStringLiteral("toolId"), toolId}, {QStringLiteral("arguments"), arguments}}))) {
        auto callback = pending_.take(id);
        invocations_.remove(id);
        authorizations_.remove(id);
        brokerFailures_.remove(id);
        callback({{QStringLiteral("ok"), false}, {QStringLiteral("category"), QStringLiteral("PluginHostUnavailable")}});
        return {};
    }
    QTimer::singleShot(30000, this, [this, id] {
        if (pending_.contains(id)) {
            auto callback = pending_.take(id);
            invocations_.remove(id);
            authorizations_.remove(id);
            brokerFailures_.remove(id);
            cancelBrokerOperations(id);
            callback({{QStringLiteral("ok"), false}, {QStringLiteral("category"), QStringLiteral("PluginTimeout")}});
            fail(QStringLiteral("PluginTimeout"));
        }
    });
    return id;
}

void PluginHostSession::cancel(const QString& requestId) {
    if (!pending_.contains(requestId)) return;
    auto callback = pending_.take(requestId);
    invocations_.remove(requestId);
    authorizations_.remove(requestId);
    brokerFailures_.remove(requestId);
    cancelBrokerOperations(requestId);
    callback({{QStringLiteral("ok"), false}, {QStringLiteral("category"), QStringLiteral("PluginCancelled")}});
    // Synchronous plugin calls have no cancellation hook. Killing the host bounds the call.
    process_.kill(processId_);
}

void PluginHostSession::shutdown() {
    if (stopping_) return;
    stopping_ = true;
    auto pending = std::move(pending_);
    pending_.clear();
    invocations_.clear();
    authorizations_.clear();
    brokerFailures_.clear();
    const auto operationIds = brokerOperations_.keys();
    for (const auto& id : operationIds) cancelBrokerOperations(id);
    if (pending.isEmpty() && isRunning() && failureCategory_.isEmpty())
        call(QStringLiteral("shutdown"), {}, 1000);
    if (!processId_.isEmpty()) {
        process_.terminate(processId_);
        process_.shutdown();
        processId_.clear();
    }
    for (auto callback : std::as_const(pending))
        callback({{QStringLiteral("ok"), false}, {QStringLiteral("category"), QStringLiteral("PluginCancelled")}});
}

bool PluginHostSession::isRunning() const {
    return !processId_.isEmpty() && process_.record(processId_).state == ProcessState::Running;
}

void PluginHostSession::receive(const QByteArray& bytes) {
    incoming_ += bytes;
    if (incoming_.size() > PluginHostMaxFrame) { fail(QStringLiteral("PluginProtocolError")); return; }
    while (incoming_.contains('\n')) {
        const auto end = incoming_.indexOf('\n');
        const auto frame = incoming_.left(end);
        incoming_.remove(0, end + 1);
        QJsonObject response;
        if (!parsePluginHostFrame(frame, &response) || !response.value(QStringLiteral("id")).isString()) {
            fail(QStringLiteral("PluginProtocolError")); return;
        }
        if (response.value(QStringLiteral("op")) == QStringLiteral("hostRequest")) {
            const auto invocationId = response.value(QStringLiteral("invocationId")).toString();
            const auto requestId = response.value(QStringLiteral("id")).toString();
            if (requestId.isEmpty() || requestId.size() > 128 || invocationId.isEmpty() ||
                invocationId.size() > 128 || !response.value(QStringLiteral("kind")).isString()) {
                fail(QStringLiteral("PluginProtocolError")); return;
            }
            QJsonObject result{{QStringLiteral("ok"), false},
                {QStringLiteral("category"), QStringLiteral("PluginInvocationExpired")}};
            if (invocations_.contains(invocationId) && pending_.contains(invocationId) &&
                hostRequestHandler_) {
                try {
                    result = hostRequestHandler_(invocationId, invocations_.value(invocationId),
                                                 response, authorizations_.value(invocationId), *this);
                } catch (...) {
                    result = {{QStringLiteral("ok"), false},
                              {QStringLiteral("category"), QStringLiteral("PluginHostCapabilityUnavailable")}};
                }
            }
            if (!invocationActive(invocationId)) continue;
            if (!result.value(QStringLiteral("ok")).toBool() &&
                !brokerFailures_.contains(invocationId))
                brokerFailures_.insert(invocationId,
                    result.value(QStringLiteral("category")).toString(
                        QStringLiteral("PluginHostCapabilityDenied")));
            result.insert(QStringLiteral("protocol"), PluginHostProtocolVersion);
            result.insert(QStringLiteral("op"), QStringLiteral("hostResponse"));
            result.insert(QStringLiteral("id"), requestId);
            auto outgoing = pluginHostFrame(result);
            result = {};
            const bool written = outgoing.size() <= PluginHostMaxFrame &&
                process_.write(processId_, outgoing);
            outgoing.fill('\0');
            if (!written) {
                fail(QStringLiteral("PluginProtocolError")); return;
            }
            continue;
        }
        if (response.contains(QStringLiteral("op"))) {
            fail(QStringLiteral("PluginProtocolError")); return;
        }
        const auto id = response.value(QStringLiteral("id")).toString();
        if (!pending_.contains(id)) { fail(QStringLiteral("PluginProtocolError")); return; }
        const bool brokerStillActive = !brokerOperations_.value(id).isEmpty();
        auto callback = pending_.take(id);
        invocations_.remove(id);
        authorizations_.remove(id);
        cancelBrokerOperations(id);
        const auto brokerFailure = brokerStillActive ? QStringLiteral("PluginProtocolError")
                                                      : brokerFailures_.take(id);
        if (brokerStillActive) brokerFailures_.remove(id);
        if (!brokerFailure.isEmpty()) {
            response.insert(QStringLiteral("ok"), false);
            response.insert(QStringLiteral("category"), brokerFailure);
            response.remove(QStringLiteral("summary"));
        }
        callback(response);
    }
}

void PluginHostSession::fail(const QString& category, const QString& detail) {
    if (stopping_ || !failureCategory_.isEmpty()) return;
    failureCategory_ = category;
    failureDetail_ = detail;
    auto pending = std::move(pending_);
    pending_.clear();
    invocations_.clear();
    authorizations_.clear();
    brokerFailures_.clear();
    const auto operationIds = brokerOperations_.keys();
    for (const auto& id : operationIds) cancelBrokerOperations(id);
    for (auto callback : std::as_const(pending))
        callback({{QStringLiteral("ok"), false}, {QStringLiteral("category"), category}});
    emit failed(category);
    if (!processId_.isEmpty()) process_.kill(processId_);
}

bool PluginHostSession::invocationActive(const QString& id) const {
    return invocations_.contains(id) && pending_.contains(id) && !stopping_ && failureCategory_.isEmpty();
}

bool PluginHostSession::trackBrokerOperation(const QString& invocationId,
                                             const QString& requestId,
                                             const QString& capability,
                                             std::function<void()> cancel) {
    if (!invocationActive(invocationId) || pluginId_.isEmpty() || generationId_.isEmpty() ||
        requestId.isEmpty() ||
        brokerOperations_.value(invocationId).size() >= 8 ||
        brokerOperations_.value(invocationId).contains(requestId)) return false;
    brokerOperations_[invocationId].insert(requestId,
        {pluginId_, generationId_, invocationId, requestId, capability, std::move(cancel)});
    return true;
}

void PluginHostSession::finishBrokerOperation(const QString& invocationId,
                                               const QString& requestId) {
    auto it = brokerOperations_.find(invocationId);
    if (it == brokerOperations_.end()) return;
    it->remove(requestId);
    if (it->isEmpty()) brokerOperations_.erase(it);
}

void PluginHostSession::cancelBrokerOperations(const QString& invocationId) {
    auto operations = brokerOperations_.take(invocationId);
    for (const auto& operation : std::as_const(operations))
        if (operation.cancel) operation.cancel();
}
} // namespace sentinel::core::plugin
