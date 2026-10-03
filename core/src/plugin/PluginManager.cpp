// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginManager.h"
#include "sentinel/core/app/AppMetadata.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include "sentinel/core/plugin/PluginDependencyResolver.h"
#include "sentinel/core/plugin/PluginHostProtocol.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include "sentinel/core/security/CredentialStore.h"
#include "sentinel/core/security/ExternalDirectoryGate.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDirIterator>
#include <QEventLoop>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QSet>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <atomic>

namespace sentinel::core::plugin {

namespace {
int sdkDomain(const QString& value) {
    return QStringList{QStringLiteral("filesystem"), QStringLiteral("process"),
        QStringLiteral("network"), QStringLiteral("clipboard"), QStringLiteral("application"),
        QStringLiteral("system"), QStringLiteral("memory"), QStringLiteral("conversation"),
        QStringLiteral("audio"), QStringLiteral("browser"), QStringLiteral("agent"),
        QStringLiteral("external-service")}.indexOf(value);
}
int sdkAccess(const QString& value) {
    return QStringList{QStringLiteral("read"), QStringLiteral("write"),
        QStringLiteral("delete"), QStringLiteral("execute"),
        QStringLiteral("control"), QStringLiteral("invoke")}.indexOf(value);
}
int sdkResourceKind(const QString& value) {
    return QStringList{QStringLiteral("none"), QStringLiteral("argument"),
        QStringLiteral("argument-digest"), QStringLiteral("filesystem-path"),
        QStringLiteral("host"), QStringLiteral("provider")}.indexOf(value);
}
QStringList hostPermissionsFor(const ToolDescriptor& descriptor) {
    QStringList permissions{Permissions::ToolExecution};
    for (const auto& requirement : descriptor.authorizationRequirements) {
        if (requirement.domain == SecurityDomain::ExternalService &&
            requirement.staticResource.startsWith(QStringLiteral("credential:")) &&
            !permissions.contains(Permissions::CredentialUse))
            permissions.append(Permissions::CredentialUse);
        QString permission;
        switch (requirement.domain) {
        case SecurityDomain::FileSystem:
            permission = requirement.access == AccessMode::Read ? Permissions::FileSystemRead
                                                                 : Permissions::FileSystemWrite;
            break;
        case SecurityDomain::Browser:
            permission = Permissions::NetworkExternal;
            break;
        case SecurityDomain::Memory:
        case SecurityDomain::Conversation:
            permission = Permissions::DatabaseAccess;
            break;
        case SecurityDomain::Process:
            permission = Permissions::ProcessExecute;
            break;
        default:
            break;
        }
        if (!permission.isEmpty() && !permissions.contains(permission))
            permissions.append(permission);
    }
    return permissions;
}

QJsonObject brokerFailure(const QString& category) {
    return {{QStringLiteral("ok"), false}, {QStringLiteral("category"), category}};
}

bool onlyFields(const QJsonObject& object, const QStringList& names) {
    for (auto it = object.begin(); it != object.end(); ++it)
        if (!names.contains(it.key())) return false;
    return true;
}

bool isCancelled(const PlannedToolInvocation& invocation) {
    return (invocation.cancellation && invocation.cancellation->load()) ||
           (invocation.toolCancellation && invocation.toolCancellation->load());
}

bool authorizedRequest(const PlannedToolInvocation& invocation, SecurityDomain domain,
                       AccessMode access, const QString& resource) {
    if (!invocation.resourceSnapshot || !invocation.resourceSnapshot->authorized) return false;
    for (const auto& item : invocation.resourceSnapshot->requests)
        if (item.domain == domain && item.access == access && item.resource == resource)
            return true;
    return false;
}

QJsonObject filesystemBroker(const QString& capability, const QJsonObject& payload,
                             const PlannedToolInvocation& invocation,
                             const ExternalDirectoryGate* gate) {
    const bool writing = capability == QStringLiteral("FilesystemWrite");
    const auto denied = writing ? QStringLiteral("PluginFilesystemWriteDenied")
                                : QStringLiteral("PluginFilesystemReadDenied");
    if (isCancelled(invocation)) return brokerFailure(QStringLiteral("PluginCancelled"));
    if (!gate || !invocation.resourceSnapshot || !invocation.resourceSnapshot->authorized ||
        !onlyFields(payload, writing
            ? QStringList{QStringLiteral("operation"), QStringLiteral("argument"),
                          QStringLiteral("path"), QStringLiteral("contentBase64")}
            : QStringList{QStringLiteral("operation"), QStringLiteral("argument"),
                          QStringLiteral("path")}) ||
        payload.value(QStringLiteral("operation")).toString() !=
            (writing ? QStringLiteral("write") : QStringLiteral("read")) ||
        !payload.value(QStringLiteral("argument")).isString() ||
        !payload.value(QStringLiteral("path")).isString()) return brokerFailure(denied);
    const auto argument = payload.value(QStringLiteral("argument")).toString();
    const auto rawPath = payload.value(QStringLiteral("path")).toString();
    const auto access = writing ? AccessMode::Write : AccessMode::Read;
    const auto filesystemAccess = writing ? FileSystemAccess::Write : FileSystemAccess::Read;
    QtFileSystemService service(gate);
    for (const auto& resource : invocation.resourceSnapshot->files) {
        if (resource.argument != argument || resource.access != access ||
            !resource.patchAction.isEmpty() || resource.path.displayPath != rawPath ||
            resource.path.access != filesystemAccess ||
            !authorizedRequest(invocation, SecurityDomain::FileSystem, access,
                               resource.path.canonicalPath)) continue;
        const auto checked = service.revalidateAuthorized(resource.path,
            invocation.resourceSnapshot->workingDirectory);
        if (!checked.ok() || checked.value->canonicalPath != resource.path.canonicalPath)
            return brokerFailure(QStringLiteral("PluginResourceDenied"));
        if (!gate->isAccessAllowed(resource.path.canonicalPath,
                                   invocation.resourceSnapshot->workingDirectory, writing))
            return brokerFailure(QStringLiteral("PluginResourceDenied"));
        if (isCancelled(invocation)) return brokerFailure(QStringLiteral("PluginCancelled"));
        if (!writing) {
            const auto result = service.readFile(*checked.value, 256 * 1024);
            if (!result.ok()) return brokerFailure(denied);
            if (isCancelled(invocation)) return brokerFailure(QStringLiteral("PluginCancelled"));
            return {{QStringLiteral("ok"), true},
                    {QStringLiteral("contentBase64"), QString::fromLatin1(result.value->content.toBase64())},
                    {QStringLiteral("truncated"), result.value->truncated},
                    {QStringLiteral("binary"), result.value->binary}};
        }
        if (!payload.value(QStringLiteral("contentBase64")).isString()) return brokerFailure(denied);
        const auto encoded = payload.value(QStringLiteral("contentBase64")).toString().toLatin1();
        if (encoded.size() > 350000) return brokerFailure(denied);
        const auto decoded = QByteArray::fromBase64Encoding(encoded, QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded || decoded.decoded.size() > 256 * 1024) return brokerFailure(denied);
        const auto result = service.writeFile(*checked.value, decoded.decoded, false);
        if (!result.ok()) return brokerFailure(denied);
        if (isCancelled(invocation)) return brokerFailure(QStringLiteral("PluginCancelled"));
        return {{QStringLiteral("ok"), true},
                {QStringLiteral("bytesWritten"), result.value->bytesWritten},
                {QStringLiteral("created"), result.value->created},
                {QStringLiteral("mutation"), QJsonObject{
                    {QStringLiteral("path"), resource.path.displayPath},
                    {QStringLiteral("kind"), result.value->created
                        ? QStringLiteral("Created") : QStringLiteral("Modified")}}}};
    }
    return brokerFailure(QStringLiteral("PluginResourceDenied"));
}

bool networkResourceAuthorized(const PlannedToolInvocation& invocation,
                               const QString& argument, const QString& rawUrl, const QUrl& url,
                               const QString& method) {
    if (!invocation.descriptorSnapshot || !invocation.resourceSnapshot ||
        !invocation.resourceSnapshot->authorized) return false;
    bool matchedArgument = false;
    for (const auto& item : invocation.arguments)
        if (item.id == argument && item.value == rawUrl) matchedArgument = true;
    if (!matchedArgument) return false;
    for (const auto& requirement : invocation.descriptorSnapshot->authorizationRequirements) {
        if (requirement.domain != SecurityDomain::Network ||
            requirement.resourceKind != AuthorizationResourceKind::Host ||
            requirement.resourceArgument != argument) continue;
        if (method == QStringLiteral("GET") && requirement.access != AccessMode::Read) continue;
        if (method == QStringLiteral("POST") && requirement.access != AccessMode::Write) continue;
        if (authorizedRequest(invocation, SecurityDomain::Network, requirement.access,
                              url.host().toLower())) return true;
    }
    return false;
}

QJsonObject networkBroker(const QString& pluginId, const PluginManifest& manifest,
                          const PluginSandbox& sandbox, const QString& invocationId,
                          const QString& requestId, const QJsonObject& payload,
                          const PlannedToolInvocation& invocation,
                          PluginHostSession& session) {
    if (isCancelled(invocation) || !session.invocationActive(invocationId))
        return brokerFailure(QStringLiteral("PluginCancelled"));
    const auto method = payload.value(QStringLiteral("method")).toString();
    const auto rawUrl = payload.value(QStringLiteral("url")).toString();
    const auto argument = payload.value(QStringLiteral("urlArgument")).toString();
    const QUrl url(rawUrl);
    if (!onlyFields(payload, {QStringLiteral("method"), QStringLiteral("url"),
                              QStringLiteral("urlArgument"), QStringLiteral("headers"),
                              QStringLiteral("bodyBase64"), QStringLiteral("timeoutMs"),
                              QStringLiteral("credentialId")}) ||
        !QStringList{QStringLiteral("GET"), QStringLiteral("POST")}.contains(method) ||
        rawUrl.size() > 2048 || !url.isValid() || url.host().isEmpty() ||
        (payload.contains(QStringLiteral("headers")) &&
         !payload.value(QStringLiteral("headers")).isObject()) ||
        (payload.contains(QStringLiteral("credentialId")) &&
         !payload.value(QStringLiteral("credentialId")).isString()) ||
        (payload.contains(QStringLiteral("timeoutMs")) &&
         !payload.value(QStringLiteral("timeoutMs")).isDouble()) ||
        !QStringList{QStringLiteral("http"), QStringLiteral("https")}.contains(url.scheme()) ||
        !url.userInfo().isEmpty() || url.hasFragment() ||
        !networkResourceAuthorized(invocation, argument, rawUrl, url, method))
        return brokerFailure(QStringLiteral("PluginNetworkDenied"));
    const auto decision = NetworkPolicyService::instance().check(url);
    if (decision == NetworkDecision::Offline)
        return brokerFailure(QStringLiteral("PluginNetworkOffline"));
    if (decision != NetworkDecision::Allowed)
        return brokerFailure(QStringLiteral("PluginNetworkDenied"));
    const bool local = url.host().compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0 ||
                       QHostAddress(url.host()).isLoopback();
    if (!sandbox.checkPermission(pluginId, local ? Permissions::NetworkLoopback :
                                                 Permissions::NetworkExternal))
        return brokerFailure(QStringLiteral("PluginNetworkDenied"));
    const auto timeoutMs = payload.value(QStringLiteral("timeoutMs")).toInt(10000);
    if (timeoutMs < 1000 || timeoutMs > 15000)
        return brokerFailure(QStringLiteral("PluginNetworkDenied"));
    QByteArray body;
    if (method == QStringLiteral("POST")) {
        if (!payload.value(QStringLiteral("bodyBase64")).isString() ||
            payload.value(QStringLiteral("bodyBase64")).toString().size() > 350000)
            return brokerFailure(QStringLiteral("PluginNetworkDenied"));
        const auto decoded = QByteArray::fromBase64Encoding(
            payload.value(QStringLiteral("bodyBase64")).toString().toLatin1(),
            QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded || decoded.decoded.size() > 256 * 1024)
            return brokerFailure(QStringLiteral("PluginNetworkDenied"));
        body = decoded.decoded;
    } else if (payload.contains(QStringLiteral("bodyBase64"))) {
        return brokerFailure(QStringLiteral("PluginNetworkDenied"));
    }
    QNetworkRequest networkRequest(url);
    networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                QNetworkRequest::ManualRedirectPolicy);
    const auto headers = payload.value(QStringLiteral("headers")).toObject();
    if (headers.size() > 8) return brokerFailure(QStringLiteral("PluginNetworkDenied"));
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        const auto name = it.key().toLower();
        if (!it.value().isString() ||
            !QStringList{QStringLiteral("accept"), QStringLiteral("content-type"),
                         QStringLiteral("user-agent")}.contains(name) ||
            it.value().toString().size() > 256 ||
            it.value().toString().contains(QLatin1Char('\r')) ||
            it.value().toString().contains(QLatin1Char('\n')))
            return brokerFailure(QStringLiteral("PluginNetworkDenied"));
        networkRequest.setRawHeader(name.toLatin1(), it.value().toString().toUtf8());
    }
    QString secret;
    const auto credentialId = payload.value(QStringLiteral("credentialId")).toString();
    if (!credentialId.isEmpty()) {
        bool toolDeclaredCredential = false;
        if (invocation.descriptorSnapshot) {
            for (const auto& requirement :
                 invocation.descriptorSnapshot->authorizationRequirements) {
                if (requirement.domain == SecurityDomain::ExternalService &&
                    requirement.access == AccessMode::Invoke &&
                    requirement.staticResource ==
                        QStringLiteral("credential:%1").arg(credentialId)) {
                    toolDeclaredCredential = true;
                    break;
                }
            }
        }
        if (url.scheme() != QStringLiteral("https") ||
            !toolDeclaredCredential ||
            !sandbox.checkPermission(pluginId, Permissions::CredentialUse) ||
            !authorizedRequest(invocation, SecurityDomain::ExternalService,
                               AccessMode::Invoke, QStringLiteral("credential:%1").arg(credentialId)))
            return brokerFailure(QStringLiteral("PluginCredentialDenied"));
        bool bound = false;
        for (const auto& declaration : manifest.credentials)
            if (declaration.id == credentialId && declaration.allowedHosts.contains(url.host().toLower()) &&
                (declaration.kind == QStringLiteral("token") ||
                 declaration.kind == QStringLiteral("apiKey"))) bound = true;
        if (!bound) return brokerFailure(QStringLiteral("PluginCredentialDenied"));
        auto store = defaultCredentialStore();
        if (store.summary().status != CredentialStoreStatus::Ready)
            return brokerFailure(QStringLiteral("PluginCredentialDenied"));
        auto retrieved = store.readCredential({QStringLiteral("plugin.") + pluginId, credentialId});
        if (!retrieved.result.succeeded || !retrieved.secret || retrieved.secret->isEmpty())
            return brokerFailure(QStringLiteral("PluginCredentialDenied"));
        secret = *retrieved.secret;
        retrieved.secret->fill(QChar(0));
        retrieved.secret.reset();
        auto authorizationHeader = QByteArray("Bearer ") + secret.toUtf8();
        networkRequest.setRawHeader("Authorization", authorizationHeader);
        authorizationHeader.fill('\0');
        secret.fill(QChar(0));
    }
    QNetworkAccessManager transport;
    transport.setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    QNetworkReply* reply = method == QStringLiteral("GET")
        ? transport.get(networkRequest) : transport.post(networkRequest, body);
    networkRequest.setRawHeader("Authorization", {});
    body.fill('\0');
    if (!reply) return brokerFailure(QStringLiteral("PluginHostCapabilityUnavailable"));
    reply->setReadBufferSize(256 * 1024 + 1);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QByteArray responseBody;
    bool oversized = false;
    bool timedOut = false;
    QObject::connect(reply, &QNetworkReply::readyRead, &loop, [&] {
        responseBody += reply->read(256 * 1024 + 1 - responseBody.size());
        if (responseBody.size() > 256 * 1024) { oversized = true; reply->abort(); }
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&] {
        timedOut = true; reply->abort(); loop.quit();
    });
    if (!session.trackBrokerOperation(invocationId, requestId,
                                      QStringLiteral("NetworkRequest"),
                                      [&] { reply->abort(); loop.quit(); })) {
        reply->abort(); reply->deleteLater();
        return brokerFailure(QStringLiteral("PluginInvocationExpired"));
    }
    timer.start(timeoutMs);
    if (!reply->isFinished()) loop.exec();
    session.finishBrokerOperation(invocationId, requestId);
    responseBody += reply->read(256 * 1024 + 1 - responseBody.size());
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto error = reply->error();
    reply->deleteLater();
    if (!session.invocationActive(invocationId) || isCancelled(invocation))
        return brokerFailure(QStringLiteral("PluginCancelled"));
    if (timedOut) return brokerFailure(QStringLiteral("PluginTimeout"));
    if (oversized || responseBody.size() > 256 * 1024)
        return brokerFailure(QStringLiteral("PluginNetworkDenied"));
    if (status >= 300 && status < 400) return brokerFailure(QStringLiteral("PluginNetworkDenied"));
    if (error != QNetworkReply::NoError)
        return brokerFailure(QStringLiteral("PluginNetworkDenied"));
    return {{QStringLiteral("ok"), true}, {QStringLiteral("status"), status},
            {QStringLiteral("bodyBase64"), QString::fromLatin1(responseBody.toBase64())}};
}

