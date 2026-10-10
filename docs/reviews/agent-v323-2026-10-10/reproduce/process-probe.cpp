// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScopeGuard>

#include "sentinel/core/chat/SQLiteChatHistoryStore.h"
#include "sentinel/core/memory/InMemoryStore.h"
#include "sentinel/core/runtime/AlarmStore.h"
#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include "sentinel/core/runtime/IFileSystemService.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/runtime/ToolExecutionGateway.h"
#include "sentinel/core/security/ResourceAuthorizationResolver.h"
#include "sentinel/core/security/StaticSandboxPolicy.h"

using namespace sentinel::core;
namespace {

ToolInvocationPlan approvedPlan(const QString& toolId,
                                const QList<ToolInvocationArgument>& arguments,
                                ToolRiskLevel risk = ToolRiskLevel::Low) {
    ToolInvocationPlan plan;
    plan.status = ToolInvocationPlanStatus::Planned;
    plan.summary = QStringLiteral("test plan");
    plan.invocations.append(PlannedToolInvocation{toolId,
                                                  toolId,
                                                  QStringLiteral("test"),
                                                  QStringLiteral("test"),
                                                  risk,
                                                  ToolExecutionMode::Local,
                                                  arguments,
                                                  {}});
    return plan;
}

ToolInvocationArgument intArgument(const QString& id, int value) {
    ToolInvocationArgument argument{id, QString::number(value)};
    argument.jsonValue = QJsonValue(value);
    return argument;
}

ToolExecutionResult runTool(RealToolExecutor& executor, const QString& toolId,
                            const QList<ToolInvocationArgument>& arguments,
                            const QStringList& knownToolIds,
                            const QString& workspace = QDir::currentPath(),
                            QObject* callbackContext = nullptr, const QString& sessionId = {}) {
    Q_UNUSED(knownToolIds)
    InMemoryToolRegistry registry;
    if (!BuiltInToolProvider::registerTools(registry, executor))
        return {ToolExecutionStatus::Failed, QStringLiteral("Built-in tools did not register.")};

    ToolExecutionGateway gateway(&registry);
    ToolInvocationPlan plan = approvedPlan(toolId, arguments);

    const auto validation = gateway.validatePlan(plan);
    if (validation.status != ToolExecutionStatus::Succeeded) {
        if (qEnvironmentVariableIsSet("SENTINEL_TEST_TRACE")) {
            QStringList ids;
            for (const auto& tool : registry.listTools())
                ids.append(tool.id);
            qWarning().noquote() << toolId << "validate"
                                 << static_cast<int>(validation.status) << validation.summary
                                 << "| registered:" << ids.join(QLatin1Char(','));
        }
        return validation;
    }

    // Mirror AgentLoop: capture the authoritative descriptor, then resolve
    // and authorize resources before the gateway hands the call to a handler.
    for (auto& invocation : plan.invocations) {
        const auto registration = registry.findRegistration(invocation.toolId);
        if (registration)
            invocation.descriptorSnapshot =
                std::make_shared<const ToolDescriptor>(registration->descriptor);
    }

    // Resolve then authorize resources before the gateway
    // hands the invocation to the registered handler. A security denial maps to
    // Blocked; every other resource failure maps to InvalidArguments, matching
    // the labels AgentLoop reports for a blocked step.
    const auto resourceFailure = [](const QString& reason, FileSystemFailure failure) {
        const bool security = failure == FileSystemFailure::PermissionDenied ||
                              failure == FileSystemFailure::ResourceChanged ||
                              failure == FileSystemFailure::SymlinkEscape ||
                              failure == FileSystemFailure::UnsafeParent ||
                              failure == FileSystemFailure::SecurityBoundaryViolation;
        return ToolExecutionResult{security ? ToolExecutionStatus::Blocked
                                            : ToolExecutionStatus::InvalidArguments,
                                   reason};
    };
    for (auto& invocation : plan.invocations) {
        if (!invocation.descriptorSnapshot)
            continue;
        auto resolved = ResourceAuthorizationResolver::resolve(*invocation.descriptorSnapshot,
                                                               invocation, workspace, nullptr);
        if (!resolved.ok())
            return resourceFailure(resolved.reason, resolved.failure);
        invocation.resourceSnapshot = std::make_shared<const ResourceAuthorizationSnapshot>(
            std::move(resolved.snapshot));
        auto authorized = ResourceAuthorizationResolver::authorize(*invocation.resourceSnapshot,
                                                                   nullptr, nullptr, {});
        if (!authorized.ok())
            return resourceFailure(authorized.reason, authorized.failure);
        invocation.resourceSnapshot = std::make_shared<const ResourceAuthorizationSnapshot>(
            std::move(authorized.snapshot));
    }

    ToolExecutionResult result;
    bool completed = false;
    const ApprovalDecision approval{ApprovalStatus::Approved, QStringLiteral("test"), {}};
    // Built-in descriptors currently use the default metadata capability.
    const StaticSandboxPolicy sandboxPolicy{QSet<QString>{QStringLiteral("tool.metadata.read")}};
    // The returned cancellation handle owns the active process execution, so it
    // must stay alive until the completion callback fires.
    const IToolExecutor::Cancel active = gateway.executeAsync(
        ToolExecutionRequest{
            plan,
            approval,
            sandboxPolicy.evaluate(plan, approval),
            knownToolIds,
            callbackContext,
        },
        executor, sessionId, {}, {}, [&](ToolExecutionResult value) {
            result = std::move(value);
            completed = true;
        });

    // Process-backed tools finish from a QProcess terminal signal; immediate
    // tools already completed inline above.
    const bool waited = QTest::qWaitFor([&completed] { return completed; }, 30000);
    if (qEnvironmentVariableIsSet("SENTINEL_TEST_TRACE")) {
        QString trace = result.summary;
        trace.replace(QLatin1Char('\n'), QLatin1Char('|'));
        qWarning().noquote() << toolId << static_cast<int>(result.status) << "completed:"
                             << completed << "waited:" << waited << trace;
    }
    if (!completed)
        return {ToolExecutionStatus::Blocked,
                QStringLiteral("Tool execution did not complete in time.")};
    return result;
}

} // namespace


int main(int argc, char** argv) { QGuiApplication app(argc, argv); QTemporaryDir dir; RealToolExecutor executor; auto r = runTool(executor, QStringLiteral("run-command"), {{QStringLiteral("command"), QString::fromLocal8Bit(argv[1])}}, {}, dir.path()); qInfo().noquote() << QJsonDocument(QJsonObject{{"status", static_cast<int>(r.status)}, {"summary", r.summary}, {"category", static_cast<int>(r.failureCategory)}}).toJson(QJsonDocument::Compact); return 0; }
