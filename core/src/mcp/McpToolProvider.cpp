// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/mcp/McpToolProvider.h"
#include "sentinel/core/mcp/McpToolCatalog.h"
#include <QDebug>
#include <QDir>
#include <optional>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSet>
#include <QThread>
#include <atomic>
#include <mutex>

namespace sentinel::core {
namespace {
std::optional<FileSystemFailure> declaredFailure(const QString& value) {
    if (value == QLatin1String("not-found")) return FileSystemFailure::NotFound;
    if (value == QLatin1String("permission-denied")) return FileSystemFailure::PermissionDenied;
    if (value == QLatin1String("not-file")) return FileSystemFailure::NotFile;
    if (value == QLatin1String("not-directory")) return FileSystemFailure::NotDirectory;
    if (value == QLatin1String("already-exists")) return FileSystemFailure::AlreadyExists;
    if (value == QLatin1String("invalid-path")) return FileSystemFailure::InvalidPath;
    if (value == QLatin1String("read-failed")) return FileSystemFailure::ReadFailed;
    if (value == QLatin1String("write-failed")) return FileSystemFailure::WriteFailed;
    if (value == QLatin1String("io-error")) return FileSystemFailure::IOError;
    if (value == QLatin1String("unavailable")) return FileSystemFailure::Unavailable;
    return {};
}
QList<FileMutation> declaredMutations(const QJsonObject& data, bool contract) {
    QList<FileMutation> mutations;
    if (!contract || data.value(QStringLiteral("kind")).toString() != QLatin1String("filesystem-mutation") ||
        !data.value(QStringLiteral("mutations")).isArray()) return mutations;
    for (const auto& item : data.value(QStringLiteral("mutations")).toArray()) {
        const auto object = item.toObject();
        const auto path = object.value(QStringLiteral("path")).toString();
        const auto action = object.value(QStringLiteral("kind")).toString();
        if (!QDir::isAbsolutePath(path)) return {};
        FileMutationKind kind;
        if (action == QLatin1String("created")) kind = FileMutationKind::Created;
        else if (action == QLatin1String("modified")) kind = FileMutationKind::Modified;
        else if (action == QLatin1String("deleted")) kind = FileMutationKind::Deleted;
        else if (action == QLatin1String("moved-from")) kind = FileMutationKind::MovedFrom;
        else if (action == QLatin1String("moved-to")) kind = FileMutationKind::MovedTo;
        else return {};
        mutations.append({QDir::cleanPath(path), kind});
    }
    return mutations;
}
std::optional<FileSystemOperation> declaredOperation(const QString& value) {
    if (value == QLatin1String("read-file")) return FileSystemOperation::ReadFile;
    if (value == QLatin1String("list-directory")) return FileSystemOperation::ListDirectory;
    if (value == QLatin1String("write-file")) return FileSystemOperation::WriteFile;
    if (value == QLatin1String("delete-file")) return FileSystemOperation::Delete;
    if (value == QLatin1String("move-file")) return FileSystemOperation::Move;
    if (value == QLatin1String("glob")) return FileSystemOperation::Glob;
    if (value == QLatin1String("grep")) return FileSystemOperation::Grep;
    return {};
}
StructuredObservationPtr declaredObservation(const QJsonObject& data, bool contract) {
    if (!contract) return data.isEmpty() ? StructuredObservationPtr{} :
        std::make_shared<StructuredObservation>(StructuredObservation{StructuredObservationKind::Generic, data});
    const auto kind = data.value(QStringLiteral("kind")).toString();
    const auto resource = data.value(QStringLiteral("resource")).toString();
    if (kind == QLatin1String("filesystem-failure") && QDir::isAbsolutePath(resource)) {
        const auto reason = declaredFailure(data.value(QStringLiteral("reason")).toString());
        const auto operation = declaredOperation(data.value(QStringLiteral("operation")).toString());
        if (!reason || !operation) return {};
        auto observation = std::make_shared<StructuredObservation>();
        observation->kind = StructuredObservationKind::FileSystemFailure;
        observation->fileSystemFailure = *reason;
        observation->fileSystemOperation = *operation;
        observation->failureResource = QDir::cleanPath(resource);
        observation->data = {{QStringLiteral("resource"), observation->failureResource}};
        return observation;
    }
    if (kind == QLatin1String("directory-listing") &&
        QDir::isAbsolutePath(data.value(QStringLiteral("path")).toString()) &&
        data.value(QStringLiteral("entries")).isArray() &&
        data.value(QStringLiteral("complete")).isBool() &&
        data.value(QStringLiteral("truncated")).isBool())
        return std::make_shared<StructuredObservation>(StructuredObservation{StructuredObservationKind::DirectoryListing, data});
    return {};
}
class McpToolHandler final : public IToolHandler {
public:
    McpToolHandler(std::shared_ptr<IMcpService> service, QString server, QString remoteTool,
                   bool filesystemContract)
        : service_(std::move(service)), server_(std::move(server)),
          remoteTool_(std::move(remoteTool)), filesystemContract_(filesystemContract) {}