QJsonObject processBroker(const QString& invocationId, const QString& requestId,
                          const QJsonObject& payload,
                          const PlannedToolInvocation& invocation,
                          const ExternalDirectoryGate* gate,
                          const QString& privateDirectory,
                          PluginHostSession& session) {
    if (isCancelled(invocation) || !session.invocationActive(invocationId))
        return brokerFailure(QStringLiteral("PluginCancelled"));
    if (!onlyFields(payload, {QStringLiteral("program"), QStringLiteral("programArgument"),
                              QStringLiteral("argumentsArgument"), QStringLiteral("arguments"),
                              QStringLiteral("timeoutMs")}) ||
        !gate || !invocation.descriptorSnapshot || !invocation.resourceSnapshot ||
        !invocation.resourceSnapshot->authorized ||
        !payload.value(QStringLiteral("program")).isString() ||
        !payload.value(QStringLiteral("programArgument")).isString() ||
        !payload.value(QStringLiteral("argumentsArgument")).isString() ||
        !payload.value(QStringLiteral("arguments")).isArray() ||
        (payload.contains(QStringLiteral("timeoutMs")) &&
         !payload.value(QStringLiteral("timeoutMs")).isDouble()))
        return brokerFailure(QStringLiteral("PluginProcessDenied"));
    const auto rawProgram = payload.value(QStringLiteral("program")).toString();
    const auto programArgument = payload.value(QStringLiteral("programArgument")).toString();
    const auto argumentsArgument = payload.value(QStringLiteral("argumentsArgument")).toString();
    const QFileInfo programInfo(rawProgram);
    const auto program = programInfo.canonicalFilePath();
    if (!programInfo.isAbsolute() || !programInfo.isExecutable() || program.isEmpty() ||
        rawProgram != program) return brokerFailure(QStringLiteral("PluginProcessDenied"));
    bool argumentBound = false;
    for (const auto& arg : invocation.arguments)
        if (arg.id == programArgument && arg.value == rawProgram) argumentBound = true;
    bool requirementBound = false;
    for (const auto& requirement : invocation.descriptorSnapshot->authorizationRequirements)
        if (requirement.domain == SecurityDomain::Process &&
            requirement.access == AccessMode::Execute &&
            requirement.resourceKind == AuthorizationResourceKind::Argument &&
            requirement.resourceArgument == programArgument) requirementBound = true;
    if (!argumentBound || !requirementBound ||
        !authorizedRequest(invocation, SecurityDomain::Process, AccessMode::Execute, rawProgram))
        return brokerFailure(QStringLiteral("PluginResourceDenied"));
    QStringList arguments;
    const auto suppliedArguments = payload.value(QStringLiteral("arguments")).toArray();
    if (suppliedArguments.size() > 32) return brokerFailure(QStringLiteral("PluginProcessDenied"));
    bool argumentsBound = false;
    for (const auto& arg : invocation.arguments)
        if (arg.id == argumentsArgument && arg.jsonValue.isArray() &&
            arg.jsonValue.toArray() == suppliedArguments) argumentsBound = true;
    if (!argumentsBound) return brokerFailure(QStringLiteral("PluginResourceDenied"));
    for (const auto& value : suppliedArguments) {
        if (!value.isString() || value.toString().size() > 4096 ||
            value.toString().contains(QChar(0)))
            return brokerFailure(QStringLiteral("PluginProcessDenied"));
        arguments.append(value.toString());
    }
    const auto timeoutMs = payload.value(QStringLiteral("timeoutMs")).toInt(10000);
    if (timeoutMs < 1000 || timeoutMs > 15000)
        return brokerFailure(QStringLiteral("PluginProcessDenied"));
    const auto resourceCwd = invocation.resourceSnapshot->workingDirectory;
    const auto workdir = QFileInfo(privateDirectory).canonicalFilePath();
    if (workdir.isEmpty() || !QFileInfo(workdir).isDir())
        return brokerFailure(QStringLiteral("PluginProcessDenied"));
    QtFileSystemService filesystem(gate);
    SandboxExecutionPlan plan;
    plan.workingDirectory = workdir;
    plan.readablePaths.append(program);
    plan.writablePaths.append(workdir);
    plan.networkAllowed = false;
    plan.requireEnforcement = true;
    plan.restrictedEnvironment = true;
    plan.forbidDetachedChildren = true;
    for (const auto& resource : invocation.resourceSnapshot->files) {
        const auto checked = filesystem.revalidateAuthorized(resource.path, resourceCwd);
        if (!checked.ok() || checked.value->canonicalPath != resource.path.canonicalPath)
            return brokerFailure(QStringLiteral("PluginResourceDenied"));
        if (!gate->isAccessAllowed(resource.path.canonicalPath, resourceCwd,
                                   resource.access != AccessMode::Read))
            return brokerFailure(QStringLiteral("PluginResourceDenied"));
        if (resource.access == AccessMode::Read)
            plan.readablePaths.append(resource.path.canonicalPath);
        else if (resource.access == AccessMode::Write)
            plan.writablePaths.append(resource.path.canonicalPath);
    }
    ProcessRequest processRequest;
    processRequest.program = program;
    processRequest.arguments = arguments;
    processRequest.workingDirectory = workdir;
    processRequest.environment = QProcessEnvironment();
    processRequest.environment.insert(QStringLiteral("PATH"), QFileInfo(program).absolutePath());
    processRequest.timeoutMs = timeoutMs;
    processRequest.sandbox = plan;
    processRequest.sessionId = invocationId;
    processRequest.toolCallId = requestId;
    QEventLoop loop;
    ProcessRecord terminal;
    bool done = false;
    bool oversized = false;
    QByteArray output;
    QByteArray errors;
    ProcessExecutor executor;
    const auto processId = executor.start(processRequest, [&](const ProcessRecord& record) {
        if (record.state == ProcessState::Failed || record.state == ProcessState::Exited ||
            record.state == ProcessState::Cancelled) {
            terminal = record;
            done = true;
            loop.quit();
        }
    }, [&](const QString& id, ProcessStream stream, const QByteArray& bytes) {
        auto& target = stream == ProcessStream::Stdout ? output : errors;
        const auto remaining = 64 * 1024 - target.size();
        if (bytes.size() > remaining) {
            target += bytes.left(remaining);
            oversized = true;
            executor.kill(id);
        } else target += bytes;
    });
    if (processId.isEmpty()) return brokerFailure(QStringLiteral("PluginProcessDenied"));
    if (!done && !session.trackBrokerOperation(invocationId, requestId,
                                                QStringLiteral("ProcessExecute"),
                                                [&] { executor.kill(processId); loop.quit(); })) {
        executor.kill(processId);
        return brokerFailure(QStringLiteral("PluginInvocationExpired"));
    }
    if (!done) loop.exec();
    session.finishBrokerOperation(invocationId, requestId);
    if (!session.invocationActive(invocationId) || isCancelled(invocation))
        return brokerFailure(QStringLiteral("PluginCancelled"));
    if (terminal.timedOut) return brokerFailure(QStringLiteral("PluginTimeout"));
    if (oversized || terminal.state != ProcessState::Exited ||
        terminal.sandbox.enforcement != SandboxEnforcement::Enforced)
        return brokerFailure(QStringLiteral("PluginProcessDenied"));
    if (terminal.exitCode != 0)
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("category"), QStringLiteral("PluginProcessFailed")},
                {QStringLiteral("exitCode"), terminal.exitCode},
                {QStringLiteral("stdoutBase64"), QString::fromLatin1(output.toBase64())},
                {QStringLiteral("stderrBase64"), QString::fromLatin1(errors.toBase64())}};
    return {{QStringLiteral("ok"), true}, {QStringLiteral("exitCode"), terminal.exitCode},
            {QStringLiteral("stdoutBase64"), QString::fromLatin1(output.toBase64())},
            {QStringLiteral("stderrBase64"), QString::fromLatin1(errors.toBase64())}};
}

