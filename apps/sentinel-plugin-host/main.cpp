// SPDX-License-Identifier: GPL-3.0-or-later
#include "SentinelPluginSdk.h"
#include "sentinel/core/plugin/PluginHostProtocol.h"
#include "sentinel/core/plugin/PluginManifest.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QSet>
#include <cstdio>
#include <cstring>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <utility>
#include <QPluginLoader>
#include <QTimer>
#include <QHash>
#include <QEventLoop>
#include <QUuid>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

using namespace sentinel::plugin_sdk;
using sentinel::core::plugin::PluginHostMaxFrame;
using sentinel::core::plugin::PluginHostProtocolVersion;
using sentinel::core::plugin::pluginHostFrame;
using sentinel::core::plugin::parsePluginHostFrame;
using sentinel::core::plugin::checkVersionRequirement;
static_assert(NativeAbiVersion == sentinel::core::plugin::NativePluginAbiVersion);

namespace {
thread_local QString executingInvocation;
class HostContext final : public IPluginContext {
public:
    QString coreVersion() const override { return QStringLiteral("1.0.0"); }
    QString pluginDataDir() const override { return QDir::currentPath(); }
    bool hasPermission(const QString& permission) const override { return permissions.contains(permission); }
    void logMessage(const QString&, const QString&) override {} // Never forward plugin text into diagnostics.
    QJsonObject pluginConfig() const override { return {}; }
    bool credential(const QString& credentialId, QString* value) const override {
        const auto invocation = invocationId();
        if (!value || invocation.isEmpty() || !requestCredential)
            return false;
        if (executingInvocation != invocation) {
            QMutexLocker lock(&invocationMutex);
            credentialFailure = QStringLiteral("PluginInvocationExpired");
            return false;
        }
        if (credentialId.isEmpty() || credentialId.size() > 128) {
            QMutexLocker lock(&invocationMutex);
            credentialFailure = QStringLiteral("PluginCredentialNotDeclared");
            return false;
        }
        auto response = requestCredential(invocation, credentialId);
        if (!response.value(QStringLiteral("ok")).toBool()) {
            QMutexLocker lock(&invocationMutex);
            credentialFailure = response.value(QStringLiteral("category")).toString(
                QStringLiteral("PluginCredentialUnavailable"));
            return false;
        }
        if (!response.value(QStringLiteral("secret")).isString()) {
            QMutexLocker lock(&invocationMutex);
            credentialFailure = QStringLiteral("PluginProtocolError");
            return false;
        }
        *value = response.value(QStringLiteral("secret")).toString();
        response = {};
        return true;
    }
    QString invocationId() const { QMutexLocker lock(&invocationMutex); return activeInvocation; }
    QString takeCredentialFailure() {
        QMutexLocker lock(&invocationMutex);
        return std::exchange(credentialFailure, QString());
    }
    void beginInvocation(const QString& id) {
        QMutexLocker lock(&invocationMutex);
        credentialFailure.clear(); activeInvocation = id;
    }
    void endInvocation(const QString& id) {
        QMutexLocker lock(&invocationMutex);
        if (activeInvocation == id) activeInvocation.clear();
    }
    bool registerTool(const PluginToolDescriptor& descriptor, IPluginTool* tool) override {
        if (!accepting || !tool || descriptor.id.isEmpty() || tools.contains(descriptor.id)) {
            registrationFailed = true; return false;
        }
        if (descriptor.inputSchema.value(QStringLiteral("type")) != QStringLiteral("object") ||
            descriptor.inputSchema.value(QStringLiteral("additionalProperties")) != false) {
            registrationFailed = true; return false;
        }
        if (descriptor.risk != QStringLiteral("low") && descriptor.risk != QStringLiteral("medium") &&
            descriptor.risk != QStringLiteral("high")) {
            registrationFailed = true; return false;
        }
        tools.insert(descriptor.id, tool);
        descriptors.append(descriptor);
        return true;
    }
    bool accepting = false;
    bool registrationFailed = false;
    QSet<QString> permissions;
    QHash<QString, IPluginTool*> tools;
    QList<PluginToolDescriptor> descriptors;
    QString pluginId;
    mutable QMutex invocationMutex;
    QString activeInvocation;
    mutable QString credentialFailure;
    std::function<QJsonObject(const QString&, const QString&)> requestCredential;
};

QJsonObject descriptorJson(const PluginToolDescriptor& descriptor) {
    return {{QStringLiteral("id"), descriptor.id}, {QStringLiteral("name"), descriptor.name},
        {QStringLiteral("description"), descriptor.description}, {QStringLiteral("category"), descriptor.category},
        {QStringLiteral("risk"), descriptor.risk}, {QStringLiteral("schema"), descriptor.inputSchema},
        {QStringLiteral("requirements"), descriptor.authorizationRequirements}};
}

class Host final : public QObject {
public:
    Host() {
        reader = std::thread([this] {
            char line[PluginHostMaxFrame + 2];
            while (std::fgets(line, sizeof(line), stdin)) {
                QByteArray frame(line);
                std::memset(line, 0, sizeof(line));
                QMetaObject::invokeMethod(this, [this, frame = std::move(frame)]() mutable {
                    readFrame(frame);
                    frame.fill('\0');
                }, Qt::QueuedConnection);
            }
            QMetaObject::invokeMethod(qApp, &QCoreApplication::quit, Qt::QueuedConnection);
        });
        reader.detach();
    }
private:
    QJsonObject credential(const QString& invocationId, const QString& credentialId) {
        if (QThread::currentThread() != thread()) {
            QJsonObject value;
            QMetaObject::invokeMethod(this, [this, &value, invocationId, credentialId] {
                value = credential(invocationId, credentialId);
            }, Qt::BlockingQueuedConnection);
            return value;
        }
        const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QEventLoop loop;
        QJsonObject response;
        waiting.insert(id, [&response, &loop](const QJsonObject& value) {
            response = value; loop.quit();
        });
        send({{QStringLiteral("op"), QStringLiteral("hostRequest")},
              {QStringLiteral("id"), id},
              {QStringLiteral("invocationId"), invocationId},
              {QStringLiteral("pluginId"), context ? context->pluginId : QString()},
              {QStringLiteral("kind"), QStringLiteral("credential")},
              {QStringLiteral("credentialId"), credentialId},
              {QStringLiteral("purpose"), QStringLiteral("tool-execution")}});
        QTimer::singleShot(10000, &loop, &QEventLoop::quit);
        if (response.isEmpty()) loop.exec();
        waiting.remove(id);
        if (response.isEmpty()) return {{QStringLiteral("ok"), false},
            {QStringLiteral("category"), QStringLiteral("PluginTimeout")}};
        return response;
    }
    void send(const QJsonObject& value) {
        auto frame = value;
        frame.insert(QStringLiteral("protocol"), PluginHostProtocolVersion);
        auto bytes = pluginHostFrame(frame);
        frame = {};
        QMutexLocker lock(&sendMutex);
        std::fwrite(bytes.constData(), 1, size_t(bytes.size()), stdout);
        std::fflush(stdout);
        bytes.fill('\0');
    }
    void reply(const QJsonObject& request, bool ok, const QString& category = {}, QJsonObject fields = {}) {
        fields.insert(QStringLiteral("id"), request.value(QStringLiteral("id")));
        fields.insert(QStringLiteral("ok"), ok);
        if (!ok) fields.insert(QStringLiteral("category"), category);
        send(fields);
    }
    void readFrame(const QByteArray& line) {
        QJsonObject request;
        if (!parsePluginHostFrame(line.trimmed(), &request) || !request.value(QStringLiteral("id")).isString() ||
            !request.value(QStringLiteral("op")).isString()) { qApp->quit(); return; }
        handle(request);
    }
    void handle(const QJsonObject& request) {
        const auto op = request.value(QStringLiteral("op")).toString();
        if (op == QStringLiteral("hostResponse")) {
            const auto id = request.value(QStringLiteral("id")).toString();
            if (waiting.contains(id)) waiting.take(id)(request);
            return;
        }
        if (!waiting.isEmpty()) {
            reply(request, false, QStringLiteral("PluginProtocolError"));
            return;
        }
        if (op == QStringLiteral("health")) { reply(request, true, {},
            {{QStringLiteral("loaded"), bool(plugin)},
             {QStringLiteral("abi"), NativeAbiVersion},
             {QStringLiteral("protocolVersion"), PluginHostProtocolVersion}}); return; }
        if (op == QStringLiteral("shutdown")) {
            if (plugin) { try { plugin->stop(); plugin->shutdown(); } catch (...) {} }
            started = false; plugin = nullptr; context.reset(); loader.unload();
            reply(request, true); QTimer::singleShot(0, qApp, &QCoreApplication::quit); return;
        }
        if (op == QStringLiteral("load")) {
            if (plugin) { reply(request, false, QStringLiteral("PluginProtocolError")); return; }
            const auto path = request.value(QStringLiteral("path")).toString();
            const auto expectedId = request.value(QStringLiteral("pluginId")).toString();
            if (!QFileInfo(path).isFile() || !QFileInfo(path).isAbsolute() || expectedId.isEmpty() ||
                request.value(QStringLiteral("abi")).toInt(-1) != NativeAbiVersion) {
                reply(request, false, QStringLiteral("PluginIncompatible")); return;
            }
            loader.setFileName(path);
            if (!loader.load()) { reply(request, false, QStringLiteral("PluginLoadFailure")); return; }
            plugin = qobject_cast<ISentinelPlugin*>(loader.instance());
            if (!plugin || plugin->pluginId() != expectedId ||
                !checkVersionRequirement(QStringLiteral("1.0.0"), plugin->requiredCoreVersion())) {
                plugin = nullptr; loader.unload(); reply(request, false, QStringLiteral("PluginIncompatible")); return;
            }
            context = std::make_shared<HostContext>();
            context->pluginId = expectedId;
            context->requestCredential = [this](const QString& invocationId, const QString& credentialId) {
                return credential(invocationId, credentialId);
            };
            for (const auto& permission : request.value(QStringLiteral("permissions")).toArray())
                if (permission.isString()) context->permissions.insert(permission.toString());
            context->accepting = true;
            bool initialized = false;
            try { initialized = plugin->initialize(context.get()); } catch (...) {}
            context->accepting = false;
            if (!initialized || context->registrationFailed) { plugin->shutdown(); plugin = nullptr; context.reset(); loader.unload();
                reply(request, false, QStringLiteral("PluginLoadFailure")); return; }
            QJsonArray tools;
            for (const auto& descriptor : context->descriptors) tools.append(descriptorJson(descriptor));
            reply(request, true, {}, {{QStringLiteral("tools"), tools}});
            return;
        }
        if (op == QStringLiteral("enumerate")) {
            if (!plugin || !context) { reply(request, false, QStringLiteral("PluginHostUnavailable")); return; }
            QJsonArray tools;
            for (const auto& descriptor : context->descriptors) tools.append(descriptorJson(descriptor));
            reply(request, true, {}, {{QStringLiteral("tools"), tools}});
            return;
        }
        if (op == QStringLiteral("start")) {
            bool ok = false;
            try { if (plugin) ok = plugin->start(); } catch (...) {}
            started = ok;
            reply(request, ok, QStringLiteral("PluginLoadFailure")); return;
        }
        if (op == QStringLiteral("stop")) {
            if (plugin) { try { plugin->stop(); } catch (...) {} }
            started = false;
            reply(request, true); return;
        }
        if (op == QStringLiteral("unload")) {
            if (plugin) { try { plugin->stop(); plugin->shutdown(); } catch (...) {} }
            started = false; plugin = nullptr; context.reset(); loader.unload(); reply(request, true); return;
        }
        if (op == QStringLiteral("invoke")) {
            if (!plugin || !context || !started) { reply(request, false, QStringLiteral("PluginHostUnavailable")); return; }
            if (!context->invocationId().isEmpty()) {
                reply(request, false, QStringLiteral("PluginHostUnavailable")); return;
            }
            const auto toolId = request.value(QStringLiteral("toolId")).toString();
            auto* handler = context->tools.value(toolId);
            if (!handler || !request.value(QStringLiteral("arguments")).isObject()) {
                reply(request, false, QStringLiteral("PluginProtocolError")); return;
            }
            const auto id = request.value(QStringLiteral("id")).toString();
            auto invocationContext = context;
            invocationContext->beginInvocation(id);
            executingInvocation = id;
            try {
                const auto result = handler->execute({id, toolId,
                    request.value(QStringLiteral("arguments")).toObject()});
                const auto credentialFailure = invocationContext->takeCredentialFailure();
                invocationContext->endInvocation(id);
                executingInvocation.clear();
                send({{QStringLiteral("id"), id},
                    {QStringLiteral("ok"), credentialFailure.isEmpty() && result.ok},
                    {QStringLiteral("category"), !credentialFailure.isEmpty() ? credentialFailure :
                        result.ok ? QString() : QStringLiteral("PluginLoadFailure")},
                    {QStringLiteral("summary"), result.summary.left(65536)}});
            } catch (...) { invocationContext->endInvocation(id); executingInvocation.clear();
                reply(request, false, QStringLiteral("PluginLoadFailure")); }
            return;
        }
        reply(request, false, QStringLiteral("PluginProtocolError"));
    }
    std::thread reader;
    QPluginLoader loader;
    ISentinelPlugin* plugin = nullptr;
    bool started = false;
    std::shared_ptr<HostContext> context;
    QHash<QString, std::function<void(const QJsonObject&)>> waiting;
    QMutex sendMutex;
};
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().size() != 1) return 2;
    Host host;
    return app.exec();
}
