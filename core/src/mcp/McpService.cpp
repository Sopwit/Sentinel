// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/mcp/McpService.h"
#include "sentinel/core/app/AppMetadata.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include "sentinel/core/security/CredentialStore.h"
#include <QDebug>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSet>
#include <QScopedValueRollback>
#include <QTimer>
#include <QUuid>
#include <algorithm>
#include <cmath>
#if defined(Q_OS_UNIX)
#include <signal.h>
#include <unistd.h>
#endif

namespace sentinel::core {
namespace {
bool secureRemoteHeaders(McpServerConfig& config) {
    if (config.type != QLatin1String("remote")) return true;
    const QUrl url(config.url);
    if (!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() ||
        !url.query().isEmpty() || !url.fragment().isEmpty()) return false;
    auto store = defaultCredentialStore();
    for (auto it = config.headers.begin(); it != config.headers.end(); ++it) {
        if (!it.value().isString() || it.value().toString().isEmpty()) continue;
        const QByteArray identity = (config.name + QLatin1Char(':') + it.key()).toUtf8();
        const auto id = QStringLiteral("mcp-") + QString::fromLatin1(
            QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
        const auto reference = QStringLiteral("secret://") + id;
        if (it.value().toString() == reference) continue;
        const CredentialKey key{id, QStringLiteral("header")};
        const auto written = store.storeCredential(key, it.value().toString());
        const auto confirmed = written.succeeded ? store.readCredential(key) : CredentialReadResult{};
        if (!confirmed.secret.has_value() || *confirmed.secret != it.value().toString()) return false;
        it.value() = reference;
    }
    return true;
}

QJsonObject failure(McpFailureCategory category, const QString& detail) {
    QString safe = detail;
    safe.replace(QRegularExpression(QStringLiteral("https?://\\S+")),
                 QStringLiteral("[redacted URL]"));
    safe.replace(QRegularExpression(QStringLiteral("[\\x00-\\x1f\\x7f]")), QLatin1String(" "));
    return {{QStringLiteral("error"), QJsonObject{
        {QStringLiteral("message"), safe.left(300)},
        {QStringLiteral("category"), mcpFailureCategoryName(category)}}}};
}
QJsonObject checkedResponse(const QJsonObject& response, int id,
                            bool allowInputRequired = false) {
    if (response.value(QStringLiteral("jsonrpc")).toString() != QLatin1String("2.0") ||
        response.value(QStringLiteral("id")).toInt(-1) != id ||
        (!response.value(QStringLiteral("result")).isObject() &&
         !response.value(QStringLiteral("error")).isObject()))
        return failure(McpFailureCategory::ProtocolError,
                       QStringLiteral("Invalid MCP JSON-RPC response"));
    if (response.contains(QStringLiteral("error"))) {
        const auto error = response.value(QStringLiteral("error")).toObject();
        return failure(McpFailureCategory::RemoteExecutionFailure,
                       error.value(QStringLiteral("message")).toString());
    }
    const auto resultType = response.value(QStringLiteral("result")).toObject()
        .value(QStringLiteral("resultType")).toString();
    if (resultType == QLatin1String("input_required") && !allowInputRequired)
        return failure(McpFailureCategory::ProtocolError,
                       QStringLiteral("MCP input requests are not supported"));
    if (!resultType.isEmpty() && resultType != QLatin1String("complete") &&
        !(allowInputRequired && resultType == QLatin1String("input_required")))
        return failure(McpFailureCategory::ProtocolError,
                       QStringLiteral("Unsupported MCP result type"));
    return response;
}
McpFailureCategory httpFailure(int status) {
    if (status == 401 || status == 403) return McpFailureCategory::AuthenticationFailure;
    if (status == 400 || status == 404 || status == 405 || status == 406 || status == 415)
        return McpFailureCategory::ConfigurationFailure;
    return McpFailureCategory::TransportFailure;
}
McpFailureCategory categoryFromName(const QString& name) {
    if (name == QLatin1String("Timeout")) return McpFailureCategory::Timeout;
    if (name == QLatin1String("Cancelled")) return McpFailureCategory::Cancelled;
    if (name == QLatin1String("TransportFailure")) return McpFailureCategory::TransportFailure;
    if (name == QLatin1String("ServerUnavailable")) return McpFailureCategory::ServerUnavailable;
    if (name == QLatin1String("AuthenticationFailure")) return McpFailureCategory::AuthenticationFailure;
    if (name == QLatin1String("ConfigurationFailure")) return McpFailureCategory::ConfigurationFailure;
    if (name == QLatin1String("UnsupportedProtocolVersion")) return McpFailureCategory::UnsupportedProtocolVersion;
    if (name == QLatin1String("InvalidToolSchema")) return McpFailureCategory::InvalidToolSchema;
    if (name == QLatin1String("SecurityDenied")) return McpFailureCategory::SecurityDenied;
    if (name == QLatin1String("RemoteExecutionFailure")) return McpFailureCategory::RemoteExecutionFailure;
    if (name == QLatin1String("UnsupportedLegacyTransport")) return McpFailureCategory::UnsupportedLegacyTransport;
    if (name == QLatin1String("Offline")) return McpFailureCategory::Offline;
    return McpFailureCategory::ProtocolError;
}
QJsonObject modernMeta() {
    return {{QStringLiteral("io.modelcontextprotocol/protocolVersion"), QStringLiteral("2026-07-28")},
            {QStringLiteral("io.modelcontextprotocol/clientInfo"),
             QJsonObject{{QStringLiteral("name"), QStringLiteral("Sentinel")},
                         {QStringLiteral("version"), AppMetadata::version()}}},
            {QStringLiteral("io.modelcontextprotocol/clientCapabilities"), QJsonObject{
                {QStringLiteral("elicitation"), QJsonObject{
                    {QStringLiteral("form"), QJsonObject{}},
                    {QStringLiteral("url"), QJsonObject{}}}}}}};
}
QByteArray headerValue(const QString& value) {
    const auto bytes = value.toUtf8();
    bool plain = !bytes.isEmpty() && bytes.front() != ' ' && bytes.back() != ' ' &&
        bytes.front() != '\t' && bytes.back() != '\t' &&
        !bytes.startsWith("=?base64?");
    for (const char ch : bytes)
        plain &= ch >= 0x20 && ch <= 0x7e;
    return plain ? bytes : QByteArray("=?base64?") + bytes.toBase64() + "?=";
}
struct HeaderBinding {
    QString header;
    QStringList path;
    QString type;
};
bool hasHeaderAnnotation(const QJsonValue& value) {
    if (value.isObject()) {
        const auto object = value.toObject();
        if (object.contains(QStringLiteral("x-mcp-header"))) return true;
        for (auto it = object.constBegin(); it != object.constEnd(); ++it)
            if (hasHeaderAnnotation(it.value())) return true;
    } else if (value.isArray()) {
        for (const auto& item : value.toArray())
            if (hasHeaderAnnotation(item)) return true;
    }
    return false;
}
bool collectHeaders(const QJsonObject& schema, const QStringList& path,
                    QSet<QString>& names, QList<HeaderBinding>& bindings) {
    const auto annotation = schema.value(QStringLiteral("x-mcp-header"));
    if (!annotation.isUndefined()) {
        const auto name = annotation.toString();
        static const QRegularExpression token(QStringLiteral("^[A-Za-z0-9!#$%&'*+.^_`|~-]+$"));
        const auto type = schema.value(QStringLiteral("type")).toString();
        if (path.isEmpty() || !token.match(name).hasMatch() || names.contains(name.toLower()) ||
            (type != QLatin1String("string") && type != QLatin1String("integer") &&
             type != QLatin1String("boolean"))) return false;
        names.insert(name.toLower());
        bindings.append({name, path, type});
    }
    for (auto it = schema.constBegin(); it != schema.constEnd(); ++it) {
        if (it.key() == QLatin1String("properties")) {
            if (!it.value().isObject()) return false;
            const auto properties = it.value().toObject();
            for (auto property = properties.constBegin(); property != properties.constEnd(); ++property) {
                if (!property.value().isObject()) return false;
                auto childPath = path;
                childPath.append(property.key());
                if (!collectHeaders(property.value().toObject(), childPath, names, bindings))
                    return false;
            }
        } else if (it.key() != QLatin1String("x-mcp-header") &&
                   hasHeaderAnnotation(it.value())) return false;
    }
    return true;
}
bool applyToolHeaders(QNetworkRequest& request, const QJsonObject& schema,
                      const QJsonObject& arguments) {
    QSet<QString> names;
    QList<HeaderBinding> bindings;
    if (!collectHeaders(schema, {}, names, bindings)) return false;
    for (const auto& binding : bindings) {
        QJsonValue value(arguments);
        for (const auto& segment : binding.path) {
            if (!value.isObject()) { value = QJsonValue(); break; }
            value = value.toObject().value(segment);
        }
        if (value.isUndefined() || value.isNull()) continue;
        QString text;
        if (binding.type == QLatin1String("string") && value.isString()) text = value.toString();
        else if (binding.type == QLatin1String("boolean") && value.isBool())
            text = value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
        else if (binding.type == QLatin1String("integer") && value.isDouble() &&
                 std::abs(value.toDouble()) <= 9007199254740991.0 &&
                 std::floor(value.toDouble()) == value.toDouble())
            text = QString::number(static_cast<qint64>(value.toDouble()));
        else return false;
        request.setRawHeader(QByteArray("Mcp-Param-") + binding.header.toUtf8(),
                             headerValue(text));
    }
    return true;
}
}

struct McpService::RemoteToolCall {
    QString serverName;
    QString toolName;
    QJsonObject arguments;
    ToolCompletion completion;
    quint64 generation{0};
    int correlationId{-1};
    int rounds{0};
    bool active{true};
    QPointer<QNetworkReply> reply;
    QMap<QString, QJsonObject> remaining;
    QJsonObject inputResponses;
    QString requestState;
    bool hasRequestState{false};
    QString pendingId;
    QString pendingKey;
    McpInteractionRequest pendingRequest;
};

McpService::McpService(QObject* parent) : QObject(parent) {}

McpService::~McpService() {
    disconnectFromAll();
    // Clean up owned processes
    for (QProcess* process : m_processes) {
        if (process) {
            process->terminate();
            if (!process->waitForFinished(3000)) {
                process->kill();
            }
            delete process;
        }
    }
}

bool McpService::addServer(const McpServerConfig& config) {
    if (m_processingStdout > 0 || config.name.trimmed().isEmpty() ||
        m_servers.contains(config.name)) {
        qWarning() << QStringLiteral("McpService: Server '%1' already exists").arg(config.name);
        return false;
    }

    McpServerConfig safe = config;
    if (!secureRemoteHeaders(safe)) return false;
    McpServerState state;
    state.config = safe;
    m_servers[config.name] = state;
    emit serverAdded(config.name);

    qDebug()
        << QStringLiteral("McpService: Added server '%1' (type: %2)").arg(config.name, config.type);
    return true;
}

bool McpService::updateServer(const McpServerConfig& config) {
    auto* state = findServer(config.name);
    if (!state || (config.type != QLatin1String("local") &&
                   config.type != QLatin1String("remote")) || state->activeSynchronousCalls > 0 ||
        m_processingStdout > 0)
        return false;
    McpServerConfig safe = config;
    if (!secureRemoteHeaders(safe)) return false;
    const bool reconnect = state->state == McpConnectionState::Connected && config.enabled;
    const bool originChanged = state->config.type != config.type ||
        state->config.command != config.command || state->config.arguments != config.arguments ||
        state->config.url != config.url;
    disconnectFromServer(config.name);
    state->config = safe;
    if (originChanged) {
        state->lastKnownTools.clear();
        state->lastKnownResources.clear();
    }
    emit serverConfigChanged(config.name);
    return !reconnect || connectToServer(config.name);
}

bool McpService::removeServer(const QString& serverName) {
    auto it = m_servers.find(serverName);
    if (it == m_servers.end()) {
        return false;
    }
    if (it->state == McpConnectionState::Connecting || it->activeSynchronousCalls > 0 ||
        m_processingStdout > 0)
        return false;

    disconnectFromServer(serverName);

    m_servers.erase(it);
    emit serverRemoved(serverName);
    qDebug() << QStringLiteral("McpService: Removed server '%1'").arg(serverName);
    return true;
}

QList<McpServerConfig> McpService::servers() const {
    QList<McpServerConfig> configs;
    for (const auto& state : m_servers) {
        configs.append(state.config);
    }
    return configs;
}

McpServerConfig McpService::serverConfig(const QString& serverName) const {
    auto it = m_servers.find(serverName);
    if (it == m_servers.end()) {
        return {};
    }
    return it->config;
}

bool McpService::connectToServer(const QString& serverName) {
    auto* state = findServer(serverName);
    if (!state) {
        return false;
    }

    if (!state->config.enabled)
        return false;
    if (state->state == McpConnectionState::Connected) {
        return true;
    }
    if (state->state == McpConnectionState::Connecting)
        return false;
    if (state->process)
        disconnectServer(*state);
    ++state->generation;
    state->remoteSessionId.clear();
    state->protocolVersion.clear();
    state->lastEventId.clear();
    const auto generation = state->generation;

    state->state = McpConnectionState::Connecting;
    state->errorString.clear();
    state->failureCategory = McpFailureCategory::None;
    state->resourcesSupported = false;

    bool success = false;
    if (state->config.type == "local") {
        success = connectToLocalServer(*state);
    } else if (state->config.type == "remote") {
        success = connectToRemoteServer(*state);
    } else {
        state->errorString = QStringLiteral("Unknown server type: %1").arg(state->config.type);
        state->state = McpConnectionState::Error;
        state->failureCategory = McpFailureCategory::ProtocolError;
        state->tools.clear();
        state->resources.clear();
        emit serverError(serverName, state->errorString);
        return false;
    }
    if (state->generation != generation || state->state == McpConnectionState::Disconnected)
        return false;

    if (success && state->state == McpConnectionState::Connecting) {
        state->state = McpConnectionState::Connected;
        emit serverConnected(serverName);

        // List tools after connection
        listTools(*state);
        if (state->generation != generation) return false;
        if (state->state == McpConnectionState::Connected && state->resourcesSupported)
            listResources(*state);
        if (state->generation != generation) return false;
        if (state->state == McpConnectionState::Connected && state->config.type == QLatin1String("remote"))
            startRemoteEventStream(serverName);
    } else {
        disconnectServer(*state);
        state->state = McpConnectionState::Error;
        if (state->failureCategory == McpFailureCategory::None)
            state->failureCategory = McpFailureCategory::StartupFailure;
        state->tools.clear();
        state->resources.clear();
        emit serverError(serverName, state->errorString);
    }

    return state->state == McpConnectionState::Connected;
}

bool McpService::disconnectFromServer(const QString& serverName) {
    auto* state = findServer(serverName);
    if (!state) {
        return false;
    }
    if (state->config.type == QLatin1String("local") &&
        (state->activeSynchronousCalls > 0 || m_processingStdout > 0))
        return false;

    state->state = McpConnectionState::Disconnected;
    ++state->generation;
    state->tools.clear();
    state->resources.clear();
    const auto replies = m_remoteReplies.take(serverName);
    const auto interactions = m_interactions.values();
    for (const auto& call : interactions)
        if (call->serverName == serverName)
            finishRemoteToolCall(call, failure(McpFailureCategory::ServerUnavailable,
                QStringLiteral("MCP server disconnected")));
    auto pending = m_pendingCalls.take(serverName);
    m_readBuffers.remove(serverName);
    m_toolRefreshScheduled.remove(serverName);
    m_resourceRefreshScheduled.remove(serverName);
    disconnectServer(*state);
    if (state->config.type == QLatin1String("remote") && !state->remoteSessionId.isEmpty()) {
        auto* ending = m_networkManager.deleteResource(remoteRequest(*state));
        connect(ending, &QNetworkReply::finished, ending, &QObject::deleteLater);
        state->remoteSessionId.clear();
    }
    emit serverDisconnected(serverName);
    for (const auto& reply : replies)
        if (reply) reply->abort();
    for (auto& completion : pending)
        completion(failure(McpFailureCategory::ServerUnavailable, QStringLiteral("MCP server disconnected")));

    return true;
}

McpConnectionState McpService::connectionState(const QString& serverName) const {
    const auto* state = findServer(serverName);
    if (!state) {
        return McpConnectionState::Disconnected;
    }
    return state->state;
}

QString McpService::lastError(const QString& serverName) const {
    const auto* state = findServer(serverName);
    return state ? state->errorString : QString();
}

McpFailureCategory McpService::failureCategory(const QString& serverName) const {
    const auto* state = findServer(serverName);
    return state ? state->failureCategory : McpFailureCategory::None;
}

bool McpService::hasRemoteSession(const QString& serverName) const {
    const auto* state = findServer(serverName);
    return state && state->state == McpConnectionState::Connected &&
        !state->remoteSessionId.isEmpty();
}

QList<McpToolDefinition> McpService::lastKnownTools(const QString& serverName) const {
    const auto* state = findServer(serverName);
    return state ? state->lastKnownTools : QList<McpToolDefinition>{};
}

QList<McpResource> McpService::resources(const QString& serverName) const {
    if (serverName.isEmpty()) {
        QList<McpResource> result;
        for (const auto& state : m_servers)
            result.append(state.resources);
        return result;
    }
    const auto* state = findServer(serverName);
    return state ? state->resources : QList<McpResource>{};
}

QList<McpResource> McpService::lastKnownResources(const QString& serverName) const {
    const auto* state = findServer(serverName);
    return state ? state->lastKnownResources : QList<McpResource>{};
}

bool McpService::refreshResources(const QString& serverName) {
    auto* state = findServer(serverName);
    if (!state || state->state != McpConnectionState::Connected || !state->resourcesSupported)
        return false;
    const auto generation = state->generation;
    listResources(*state);
    return state->generation == generation && state->state == McpConnectionState::Connected;
}

bool McpService::refreshTools(const QString& serverName) {
    auto* state = findServer(serverName);
    if (!state || state->state != McpConnectionState::Connected)
        return false;
    const auto generation = state->generation;
    listTools(*state);
    if (state->generation != generation) return false;
    if (state->state == McpConnectionState::Connected && state->resourcesSupported)
        listResources(*state);
    return state->generation == generation && state->state == McpConnectionState::Connected;
}

bool McpService::setEnabled(const QString& serverName, bool enabled) {
    auto* state = findServer(serverName);
    if (!state)
        return false;
    if (!enabled) {
        if (!disconnectFromServer(serverName))
            return false;
        state->config.enabled = false;
        emit serverConfigChanged(serverName);
        return true;
    }
    state->config.enabled = true;
    emit serverConfigChanged(serverName);
    return connectToServer(serverName);
}

void McpService::invalidateInventory(const QString& serverName, McpFailureCategory category,
                                     const QString& detail) {
    auto* state = findServer(serverName);
    if (!state)
        return;
    ++state->generation;
    state->tools.clear();
    state->resources.clear();
    state->state = McpConnectionState::Error;
    state->failureCategory = category;
    state->errorString = detail.left(300);
    auto pending = m_pendingCalls.take(serverName);
    const auto replies = m_remoteReplies.take(serverName);
    const auto interactions = m_interactions.values();
    for (const auto& call : interactions)
        if (call->serverName == serverName)
            finishRemoteToolCall(call, failure(category, detail));
    const auto safeError = state->errorString;
    emit serverError(serverName, safeError);
    for (const auto& reply : replies)
        if (reply) reply->abort();
    for (auto& completion : pending)
        completion(failure(category, safeError));
}

void McpService::confirmToolInventory(const QString& serverName) {
    auto* state = findServer(serverName);
    if (state && state->state == McpConnectionState::Connected)
        state->lastKnownTools = state->tools;
}

QList<McpToolDefinition> McpService::tools(const QString& serverName) const {
    if (serverName.isEmpty()) {
        QList<McpToolDefinition> allTools;
        for (const auto& state : m_servers) {
            allTools.append(state.tools);
        }
        return allTools;
    }

    const auto* state = findServer(serverName);
    if (!state) {
        return {};
    }
    return state->tools;
}

QJsonObject McpService::callTool(const QString& serverName, const QString& toolName,
                                 const QJsonObject& arguments) {
    const auto* state = findServer(serverName);
    if (!state || state->state != McpConnectionState::Connected ||
        std::none_of(state->tools.cbegin(), state->tools.cend(),
                     [&](const auto& tool) { return tool.name == toolName; }))
        return failure(McpFailureCategory::ServerUnavailable,
                       QStringLiteral("MCP tool is not currently available"));
    QJsonObject params;
    params["name"] = toolName;
    params["arguments"] = arguments;

    return sendJsonRpc(serverName, "tools/call", params);
}

IMcpService::Cancel McpService::callToolAsync(const QString& serverName, const QString& toolName,
                                              const QJsonObject& arguments,
                                              ToolCompletion completion) {
    auto* state = findServer(serverName);
    if (!state || state->state != McpConnectionState::Connected) {
        completion(failure(McpFailureCategory::ServerUnavailable, QStringLiteral("MCP server unavailable")));
        return {};
    }
    const auto currentTools = state->tools;
    if (std::none_of(currentTools.cbegin(), currentTools.cend(),
                     [&](const auto& tool) { return tool.name == toolName; })) {
        completion(failure(McpFailureCategory::InvalidToolSchema,
                           QStringLiteral("MCP tool is no longer available")));
        return {};
    }
    const int id = state->requestId++;
    QJsonObject request{{"jsonrpc", "2.0"},
                        {"id", id},
                        {"method", "tools/call"},
                        {"params", QJsonObject{{"name", toolName}, {"arguments", arguments}}}};
    if (state->config.type == QLatin1String("remote") &&
        state->protocolVersion == "2026-07-28") {
        auto params = request.value(QStringLiteral("params")).toObject();
        params.insert(QStringLiteral("_meta"), modernMeta());
        request.insert(QStringLiteral("params"), params);
    }
    const auto payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    if (state->config.type == QStringLiteral("local") &&
        (state->process || !state->managedProcessId.isEmpty())) {
        m_pendingCalls[serverName].insert(id, std::move(completion));
        const bool written = state->process ? state->process->write(payload) == payload.size()
            : m_processExecutor.write(state->managedProcessId, payload);
        if (!written) {
            invalidateInventory(serverName, McpFailureCategory::TransportFailure,
                                QStringLiteral("MCP stdio write failed"));
            return {};
        }
        QPointer<McpService> self(this);
        QTimer::singleShot(30000, this, [self, serverName, id] {
            if (!self)
                return;
            auto callback = self->m_pendingCalls[serverName].take(id);
            if (callback)
                callback(failure(McpFailureCategory::Timeout, QStringLiteral("MCP local request timed out")));
        });
        return [self, serverName, id] {
            if (self)
                self->m_pendingCalls[serverName].remove(id);
        };
    }
    if (state->config.type == QStringLiteral("remote")) {
        auto call = std::make_shared<RemoteToolCall>();
        call->serverName = serverName;
        call->toolName = toolName;
        call->arguments = arguments;
        call->generation = state->generation;
        call->completion = std::move(completion);
        startRemoteToolRound(call);
        QPointer<McpService> self(this);
        return [self, call] {
            if (self)
                self->finishRemoteToolCall(call,
                    failure(McpFailureCategory::Cancelled, QStringLiteral("MCP tool call cancelled")));
        };
    }
    completion(failure(McpFailureCategory::ProtocolError, QStringLiteral("Unsupported MCP transport")));
    return {};
}

QList<McpInteractionRequest> McpService::pendingInteractions() const {
    QList<McpInteractionRequest> result;
    for (const auto& call : m_interactions)
        result.append(call->pendingRequest);
    return result;
}

bool McpService::respondToInteraction(const QString& id, quint64 generation,
                                      McpInteractionDecision decision, const QJsonValue& value) {
    const auto call = m_interactions.value(id);
    const auto* state = call ? findServer(call->serverName) : nullptr;
    if (!call || !call->active || call->pendingId != id || call->generation != generation ||
        !state || state->generation != generation || state->state != McpConnectionState::Connected)
        return false;
    if (decision == McpInteractionDecision::Cancel) {
        finishRemoteToolCall(call, failure(McpFailureCategory::Cancelled,
                                           QStringLiteral("MCP interaction cancelled")));
        return true;
    }
    QJsonObject response{{QStringLiteral("action"), decision == McpInteractionDecision::Decline
        ? QStringLiteral("decline") : QStringLiteral("accept")}};
    if (decision == McpInteractionDecision::Accept) {
        QJsonObject content;
        if (call->pendingRequest.type == McpInteractionType::ExternalUrl) {
            if (!value.isUndefined() && !value.isNull()) return false;
        } else {
            if (!value.isObject()) return false;
            const auto supplied = value.toObject();
            if (supplied.size() > call->pendingRequest.fields.size()) return false;
            for (auto it = supplied.constBegin(); it != supplied.constEnd(); ++it) {
                auto field = std::find_if(call->pendingRequest.fields.cbegin(),
                    call->pendingRequest.fields.cend(), [&](const auto& entry) { return entry.name == it.key(); });
                if (field == call->pendingRequest.fields.cend()) return false;
                if (field->type == McpInteractionType::Confirmation) {
                    if (!it.value().isBool()) return false;
                } else if (!it.value().isString() || it.value().toString().size() > field->maxLength ||
                           (field->type == McpInteractionType::Selection &&
                            !field->options.contains(it.value().toString()))) return false;
            }
            for (const auto& field : call->pendingRequest.fields)
                if (field.required && !supplied.contains(field.name)) return false;
            content = supplied;
        }
        response.insert(QStringLiteral("content"), content);
    }
    m_interactions.remove(id);
    emit interactionResolved(id);
    call->pendingId.clear();
    call->inputResponses.insert(call->pendingKey, response);
    call->pendingKey.clear();
    advanceInteraction(call);
    return true;
}

void McpService::finishRemoteToolCall(const std::shared_ptr<RemoteToolCall>& call,
                                      QJsonObject result) {
    if (!call || !call->active) return;
    call->active = false;
    if (!call->pendingId.isEmpty()) {
        m_interactions.remove(call->pendingId);
        emit interactionResolved(call->pendingId);
        call->pendingId.clear();
    }
    auto reply = call->reply;
    call->reply = nullptr;
    if (reply && !reply->isFinished()) reply->abort();
    call->inputResponses = {};
    call->requestState.clear();
    call->hasRequestState = false;
    call->remaining.clear();
    auto completion = std::move(call->completion);
    if (completion) completion(std::move(result));
}

void McpService::startRemoteToolRound(const std::shared_ptr<RemoteToolCall>& call) {
    auto* state = findServer(call->serverName);
    if (!call->active || !state || state->generation != call->generation ||
        state->state != McpConnectionState::Connected) {
        finishRemoteToolCall(call, failure(McpFailureCategory::ServerUnavailable,
                                           QStringLiteral("MCP session unavailable")));
        return;
    }
    if (NetworkPolicyService::instance().check(QUrl(state->config.url)) !=
        NetworkDecision::Allowed) {
        finishRemoteToolCall(call, failure(McpFailureCategory::Offline,
                                           QStringLiteral("Remote MCP blocked by network policy")));
        return;
    }
    if (++call->rounds > 8) {
        finishRemoteToolCall(call, failure(McpFailureCategory::ProtocolError,
                                           QStringLiteral("MCP interaction round limit exceeded")));
        return;
    }
    const int id = state->requestId++;
    if (call->correlationId < 0) call->correlationId = id;
    QJsonObject params{{QStringLiteral("name"), call->toolName},
                       {QStringLiteral("arguments"), call->arguments}};
    if (state->protocolVersion == "2026-07-28") {
        params.insert(QStringLiteral("_meta"), modernMeta());
        if (!call->inputResponses.isEmpty()) params.insert(QStringLiteral("inputResponses"), call->inputResponses);
        if (call->hasRequestState) params.insert(QStringLiteral("requestState"), call->requestState);
    }
    call->inputResponses = {};
    QJsonObject request{{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                        {QStringLiteral("id"), id},
                        {QStringLiteral("method"), QStringLiteral("tools/call")},
                        {QStringLiteral("params"), params}};
    auto networkRequest = remoteRequest(*state);
    networkRequest.setRawHeader("Mcp-Method", "tools/call");
    networkRequest.setRawHeader("Mcp-Name", headerValue(call->toolName));
    const auto tool = std::find_if(state->tools.cbegin(), state->tools.cend(),
        [&](const auto& item) { return item.name == call->toolName; });
    if (tool == state->tools.cend() || !applyToolHeaders(networkRequest, tool->inputSchema, call->arguments)) {
        finishRemoteToolCall(call, failure(McpFailureCategory::InvalidToolSchema,
                                           QStringLiteral("Invalid MCP tool headers")));
        return;
    }
    auto* reply = m_networkManager.post(networkRequest, QJsonDocument(request).toJson(QJsonDocument::Compact));
    call->reply = reply;
    m_remoteReplies[call->serverName].append(reply);
    auto timedOut = std::make_shared<bool>(false);
    connect(reply, &QNetworkReply::readyRead, this, [reply] {
        if (reply->bytesAvailable() > 1024 * 1024) reply->abort();
    });
    QPointer<McpService> self(this);
    QTimer::singleShot(30000, this, [self, call, reply, timedOut] {
        if (self && call->active && call->reply == reply && !reply->isFinished()) {
            *timedOut = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [self, call, reply, id, timedOut] {
        if (!self) return;
        self->m_remoteReplies[call->serverName].removeAll(reply);
        if (call->reply == reply) call->reply = nullptr;
        const auto* current = self->findServer(call->serverName);
        if (!call->active || !current || current->generation != call->generation) {
            reply->deleteLater();
            if (call->active) self->finishRemoteToolCall(call,
                failure(McpFailureCategory::ServerUnavailable, QStringLiteral("MCP session changed")));
            return;
        }
        auto result = *timedOut ? failure(McpFailureCategory::Timeout,
            QStringLiteral("MCP request timed out")) : self->parseRemoteReply(reply, id, true);
        reply->deleteLater();
        if (result.contains(QStringLiteral("error"))) {
            self->finishRemoteToolCall(call, std::move(result));
            return;
        }
        const auto body = result.value(QStringLiteral("result")).toObject();
        if (body.value(QStringLiteral("resultType")).toString() != QLatin1String("input_required")) {
            self->finishRemoteToolCall(call, std::move(result));
            return;
        }
        if (current->protocolVersion != "2026-07-28" ||
            (!body.contains(QStringLiteral("requestState")) &&
             !body.contains(QStringLiteral("inputRequests"))) ||
            (body.contains(QStringLiteral("inputRequests")) &&
             !body.value(QStringLiteral("inputRequests")).isObject()) ||
            body.value(QStringLiteral("inputRequests")).toObject().size() > 8 ||
            (!body.value(QStringLiteral("requestState")).isUndefined() &&
             (!body.value(QStringLiteral("requestState")).isString() ||
              body.value(QStringLiteral("requestState")).toString().size() > 8192))) {
            self->finishRemoteToolCall(call, failure(McpFailureCategory::ProtocolError,
                QStringLiteral("Invalid MCP input request")));
            return;
        }
        call->requestState = body.value(QStringLiteral("requestState")).toString();
        call->hasRequestState = body.contains(QStringLiteral("requestState"));
        const auto requests = body.value(QStringLiteral("inputRequests")).toObject();
        for (auto it = requests.constBegin(); it != requests.constEnd(); ++it) {
            if (it.key().size() > 80 || !it.value().isObject()) {
                self->finishRemoteToolCall(call, failure(McpFailureCategory::ProtocolError,
                    QStringLiteral("Invalid MCP input request key")));
                return;
            }
            call->remaining.insert(it.key(), it.value().toObject());
        }
        self->advanceInteraction(call);
    });
}

void McpService::advanceInteraction(const std::shared_ptr<RemoteToolCall>& call) {
    if (!call->active) return;
    if (call->remaining.isEmpty()) { startRemoteToolRound(call); return; }
    auto it = call->remaining.begin();
    const auto key = it.key();
    const auto item = it.value();
    call->remaining.erase(it);
    if (item.value(QStringLiteral("method")).toString() != QLatin1String("elicitation/create")) {
        finishRemoteToolCall(call, failure(McpFailureCategory::InteractionRejected,
            QStringLiteral("Unsupported MCP interaction method")));
        return;
    }
    const auto params = item.value(QStringLiteral("params")).toObject();
    McpInteractionRequest request;
    request.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.serverId = call->serverName;
    request.generation = call->generation;
    request.correlationId = call->correlationId;
    request.title = QStringLiteral("MCP input requested");
    auto safeText = [](QString value, int limit) {
        value.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
        value.replace(QRegularExpression(QStringLiteral("[\\x00-\\x1f\\x7f]")), QStringLiteral(" "));
        return value.simplified().left(limit);
    };
    request.detail = safeText(params.value(QStringLiteral("message")).toString(), 1000);
    static const QRegularExpression credentialPrompt(
        QStringLiteral("password|secret|token|api.?key|credential|private.?key"),
        QRegularExpression::CaseInsensitiveOption);
    if (credentialPrompt.match(request.detail).hasMatch() &&
        params.value(QStringLiteral("mode")).toString(QStringLiteral("form")) != QLatin1String("url")) {
        finishRemoteToolCall(call, failure(McpFailureCategory::InteractionRejected,
            QStringLiteral("MCP credential collection is unsupported")));
        return;
    }
    request.expiresAt = QDateTime::currentDateTimeUtc().addSecs(300);
    const auto mode = params.value(QStringLiteral("mode")).toString(QStringLiteral("form"));
    if (mode == QLatin1String("url")) {
        const QUrl url(params.value(QStringLiteral("url")).toString());
        if (!url.isValid() || url.scheme() != QLatin1String("https") || url.host().isEmpty() ||
            !url.userInfo().isEmpty() || url.toString().size() > 2048) {
            finishRemoteToolCall(call, failure(McpFailureCategory::InteractionRejected,
                QStringLiteral("Unsafe MCP interaction URL")));
            return;
        }
        request.type = McpInteractionType::ExternalUrl;
        request.url = url.toString();
        request.sensitive = true;
    } else if (mode == QLatin1String("form")) {
        const auto schema = params.value(QStringLiteral("requestedSchema")).toObject();
        const auto properties = schema.value(QStringLiteral("properties")).toObject();
        if (schema.value(QStringLiteral("type")).toString() != QLatin1String("object") ||
            properties.isEmpty() || properties.size() > 8) {
            finishRemoteToolCall(call, failure(McpFailureCategory::InteractionRejected,
                QStringLiteral("Unsupported MCP input schema")));
            return;
        }
        const auto required = schema.value(QStringLiteral("required")).toArray();
        for (auto field = properties.constBegin(); field != properties.constEnd(); ++field) {
            const auto definition = field.value().toObject();
            const auto type = definition.value(QStringLiteral("type")).toString();
            const auto label = safeText(definition.value(QStringLiteral("title")).toString(field.key()), 120);
            static const QRegularExpression credential(QStringLiteral("password|secret|token|api.?key|credential|private.?key"),
                QRegularExpression::CaseInsensitiveOption);
            if (field.key().size() > 80 || credential.match(field.key()).hasMatch() ||
                credential.match(label).hasMatch() || definition.value(QStringLiteral("format")).toString() == QLatin1String("password") ||
                (type != QLatin1String("string") && type != QLatin1String("boolean"))) {
                finishRemoteToolCall(call, failure(McpFailureCategory::InteractionRejected,
                    QStringLiteral("Unsupported or sensitive MCP input field")));
                return;
            }
            McpInteractionField output;
            output.name = field.key();
            output.label = label;
            output.required = required.contains(field.key());
            output.maxLength = type == QLatin1String("string") ? std::clamp(definition.value(QStringLiteral("maxLength")).toInt(500), 1, 500) : 0;
            const auto choices = definition.value(QStringLiteral("enum")).toArray();
            if (type == QLatin1String("boolean")) output.type = McpInteractionType::Confirmation;
            else if (!choices.isEmpty() && choices.size() <= 20) {
                output.type = McpInteractionType::Selection;
                for (const auto& choice : choices) {
                    if (!choice.isString() || choice.toString().size() > 120) {
                        finishRemoteToolCall(call, failure(McpFailureCategory::InteractionRejected,
                            QStringLiteral("Invalid MCP selection option")));
                        return;
                    }
                    output.options.append(safeText(choice.toString(), 120));
                }
            } else if (!choices.isEmpty()) {
                finishRemoteToolCall(call, failure(McpFailureCategory::InteractionRejected,
                    QStringLiteral("MCP selection limit exceeded")));
                return;
            }
            request.fields.append(output);
        }
        request.type = request.fields.size() == 1 ? request.fields.first().type : McpInteractionType::Form;
    } else {
        finishRemoteToolCall(call, failure(McpFailureCategory::InteractionRejected,
            QStringLiteral("Unsupported MCP interaction mode")));
        return;
    }
    call->pendingId = request.id;
    call->pendingKey = key;
    call->pendingRequest = request;
    m_interactions.insert(request.id, call);
    emit interactionRequested(request);
    QPointer<McpService> self(this);
    QTimer::singleShot(300000, this, [self, call, id = request.id] {
        if (self && call->active && call->pendingId == id)
            self->finishRemoteToolCall(call, failure(McpFailureCategory::Timeout,
                QStringLiteral("MCP interaction timed out")));
    });
}

bool McpService::connectToAll() {
    bool allSuccess = true;
    for (auto it = m_servers.begin(); it != m_servers.end(); ++it) {
        if (it->config.enabled) {
            if (!connectToServer(it.key())) {
                allSuccess = false;
            }
        }
    }
    return allSuccess;
}

void McpService::disconnectFromAll() {
    const auto names = m_servers.keys();
    for (const auto& name : names)
        disconnectFromServer(name);
}

bool McpService::connectToLocalServer(McpServerState& state) {
    auto temporaryDirectory = std::make_shared<QTemporaryDir>();
    if (!temporaryDirectory->isValid()) {
        state.errorString = QStringLiteral("MCP sandbox temporary directory is unavailable.");
        return false;
    }
    SandboxExecutionPlan plan;
    plan.workingDirectory = QDir::currentPath();
    plan.readablePaths = {plan.workingDirectory};
    plan.temporaryDirectory = temporaryDirectory->path();
#if defined(Q_OS_WIN)
    plan.requireEnforcement = !state.config.allowPartialConfinement;
    plan.forbidDetachedChildren = true;
    ProcessRequest request;
    request.program = state.config.command;
    request.arguments = state.config.arguments;
    request.workingDirectory = plan.workingDirectory;
    request.environment = QProcessEnvironment::systemEnvironment();
    request.timeoutMs = 0;
    request.sandbox = plan;
    const auto serverName = state.config.name;
    const auto generation = state.generation;
    state.managedProcessId = m_processExecutor.start(request,
        [this, serverName, generation](const ProcessRecord& record) {
            auto* current = findServer(serverName);
            if (!current || current->generation != generation) return;
            if (record.state == ProcessState::Exited || record.state == ProcessState::Failed)
                invalidateInventory(serverName, McpFailureCategory::ServerUnavailable,
                                    QStringLiteral("MCP process exited"));
        },
        [this, serverName, generation](const QString&, ProcessStream stream, const QByteArray& bytes) {
            auto* current = findServer(serverName);
            if (!current || current->generation != generation || stream != ProcessStream::Stdout)
                return;
            m_readBuffers[serverName].append(bytes);
            onProcessReadyRead();
        });
    const auto record = m_processExecutor.record(state.managedProcessId);
    if (record.state != ProcessState::Running) {
        state.failureCategory = McpFailureCategory::SecurityDenied;
        state.errorString = record.error.left(300);
        return false;
    }
    state.sandboxTemporaryDirectory = std::move(temporaryDirectory);
#else
    const auto environment = QProcessEnvironment::systemEnvironment();
    const auto executable = QStandardPaths::findExecutable(state.config.command,
        environment.value(QStringLiteral("PATH")).split(QDir::listSeparator(), Qt::SkipEmptyParts));
    const auto launch = m_processSandbox.prepare(plan, executable, state.config.arguments,
                                                  environment);
    if (!launch.permitted) {
        state.errorString = QStringLiteral("MCP sandbox unavailable: %1.")
                                .arg(launch.result.failureCategory);
        state.failureCategory = McpFailureCategory::SecurityDenied;
        return false;
    }
    QProcess* process = new QProcess(this);
    m_processes.append(process);
    state.process = process;
    state.sandboxTemporaryDirectory = std::move(temporaryDirectory);
    process->setWorkingDirectory(plan.workingDirectory);
    process->setProcessEnvironment(launch.environment);
#if defined(Q_OS_UNIX)
    process->setChildProcessModifier([] { if (setsid() < 0) _exit(127); });
#endif

    connect(process, &QProcess::readyReadStandardOutput, this, &McpService::onProcessReadyRead);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &McpService::onProcessFinished);
    connect(process, &QProcess::errorOccurred, this, &McpService::onProcessErrorOccurred);

    process->start(launch.program, launch.arguments);
    if (!process->waitForStarted(5000)) {
        state.errorString = QStringLiteral("Failed to start MCP server process");
        return false;
    }
    state.processGroupId = static_cast<qint64>(process->processId());
#endif

    // Send initialize request
    QJsonObject params;
    params["protocolVersion"] = "2024-11-05";
    params["capabilities"] = QJsonObject();
    params["clientInfo"] = QJsonObject{{"name", "Sentinel"}, {"version", AppMetadata::version()}};

    QJsonObject response = sendJsonRpc(state.config.name, "initialize", params);
    if (response.contains("error") ||
#if defined(Q_OS_WIN)
        m_processExecutor.record(state.managedProcessId).state != ProcessState::Running
#else
        process->state() != QProcess::Running
#endif
        ) {
        state.errorString = response["error"].toObject()["message"].toString();
        // Keep a bounded child diagnostic when initialization cannot receive a
        // response.  It is transport-only stderr (not a prompt or credential)
        // and is essential for distinguishing sandbox launch failures from
        // JSON-RPC framing failures.
#if !defined(Q_OS_WIN)
        const auto childDiagnostic = QString::fromUtf8(process->readAllStandardError())
                                         .simplified().left(512);
        if (!childDiagnostic.isEmpty())
            state.errorString += state.errorString.isEmpty() ? childDiagnostic
                                                               : QStringLiteral(": ") + childDiagnostic;
#endif
        if (state.errorString.isEmpty())
            state.errorString = QStringLiteral("MCP process exited during initialization");
        return false;
    }
    state.resourcesSupported = response.value(QStringLiteral("result")).toObject()
        .value(QStringLiteral("capabilities")).toObject()
        .value(QStringLiteral("resources")).isObject();

    // Send initialized notification
    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = "notifications/initialized";
#if defined(Q_OS_WIN)
    m_processExecutor.write(state.managedProcessId,
        QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n");
#else
    process->write(QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n");
#endif

    return true;
}

bool McpService::connectToRemoteServer(McpServerState& state) {
    const QUrl url(state.config.url);
    if (NetworkPolicyService::instance().check(url) != NetworkDecision::Allowed) {
        state.errorString = QStringLiteral("Remote MCP blocked by network policy");
        state.failureCategory = McpFailureCategory::Offline;
        return false;
    }
    const bool loopback = url.host().compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0 ||
        QHostAddress(url.host()).isLoopback();
    if (!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() ||
        !url.fragment().isEmpty() || (url.scheme() != QLatin1String("https") &&
        !(loopback && url.scheme() == QLatin1String("http")))) {
        state.errorString = QStringLiteral("Remote MCP endpoint must use HTTPS or loopback HTTP without URL credentials.");
        state.failureCategory = McpFailureCategory::ConfigurationFailure;
        return false;
    }

    state.protocolVersion = "2026-07-28";
    const auto modern = sendJsonRpc(state.config.name, QStringLiteral("server/discover"));
    if (!modern.contains(QStringLiteral("error"))) {
        const auto result = modern.value(QStringLiteral("result")).toObject();
        bool supported = false;
        for (const auto& version : result.value(QStringLiteral("supportedVersions")).toArray())
            supported |= version.toString() == QLatin1String("2026-07-28");
        if (!supported) {
            state.errorString = QStringLiteral("Remote MCP server does not support 2026-07-28");
            state.failureCategory = McpFailureCategory::ProtocolError;
            return false;
        }
        state.resourcesSupported = result.value(QStringLiteral("capabilities")).toObject()
            .value(QStringLiteral("resources")).isObject();
        return true;
    }
    const auto modernError = modern.value(QStringLiteral("error")).toObject();
    const auto modernCategory = modernError.value(QStringLiteral("category")).toString();
    bool legacyAdvertised = false;
    for (const auto& version : modernError.value(QStringLiteral("supportedVersions")).toArray())
        legacyAdvertised |= version.toString() == QLatin1String("2025-11-25");
    if (modernCategory != QLatin1String("ConfigurationFailure") &&
        !(modernCategory == QLatin1String("UnsupportedProtocolVersion") && legacyAdvertised)) {
        state.errorString = modernError.value(QStringLiteral("message")).toString();
        state.failureCategory = categoryFromName(modernCategory);
        return false;
    }
    state.protocolVersion.clear();
    // Legacy Streamable HTTP handshake.
    QJsonObject params;
    params["protocolVersion"] = "2025-11-25";
    params["capabilities"] = QJsonObject();
    params["clientInfo"] = QJsonObject{{"name", "Sentinel"}, {"version", AppMetadata::version()}};

    QJsonObject response = sendJsonRpc(state.config.name, "initialize", params);
    if (response.contains("error")) {
        state.errorString = response["error"].toObject()["message"].toString();
        const auto category = response["error"].toObject()["category"].toString();
        state.failureCategory = categoryFromName(category);
        if (category == QLatin1String("ConfigurationFailure") &&
            (state.errorString.contains(QStringLiteral("HTTP status 404")) ||
             state.errorString.contains(QStringLiteral("HTTP status 405")))) {
            state.failureCategory = McpFailureCategory::UnsupportedLegacyTransport;
            state.errorString = QStringLiteral("Endpoint may require legacy 2024 HTTP+SSE, which is unsupported");
        }
        return false;
    }
    const auto negotiated = response.value(QStringLiteral("result")).toObject()
        .value(QStringLiteral("protocolVersion")).toString().toUtf8();
    if (negotiated != "2025-11-25") {
        state.errorString = QStringLiteral("Unsupported remote MCP protocol version");
        state.failureCategory = McpFailureCategory::ProtocolError;
        return false;
    }
    state.protocolVersion = negotiated;
    state.resourcesSupported = response.value(QStringLiteral("result")).toObject()
        .value(QStringLiteral("capabilities")).toObject()
        .value(QStringLiteral("resources")).isObject();

    sendRemoteNotification(state, QStringLiteral("notifications/initialized"));

    return true;
}

QNetworkRequest McpService::remoteRequest(const McpServerState& state) const {
    QNetworkRequest request(QUrl(state.config.url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json, text/event-stream");
    if (!state.protocolVersion.isEmpty())
        request.setRawHeader("MCP-Protocol-Version", state.protocolVersion);
    if (!state.remoteSessionId.isEmpty())
        request.setRawHeader("MCP-Session-Id", state.remoteSessionId);
    for (auto it = state.config.headers.constBegin(); it != state.config.headers.constEnd(); ++it) {
        const auto key = it.key().toLower();
        static const QRegularExpression token(QStringLiteral("^[A-Za-z0-9!#$%&'*+.^_`|~-]+$"));
        if (key == QLatin1String("host") || key == QLatin1String("content-type") ||
            key == QLatin1String("accept") ||
            key.startsWith(QLatin1String("mcp-")) || !token.match(it.key()).hasMatch() ||
            !it.value().isString())
            continue;
        QString configured = it.value().toString();
        if (configured.startsWith(QLatin1String("secret://"))) {
            const QByteArray identity = (state.config.name + QLatin1Char(':') + it.key()).toUtf8();
            const auto id = QStringLiteral("mcp-") + QString::fromLatin1(
                QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
            if (configured != QStringLiteral("secret://") + id) continue;
            const auto credential = defaultCredentialStore().readCredential(
                CredentialKey{id, QStringLiteral("header")});
            if (!credential.secret.has_value()) continue;
            configured = *credential.secret;
        }
        const auto value = configured.toUtf8();
        if (!value.contains('\r') && !value.contains('\n'))
            request.setRawHeader(it.key().toUtf8(), value);
    }
    return request;
}

QJsonObject McpService::parseRemoteReply(QNetworkReply* reply, int id,
                                         bool allowInputRequired) const {
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 404 && reply->request().hasRawHeader("MCP-Session-Id"))
        return failure(McpFailureCategory::ServerUnavailable, QStringLiteral("MCP session expired"));
    if (status == 400) {
        const auto document = QJsonDocument::fromJson(reply->peek(8192));
        const auto error = document.object().value(QStringLiteral("error")).toObject();
        const auto code = error.value(QStringLiteral("code")).toInt();
        if (code == -32022) {
            auto result = failure(McpFailureCategory::UnsupportedProtocolVersion,
                                  QStringLiteral("MCP protocol version unsupported"));
            auto details = result.value(QStringLiteral("error")).toObject();
            details.insert(QStringLiteral("supportedVersions"),
                           error.value(QStringLiteral("data")).toObject()
                               .value(QStringLiteral("supported")).toArray());
            result.insert(QStringLiteral("error"), details);
            return result;
        }
        if (code == -32020)
            return failure(McpFailureCategory::ProtocolError,
                           QStringLiteral("MCP protocol version or headers rejected"));
    }
    if (status >= 300)
        return failure(httpFailure(status), QStringLiteral("MCP HTTP status %1").arg(status));
    if (reply->error() != QNetworkReply::NoError)
        return failure(McpFailureCategory::TransportFailure, QStringLiteral("MCP network request failed"));
    const auto payload = reply->readAll();
    if (payload.size() > 1024 * 1024)
        return failure(McpFailureCategory::ProtocolError, QStringLiteral("MCP response exceeds limit"));
    const auto contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString()
        .section(QLatin1Char(';'), 0, 0).trimmed().toLower();
    if (contentType == QLatin1String("application/json")) {
        const auto document = QJsonDocument::fromJson(payload);
        return document.isObject() ? checkedResponse(document.object(), id, allowInputRequired)
            : failure(McpFailureCategory::ProtocolError, QStringLiteral("Invalid MCP JSON response"));
    }
    if (contentType == QLatin1String("text/event-stream")) {
        QByteArray data;
        const auto lines = payload.split('\n');
        for (auto line : lines) {
            if (line.endsWith('\r')) line.chop(1);
            if (line.startsWith("data:")) {
                if (!data.isEmpty()) data.append('\n');
                data.append(line.mid(5).trimmed());
            } else if (line.isEmpty() && !data.isEmpty()) {
                const auto document = QJsonDocument::fromJson(data);
                if (document.isObject() && document.object().value(QStringLiteral("id")).toInt(-1) == id)
                    return checkedResponse(document.object(), id, allowInputRequired);
                data.clear();
            }
        }
        return failure(McpFailureCategory::ProtocolError, QStringLiteral("MCP SSE response missing"));
    }
    return failure(McpFailureCategory::ProtocolError, QStringLiteral("Unsupported MCP content type"));
}

void McpService::sendRemoteNotification(McpServerState& state, const QString& method) {
    if (NetworkPolicyService::instance().check(QUrl(state.config.url)) !=
        NetworkDecision::Allowed) return;
    auto* reply = m_networkManager.post(remoteRequest(state),
        QJsonDocument(QJsonObject{{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                                  {QStringLiteral("method"), method}})
            .toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

void McpService::startRemoteEventStream(const QString& serverName) {
    auto* state = findServer(serverName);
    if (!state || state->state != McpConnectionState::Connected ||
        state->config.type != QLatin1String("remote")) return;
    if (NetworkPolicyService::instance().check(QUrl(state->config.url)) !=
        NetworkDecision::Allowed) return;
    const auto generation = state->generation;
    auto request = remoteRequest(*state);
    const bool modern = state->protocolVersion == "2026-07-28";
    QNetworkReply* reply = nullptr;
    if (modern) {
        request.setRawHeader("Mcp-Method", "subscriptions/listen");
        const QJsonObject params{
            {QStringLiteral("notifications"), QJsonObject{
                {QStringLiteral("toolsListChanged"), true},
                {QStringLiteral("resourcesListChanged"), true}}},
            {QStringLiteral("_meta"), modernMeta()}};
        reply = m_networkManager.post(request, QJsonDocument(QJsonObject{
            {QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
            {QStringLiteral("id"), state->requestId++},
            {QStringLiteral("method"), QStringLiteral("subscriptions/listen")},
            {QStringLiteral("params"), params}}).toJson(QJsonDocument::Compact));
    } else {
        request.setRawHeader("Accept", "text/event-stream");
        if (!state->lastEventId.isEmpty()) request.setRawHeader("Last-Event-ID", state->lastEventId);
        reply = m_networkManager.get(request);
    }
    m_remoteReplies[serverName].append(QPointer<QNetworkReply>(reply));
    auto buffer = std::make_shared<QByteArray>();
    auto retryMs = std::make_shared<int>(1000);
    connect(reply, &QIODevice::readyRead, this, [this, reply, buffer, retryMs, serverName, generation, modern] {
        auto* current = findServer(serverName);
        if (!current || current->generation != generation) return;
        buffer->append(reply->readAll());
        if (buffer->size() > 1024 * 1024) {
            reply->abort();
            return;
        }
        while (true) {
            const auto end = buffer->indexOf("\n\n");
            if (end < 0) break;
            const auto event = buffer->left(end);
            buffer->remove(0, end + 2);
            QByteArray data;
            for (auto line : event.split('\n')) {
                if (line.endsWith('\r')) line.chop(1);
                if (line.startsWith("data:")) data.append(line.mid(5).trimmed());
                else if (!modern && line.startsWith("id:")) {
                    const auto id = line.mid(3).trimmed();
                    if (id.size() <= 256 && std::all_of(id.cbegin(), id.cend(), [](char ch) {
                        return ch >= 0x21 && ch <= 0x7e;
                    })) current->lastEventId = id;
                } else if (line.startsWith("retry:")) {
                    bool ok = false;
                    const auto value = line.mid(6).trimmed().toInt(&ok);
                    if (ok) *retryMs = qBound(1000, value, 30000);
                }
            }
            const auto document = QJsonDocument::fromJson(data);
            if (!document.isObject()) continue;
            const auto method = document.object().value(QStringLiteral("method")).toString();
            if (method == QLatin1String("notifications/tools/list_changed"))
                QTimer::singleShot(0, this, [this, serverName, generation] {
                    const auto* current = findServer(serverName);
                    if (current && current->generation == generation) refreshTools(serverName);
                });
            else if (method == QLatin1String("notifications/resources/list_changed"))
                QTimer::singleShot(0, this, [this, serverName, generation] {
                    const auto* current = findServer(serverName);
                    if (current && current->generation == generation) refreshResources(serverName);
                });
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, retryMs, serverName, generation, modern] {
        auto replies = m_remoteReplies.find(serverName);
        if (replies != m_remoteReplies.end()) {
            replies->removeOne(QPointer<QNetworkReply>(reply));
            if (replies->isEmpty()) m_remoteReplies.erase(replies);
        }
        const auto* current = findServer(serverName);
        const bool active = current && current->generation == generation &&
            current->state == McpConnectionState::Connected;
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        if (!active) return;
        if (status == 405 || (modern && status == 404)) {
            QTimer::singleShot(60000, this, [this, serverName, generation] {
                const auto* current = findServer(serverName);
                if (current && current->generation == generation && refreshTools(serverName))
                    startRemoteEventStream(serverName);
            });
            return;
        }
        if (status == 401 || status == 403 || status == 404) {
            invalidateInventory(serverName, httpFailure(status),
                                QStringLiteral("MCP event stream rejected"));
            return;
        }
        QTimer::singleShot(*retryMs, this, [this, serverName, generation] {
            const auto* current = findServer(serverName);
            if (!current || current->generation != generation ||
                current->state != McpConnectionState::Connected) return;
            if (refreshTools(serverName)) startRemoteEventStream(serverName);
        });
    });
}

void McpService::disconnectServer(McpServerState& state) {
    if (!state.managedProcessId.isEmpty()) {
        m_processExecutor.kill(state.managedProcessId);
        state.managedProcessId.clear();
    }
    if (state.process) {
        disconnect(state.process, nullptr, this, nullptr);
#if defined(Q_OS_UNIX)
        if (state.processGroupId > 0)
            ::kill(-static_cast<pid_t>(state.processGroupId), SIGTERM);
#endif
        state.process->terminate();
        if (!state.process->waitForFinished(3000)) {
#if defined(Q_OS_UNIX)
            if (state.processGroupId > 0)
                ::kill(-static_cast<pid_t>(state.processGroupId), SIGKILL);
#endif
            state.process->kill();
            state.process->waitForFinished(1000);
        }
#if defined(Q_OS_UNIX)
        // The leader may exit before its children; clean the remaining group.
        if (state.processGroupId > 0)
            ::kill(-static_cast<pid_t>(state.processGroupId), SIGKILL);
#endif
        m_processes.removeOne(state.process);
        delete state.process;
        state.process = nullptr;
        state.processGroupId = 0;
    }
    state.sandboxTemporaryDirectory.reset();
}

void McpService::listTools(McpServerState& state) {
    const auto generation = state.generation;
    QList<McpToolDefinition> next;
    QString cursor;
    QSet<QString> seenCursors;
    for (int page = 0; page < 32; ++page) {
        QJsonObject params;
        if (!cursor.isEmpty()) params.insert(QStringLiteral("cursor"), cursor);
        const auto response = sendJsonRpc(state.config.name, QStringLiteral("tools/list"), params);
        if (state.generation != generation) return;
        if (response.contains(QStringLiteral("error"))) {
            const auto error = response.value(QStringLiteral("error")).toObject();
            const auto category = error.value(QStringLiteral("category")).toString();
            const auto failureCategory = categoryFromName(category);
            invalidateInventory(state.config.name, failureCategory,
                                error.value(QStringLiteral("message")).toString());
            return;
        }
        const auto result = response.value(QStringLiteral("result")).toObject();
        if (!result.value(QStringLiteral("tools")).isArray()) {
            invalidateInventory(state.config.name, McpFailureCategory::ProtocolError,
                                QStringLiteral("Invalid MCP tools/list response"));
            return;
        }
        for (const auto& toolValue : result.value(QStringLiteral("tools")).toArray()) {
            if (!toolValue.isObject()) {
                invalidateInventory(state.config.name, McpFailureCategory::InvalidToolSchema,
                                    QStringLiteral("Invalid MCP tool definition"));
                return;
            }
            const auto toolObj = toolValue.toObject();
            if (toolObj.contains(QStringLiteral("inputSchema")) &&
                !toolObj.value(QStringLiteral("inputSchema")).isObject()) {
                invalidateInventory(state.config.name, McpFailureCategory::InvalidToolSchema,
                                    QStringLiteral("MCP input schema is not an object"));
                return;
            }
            if (state.config.type == QLatin1String("remote") &&
                state.protocolVersion == "2026-07-28") {
                QSet<QString> headerNames;
                QList<HeaderBinding> bindings;
                if (!collectHeaders(toolObj.value(QStringLiteral("inputSchema")).toObject(),
                                    {}, headerNames, bindings)) {
                    qWarning() << "McpService: excluded tool with invalid x-mcp-header annotation"
                               << toolObj.value(QStringLiteral("name")).toString();
                    continue;
                }
            }
            McpToolDefinition tool;
            tool.name = toolObj.value(QStringLiteral("name")).toString();
            tool.description = toolObj.value(QStringLiteral("description")).toString();
            tool.serverName = state.config.name;
            tool.inputSchema = toolObj.value(QStringLiteral("inputSchema")).toObject();
            tool.filesystemSemanticContract = toolObj.value(QStringLiteral("_meta")).toObject()
                .value(QStringLiteral("sentinel.failureSemanticContract")).toString() == QLatin1String("filesystem");
            next.append(std::move(tool));
            if (next.size() > 512) {
                invalidateInventory(state.config.name, McpFailureCategory::ProtocolError,
                                    QStringLiteral("MCP tool inventory exceeds limit"));
                return;
            }
        }
        cursor = result.value(QStringLiteral("nextCursor")).toString();
        if (cursor.isEmpty()) {
            state.tools = std::move(next);
            state.errorString.clear();
            state.failureCategory = McpFailureCategory::None;
            emit toolsUpdated(state.config.name);
            return;
        }
        if (seenCursors.contains(cursor)) break;
        seenCursors.insert(cursor);
    }
    invalidateInventory(state.config.name, McpFailureCategory::ProtocolError,
                        QStringLiteral("MCP tool pagination did not terminate"));
}

void McpService::listResources(McpServerState& state) {
    const auto generation = state.generation;
    QList<McpResource> next;
    QSet<QString> seenUris;
    QSet<QString> seenCursors;
    QString cursor;
    for (int page = 0; page < 32; ++page) {
        QJsonObject params;
        if (!cursor.isEmpty()) params.insert(QStringLiteral("cursor"), cursor);
        const auto response = sendJsonRpc(state.config.name, QStringLiteral("resources/list"), params);
        if (state.generation != generation) return;
        if (response.contains(QStringLiteral("error"))) {
            const auto error = response.value(QStringLiteral("error")).toObject();
            const auto category = error.value(QStringLiteral("category")).toString();
            const auto failureCategory = categoryFromName(category);
            invalidateInventory(state.config.name, failureCategory,
                                error.value(QStringLiteral("message")).toString());
            return;
        }
        const auto result = response.value(QStringLiteral("result")).toObject();
        if (!result.value(QStringLiteral("resources")).isArray()) {
            invalidateInventory(state.config.name, McpFailureCategory::ProtocolError,
                                QStringLiteral("Invalid MCP resources/list response"));
            return;
        }
        for (const auto& value : result.value(QStringLiteral("resources")).toArray()) {
            if (!value.isObject()) {
                invalidateInventory(state.config.name, McpFailureCategory::ProtocolError,
                                    QStringLiteral("Invalid MCP resource definition"));
                return;
            }
            const auto object = value.toObject();
            const auto uri = object.value(QStringLiteral("uri")).toString();
            if (uri.isEmpty() || seenUris.contains(uri)) {
                invalidateInventory(state.config.name, McpFailureCategory::ProtocolError,
                                    QStringLiteral("Missing or duplicate MCP resource URI"));
                return;
            }
            seenUris.insert(uri);
            next.append({uri, object.value(QStringLiteral("name")).toString(),
                         object.value(QStringLiteral("description")).toString(),
                         object.value(QStringLiteral("mimeType")).toString(), state.config.name});
            if (next.size() > 512) {
                invalidateInventory(state.config.name, McpFailureCategory::ProtocolError,
                                    QStringLiteral("MCP resource inventory exceeds limit"));
                return;
            }
        }
        cursor = result.value(QStringLiteral("nextCursor")).toString();
        if (cursor.isEmpty()) {
            state.resources = std::move(next);
            state.lastKnownResources = state.resources;
            emit resourcesUpdated(state.config.name);
            return;
        }
        if (seenCursors.contains(cursor)) break;
        seenCursors.insert(cursor);
    }
    invalidateInventory(state.config.name, McpFailureCategory::ProtocolError,
                        QStringLiteral("MCP resource pagination did not terminate"));
}

QJsonObject McpService::sendJsonRpc(const QString& serverName, const QString& method,
                                    const QJsonObject& params) {
    auto* state = findServer(serverName);
    if (!state) {
        return failure(McpFailureCategory::ServerUnavailable, QStringLiteral("Server not found"));
    }
    QScopedValueRollback active(state->activeSynchronousCalls,
                                state->activeSynchronousCalls + 1);

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = state->requestId++;
    request["method"] = method;
    QJsonObject requestParams = params;
    if (state->config.type == QLatin1String("remote") &&
        state->protocolVersion == "2026-07-28")
        requestParams.insert(QStringLiteral("_meta"), modernMeta());
    if (!requestParams.isEmpty()) request["params"] = requestParams;

    QByteArray jsonData = QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n";

    if (state->config.type == "local" &&
        (state->process || !state->managedProcessId.isEmpty())) {
        state->synchronousRequestId = request.value("id").toInt();
        state->synchronousResponseReceived = false;
        state->synchronousResponse = {};
        const bool written = state->process ? state->process->write(jsonData) == jsonData.size()
            : m_processExecutor.write(state->managedProcessId, jsonData);
        if (!written) {
            state->synchronousRequestId = -1;
            return failure(McpFailureCategory::TransportFailure, QStringLiteral("MCP stdio write failed"));
        }
        QElapsedTimer timer;
        timer.start();
        auto running = [this, state] {
            return state->process ? state->process->state() == QProcess::Running
                : m_processExecutor.record(state->managedProcessId).state == ProcessState::Running;
        };
        while (!state->synchronousResponseReceived && timer.elapsed() < 30000 && running()) {
            onProcessReadyRead();
            if (state->synchronousResponseReceived)
                break;
            if (state->process)
                state->process->waitForReadyRead(qMax(1, 30000 - static_cast<int>(timer.elapsed())));
            else QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
        }
        const auto response = state->synchronousResponse;
        const bool received = state->synchronousResponseReceived;
        state->synchronousRequestId = -1;
        state->synchronousResponseReceived = false;
        if (received)
            return checkedResponse(response, request.value("id").toInt());
        return failure(running()
                           ? McpFailureCategory::Timeout : McpFailureCategory::ServerUnavailable,
                       QStringLiteral("MCP stdio response unavailable"));
    }

    if (state->config.type == "remote") {
        if (NetworkPolicyService::instance().check(QUrl(state->config.url)) !=
            NetworkDecision::Allowed)
            return failure(McpFailureCategory::Offline,
                           QStringLiteral("Remote MCP blocked by network policy"));
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        const quint64 generation = state->generation;
        auto networkRequest = remoteRequest(*state);
        if (state->protocolVersion == "2026-07-28") {
            networkRequest.setRawHeader("Mcp-Method", method.toUtf8());
            if (method == QLatin1String("tools/call")) {
                const auto name = params.value(QStringLiteral("name")).toString().toUtf8();
                networkRequest.setRawHeader("Mcp-Name", headerValue(QString::fromUtf8(name)));
                const auto tool = std::find_if(state->tools.cbegin(), state->tools.cend(),
                    [&](const auto& item) { return item.name.toUtf8() == name; });
                if (tool == state->tools.cend() ||
                    !applyToolHeaders(networkRequest, tool->inputSchema,
                                      params.value(QStringLiteral("arguments")).toObject()))
                    return failure(McpFailureCategory::InvalidToolSchema,
                                   QStringLiteral("Invalid MCP tool header schema"));
            }
        }
        QNetworkReply* reply = m_networkManager.post(networkRequest, jsonData);
        connect(reply, &QIODevice::readyRead, reply, [reply] {
            if (reply->bytesAvailable() > 1024 * 1024) reply->abort();
        });
        m_remoteReplies[serverName].append(QPointer<QNetworkReply>(reply));
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(30000);
        loop.exec();
        auto replies = m_remoteReplies.find(serverName);
        if (replies != m_remoteReplies.end()) {
            replies->removeOne(QPointer<QNetworkReply>(reply));
            if (replies->isEmpty()) m_remoteReplies.erase(replies);
        }
        if (state->generation != generation || state->state == McpConnectionState::Disconnected) {
            reply->deleteLater();
            return failure(McpFailureCategory::Cancelled,
                           QStringLiteral("MCP server disconnected"));
        }
        if (!reply->isFinished()) {
            reply->abort();
            reply->deleteLater();
            return failure(McpFailureCategory::Timeout, QStringLiteral("MCP remote request timed out"));
        }
        const auto result = parseRemoteReply(reply, request.value("id").toInt());
        if (method == QLatin1String("initialize") && !result.contains(QStringLiteral("error"))) {
            const auto session = reply->rawHeader("Mcp-Session-Id");
            if (session.size() <= 256 &&
                std::all_of(session.cbegin(), session.cend(), [](char ch) {
                    return ch >= 0x21 && ch <= 0x7e;
                }))
                state->remoteSessionId = session;
        }
        reply->deleteLater();
        return result;
    }

    return failure(McpFailureCategory::ServerUnavailable, QStringLiteral("Not connected"));
}

void McpService::onProcessReadyRead() {
    QScopedValueRollback processing(m_processingStdout, m_processingStdout + 1);
    for (auto& state : m_servers) {
        if (state.process || !state.managedProcessId.isEmpty()) {
            auto& buffer = m_readBuffers[state.config.name];
            if (state.process && state.process->bytesAvailable() > 0)
                buffer += state.process->readAllStandardOutput();
            if (buffer.size() > 1024 * 1024) {
                buffer.clear();
                invalidateInventory(state.config.name, McpFailureCategory::ProtocolError,
                                    QStringLiteral("MCP stdio message exceeds limit"));
                return;
            }
            while (true) {
                const auto end = buffer.indexOf('\n');
                if (end < 0)
                    break;
                const auto line = buffer.left(end);
                buffer.remove(0, end + 1);
                const auto document = QJsonDocument::fromJson(line);
                if (!document.isObject()) {
                    invalidateInventory(state.config.name, McpFailureCategory::ProtocolError,
                                        QStringLiteral("Invalid MCP stdio JSON"));
                    return;
                }
                const auto response = document.object();
                const auto id = response.value(QStringLiteral("id")).toInt(-1);
                if (id < 0) {
                    const auto method = response.value(QStringLiteral("method")).toString();
                    if (method == QLatin1String("notifications/tools/list_changed") &&
                        !m_toolRefreshScheduled.contains(state.config.name)) {
                        const auto name = state.config.name;
                        m_toolRefreshScheduled.insert(name);
                        QTimer::singleShot(0, this, [this, name] {
                            m_toolRefreshScheduled.remove(name);
                            refreshTools(name);
                        });
                    } else if (method == QLatin1String("notifications/resources/list_changed") &&
                               !m_resourceRefreshScheduled.contains(state.config.name)) {
                        const auto name = state.config.name;
                        m_resourceRefreshScheduled.insert(name);
                        QTimer::singleShot(0, this, [this, name] {
                            m_resourceRefreshScheduled.remove(name);
                            refreshResources(name);
                        });
                    }
                    continue;
                }
                if (id == state.synchronousRequestId) {
                    state.synchronousResponse = response;
                    state.synchronousResponseReceived = true;
                    continue;
                }
                auto completion = m_pendingCalls[state.config.name].take(id);
                if (completion) {
                    const auto checked = checkedResponse(response, id);
                    const bool protocolError = checked.value(QStringLiteral("error")).toObject()
                            .value(QStringLiteral("category")).toString() ==
                        QLatin1String("ProtocolError");
                    if (protocolError)
                        invalidateInventory(state.config.name, McpFailureCategory::ProtocolError,
                                            QStringLiteral("Invalid MCP tool response"));
                    completion(checked);
                    if (protocolError)
                        return;
                }
            }
        }
    }
}

void McpService::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    QScopedValueRollback processing(m_processingStdout, m_processingStdout + 1);
    auto* process = qobject_cast<QProcess*>(sender());
    for (auto& state : m_servers) {
        if (state.process == process && state.state != McpConnectionState::Disconnected) {
#if defined(Q_OS_UNIX)
            if (state.processGroupId > 0)
                ::kill(-static_cast<pid_t>(state.processGroupId), SIGKILL);
#endif
            state.processGroupId = 0;
            invalidateInventory(state.config.name, McpFailureCategory::ServerUnavailable,
                                QStringLiteral("MCP process exited"));
            break;
        }
    }
}

void McpService::onProcessErrorOccurred(QProcess::ProcessError error) {
    Q_UNUSED(error)

    QScopedValueRollback processing(m_processingStdout, m_processingStdout + 1);
    auto* process = qobject_cast<QProcess*>(sender());
    for (auto& state : m_servers) {
        if (state.process == process && state.state != McpConnectionState::Disconnected) {
            invalidateInventory(state.config.name, McpFailureCategory::ServerUnavailable,
                                state.process->errorString());
            break;
        }
    }
}

McpServerState* McpService::findServer(const QString& serverName) {
    auto it = m_servers.find(serverName);
    if (it == m_servers.end()) {
        return nullptr;
    }
    return &it.value();
}

const McpServerState* McpService::findServer(const QString& serverName) const {
    auto it = m_servers.find(serverName);
    if (it == m_servers.end()) {
        return nullptr;
    }
    return &it.value();
}

} // namespace sentinel::core