QString escapedId(const QString& value) {
    QString result;
    for (const auto byte : value.toUtf8()) {
        const auto c = static_cast<unsigned char>(byte);
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            result += QLatin1Char(c);
        else
            result += QStringLiteral("_%1_").arg(c, 2, 16, QLatin1Char('0'));
    }
    return result;
}

class RemotePluginToolHandler final : public IToolHandler {
public:
    RemotePluginToolHandler(std::weak_ptr<PluginHostSession> host,
                            std::shared_ptr<PluginSandbox> sandbox, QString pluginId,
                            QString localId, QStringList permissions, bool processRequired)
        : host_(std::move(host)), sandbox_(std::move(sandbox)), pluginId_(std::move(pluginId)),
          localId_(std::move(localId)), permissions_(std::move(permissions)),
          processRequired_(processRequired) {}
    IToolExecutor::Cancel execute(const ToolExecutionRequest& request, const QString&,
                                  const QString&, IToolExecutor::Output,
                                  IToolExecutor::Completion completion) override {
        const auto host = host_.lock();
        if (!host ||
            !std::all_of(permissions_.cbegin(), permissions_.cend(),
                         [this](const QString& value) { return sandbox_->checkPermission(pluginId_, value); })) {
            ToolExecutionResult result{ToolExecutionStatus::Blocked, QStringLiteral("Plugin unavailable or permission denied")};
            result.failureCategory = host ? ToolFailureCategory::SecurityDenied : ToolFailureCategory::RuntimeUnavailable;
            completion(result);
            return {};
        }
        if (request.plan.invocations.isEmpty()) {
            completion({ToolExecutionStatus::InvalidArguments, QStringLiteral("Missing plugin invocation")});
            return {};
        }
        if (processRequired_ && request.sandbox.status != SandboxStatus::Allowed) {
            ToolExecutionResult result{ToolExecutionStatus::Blocked, QStringLiteral("PluginProcessDenied")};
            result.failureCategory = ToolFailureCategory::SecurityDenied;
            completion(result);
            return {};
        }
        QJsonObject arguments;
        for (const auto& argument : request.plan.invocations.first().arguments)
            arguments.insert(argument.id, argument.jsonValue.isUndefined()
                ? QJsonValue(argument.value) : argument.jsonValue);
        auto requestId = std::make_shared<QString>();
        auto cancelled = std::make_shared<std::atomic_bool>(false);
        QPointer<QObject> callbackContext = request.callbackContext;
        auto authorization = request.plan.invocations.first();
        if (!authorization.toolCancellation) authorization.toolCancellation = cancelled;
        const auto toolCancellation = authorization.toolCancellation;
        QMetaObject::invokeMethod(host.get(), [host, localId = localId_, arguments,
            authorization, requestId, cancelled, callbackContext,
            completion = std::move(completion)]() mutable {
          *requestId = host->invoke(localId, arguments, authorization,
            [callbackContext, completion = std::move(completion)](QJsonObject response) mutable {
                auto deliver = [completion = std::move(completion), response]() mutable {
                ToolExecutionResult result;
                const auto category = response.value(QStringLiteral("category")).toString();
                result.status =
                    response.value(QStringLiteral("ok")).toBool()   ? ToolExecutionStatus::Succeeded
                    : category == QStringLiteral("PluginCancelled") ? ToolExecutionStatus::Cancelled
                                                                    : ToolExecutionStatus::Failed;
                result.summary = response.value(QStringLiteral("summary")).toString().left(65536);
                if (result.status == ToolExecutionStatus::Succeeded) {
                    auto observation = std::make_shared<StructuredObservation>();
                    observation->kind = StructuredObservationKind::Generic;
                    observation->data = {{QStringLiteral("summary"), result.summary}};
                    result.structuredObservation = std::move(observation);
                }
                if (result.status != ToolExecutionStatus::Succeeded) {
                    result.summary = category;
                    result.failureCategory =
                        category == QStringLiteral("PluginCancelled")
                            ? ToolFailureCategory::Cancelled
                        : category == QStringLiteral("PluginTimeout") ? ToolFailureCategory::Timeout
                        : (category == QStringLiteral("PluginCancelled") ||
                           category == QStringLiteral("PluginInvocationExpired"))
                            ? ToolFailureCategory::Cancelled
                        : category == QStringLiteral("PluginProtocolError")
                            ? ToolFailureCategory::ProtocolError
                        : (category == QStringLiteral("PluginCredentialDenied") ||
                           category == QStringLiteral("PluginCredentialNotDeclared") ||
                           category == QStringLiteral("PluginHostCapabilityDenied") ||
                           category == QStringLiteral("PluginHostCapabilityNotDeclared") ||
                           category == QStringLiteral("PluginResourceDenied") ||
                           category == QStringLiteral("PluginFilesystemReadDenied") ||
                           category == QStringLiteral("PluginFilesystemWriteDenied") ||
                           category == QStringLiteral("PluginNetworkDenied") ||
                           category == QStringLiteral("PluginProcessDenied"))
                            ? ToolFailureCategory::SecurityDenied
                        : category == QStringLiteral("PluginNetworkOffline")
                            ? ToolFailureCategory::NetworkFailure
                        : category == QStringLiteral("PluginProcessFailed")
                            ? ToolFailureCategory::RemoteExecutionFailure
                            : ToolFailureCategory::RuntimeUnavailable;
                }
                completion(std::move(result));
                };
                if (callbackContext) QMetaObject::invokeMethod(callbackContext.data(), std::move(deliver), Qt::QueuedConnection);
                else deliver();
            });
          if (cancelled->load() && !requestId->isEmpty()) host->cancel(*requestId);
        }, Qt::QueuedConnection);
        return [host, requestId, cancelled, toolCancellation] {
            cancelled->store(true);
            toolCancellation->store(true);
            QMetaObject::invokeMethod(host.get(), [host, requestId] {
                if (!requestId->isEmpty()) host->cancel(*requestId);
            }, Qt::QueuedConnection);
        };
    }
private:
    std::weak_ptr<PluginHostSession> host_;
    std::shared_ptr<PluginSandbox> sandbox_;
    QString pluginId_;
    QString localId_;
    QStringList permissions_;
    bool processRequired_ = false;
};
} // namespace

