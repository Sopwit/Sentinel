// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/runtime/ProcessExecutor.h"
#include "sentinel/core/runtime/ToolInvocationPlan.h"
#include <QJsonObject>
#include <QObject>
#include <QHash>
#include <functional>

namespace sentinel::core::plugin {

// One sandboxed process per native plugin. All methods run on the owning Qt thread.
class PluginHostSession final : public QObject {
    Q_OBJECT
public:
    explicit PluginHostSession(QObject* parent = nullptr);
    ~PluginHostSession() override;
    void setPluginIdentity(QString pluginId);
    bool start(const QString& pluginBinary, const QString& dataDirectory);
    QJsonObject call(const QString& operation, QJsonObject fields = {}, int timeoutMs = 10000);
    QString invoke(const QString& toolId, const QJsonObject& arguments,
                   const PlannedToolInvocation& authorization,
                   std::function<void(QJsonObject)> completion);
    // Reverse IPC is accepted only while the named invocation is pending.
    using HostRequestHandler = std::function<QJsonObject(const QString& invocationId,
                                                          const QString& toolId,
                                                          const QJsonObject& request,
                                                          const PlannedToolInvocation& authorization,
                                                          PluginHostSession& session)>;
    void setHostRequestHandler(HostRequestHandler handler);
    bool trackBrokerOperation(const QString& invocationId, const QString& requestId,
                              const QString& capability,
                              std::function<void()> cancel);
    void finishBrokerOperation(const QString& invocationId, const QString& requestId);
    bool invocationActive(const QString& invocationId) const;
    void cancel(const QString& requestId);
    void shutdown();
    bool isRunning() const;
    QString failureCategory() const { return failureCategory_; }
    QString failureDetail() const { return failureDetail_; }

signals:
    void failed(const QString& category);

private:
    void receive(const QByteArray& bytes);
    void fail(const QString& category, const QString& detail = {});
    ProcessExecutor process_;
    QString processId_;
    QByteArray incoming_;
    QHash<QString, std::function<void(QJsonObject)>> pending_;
    QHash<QString, QString> invocations_;
    QHash<QString, PlannedToolInvocation> authorizations_;
    QHash<QString, QString> brokerFailures_;
    struct BrokerOperation {
        QString pluginId;
        QString generationId;
        QString invocationId;
        QString requestId;
        QString capability;
        std::function<void()> cancel;
    };
    QHash<QString, QHash<QString, BrokerOperation>> brokerOperations_;
    QString pluginId_;
    QString generationId_;
    void cancelBrokerOperations(const QString& invocationId);
    HostRequestHandler hostRequestHandler_;
    QString failureCategory_;
    QString failureDetail_;
    bool stopping_ = false;
};
} // namespace sentinel::core::plugin