    IToolExecutor::Cancel execute(const ToolExecutionRequest& request, const QString&,
                                  const QString&, IToolExecutor::Output,
                                  IToolExecutor::Completion completion) override {
        // The gateway has already applied the remote inputSchema. Preserve JSON types.
        QJsonObject arguments;
        for (const auto& argument : request.plan.invocations.first().arguments)
            arguments.insert(argument.id, argument.jsonValue.isUndefined()
                                              ? QJsonValue(argument.value)
                                              : argument.jsonValue);
        struct Invocation {
            std::atomic_bool active{true};
            std::mutex mutex;
            IMcpService::Cancel cancel;
        };
        auto invocation = std::make_shared<Invocation>();
        auto onResult = [invocation, contract = filesystemContract_, completion = std::move(completion)](QJsonObject response) {
            if (!invocation->active.exchange(false))
                return;
            if (response.contains(QStringLiteral("error"))) {
                const auto message = response.value(QStringLiteral("error"))
                                         .toObject()
                                         .value(QStringLiteral("message"))
                                         .toString();
                completion({ToolExecutionStatus::Blocked,
                            message.isEmpty() ? QStringLiteral("MCP call failed") : message});
                return;
            }
            if (!response.value(QStringLiteral("result")).isObject()) {
                completion(
                    {ToolExecutionStatus::Blocked, QStringLiteral("Invalid MCP tool response")});
                return;
            }
            const auto result = response.value(QStringLiteral("result")).toObject();
            const QJsonObject data = result.value(QStringLiteral("structuredContent")).toObject();
            const auto observation = declaredObservation(data, contract);
            const auto summary = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
            if (result.value(QStringLiteral("isError")).toBool()) {
                completion({ToolExecutionStatus::Failed, summary, observation});
                return;
            }
            completion({ToolExecutionStatus::Succeeded, summary, observation,
                        declaredMutations(data, contract)});
        };
        if (auto* object = dynamic_cast<McpService*>(service_.get());
            object && object->thread() != QThread::currentThread()) {
            auto context =
                std::shared_ptr<QObject>(new QObject, [](QObject* value) { value->deleteLater(); });
            QMetaObject::invokeMethod(
                object,
                [service = service_, object, context, invocation, server = server_,
                 tool = remoteTool_, arguments, onResult = std::move(onResult)]() mutable {
                    if (!invocation->active.load())
                        return;
                    auto cancel = service->callToolAsync(
                        server, tool, arguments,
                        [context, onResult = std::move(onResult)](QJsonObject response) mutable {
                            QMetaObject::invokeMethod(
                                context.get(),
                                [context, onResult = std::move(onResult),
                                 response = std::move(response)]() mutable {
                                    onResult(std::move(response));
                                },
                                Qt::QueuedConnection);
                        });
                    std::lock_guard lock(invocation->mutex);
                    invocation->cancel = std::move(cancel);
                },
                Qt::QueuedConnection);
            return [invocation, service = service_, object] {
                invocation->active = false;
                QMetaObject::invokeMethod(
                    object,
                    [invocation, service] {
                        std::lock_guard lock(invocation->mutex);
                        if (invocation->cancel)
                            invocation->cancel();
                    },
                    Qt::QueuedConnection);
            };
        }
        auto cancel = service_->callToolAsync(server_, remoteTool_, arguments, std::move(onResult));
        return [invocation, cancel = std::move(cancel)] {
            invocation->active = false;
            if (cancel)
                cancel();
        };
    }

private:
    std::shared_ptr<IMcpService> service_;
    QString server_;
    QString remoteTool_;
    bool filesystemContract_ = false;
};
} // namespace

McpToolProvider::McpToolProvider(std::shared_ptr<IMcpService> service, IToolRegistry& registry,
                                 QObject* parent)
    : QObject(parent), service_(std::move(service)), registry_(registry) {
    if (auto* object = dynamic_cast<McpService*>(service_.get())) {
        connect(object, &McpService::toolsUpdated, this,
                [this](const QString& server) { refresh(server); });
        connect(object, &McpService::serverDisconnected, this,
                [this](const QString& server) { disconnectServer(server); });
        connect(object, &McpService::serverError, this,
                [this](const QString& server, const QString&) { disconnectServer(server); });
    }
    for (const auto& server : service_->servers())
        if (service_->connectionState(server.name) == McpConnectionState::Connected)
            refresh(server.name);
}

McpToolProvider::~McpToolProvider() {
    for (const auto& server : registeredServers_)
        registry_.unregisterProvider(ToolSource::MCP, QStringLiteral("mcp:%1").arg(server));
}

bool McpToolProvider::refresh(const QString& serverName) {
    if (service_->connectionState(serverName) != McpConnectionState::Connected) {
        disconnectServer(serverName);
        return false;
    }
    const auto providerId = QStringLiteral("mcp:%1").arg(serverName);
    QList<IToolRegistry::Registration> next;
    QSet<QString> ids;
    for (auto tool : service_->tools(serverName)) {
        tool.serverName = serverName;
        if (tool.name.isEmpty()) {
            qWarning() << "MCP tool has an empty name on" << serverName;
            return false;
        }
        auto descriptor = McpToolCatalog::mcpToolToDescriptor(tool);
        if (ids.contains(descriptor.id)) {
            qWarning() << "Duplicate MCP tool" << descriptor.id;
            return false;
        }
        ids.insert(descriptor.id);
        const auto existing = registry_.findRegistration(descriptor.id);
        if (existing && (existing->descriptor.source != ToolSource::MCP ||
                         existing->descriptor.providerId != providerId)) {
            qWarning() << "MCP tool ID collision" << descriptor.id;
            return false;
        }
        next.append({std::move(descriptor),
                     std::make_shared<McpToolHandler>(service_, serverName, tool.name,
                                                      tool.filesystemSemanticContract)});
    }
    if (!registry_.replaceProvider(ToolSource::MCP, providerId, std::move(next)))
        return false;
    registeredServers_.insert(serverName);
    return true;
}

void McpToolProvider::disconnectServer(const QString& serverName) {
    registry_.unregisterProvider(ToolSource::MCP, QStringLiteral("mcp:%1").arg(serverName));
    registeredServers_.remove(serverName);
}
} // namespace sentinel::core