PluginManager::PluginManager(QString coreVersion, QString pluginStorageDir, QObject* parent)
    : QObject(parent),
      m_coreVersion(coreVersion.isEmpty() ? AppMetadata::version() : std::move(coreVersion)),
      m_pluginStorageDir(std::move(pluginStorageDir)) {
    if (m_pluginStorageDir.isEmpty()) {
        m_pluginStorageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                             QStringLiteral("/plugins");
    }
}

PluginManager::~PluginManager() {
    unloadAll();
}

void PluginManager::setPluginStorageDir(const QString& dir) {
    m_pluginStorageDir = dir;
}

QString PluginManager::pluginStorageDir() const {
    return m_pluginStorageDir;
}

PluginSandbox& PluginManager::sandbox() {
    return *m_sandbox;
}

const PluginSandbox& PluginManager::sandbox() const {
    return *m_sandbox;
}

void PluginManager::setToolRegistry(IToolRegistry* registry) {
    m_toolRegistry = registry;
}

QList<PluginCredentialState> PluginManager::credentialStates(const QString& pluginId) const {
    QList<PluginCredentialState> states;
    const auto it = m_plugins.constFind(pluginId);
    if (it == m_plugins.cend()) return states;
    auto store = defaultCredentialStore();
    for (const auto& declaration : it->manifest.credentials) {
        const auto result = store.containsCredential(
            {QStringLiteral("plugin.") + pluginId, declaration.id});
        states.append({pluginId, declaration, result.succeeded,
                       store.summary().status == CredentialStoreStatus::Ready});
    }
    return states;
}

