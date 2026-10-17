// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// P4-01 baseline: AgentExecutionScheduler plus its shared Budget is the only
// existing admission/concurrency contract. These cases pin admission,
// authorization independence, conflict serialization and cancellation budget
// release before any pressure-aware policy exists. No second scheduler and no
// provisional resource policy is introduced here.

#include "sentinel/core/agent/AgentExecutionScheduler.h"
#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/runtime/IToolHandler.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/ToolDescriptor.h"
#include "sentinel/core/runtime/ToolExecutionGateway.h"
#include "sentinel/core/runtime/ToolInvocationPlan.h"

#include <QtTest>

using sentinel::core::AccessMode;
using sentinel::core::AgentExecutionScheduler;
using sentinel::core::ApprovalStatus;
using sentinel::core::AuthorizationRequest;
using sentinel::core::AuthorizationResourceKind;
using sentinel::core::InMemoryToolRegistry;
using sentinel::core::IToolExecutor;
using sentinel::core::PlannedToolInvocation;
using sentinel::core::ResourceAuthorizationSnapshot;
using sentinel::core::SandboxStatus;
using sentinel::core::SecurityDomain;
using sentinel::core::ToolAuthorizationRequirement;
using sentinel::core::ToolDescriptor;
using sentinel::core::ToolExecutionGateway;
using sentinel::core::ToolExecutionRequest;
using sentinel::core::ToolExecutionResult;
using sentinel::core::ToolExecutionStatus;
using sentinel::core::ToolInvocationPlanStatus;

namespace {

ToolExecutionRequest authorizedRequest(const QString& toolId, const QString& resource,
                                       AccessMode access) {
    ToolExecutionRequest request;
    request.plan.status = ToolInvocationPlanStatus::Planned;
    PlannedToolInvocation invocation;
    invocation.toolId = toolId;
    invocation.toolName = toolId;
    auto descriptor = std::make_shared<ToolDescriptor>();
    descriptor->id = toolId;
    descriptor->name = toolId;
    descriptor->enabled = true;
    descriptor->parallelSafe = true;
    invocation.descriptorSnapshot = descriptor;
    auto snapshot = std::make_shared<ResourceAuthorizationSnapshot>();
    AuthorizationRequest authorization;
    authorization.domain = SecurityDomain::FileSystem;
    authorization.resource = resource;
    authorization.access = access;
    snapshot->requests = {authorization};
    snapshot->authorized = true;
    invocation.resourceSnapshot = snapshot;
    request.plan.invocations = {invocation};
    request.approval.status = ApprovalStatus::NotRequired;
    return request;
}

// Completion is held by the fixture instead of running inline, so a test can
// observe how many tool starts the scheduler admitted before any release.
class DeferredExecutor final : public IToolExecutor {
public:
    struct Pending {
        int id = 0;
        QString toolId;
        Completion completion;
    };

    mutable QList<Pending> pending;
    mutable int startedCount = 0;
    mutable int cancelCount = 0;
    mutable int nextId = 1;

    ToolExecutionResult execute(const ToolExecutionRequest&) const override {
        return {ToolExecutionStatus::Blocked,
                QStringLiteral("Deferred fixture only serves executeAsync.")};
    }

    Cancel executeAsync(const ToolExecutionRequest& request, const QString&, const QString&, Output,
                        Completion completion) const override {
        const int id = nextId++;
        ++startedCount;
        pending.append({id, request.plan.invocations.first().toolId, std::move(completion)});
        // Cooperative cancellation withdraws the in-flight fixture work; the
        // terminal result still belongs to the executor, never to cancel().
        return [this, id] {
            ++cancelCount;
            for (int i = 0; i < pending.size(); ++i) {
                if (pending.at(i).id == id) {
                    pending.removeAt(i);
                    break;
                }
            }
        };
    }

    void completeNext() {
        if (pending.isEmpty()) {
            return;
        }
        const auto item = pending.takeFirst();
        item.completion({ToolExecutionStatus::Succeeded, QStringLiteral("fixture success")});
    }
};

} // namespace

class ExecutionBudgetPolicyTest final : public QObject {
    Q_OBJECT
private slots:
    void sharedBudgetCapsConcurrentToolStarts();
    void cancelRestoresSharedBudgetForTheNextRun();
    void conflictingMutationsSerializeWhileReadOnlyRunsShare();
    void gatewayBlocksUnauthorizedResourcesBeforeExecution();
};

