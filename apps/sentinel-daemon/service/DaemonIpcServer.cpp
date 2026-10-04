// SPDX-License-Identifier: GPL-3.0-or-later
#include "DaemonIpcServer.h"
#include "IpcContract.generated.h"
#include "sentinel/core/agent/IAgentRuntime.h"
#include "sentinel/core/app/AppMetadata.h"
#include "sentinel/core/app/ApplicationController.h"
#include "sentinel/core/chat/ChatModeService.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>
#include <QUuid>
#ifdef Q_OS_UNIX
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace sentinel::daemon {
namespace {
QJsonObject envelope(const QString& type, const QString& id, const QString& name,
                     const QJsonObject& payload) {
    return {{"version", QJsonObject{{"major", protocol::major}, {"minor", protocol::minor}}},
            {"type", type},
            {"id", id},
            {"name", name},
            {"payload", payload}};
}
bool withinBudget(const QJsonValue& value, qsizetype& budget) {
    budget -= 2;
    if (value.isString()) {
        budget -= value.toString().size();
    }
    if (budget < 0) {
        return false;
    }
    if (value.isArray()) {
        for (const auto& child : value.toArray()) {
            if (!withinBudget(child, budget)) {
                return false;
            }
        }
    }
    if (value.isObject()) {
        const auto object = value.toObject();
        for (auto it = object.begin(); it != object.end(); ++it) {
            budget -= it.key().size() + 3;
            if (budget < 0 || !withinBudget(it.value(), budget)) {
                return false;
            }
        }
    }
    return budget >= 0;
}
QString uuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
} // namespace
DaemonIpcServer::DaemonIpcServer(core::ApplicationController* controller, QObject* parent)
    : QObject(parent) {
    connect(&m_server, &QLocalServer::newConnection, this, &DaemonIpcServer::handleNewConnection);
    setController(controller);
}
DaemonIpcServer::~DaemonIpcServer() {
    stopServer();
    if (m_controller && !m_agentSubscription.isEmpty()) {
        m_controller->agentRuntime()->unsubscribe(m_agentSubscription);
    }
}
QString DaemonIpcServer::defaultSocketPath() {
#ifdef Q_OS_UNIX
    return QDir::homePath() + QStringLiteral("/.sentinel/run/daemon.sock");
#else
    return QStringLiteral("sentinel-daemon-%1").arg(qEnvironmentVariable("USERNAME"));
#endif
}
void DaemonIpcServer::setController(core::ApplicationController* controller) {
    if (m_controller) {
        disconnect(m_controller, nullptr, this, nullptr);
        if (!m_agentSubscription.isEmpty()) {
            m_controller->agentRuntime()->unsubscribe(m_agentSubscription);
        }
    }
    m_controller = controller;
    m_agentSubscription.clear();
    if (!controller) {
        return;
    }
    connect(controller, &core::ApplicationController::chatMessagesChanged, this,
            &DaemonIpcServer::onChatChanged);
    if (controller->agentRuntime()) {
        m_agentSubscription = controller->agentRuntime()->subscribe([this](
                                                                        const core::AgentEvent& e) {
            QMetaObject::invokeMethod(this, [this, e] { onAgentEvent(e); }, Qt::QueuedConnection);
        });
    }
}
bool DaemonIpcServer::startServer(const QString& requested) {
    if (m_server.isListening()) {
        return false;
    }
    m_path = requested.isEmpty() ? defaultSocketPath() : requested;
    if (!QFileInfo(m_path).isAbsolute()) {
        m_path = QDir::tempPath() + "/" + m_path;
    }
    const auto directory = QFileInfo(m_path).absolutePath();
    if (requested.isEmpty()) {
        const auto root = QFileInfo(directory).absolutePath();
        if (!QDir().mkpath(root)) {
            return false;
        }
#ifdef Q_OS_UNIX
        struct stat rootInfo{};
        if (::lstat(root.toLocal8Bit().constData(), &rootInfo) != 0 || !S_ISDIR(rootInfo.st_mode) ||
            rootInfo.st_uid != ::getuid()) {
            return false;
        }
        if (::chmod(root.toLocal8Bit().constData(), 0700) != 0) {
            return false;
        }
#endif
        if (!QDir().mkpath(directory)) {
            return false;
        }
    }
#ifdef Q_OS_UNIX
    struct stat directoryInfo{};
    if (::lstat(directory.toLocal8Bit().constData(), &directoryInfo) != 0 ||
        !S_ISDIR(directoryInfo.st_mode) || directoryInfo.st_uid != ::getuid()) {
        return false;
    }
    if (requested.isEmpty() && ::chmod(directory.toLocal8Bit().constData(), 0700) != 0) {
        return false;
    }
    if (!requested.isEmpty() && (directoryInfo.st_mode & 0077U) != 0) {
        return false;
    }
#endif
    m_lock = std::make_unique<QLockFile>(m_path + ".lock");
    m_lock->setStaleLockTime(0);
    if (!m_lock->tryLock()) {
        qWarning("Daemon socket already owned");
        return false;
    }
    if (QFileInfo::exists(m_path)) {
#ifdef Q_OS_UNIX
        struct stat info{};
        if (::lstat(m_path.toLocal8Bit().constData(), &info) != 0 || !S_ISSOCK(info.st_mode) ||
            info.st_uid != ::getuid()) {
            m_lock.reset();
            return false;
        }
#endif
        QLocalSocket probe;
        probe.connectToServer(m_path);
        if (probe.waitForConnected(100)) {
            m_lock.reset();
            return false;
        }
        if (probe.error() != QLocalSocket::ConnectionRefusedError &&
            probe.error() != QLocalSocket::ServerNotFoundError) {
            m_lock.reset();
            return false;
        }
        if (!QLocalServer::removeServer(m_path)) {
            m_lock.reset();
            return false;
        }
    }
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_server.listen(m_path)) {
        m_lock.reset();
        return false;
    }
    m_uptime.start();
    qInfo("Daemon IPC listening");
    return true;
}
void DaemonIpcServer::stopServer() {
    m_server.close();
    const auto sockets = m_clients.keys();
    for (auto* socket : sockets) {
        socket->abort();
        socket->deleteLater();
    }
    m_clients.clear();
    m_lock.reset();
}
void DaemonIpcServer::send(QLocalSocket* socket, const QJsonObject& message) {
    qsizetype budget = protocol::max_frame_bytes;
    QByteArray bytes;
    if (withinBudget(message, budget)) {
        bytes = QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
    }
    if (bytes.isEmpty() || bytes.size() > protocol::max_frame_bytes) {
        const auto failure = envelope("error", message.value("id").toString(), "ProtocolError",
                                      {{"code", "message-too-large"}});
        socket->write(QJsonDocument(failure).toJson(QJsonDocument::Compact) + '\n');
        socket->disconnectFromServer();
        return;
    }
    if (socket->bytesToWrite() + bytes.size() > protocol::max_pending_bytes) {
        qWarning("IPC slow client disconnected");
        socket->abort();
        return;
    }
    socket->write(bytes);
}
void DaemonIpcServer::error(QLocalSocket* socket, const QString& id, const QString& code) {
    qWarning().noquote() << "IPC protocol error:" << code;
    send(socket, envelope("error", id, "ProtocolError", {{"code", code}}));
}
void DaemonIpcServer::handleNewConnection() {
    while (auto* socket = m_server.nextPendingConnection()) {
        if (m_clients.size() >= 32) {
            socket->abort();
            socket->deleteLater();
            continue;
        }
#ifdef Q_OS_MACOS
        uid_t peerUid;
        gid_t peerGid;
        if (::getpeereid(static_cast<int>(socket->socketDescriptor()), &peerUid, &peerGid) != 0 ||
            peerUid != ::getuid()) {
            socket->abort();
            socket->deleteLater();
            continue;
        }
#elif defined(Q_OS_LINUX)
        struct ucred peer{};
        socklen_t size = sizeof(peer);
        if (::getsockopt(static_cast<int>(socket->socketDescriptor()), SOL_SOCKET, SO_PEERCRED,
                         &peer, &size) != 0 ||
            peer.uid != ::getuid()) {
            socket->abort();
            socket->deleteLater();
            continue;
        }
#endif
        socket->setReadBufferSize(protocol::max_frame_bytes + 1);
        m_clients.insert(socket, {});
        qInfo("IPC client connected");
        connect(socket, &QLocalSocket::disconnected, this, [this, socket] {
            m_clients.remove(socket);
            socket->deleteLater();
            qInfo("IPC client disconnected");
        });
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] {
            while (socket->bytesAvailable() > 0 && m_clients.contains(socket)) {
                auto& input = m_clients[socket].input;
                const auto chunk = socket->read(qMin<qint64>(
                    socket->bytesAvailable(), protocol::max_frame_bytes + 1 - input.size()));
                input += chunk;
                if (input.size() > protocol::max_frame_bytes) {
                    error(socket, {}, "message-too-large");
                    socket->disconnectFromServer();
                    return;
                }
                qsizetype newline;
                while ((newline = input.indexOf('\n')) >= 0) {
                    const auto line = input.left(newline);
                    input.remove(0, newline + 1);
                    QJsonParseError parse;
                    const auto doc = QJsonDocument::fromJson(line, &parse);
                    if (parse.error != QJsonParseError::NoError || !doc.isObject()) {
                        error(socket, {}, "malformed-json");
                        continue;
                    }
                    handleRequest(doc.object(), socket);
                    if (!m_clients.contains(socket)) {
                        return;
                    }
                }
            }
        });
    }
}
QJsonObject DaemonIpcServer::session(const QString& id) const {
    if (!m_controller || !m_controller->conversationStore()) {
        return {};
    }
    for (const auto& record : m_controller->conversationStore()->listConversations()) {
        if (record.id == id) {
            auto selected = m_controller->modelService()->selectedModel();
            QString state = "idle", output;
            const auto messages = m_controller->conversationStore()->loadMessages(id);
            for (auto it = messages.crbegin(); it != messages.crend(); ++it) {
                if (it->role != core::ChatRole::Assistant) {
                    continue;
                }
                state = core::chatMessageStatusName(it->status);
                output = it->content;
                selected = {it->providerId, it->modelId};
                break;
            }
            return {
                {"session_id", id},
                {"conversation_id", id},
                {"title", record.title},
                {"created_at", record.createdAtUtc.toString(Qt::ISODateWithMs)},
                {"type", "conversation"},
                {"run_type", id == m_sessionId ? m_kind : QString()},
                {"run_id", id == m_sessionId ? m_runId : QString()},
                {"state", id == m_sessionId ? m_state : state},
                {"output", id == m_sessionId ? m_output : output.left(65536)},
                {"output_truncated", id == m_sessionId ? m_outputTruncated : output.size() > 65536},
                {"approval_id", id == m_sessionId ? m_approvalId : QString()},
                {"provider_id", id == m_sessionId ? m_providerId : selected.providerId},
                {"model_id", id == m_sessionId ? m_modelId : selected.modelId},
                {"approval", id == m_sessionId ? m_approvalPayload : QJsonObject{}}};
        }
    }
    return {};
}
void DaemonIpcServer::handleRequest(const QJsonObject& message, QLocalSocket* socket) {
    const auto id = message.value("id").toString();
    const auto name = message.value("name").toString();
    const auto payload = message.value("payload").toObject();
    const auto version = message.value("version").toObject();
    if (message.value("type") != "request" || id.isEmpty() || id.size() > 128 ||
        !message.value("payload").isObject() || !version.value("major").isDouble() ||
        !version.value("minor").isDouble() ||
        version.value("major").toDouble() != version.value("major").toInt(-1) ||
        version.value("minor").toDouble() != version.value("minor").toInt(-1) ||
        version.value("minor").toInt(-1) < 0) {
        error(socket, id, "invalid-envelope");
        return;
    }
    if (version.value("major").toInt(-1) != protocol::major) {
        error(socket, id, "incompatible-major");
        return;
    }
    if (!protocol::commands().contains(name)) {
        error(socket, id, "unknown-command");
        return;
    }
    const auto fields = protocol::commands().value(name).toObject();
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        const auto value = payload.value(it.key());
        const auto type = it.value().toString();
        if ((type == "string" && (!value.isString() || value.toString().size() > 65536)) ||
            (type == "integer" &&
             (!value.isDouble() || value.toDouble() != value.toInt() || value.toInt(-1) < 0)) ||
            (type == "boolean" && !value.isBool()) || (type == "array" && !value.isArray())) {
            error(socket, id, "invalid-payload");
            return;
        }
    }
    auto reply = [&](const QJsonObject& data) {
        send(socket, envelope("response", id, name, data));
    };
    if (name == "hello") {
        if (m_clients[socket].hello || payload.value("client_id").toString().isEmpty()) {
            error(socket, id, "invalid-state");
            return;
        }
        if (payload.value("major").toInt(-1) != protocol::major) {
            error(socket, id, "incompatible-major");
            return;
        }
        m_clients[socket].hello = true;
        reply({{"major", protocol::major},
               {"minor", protocol::minor},
               {"daemon_version", core::AppMetadata::version()},
               {"capabilities", protocol::capabilities()}});
        return;
    }
    if (!m_clients[socket].hello) {
        error(socket, id, "handshake-required");
        return;
    }
    if (m_shuttingDown) {
        error(socket, id, "daemon-shutting-down");
        return;
    }
    if (name == "daemon.status") {
        reply({{"running", true},
               {"daemon_version", core::AppMetadata::version()},
               {"uptime_ms", m_uptime.elapsed()},
               {"active_runs", m_state == "running" || m_state == "approval" ? 1 : 0},
               {"sessions", m_controller && m_controller->conversationStore()
                                ? m_controller->conversationStore()->listConversations().size()
                                : 0}});
        return;
    }
    if (name == "daemon.shutdown") {
        m_shuttingDown = true;
        reply({{"shutting_down", true}});
        m_server.close();
        if (m_controller) {
            m_controller->stopChatGeneration();
            m_controller->agentRuntime()->shutdown();
        }
        QTimer::singleShot(50, QCoreApplication::instance(), &QCoreApplication::quit);
        return;
    }
    if (!m_controller) {
        error(socket, id, "runtime-unavailable");
        return;
    }
    if (name == "model.current") {
        const auto model = m_controller->modelService()->selectedModel();
        reply({{"provider_id", model.providerId}, {"model_id", model.modelId}});
        return;
    }
    if (name == "model.list") {
        QJsonArray models;
        for (const auto& provider : m_controller->modelService()->knownProviderIds()) {
            for (const auto& model :
                 m_controller->modelService()->providerDiscoveredModels(provider)) {
                models.append(QJsonObject{{"provider_id", provider}, {"model_id", model.name}});
            }
        }
        reply({{"models", models}});
        return;
    }
    if (name == "session.list") {
        QJsonArray sessions;
        if (m_controller->conversationStore()) {
            for (const auto& record : m_controller->conversationStore()->listConversations()) {
                sessions.append(session(record.id));
            }
        }
        reply({{"sessions", sessions}});
        return;
    }
    if (name == "session.create") {
        if (m_state == "running" || m_state == "approval") {
            error(socket, id, "runtime-busy");
            return;
        }
        const auto created = m_controller->createConversation(payload.value("title").toString());
        if (created.isEmpty()) {
            error(socket, id, "storage-failure");
            return;
        }
        m_clients[socket].sessions.insert(created);
        reply(session(created));
        return;
    }
    if (name == "session.attach") {
        const auto sid = payload.value("session_id").toString();
        const auto current = session(sid);
        if (current.isEmpty()) {
            error(socket, id, "unknown-session");
            return;
        }
        m_clients[socket].sessions.insert(sid);
        reply(current);
        return;
    }
    if (name == "run.cancel") {
        if (payload.value("run_id") != m_runId || (m_state != "running" && m_state != "approval")) {
            error(socket, id, "unknown-run");
            return;
        }
        const bool cancelled =
            m_kind == "agent" ? m_controller->cancelAgentRun() : m_controller->stopChatGeneration();
        if (!cancelled) {
            error(socket, id, "invalid-state");
            return;
        }
        reply({{"accepted", true}});
        return;
    }
    if (name == "approval.respond") {
        if (m_state != "approval" || payload.value("run_id") != m_runId ||
            payload.value("approval_id") != m_approvalId || m_approvalId.isEmpty()) {
            error(socket, id, "invalid-approval");
            return;
        }
        m_approvalId.clear();
        m_state = "running";
        if (!m_controller->respondToAgentApproval(payload.value("allow").toBool())) {
            error(socket, id, "invalid-state");
            return;
        }
        reply({{"accepted", true}});
        return;
    }
    if (m_state == "running" || m_state == "approval" || m_controller->agentLoopActive() ||
        m_controller->chatModeService()->busy()) {
        error(socket, id, "runtime-busy");
        return;
    }
    const auto sid = payload.value("session_id").toString();
    if (session(sid).isEmpty()) {
        error(socket, id, "unknown-session");
        return;
    }
    if (payload.value("text").toString().trimmed().isEmpty()) {
        error(socket, id, "invalid-payload");
        return;
    }
    if (!m_controller->switchConversation(sid)) {
        error(socket, id, "invalid-state");
        return;
    }
    const auto selection = m_controller->currentWorkspaceModelSelection();
    m_providerId = selection.providerId;
    m_modelId = selection.modelId;
    m_sessionId = sid;
    m_runId = uuid();
    m_kind = name == "agent.start" ? "agent" : "chat";
    m_output.clear();
    m_outputTruncated = false;
    m_chatCursor = 0;
    m_approvalId.clear();
    m_approvalPayload = {};
    m_state = "running";
    m_starting = true;
    m_clients[socket].sessions.insert(sid);
    const bool accepted = m_kind == "agent"
                              ? m_controller->runAgentRequest(payload.value("text").toString())
                              : m_controller->sendMessage(payload.value("text").toString());
    m_starting = false;
    if (!accepted) {
        m_state = "failed";
        error(socket, id, "runtime-rejected");
        return;
    }
    reply({{"run_id", m_runId}, {"session_id", sid}});
    publishEvent("run.started", session(sid));
    if (m_kind == "chat") {
        onChatChanged();
    }
}
void DaemonIpcServer::publishEvent(const QString& name, const QJsonObject& data) {
    auto payload = data;
    if (name == "run.completed" || name == "run.failed" || name == "run.cancelled") {
        payload.insert("state", m_state);
    }
    payload.insert("run_id", m_runId);
    payload.insert("session_id", m_sessionId);
    const auto message = envelope("event", {}, name, payload);
    const auto sockets = m_clients.keys();
    for (auto* socket : sockets) {
        if (m_clients.value(socket).hello &&
            m_clients.value(socket).sessions.contains(m_sessionId)) {
            send(socket, message);
        }
    }
}
void DaemonIpcServer::onChatChanged() {
    if (m_starting || m_kind != "chat" || m_state != "running") {
        return;
    }
    const auto messages = m_controller->chatHistory();
    if (messages.isEmpty()) {
        return;
    }
    const auto& message = messages.last();
    if (message.role != core::ChatRole::Assistant) {
        return;
    }
    if (message.content.size() > m_chatCursor) {
        publishEvent("output.delta", {{"text", message.content.mid(m_chatCursor)}});
    }
    m_chatCursor = static_cast<int>(message.content.size());
    m_outputTruncated = message.content.size() > 65536;
    m_output = message.content.left(65536);
    if (message.status == core::ChatMessageStatus::Completed ||
        message.status == core::ChatMessageStatus::Cancelled ||
        message.status == core::ChatMessageStatus::Failed) {
        m_state = core::chatMessageStatusName(message.status);
        publishEvent(
            "run." + m_state,
            {{"text", message.content},
             {"detail", m_state == "failed" ? m_controller->chatErrorCategory() : QString{}}});
    }
}
void DaemonIpcServer::onAgentEvent(const core::AgentEvent& e) {
    if (m_kind != "agent" || e.sessionId != m_controller->activeAgentSessionId() ||
        (m_state != "running" && m_state != "approval")) {
        return;
    }
    using core::AgentEventType;
    if (e.type == AgentEventType::ModelOutputDelta) {
        if (const auto* text = std::get_if<core::AgentTextEvent>(&e.payload)) {
            m_outputTruncated = m_outputTruncated || m_output.size() + text->text.size() > 65536;
            m_output += text->text.left(65536 - m_output.size());
            publishEvent("output.delta", {{"text", text->text}});
        }
    } else if (e.type == AgentEventType::ToolRequested ||
               e.type == AgentEventType::ToolApprovalRequired ||
               e.type == AgentEventType::ToolExecutionCompleted ||
               e.type == AgentEventType::ToolExecutionFailed) {
        QJsonObject data;
        if (const auto* tool = std::get_if<core::AgentToolEvent>(&e.payload)) {
            data.insert("tool", tool->toolId);
            data.insert("detail", tool->output);
            data.insert("risk", static_cast<int>(tool->risk));
            QJsonArray resources;
            for (const auto& request : tool->authorizationRequests) {
                resources.append(QJsonObject{{"resource", request.resource},
                                             {"domain", static_cast<int>(request.domain)},
                                             {"access", static_cast<int>(request.access)}});
            }
            data.insert("resources", resources);
        }
        if (e.type == AgentEventType::ToolApprovalRequired) {
            m_approvalId = uuid();
            m_state = "approval";
            data.insert("approval_id", m_approvalId);
            data.insert("detail",
                        QStringLiteral("Tool authorization requires an explicit user decision."));
            m_approvalPayload = data;
            publishEvent("approval.requested", data);
        } else {
            publishEvent(e.type == AgentEventType::ToolRequested ? "tool.requested" : "tool.result",
                         data);
        }
    } else if (e.type == AgentEventType::AgentCompleted || e.type == AgentEventType::AgentFailed ||
               e.type == AgentEventType::AgentCancelled) {
        m_state = e.type == AgentEventType::AgentCompleted   ? "completed"
                  : e.type == AgentEventType::AgentCancelled ? "cancelled"
                                                             : "failed";
        if (const auto* result = std::get_if<core::AgentRunEvent>(&e.payload)) {
            m_outputTruncated = result->finalAnswer.size() > 65536;
            m_output = result->finalAnswer.left(65536);
            publishEvent("run." + m_state,
                         {{"text", result->finalAnswer}, {"detail", result->abortReason}});
        } else if (const auto* text = std::get_if<core::AgentTextEvent>(&e.payload)) {
            const bool completed = e.type == AgentEventType::AgentCompleted;
            m_outputTruncated = completed && text->text.size() > 65536;
            m_output = completed ? text->text.left(65536) : QString{};
            publishEvent("run." + m_state, {{"text", completed ? text->text : QString{}},
                                            {"detail", completed ? QString{} : text->text}});
        } else {
            publishEvent("run." + m_state, {{"detail", "Runtime terminal payload unavailable"}});
        }
        m_approvalId.clear();
        m_approvalPayload = {};
    }
}
} // namespace sentinel::daemon