bool PluginManager::setCredential(const QString& pluginId, const QString& credentialId,
                                  const QString& value) {
    const auto it = m_plugins.constFind(pluginId);
    if (it == m_plugins.cend() || value.isEmpty() || value.size() > 8192) return false;
    for (const auto& declaration : it->manifest.credentials) {
        if (declaration.id != credentialId) continue;
        auto store = defaultCredentialStore();
        const CredentialKey key{QStringLiteral("plugin.") + pluginId, credentialId};
        if (!store.storeCredential(key, value).succeeded) return false;
        const auto readback = store.readCredential(key);
        return readback.result.succeeded && readback.secret == value;
    }
    return false;
}

bool PluginManager::clearCredential(const QString& pluginId, const QString& credentialId) {
    const auto it = m_plugins.constFind(pluginId);
    if (it == m_plugins.cend()) return false;
    for (const auto& declaration : it->manifest.credentials) {
        if (declaration.id == credentialId)
            return defaultCredentialStore().deleteCredential(
                {QStringLiteral("plugin.") + pluginId, credentialId}).succeeded;
    }
    return false;
}

int PluginManager::discoverPlugins(const QString& searchDir) {
    QString targetDir = searchDir.isEmpty() ? m_pluginStorageDir : searchDir;
    QDir dir(targetDir);
    const QString absoluteRoot = QFileInfo(targetDir).absoluteFilePath();
    if (!dir.exists()) {
        for (auto it = m_plugins.begin(); it != m_plugins.end();) {
            if (it->pluginFilePath == absoluteRoot ||
                it->pluginFilePath.startsWith(absoluteRoot + QDir::separator())) {
                const auto id = it.key();
                unloadPlugin(id);
                m_sandbox->clearPlugin(id);
                it = m_plugins.erase(it);
                emit pluginRemoved(id);
            } else ++it;
        }
        m_orderedIds = m_plugins.keys();
        return 0;
    }

    int discoveredCount = 0;
    QSet<QString> seenIds;
    const QString root = absoluteRoot + QDir::separator();

    // 1. Search for directory-based plugins containing plugin.json
    QDirIterator it(targetDir, QStringList() << QStringLiteral("plugin.json"), QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString manifestPath = it.filePath();
        QString error;
        PluginManifest manifest = PluginManifest::parseFile(manifestPath, &error);

        if (manifest.isValid()) {
            if (seenIds.contains(manifest.id)) {
                unloadPlugin(manifest.id);
                auto& duplicate = m_plugins[manifest.id];
                duplicate.failureCategory = QStringLiteral("PluginDuplicateId");
                duplicate.errorString = duplicate.failureCategory;
                updateState(duplicate, PluginState::Error);
                emit pluginError(manifest.id, duplicate.errorString);
                continue;
            }
            seenIds.insert(manifest.id);
            const auto existing = m_plugins.constFind(manifest.id);
            if (existing != m_plugins.cend() &&
                QFileInfo(existing->pluginFilePath).absoluteFilePath() !=
                    QFileInfo(manifestPath).absolutePath()) {
                emit pluginError(manifest.id, QStringLiteral("PluginIdentityCollision"));
                continue;
            }
            if (m_plugins.contains(manifest.id) &&
                m_plugins.value(manifest.id).state != PluginState::Unloaded)
                continue;
            PluginDescriptor desc;
            desc.manifest = manifest;
            desc.pluginFilePath = QFileInfo(manifestPath).absolutePath(); // directory
            if (!manifest.isCompatibleWithCore(m_coreVersion) ||
                manifest.apiVersion != QStringLiteral("5.0")) {
                desc.state = PluginState::Error;
                desc.failureCategory = QStringLiteral("PluginIncompatible");
                desc.errorString = desc.failureCategory;
            }

            m_sandbox->registerPluginPermissions(manifest.id, manifest.permissions);
            m_plugins[manifest.id] = desc;
            emit pluginDiscovered(manifest.id);
            discoveredCount++;
        }
    }

    // Native plugins require an explicit directory manifest. Binary metadata is never
    // inspected through a loader in the main process.

    for (auto pluginIt = m_plugins.begin(); pluginIt != m_plugins.end();) {
        if ((pluginIt->pluginFilePath == absoluteRoot || pluginIt->pluginFilePath.startsWith(root)) &&
            !seenIds.contains(pluginIt.key())) {
            const auto id = pluginIt.key();
            unloadPlugin(id);
            m_sandbox->clearPlugin(id);
            pluginIt = m_plugins.erase(pluginIt);
            emit pluginRemoved(id);
        } else ++pluginIt;
    }

    // Re-resolve load order
    QList<PluginManifest> manifests;
    for (const auto& desc : m_plugins) {
        manifests.append(desc.manifest);
    }
    ResolutionResult res = PluginDependencyResolver::resolve(manifests);
    if (res.success) {
        m_orderedIds = res.loadOrder;
    } else {
        qWarning()
            << QStringLiteral("Plugin dependency resolution warning: %1").arg(res.errorMessage);
        m_orderedIds = m_plugins.keys();
    }

    return discoveredCount;
}