void ExecutionBudgetPolicyTest::sharedBudgetCapsConcurrentToolStarts() {
    const ToolExecutionGateway gateway;
    DeferredExecutor executor;
    auto budget = std::make_shared<AgentExecutionScheduler::Budget>(1);
    const auto scheduler = std::make_shared<AgentExecutionScheduler>(gateway, executor, 4, budget);

    int started = 0;
    int completed = 0;
    QList<ToolExecutionResult> results;
    scheduler->start(
        {authorizedRequest(QStringLiteral("fixture.one"), QStringLiteral("/tmp/one"),
                           AccessMode::Read),
         authorizedRequest(QStringLiteral("fixture.two"), QStringLiteral("/tmp/two"),
                           AccessMode::Read),
         authorizedRequest(QStringLiteral("fixture.three"), QStringLiteral("/tmp/three"),
                           AccessMode::Read)},
        QStringLiteral("session"), {QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")},
        [&](int) { ++started; }, {},
        [&](QList<ToolExecutionResult> value) {
            ++completed;
            results = std::move(value);
        });

    // One shared grant admits exactly one active tool regardless of the
    // scheduler's own parallel setting.
    QCOMPARE(executor.startedCount, 1);
    QCOMPARE(started, 1);
    QCOMPARE(budget->active.load(), 1);

    executor.completeNext();
    QCOMPARE(executor.startedCount, 2);
    QCOMPARE(budget->active.load(), 1);
    executor.completeNext();
    QCOMPARE(executor.startedCount, 3);
    QCOMPARE(budget->active.load(), 1);
    executor.completeNext();

    QCOMPARE(completed, 1);
    QCOMPARE(results.size(), 3);
    for (const auto& result : results) {
        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
    }
    QCOMPARE(budget->active.load(), 0);
    QVERIFY(executor.pending.isEmpty());
}

void ExecutionBudgetPolicyTest::cancelRestoresSharedBudgetForTheNextRun() {
    const ToolExecutionGateway gateway;
    DeferredExecutor executor;
    auto budget = std::make_shared<AgentExecutionScheduler::Budget>(1);
    bool foregroundCompleted = false;

    {
        const auto foreground =
            std::make_shared<AgentExecutionScheduler>(gateway, executor, 4, budget);
        foreground->start({authorizedRequest(QStringLiteral("fixture.a"), QStringLiteral("/tmp/a"),
                                             AccessMode::Write),
                           authorizedRequest(QStringLiteral("fixture.b"), QStringLiteral("/tmp/b"),
                                             AccessMode::Write)},
                          QStringLiteral("session"), {QStringLiteral("1"), QStringLiteral("2")}, {},
                          {},
                          [&](const QList<ToolExecutionResult>&) { foregroundCompleted = true; });

        QCOMPARE(executor.startedCount, 1);
        QCOMPARE(budget->active.load(), 1);

        foreground->cancel();
        // Cancellation releases every started-but-unfinished grant, so active
        // work is preserved without leaking admission capacity.
        QCOMPARE(budget->active.load(), 0);
        QCOMPARE(executor.cancelCount, 1);
        QVERIFY(!foregroundCompleted);
    }

    // The next run acquires the same shared budget immediately.
    const auto next = std::make_shared<AgentExecutionScheduler>(gateway, executor, 4, budget);
    bool nextCompleted = false;
    next->start({authorizedRequest(QStringLiteral("fixture.c"), QStringLiteral("/tmp/c"),
                                   AccessMode::Read)},
                QStringLiteral("session"), {QStringLiteral("1")}, {}, {},
                [&](QList<ToolExecutionResult> value) {
                    nextCompleted = true;
                    QCOMPARE(value.size(), 1);
                    QCOMPARE(value.first().status, ToolExecutionStatus::Succeeded);
                });

    QCOMPARE(executor.startedCount, 2);
    QCOMPARE(budget->active.load(), 1);
    executor.completeNext();
    QVERIFY(nextCompleted);
    QCOMPARE(budget->active.load(), 0);
}

void ExecutionBudgetPolicyTest::conflictingMutationsSerializeWhileReadOnlyRunsShare() {
    const ToolExecutionGateway gateway;

    {
        DeferredExecutor executor;
        auto budget = std::make_shared<AgentExecutionScheduler::Budget>(4);
        const auto scheduler =
            std::make_shared<AgentExecutionScheduler>(gateway, executor, 4, budget);
        int completed = 0;
        scheduler->start(
            {authorizedRequest(QStringLiteral("fixture.write.one"),
                               QStringLiteral("/workspace/shared"), AccessMode::Write),
             authorizedRequest(QStringLiteral("fixture.write.two"),
                               QStringLiteral("/workspace/shared"), AccessMode::Write)},
            QStringLiteral("session"), {QStringLiteral("1"), QStringLiteral("2")}, {}, {},
            [&](const QList<ToolExecutionResult>&) { ++completed; });

        // Overlapping mutable resources never run concurrently.
        QCOMPARE(executor.startedCount, 1);
        executor.completeNext();
        QCOMPARE(executor.startedCount, 2);
        executor.completeNext();
        QCOMPARE(completed, 1);
        QCOMPARE(budget->active.load(), 0);
    }

    {
        DeferredExecutor executor;
        auto budget = std::make_shared<AgentExecutionScheduler::Budget>(4);
        const auto scheduler =
            std::make_shared<AgentExecutionScheduler>(gateway, executor, 4, budget);
        int completed = 0;
        scheduler->start({authorizedRequest(QStringLiteral("fixture.read.one"),
                                            QStringLiteral("/workspace/shared"), AccessMode::Read),
                          authorizedRequest(QStringLiteral("fixture.read.two"),
                                            QStringLiteral("/workspace/shared"), AccessMode::Read)},
                         QStringLiteral("session"), {QStringLiteral("1"), QStringLiteral("2")}, {},
                         {}, [&](const QList<ToolExecutionResult>&) { ++completed; });

        // Overlapping read-only resources may share the budget concurrently.
        QCOMPARE(executor.startedCount, 2);
        executor.completeNext();
        QCOMPARE(completed, 0);
        executor.completeNext();
        QCOMPARE(completed, 1);
        QCOMPARE(budget->active.load(), 0);
    }
}

void ExecutionBudgetPolicyTest::gatewayBlocksUnauthorizedResourcesBeforeExecution() {
    // Authorization invariance at the execution boundary (AC-P4-03): a resource
    // snapshot that is missing or not authorized is refused before any handler
    // or executor runs. No resource policy may weaken that gate.
    class Handler final : public sentinel::core::IToolHandler {
    public:
        int calls = 0;
        IToolExecutor::Cancel execute(const ToolExecutionRequest&, const QString&, const QString&,
                                      IToolExecutor::Output,
                                      IToolExecutor::Completion completion) override {
            ++calls;
            completion({ToolExecutionStatus::Succeeded, QStringLiteral("fixture execution")});
            return {};
        }
    };
    InMemoryToolRegistry registry;
    auto handler = std::make_shared<Handler>();
    ToolDescriptor descriptor;
    descriptor.id = QStringLiteral("p4-01.fixture.write");
    descriptor.name = QStringLiteral("Fixture Write");
    descriptor.providerId = QStringLiteral("test");
    descriptor.inputSchema = {{QStringLiteral("type"), QStringLiteral("object")},
                              {QStringLiteral("additionalProperties"), false}};
    ToolAuthorizationRequirement requirement;
    requirement.domain = SecurityDomain::FileSystem;
    requirement.access = AccessMode::Write;
    requirement.resourceKind = AuthorizationResourceKind::FileSystemPath;
    requirement.staticResource = QStringLiteral("/workspace/fixture");
    descriptor.authorizationRequirements = {requirement};
    QVERIFY(registry.registerTool({descriptor, handler}));

    ToolExecutionGateway gateway(&registry);
    auto request =
        authorizedRequest(descriptor.id, QStringLiteral("/workspace/fixture"), AccessMode::Write);
    request.approval.status = ApprovalStatus::Approved;
    request.sandbox.status = SandboxStatus::Allowed;
    request.plan.invocations.first().descriptorSnapshot =
        std::make_shared<ToolDescriptor>(descriptor);

    ToolExecutionResult result;
    DeferredExecutor executor;
    auto run = [&](const ResourceAuthorizationSnapshot* snapshot) {
        request.plan.invocations.first().resourceSnapshot =
            snapshot ? std::make_shared<ResourceAuthorizationSnapshot>(*snapshot) : nullptr;
        result = {};
        gateway.executeAsync(request, executor, QStringLiteral("session"), QStringLiteral("call"),
                             {}, [&](auto value) { result = std::move(value); });
    };

    run(nullptr);
    QCOMPARE(result.status, ToolExecutionStatus::Blocked);
    QCOMPARE(handler->calls, 0);
    QCOMPARE(executor.startedCount, 0);

    ResourceAuthorizationSnapshot unauthorized;
    unauthorized.authorized = false;
    run(&unauthorized);
    QCOMPARE(result.status, ToolExecutionStatus::Blocked);
    QCOMPARE(handler->calls, 0);
    QCOMPARE(executor.startedCount, 0);
}

QTEST_MAIN(ExecutionBudgetPolicyTest)

#include "test_execution_budget_policy.moc"