bool PluginManager::loadPlugin(const QString& pluginId) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) return false;
    auto& desc = it.value();
    if (desc.state != PluginState::Unloaded && desc.state != PluginState::Disabled)
        return desc.state != PluginState::Error;
    desc.failureCategory.clear();
    desc.errorString.clear();
    if (!desc.manifest.isValid() || !desc.manifest.isCompatibleWithCore(m_coreVersion) ||
        desc.manifest.apiVersion != QStringLiteral("5.0")) {
        desc.failureCategory = QStringLiteral("PluginIncompatible");
        desc.errorString = desc.failureCategory;
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    const QStringList supportedPermissions{Permissions::NetworkLoopback, Permissions::NetworkExternal,
        Permissions::ModelConfigRead, Permissions::ModelConfigWrite, Permissions::FileSystemRead,
        Permissions::FileSystemWrite, Permissions::ToolExecution, Permissions::DatabaseAccess,
            Permissions::CredentialUse, Permissions::ProcessExecute};
    for (const auto& permission : desc.manifest.permissions.toList()) {
        if (!supportedPermissions.contains(permission)) {
            desc.failureCategory = QStringLiteral("PluginIncompatible");
            desc.errorString = desc.failureCategory;
            updateState(desc, PluginState::Error);
            emit pluginError(pluginId, desc.errorString);
            return false;
        }
    }
    QString libPath = desc.pluginFilePath;
    if (QFileInfo(libPath).isDir()) {
        QDir dir(libPath);
        libPath = dir.filePath(desc.manifest.entryPoint);
        if (!QLibrary::isLibrary(libPath)) {
#if defined(Q_OS_WIN)
            libPath += QStringLiteral(".dll");
#elif defined(Q_OS_MACOS)
            libPath = dir.filePath(QStringLiteral("lib") + desc.manifest.entryPoint + QStringLiteral(".dylib"));
#else
            libPath = dir.filePath(QStringLiteral("lib") + desc.manifest.entryPoint + QStringLiteral(".so"));
#endif
        }
    }
    const auto canonicalPluginDir = QFileInfo(desc.pluginFilePath).canonicalFilePath();
    const auto canonicalLibrary = QFileInfo(libPath).canonicalFilePath();
    const auto relativeLibrary = QDir(canonicalPluginDir).relativeFilePath(canonicalLibrary);
    if (!QFileInfo(libPath).isFile() || canonicalPluginDir.isEmpty() ||
        canonicalLibrary.isEmpty() ||
        QFileInfo(relativeLibrary).isAbsolute() || relativeLibrary == QStringLiteral("..") ||
        relativeLibrary.startsWith(QStringLiteral("../")) ||
        relativeLibrary.startsWith(QStringLiteral("..\\")) ||
        relativeLibrary == QStringLiteral(".")) {
        desc.failureCategory = QStringLiteral("PluginLoadFailure");
        desc.errorString = desc.failureCategory;
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    m_sandbox->setActive(pluginId, true);
    auto host = std::make_shared<PluginHostSession>();
    host->setPluginIdentity(pluginId);
    host->setHostRequestHandler([this, pluginId, generation = std::weak_ptr<PluginHostSession>(host)](
                                                 const QString& invocationId, const QString& toolId,
                                                 const QJsonObject& request,
                                                 const PlannedToolInvocation& authorization,
                                                 PluginHostSession& session) -> QJsonObject {
        const auto denied = [](const QString& category) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("category"), category}};
        };
        const auto found = m_plugins.constFind(pluginId);
        const auto currentHost = generation.lock();
        if (found == m_plugins.cend() || !currentHost || found->host != currentHost ||
            request.value(QStringLiteral("pluginId")).toString() != pluginId)
            return denied(QStringLiteral("PluginInvocationExpired"));
        const auto recordResult = [this, pluginId, &currentHost](QJsonObject result) {
            auto entry = m_plugins.find(pluginId);
            if (entry != m_plugins.end() && entry->host == currentHost &&
                !result.value(QStringLiteral("ok")).toBool())
                entry->lastCapabilityFailure = result.value(QStringLiteral("category")).toString();
            return result;
        };
        if (request.value(QStringLiteral("kind")) == QStringLiteral("capability")) {
            const auto capability = request.value(QStringLiteral("capability")).toString();
            if (!found->manifest.hostCapabilities.contains(capability))
                return recordResult(denied(QStringLiteral("PluginHostCapabilityNotDeclared")));
            bool toolDeclared = false;
            for (const auto& item : found->remoteTools) {
                const auto tool = item.toObject();
                if (tool.value(QStringLiteral("id")).toString() != toolId) continue;
                for (const auto& value : tool.value(QStringLiteral("hostCapabilities")).toArray())
                    if (value.toString() == capability) toolDeclared = true;
            }
            if (!toolDeclared)
                return recordResult(denied(QStringLiteral("PluginHostCapabilityNotDeclared")));
            if (!request.value(QStringLiteral("payload")).isObject())
                return recordResult(denied(QStringLiteral("PluginProtocolError")));
            const auto payload = request.value(QStringLiteral("payload")).toObject();
            if (capability == QStringLiteral("FilesystemRead") ||
                capability == QStringLiteral("FilesystemWrite")) {
                const auto permission = capability == QStringLiteral("FilesystemRead")
                    ? Permissions::FileSystemRead : Permissions::FileSystemWrite;
                if (!m_sandbox->checkPermission(pluginId, permission))
                    return recordResult(denied(QStringLiteral("PluginHostCapabilityDenied")));
                return recordResult(filesystemBroker(capability, payload, authorization, m_resourceGate));
            }
            if (capability == QStringLiteral("NetworkRequest")) {
                const auto manifest = found->manifest;
                return recordResult(networkBroker(pluginId, manifest, *m_sandbox, invocationId,
                    request.value(QStringLiteral("id")).toString(), payload,
                    authorization, session));
            }
            if (capability == QStringLiteral("ProcessExecute")) {
                if (!m_sandbox->checkPermission(pluginId, Permissions::ProcessExecute))
                    return recordResult(denied(QStringLiteral("PluginProcessDenied")));
                return recordResult(processBroker(invocationId, request.value(QStringLiteral("id")).toString(),
                    payload, authorization, m_resourceGate,
                    m_pluginStorageDir + QLatin1Char('/') + pluginId, session));
            }
            return recordResult(denied(QStringLiteral("PluginHostCapabilityUnavailable")));
        }
        return denied(QStringLiteral("PluginCredentialDenied"));
    });
    connect(host.get(), &PluginHostSession::failed, this,
            [this, pluginId, generation = std::weak_ptr<PluginHostSession>(host)](const QString& category) {
        auto found = m_plugins.find(pluginId);
        if (found == m_plugins.end() || !found->host ||
            found->host != generation.lock()) return;
        m_sandbox->setActive(pluginId, false);
        if (m_toolRegistry) m_toolRegistry->unregisterProvider(ToolSource::Plugin,
            QStringLiteral("plugin:%1").arg(pluginId));
        found->failureCategory = category;
        found->errorString = category;
        updateState(found.value(), PluginState::Error);
        emit pluginError(pluginId, category);
    });
    desc.host = host;
    if (!host->start(canonicalLibrary, m_pluginStorageDir + QLatin1Char('/') + pluginId)) {
        desc.failureCategory = host->failureCategory();
        desc.errorString = desc.failureCategory;
        desc.host.reset();
        m_sandbox->setActive(pluginId, false);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    const auto result = host->call(QStringLiteral("load"),
        {{QStringLiteral("path"), canonicalLibrary},
         {QStringLiteral("pluginId"), pluginId},
         {QStringLiteral("abi"), NativePluginAbiVersion},
         {QStringLiteral("hostCapabilities"), QJsonArray::fromStringList(desc.manifest.hostCapabilities)},
         {QStringLiteral("permissions"), m_sandbox->getPermissions(pluginId).toJsonArray()}});
    if (!result.value(QStringLiteral("ok")).toBool() ||
        !result.value(QStringLiteral("tools")).isArray()) {
        desc.failureCategory = result.value(QStringLiteral("category")).toString(QStringLiteral("PluginLoadFailure"));
        desc.errorString = desc.failureCategory;
        host->shutdown();
        desc.host.reset();
        m_sandbox->setActive(pluginId, false);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    desc.remoteTools = result.value(QStringLiteral("tools")).toArray();
    updateState(desc, PluginState::Loaded);
    emit pluginLoaded(pluginId);
    return true;
}

bool PluginManager::initializePlugin(const QString& pluginId) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) return false;
    if (it->state == PluginState::Unloaded && !loadPlugin(pluginId)) return false;
    if (it->state == PluginState::Initialized || it->state == PluginState::Active) return true;
    if (it->state != PluginState::Loaded || !it->host ||
        (!it->remoteTools.isEmpty() && !m_toolRegistry)) return false;
    updateState(it.value(), PluginState::Initialized);
    return true;
}

bool PluginManager::registerRemoteTools(const QString& pluginId) {
    auto& desc = m_plugins[pluginId];
    if (!desc.host) return false;
    if (desc.remoteTools.isEmpty()) return true;
    if (!m_toolRegistry ||
        !m_sandbox->checkPermission(pluginId, Permissions::ToolExecution)) {
        desc.failureCategory = QStringLiteral("PluginPermissionDenied");
        desc.errorString = desc.failureCategory;
        unloadPlugin(pluginId);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    QList<IToolRegistry::Registration> registrations;
    QSet<QString> names;
    bool valid = true;
    for (const auto& value : desc.remoteTools) {
        if (!value.isObject()) { valid = false; break; }
        const auto item = value.toObject();
        const auto localId = item.value(QStringLiteral("id")).toString().trimmed();
        const auto schema = item.value(QStringLiteral("schema")).toObject();
        if (localId.isEmpty() || names.contains(localId) ||
            schema.value(QStringLiteral("type")) != QStringLiteral("object") ||
            schema.value(QStringLiteral("additionalProperties")) != false) { valid = false; break; }
        const auto properties = schema.value(QStringLiteral("properties")).toObject();
        for (const auto& required : schema.value(QStringLiteral("required")).toArray())
            if (!required.isString() || !properties.contains(required.toString())) valid = false;
        for (auto property = properties.begin(); property != properties.end(); ++property)
            if (!property.value().isObject() ||
                !property.value().toObject().contains(QStringLiteral("type"))) valid = false;
        if (!valid) break;
        QSet<QString> toolCapabilities;
        for (const auto& capabilityValue : item.value(QStringLiteral("hostCapabilities")).toArray()) {
            const auto capability = capabilityValue.toString();
            if (!capabilityValue.isString() ||
                !desc.manifest.hostCapabilities.contains(capability) ||
                toolCapabilities.contains(capability)) valid = false;
            toolCapabilities.insert(capability);
        }
        if (!valid) break;
        names.insert(localId);
        ToolDescriptor tool;
        tool.id = QStringLiteral("plugin.%1.%2").arg(escapedId(pluginId), escapedId(localId));
        tool.name = item.value(QStringLiteral("name")).toString(localId);
        tool.description = item.value(QStringLiteral("description")).toString();
        tool.category = item.value(QStringLiteral("category")).toString();
        tool.inputSchema = schema;
        tool.source = ToolSource::Plugin;
        tool.providerId = QStringLiteral("plugin:%1").arg(pluginId);
        tool.version = desc.manifest.version;
        tool.executionMode = ToolExecutionMode::Local;
        tool.requiredPermissionDomain = QStringLiteral("tool-execution");
        const auto riskName = item.value(QStringLiteral("risk")).toString();
        const auto risk = riskName == QStringLiteral("low") ? int(ToolRiskLevel::Low) :
            riskName == QStringLiteral("medium") ? int(ToolRiskLevel::Medium) :
            riskName == QStringLiteral("high") ? int(ToolRiskLevel::High) : -1;
        if (risk < 0 || risk > int(ToolRiskLevel::High)) { valid = false; break; }
        tool.riskLevel = ToolRiskLevel(risk);
        for (const auto& requirement : item.value(QStringLiteral("requirements")).toArray()) {
            if (!requirement.isObject()) { valid = false; break; }
            const auto object = requirement.toObject();
            const auto domain = sdkDomain(object.value(QStringLiteral("domain")).toString());
            const auto access = sdkAccess(object.value(QStringLiteral("access")).toString());
            const auto kind = sdkResourceKind(object.value(QStringLiteral("resourceKind")).toString());
            if (domain < 0 || domain > int(SecurityDomain::ExternalService) ||
                access < 0 || access > int(AccessMode::Invoke) ||
                kind < 0 || kind > int(AuthorizationResourceKind::Provider)) { valid = false; break; }
            tool.authorizationRequirements.append({SecurityDomain(domain), AccessMode(access),
                AuthorizationResourceKind(kind), object.value(QStringLiteral("resourceArgument")).toString(),
                object.value(QStringLiteral("staticResource")).toString()});
        }
        if (!valid) break;
        for (const auto& capability : toolCapabilities) {
            const bool declaredResource = std::any_of(
                tool.authorizationRequirements.cbegin(),
                tool.authorizationRequirements.cend(), [&](const auto& requirement) {
                    if (capability == QStringLiteral("FilesystemRead"))
                        return requirement.domain == SecurityDomain::FileSystem &&
                               requirement.access == AccessMode::Read &&
                               requirement.resourceKind == AuthorizationResourceKind::FileSystemPath;
                    if (capability == QStringLiteral("FilesystemWrite"))
                        return requirement.domain == SecurityDomain::FileSystem &&
                               requirement.access == AccessMode::Write &&
                               requirement.resourceKind == AuthorizationResourceKind::FileSystemPath;
                    if (capability == QStringLiteral("NetworkRequest"))
                        return requirement.domain == SecurityDomain::Network &&
                               (requirement.access == AccessMode::Read ||
                                requirement.access == AccessMode::Write) &&
                               requirement.resourceKind == AuthorizationResourceKind::Host &&
                               !requirement.resourceArgument.isEmpty();
                    if (capability == QStringLiteral("ProcessExecute"))
                        return requirement.domain == SecurityDomain::Process &&
                               requirement.access == AccessMode::Execute &&
                               requirement.resourceKind == AuthorizationResourceKind::Argument;
                    return false;
                });
            if (!declaredResource) valid = false;
        }
        if (!valid) break;
        if (tool.authorizationRequirements.isEmpty())
            tool.authorizationRequirements = {{SecurityDomain::Application, AccessMode::Invoke,
                AuthorizationResourceKind::Provider, {}, tool.providerId}};
        // A plugin result proves only the actual provider invocation, never
        // filesystem/process/network facts merely claimed by plugin text.
        tool.structuredObservationKind = StructuredObservationKind::Generic;
        tool.evidenceProduced = {{ObservationDomain::ExternalService,
                                  EvidenceFreshness::TurnScoped,
                                  EvidenceScope::Provider,
                                  {},
                                  {}}};
        const auto permissions = hostPermissionsFor(tool);
        if (toolCapabilities.contains(QStringLiteral("NetworkRequest")) &&
            !m_sandbox->checkPermission(pluginId, Permissions::NetworkLoopback) &&
            !m_sandbox->checkPermission(pluginId, Permissions::NetworkExternal)) {
            desc.failureCategory = QStringLiteral("PluginPermissionDenied");
            valid = false;
        }
        for (const auto& permission : permissions)
            if (!m_sandbox->checkPermission(pluginId, permission)) {
                desc.failureCategory = QStringLiteral("PluginPermissionDenied");
                valid = false;
            }
        if (!valid) break;
        registrations.append({std::move(tool), std::make_shared<RemotePluginToolHandler>(
            desc.host, m_sandbox, pluginId, localId, permissions,
            toolCapabilities.contains(QStringLiteral("ProcessExecute")))});
    }
    if (!valid || !m_toolRegistry->replaceProvider(ToolSource::Plugin,
            QStringLiteral("plugin:%1").arg(pluginId), std::move(registrations))) {
        if (desc.failureCategory.isEmpty()) desc.failureCategory = QStringLiteral("PluginLoadFailure");
        desc.errorString = desc.failureCategory;
        unloadPlugin(pluginId);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    return true;
}

bool PluginManager::startPlugin(const QString& pluginId) {
    if (!initializePlugin(pluginId)) return false;
    auto& desc = m_plugins[pluginId];
    if (desc.state == PluginState::Active) return true;
    const auto result = desc.host->call(QStringLiteral("start"));
    if (!result.value(QStringLiteral("ok")).toBool()) {
        desc.failureCategory = result.value(QStringLiteral("category")).toString(QStringLiteral("PluginLoadFailure"));
        desc.errorString = desc.failureCategory;
        unloadPlugin(pluginId);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    if (!registerRemoteTools(pluginId)) return false;
    updateState(desc, PluginState::Active);
    return true;
}

bool PluginManager::stopPlugin(const QString& pluginId) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) return false;
    if (it->state != PluginState::Active) return true;
    if (!it->host || !it->host->call(QStringLiteral("stop")).value(QStringLiteral("ok")).toBool()) {
        it->failureCategory = QStringLiteral("PluginHostUnavailable");
        if (m_toolRegistry) m_toolRegistry->unregisterProvider(ToolSource::Plugin,
            QStringLiteral("plugin:%1").arg(pluginId));
        updateState(it.value(), PluginState::Error);
        return false;
    }
    if (m_toolRegistry) m_toolRegistry->unregisterProvider(ToolSource::Plugin,
        QStringLiteral("plugin:%1").arg(pluginId));
    updateState(it.value(), PluginState::Initialized);
    return true;
}

bool PluginManager::unloadPlugin(const QString& pluginId) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) return false;
    m_sandbox->setActive(pluginId, false);
    if (m_toolRegistry) m_toolRegistry->unregisterProvider(ToolSource::Plugin,
        QStringLiteral("plugin:%1").arg(pluginId));
    if (it->host) {
        it->host->shutdown();
        it->host.reset();
    }
    it->remoteTools = {};
    updateState(it.value(), PluginState::Unloaded);
    emit pluginUnloaded(pluginId);
    return true;
}

bool PluginManager::setEnabled(const QString& pluginId, bool enabled) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end())
        return false;
    if (!enabled) {
        if (it->state != PluginState::Disabled && !unloadPlugin(pluginId))
            return false;
        updateState(it.value(), PluginState::Disabled);
        return true;
    }
    if (it->state == PluginState::Disabled)
        updateState(it.value(), PluginState::Unloaded);
    return startPlugin(pluginId);
}

bool PluginManager::reloadPlugin(const QString& pluginId) {
    if (!m_plugins.contains(pluginId)) {
        qWarning()
            << QStringLiteral("PluginManager::reloadPlugin: Plugin '%1' not found").arg(pluginId);
        return false;
    }

    auto& desc = m_plugins[pluginId];
    if (desc.state == PluginState::Disabled)
        return false;
    PluginState previousState = desc.state;
    QSet<QString> declaredIds;
    for (const auto& credential : desc.manifest.credentials) {
        auto hosts = credential.allowedHosts;
        hosts.sort();
        declaredIds.insert(credential.id + QLatin1Char(':') + credential.kind +
                           (credential.required ? QLatin1String(":required") :
                                                  QLatin1String(":optional")) +
                           QLatin1Char(':') + hosts.join(QLatin1Char(',')));
    }
    for (const auto& capability : desc.manifest.hostCapabilities)
        declaredIds.insert(QStringLiteral("host:") + capability);
    for (const auto& permission : desc.manifest.permissions.toList())
        declaredIds.insert(QStringLiteral("permission:") + permission);
    m_reloadCredentialIds.insert(pluginId, declaredIds);
    m_reloadManifests.insert(pluginId, desc.manifest);

    qDebug() << QStringLiteral(
                    "PluginManager::reloadPlugin: Reloading plugin '%1' (previous state: %2)")
                    .arg(pluginId)
                    .arg(static_cast<int>(previousState));

    // Unload the plugin completely
    if (!unloadPlugin(pluginId)) {
        m_reloadCredentialIds.remove(pluginId);
        m_reloadManifests.remove(pluginId);
        emit pluginReloadFailed(pluginId, QStringLiteral("Failed to unload plugin"));
        return false;
    }
    QTimer::singleShot(
        20, this, [this, pluginId, previousState] { finishReload(pluginId, previousState, 0); });
    return true;
}

void PluginManager::finishReload(const QString& pluginId, PluginState previousState, int attempts) {
    if (isModuleResident(pluginId)) {
        if (attempts >= 250) {
            emit pluginReloadFailed(pluginId, QStringLiteral("Active plugin calls did not finish"));
            return;
        }
        QTimer::singleShot(20, this, [this, pluginId, previousState, attempts] {
            finishReload(pluginId, previousState, attempts + 1);
        });
        return;
    }
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) {
        emit pluginReloadFailed(pluginId, QStringLiteral("Plugin disappeared during reload"));
        return;
    }
    auto& desc = it.value();

    // Re-discover the plugin (in case manifest changed)
    QString pluginDir = QFileInfo(desc.pluginFilePath).absoluteDir().absolutePath();
    discoverPlugins(pluginDir);

    const auto previousCredentials = m_reloadCredentialIds.take(pluginId);
    const auto refreshed = m_plugins.constFind(pluginId);
    if (refreshed == m_plugins.cend()) {
        m_reloadManifests.remove(pluginId);
        emit pluginReloadFailed(pluginId, QStringLiteral("Plugin disappeared during discovery"));
        return;
    }
    const auto previousManifest = m_reloadManifests.take(pluginId);
    const auto rejectWidening = [&desc, &previousManifest, this, &pluginId](const QString& message) {
        desc.manifest = previousManifest;
        m_sandbox->registerPluginPermissions(pluginId, previousManifest.permissions);
        m_sandbox->setActive(pluginId, false);
        desc.failureCategory = QStringLiteral("PluginHostCapabilityDenied");
        desc.errorString = message;
        updateState(desc, PluginState::Error);
        emit pluginReloadFailed(pluginId, message);
    };
    for (const auto& credential : refreshed->manifest.credentials) {
        auto hosts = credential.allowedHosts;
        hosts.sort();
        if (!previousCredentials.contains(credential.id + QLatin1Char(':') + credential.kind +
            (credential.required ? QLatin1String(":required") : QLatin1String(":optional")) +
            QLatin1Char(':') + hosts.join(QLatin1Char(',')))) {
            rejectWidening(QStringLiteral("New credential declarations require a fresh plugin discovery"));
            return;
        }
    }
    for (const auto& capability : refreshed->manifest.hostCapabilities) {
        if (!previousCredentials.contains(QStringLiteral("host:") + capability)) {
            rejectWidening(QStringLiteral("New host capabilities require a fresh plugin discovery"));
            return;
        }
    }
    for (const auto& permission : refreshed->manifest.permissions.toList()) {
        if (!previousCredentials.contains(QStringLiteral("permission:") + permission)) {
            rejectWidening(QStringLiteral("New permissions require a fresh plugin discovery"));
            return;
        }
    }

    // Reload and restore previous state
    if (!loadPlugin(pluginId)) {
        emit pluginReloadFailed(pluginId, QStringLiteral("Failed to load plugin after unload"));
        return;
    }

    // Restore to the previous state
    if (previousState == PluginState::Initialized || previousState == PluginState::Active ||
        previousState == PluginState::Error) {
        if (!initializePlugin(pluginId)) {
            emit pluginReloadFailed(pluginId,
                                    QStringLiteral("Failed to initialize plugin after reload"));
            return;
        }
    }

    if (previousState == PluginState::Active || previousState == PluginState::Error) {
        if (!startPlugin(pluginId)) {
            emit pluginReloadFailed(pluginId,
                                    QStringLiteral("Failed to start plugin after reload"));
            return;
        }
    }

    qDebug() << QStringLiteral("PluginManager::reloadPlugin: Successfully reloaded plugin '%1'")
                    .arg(pluginId);
    emit pluginReloaded(pluginId);
}

void PluginManager::enableHotReload(bool enabled) {
    if (enabled == m_hotReloadEnabled) {
        return;
    }

    m_hotReloadEnabled = enabled;

    if (enabled) {
        if (!m_hotReloader) {
            m_hotReloader = std::make_unique<PluginHotReloader>(this, this);
            connect(m_hotReloader.get(), &PluginHotReloader::reloadRequested, this,
                    &PluginManager::onHotReloadRequested);
        }

        HotReloadConfig config;
        config.enabled = true;
        config.watchedDirs << m_pluginStorageDir;
        m_hotReloader->setConfig(config);
        m_hotReloader->startWatching();

        qDebug() << "PluginManager: Hot-reload enabled";
    } else {
        if (m_hotReloader) {
            m_hotReloader->stopWatching();
        }
        qDebug() << "PluginManager: Hot-reload disabled";
    }
}

bool PluginManager::isHotReloadEnabled() const {
    return m_hotReloadEnabled;
}

void PluginManager::setHotReloadConfig(const HotReloadConfig& config) {
    if (m_hotReloader) {
        m_hotReloader->setConfig(config);
    }
}

HotReloadConfig PluginManager::hotReloadConfig() const {
    if (m_hotReloader) {
        return m_hotReloader->config();
    }
    return {};
}

bool PluginManager::initializeAll() {
    bool allSuccess = true;
    for (const QString& id : m_orderedIds) {
        if (!initializePlugin(id)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool PluginManager::startAll() {
    bool allSuccess = true;
    for (const QString& id : m_orderedIds) {
        if (!startPlugin(id)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool PluginManager::stopAll() {
    bool allSuccess = true;
    for (int i = m_orderedIds.size() - 1; i >= 0; --i) {
        if (!stopPlugin(m_orderedIds[i])) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

void PluginManager::unloadAll() {
    for (int i = m_orderedIds.size() - 1; i >= 0; --i) {
        unloadPlugin(m_orderedIds[i]);
    }
}

QList<QString> PluginManager::registeredPluginIds() const {
    return m_orderedIds;
}

bool PluginManager::isLoaded(const QString& pluginId) const {
    if (!m_plugins.contains(pluginId)) {
        return false;
    }
    PluginState s = m_plugins.value(pluginId).state;
    return s == PluginState::Loaded || s == PluginState::Initialized || s == PluginState::Active;
}

PluginState PluginManager::pluginState(const QString& pluginId) const {
    if (!m_plugins.contains(pluginId)) {
        return PluginState::Unloaded;
    }
    return m_plugins.value(pluginId).state;
}

const PluginDescriptor* PluginManager::descriptor(const QString& pluginId) const {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) {
        return nullptr;
    }
    return &it.value();
}

QObject* PluginManager::pluginInstance(const QString&) const {
    return nullptr;
}

bool PluginManager::isModuleResident(const QString&) const {
    return false;
}

void PluginManager::onHotReloadRequested(const QString& pluginId) {
    qDebug() << QStringLiteral("PluginManager: Hot-reload requested for plugin '%1'").arg(pluginId);
    reloadPlugin(pluginId);
}

void PluginManager::updateState(PluginDescriptor& desc, PluginState newState) {
    if (desc.state != newState) {
        desc.state = newState;
        emit pluginStateChanged(desc.manifest.id, newState);
    }
}

} // namespace sentinel::core::plugin
