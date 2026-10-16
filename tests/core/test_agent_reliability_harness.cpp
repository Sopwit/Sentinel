// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// P1-01 deterministic Agent reliability harness (AC-P1-01).
//
// Executes the COR-01..COR-18 corpus from docs/roadmaps/ACCEPTANCE_MATRIX.md
// against the existing AgentRuntime / AgentLoop / ToolExecutionGateway seams
// with injected planners, chat providers, approval policies and tool handlers.
// Runtime path reuses AgentRuntime with a fixture metadata seam and the loop
// path reuses AgentLoop directly; no second runtime is introduced.
//
// Determinism rules:
// - No arbitrary sleeps. Synchronization uses QSemaphore barriers with bounded
//   deadlock guards and QtTest QTRY_* polling on terminal conditions.
// - Every fixture uses a disposable QTemporaryDir workspace with synthetic
//   data and a trusted baseline manifest.
// - An independent verifier outside any model/tool authority compares a fresh
//   filesystem manifest against an exact per-scenario allowlist.
// - Tool side effects are prescribed exactly per scenario on the fixture
//   handler; any invocation outside the prescription is counted as an
//   unauthorized effect and fails the case.

#include <QtTest>

#include "sentinel/core/agent/AgentEvent.h"
#include "sentinel/core/agent/AgentLoop.h"
#include "sentinel/core/agent/AgentRuntime.h"
#include "sentinel/core/agent/ClaimGroundingResolver.h"
#include "sentinel/core/agent/IAgentRuntime.h"
#include "sentinel/core/agent/LlmAgentRuntime.h"
#include "sentinel/core/agent/ObservationPolicy.h"
#include "sentinel/core/interfaces/IChatProvider.h"
#include "sentinel/core/model/ModelRouting.h"
#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/ToolApproval.h"
#include "sentinel/core/security/IApprovalPolicy.h"
#include "sentinel/core/security/StaticSandboxPolicy.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QTimer>
#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

using namespace sentinel::core;

namespace {

constexpr int kBarrierTimeoutMs = 20000;

// --------------------------------------------------------------------------
// Independent verification (outside any model/tool authority)
// --------------------------------------------------------------------------

QString sha256OfFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QStringLiteral("<unreadable>");
    return QString::fromUtf8(
        QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex());
}

struct WorkspaceState {
    QHash<QString, QString> files; // relative path -> content digest
};

WorkspaceState stateOf(const QString& root) {
    WorkspaceState state;
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        state.files.insert(QDir(root).relativeFilePath(path), sha256OfFile(path));
    }
    return state;
}

// Walks the disposable workspace directly from the filesystem (never through
// tool output) and diffs it against an exact expected allowlist.
class WorkspaceVerifier {
public:
    explicit WorkspaceVerifier(const QString& root)
        : root_(QFileInfo(root).canonicalFilePath()), baseline_(stateOf(root_)) {}

    struct Expected {
        QStringList changed; // relative paths whose content may differ
        QStringList added;   // relative paths that may be created
        QStringList removed; // relative paths that may be deleted
    };

    struct Diff {
        QStringList changed, added, removed;
        bool empty() const {
            return changed.isEmpty() && added.isEmpty() && removed.isEmpty();
        }
        QString describe() const {
            return QStringLiteral("changed=[%1] added=[%2] removed=[%3]")
                .arg(changed.join(QLatin1Char(',')), added.join(QLatin1Char(',')),
                     removed.join(QLatin1Char(',')));
        }
    };

    Diff delta() const {
        return deltaFrom(baseline_);
    }

    bool untouched() const {
        return delta().empty();
    }

    // True only for the exact expected set: nothing changed/created/removed
    // beyond the allowlist entries.
    bool matches(const Expected& expected, QString* actual = nullptr) const {
        const Diff diff = delta();
        if (actual)
            *actual = diff.describe();
        const auto sameSet = [](const QStringList& left, const QStringList& right) {
            return QSet<QString>(left.cbegin(), left.cend()) ==
                   QSet<QString>(right.cbegin(), right.cend());
        };
        return sameSet(diff.changed, expected.changed) && sameSet(diff.added, expected.added) &&
               sameSet(diff.removed, expected.removed);
    }

    QString fileContent(const QString& relativePath) const {
        QFile file(root_ + QLatin1Char('/') + relativePath);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return QString::fromUtf8(file.readAll());
    }

    const QString& root() const {
        return root_;
    }

private:
    Diff deltaFrom(const WorkspaceState& before) const {
        const auto after = stateOf(root_);
        Diff diff;
        for (auto it = after.files.cbegin(); it != after.files.cend(); ++it) {
            if (!before.files.contains(it.key()))
                diff.added.append(it.key());
            else if (before.files.value(it.key()) != it.value())
                diff.changed.append(it.key());
        }
        for (auto it = before.files.cbegin(); it != before.files.cend(); ++it)
            if (!after.files.contains(it.key()))
                diff.removed.append(it.key());
        std::sort(diff.changed.begin(), diff.changed.end());
        std::sort(diff.added.begin(), diff.added.end());
        std::sort(diff.removed.begin(), diff.removed.end());
        return diff;
    }

    QString root_;
    WorkspaceState baseline_;
};

// Terminal-state audit over the typed runtime event history.
class TerminalAudit {
public:
    void scan(const QList<AgentEvent>& events) {
        for (const auto& event : events) {
            if (event.type == AgentEventType::AgentCompleted)
                ++completed;
            else if (event.type == AgentEventType::AgentFailed)
                ++failed;
            else if (event.type == AgentEventType::AgentCancelled)
                ++cancelled;
        }
    }
    int total() const {
        return completed + failed + cancelled;
    }
    bool singleTerminal() const {
        return total() == 1;
    }
    QString describe() const {
        return QStringLiteral("completed=%1 failed=%2 cancelled=%3")
            .arg(QString::number(completed), QString::number(failed), QString::number(cancelled));
    }
    int completed = 0, failed = 0, cancelled = 0;
};

// --------------------------------------------------------------------------
// Deterministic planner fixture
// --------------------------------------------------------------------------

class ScriptedPlanner final : public IAgentStepPlanner {
public:
    QVector<AgentStepDecision> decisions;
    bool repeatLast = false;
    bool canonicalFinal = false;
    // Canonical finals must be derived from the same intent the loop
    // classified; otherwise the recompute-fence rejects the representation.
    ObservationIntent canonicalIntent;
    std::function<void(int)> onCall;
    mutable int calls = 0;

    void resetPlanningIndex() {
        indexBase_ = calls;
    }

    void setEvidence(const QList<EvidenceRecord>& value) override {
        evidence_ = value;
    }

    AgentStepDecision nextStep(const QString& goal,
                               const QList<AgentStepRecord>& history) const override {
        Q_UNUSED(goal);
        Q_UNUSED(history);
        const int index = (calls++) - indexBase_;
        if (onCall)
            onCall(index);
        if (index >= 0 && index < decisions.size()) {
            auto decision = decisions.at(index);
            if (canonicalFinal && decision.kind == AgentStepDecision::Kind::FinalAnswer) {
                if (auto answer = ClaimGroundingResolver::filesystemFinalAnswer(
                        canonicalIntent, evidence_, decision.answer))
                    decision.answer = *answer;
            }
            return decision;
        }
        if (repeatLast && !decisions.isEmpty()) {
            auto decision = decisions.last();
            if (canonicalFinal && decision.kind == AgentStepDecision::Kind::FinalAnswer) {
                if (auto answer = ClaimGroundingResolver::filesystemFinalAnswer(
                        canonicalIntent, evidence_, decision.answer))
                    decision.answer = *answer;
            }
            return decision;
        }
        AgentStepDecision fallback;
        fallback.kind = AgentStepDecision::Kind::FinalAnswer;
        fallback.answer = QStringLiteral("fallback final answer");
        return fallback;
    }

private:
    mutable int indexBase_ = 0;
    mutable QList<EvidenceRecord> evidence_;
};

AgentStepDecision toolDecision(const QString& toolId, ToolRiskLevel risk = ToolRiskLevel::Low) {
    AgentStepDecision decision;
    decision.kind = AgentStepDecision::Kind::ToolCall;
    decision.toolId = toolId;
    decision.toolName = toolId;
    decision.riskLevel = risk;
    decision.thought = QStringLiteral("use %1").arg(toolId);
    return decision;
}

void appendArg(AgentStepDecision& decision, const QString& id, const QString& value) {
    decision.arguments.append(ToolInvocationArgument{id, value});
}

AgentStepDecision finalDecision(const QString& answer) {
    AgentStepDecision decision;
    decision.kind = AgentStepDecision::Kind::FinalAnswer;
    decision.answer = answer;
    return decision;
}

AgentStepDecision giveUpDecision(const QString& reason) {
    AgentStepDecision decision;
    decision.kind = AgentStepDecision::Kind::GiveUp;
    decision.reason = reason;
    return decision;
}

class FixedIntentPolicy final : public IObservationIntentPolicy {
public:
    ObservationIntent intent;
    ObservationIntent classify(const QString&, const QString&,
                               const QList<ToolDescriptor>&) const override {
        return intent;
    }
};

class CallbackIntentPolicy final : public IObservationIntentPolicy {
public:
    std::function<void()> onClassify;
    ObservationIntent classify(const QString&, const QString&,
                               const QList<ToolDescriptor>&) const override {
        if (onClassify)
            onClassify();
        return {};
    }
};

class FixtureApprovalPolicy final : public IApprovalPolicy {
public:
    enum class Mode { ApproveAll, RequireApproval, DenyAll };
    Mode mode = Mode::ApproveAll;
    mutable QStringList requestedTools;

    ApprovalDecision evaluate(const ToolInvocationPlan& plan) const override {
        requestedTools.append(plan.invocations.isEmpty() ? QString()
                                                         : plan.invocations.first().toolId);
        switch (mode) {
        case Mode::ApproveAll:
            return {ApprovalStatus::Approved, QStringLiteral("Fixture pre-approval"), {}};
        case Mode::DenyAll:
            return {ApprovalStatus::Denied, QStringLiteral("Fixture denies every action"), {}};
        case Mode::RequireApproval:
            return {
                ApprovalStatus::RequiresApproval, QStringLiteral("Fixture approval required"), {}};
        }
        return {ApprovalStatus::RequiresApproval, QStringLiteral("unreachable"), {}};
    }
};

// Executor seam that must never be consulted once the registry carries
// handler registrations; any use indicates a lost registry seam.
class NoOpExecutor final : public IToolExecutor {
public:
    mutable int calls = 0;
    ToolExecutionResult execute(const ToolExecutionRequest&) const override {
        ++calls;
        return {ToolExecutionStatus::Blocked, QStringLiteral("Executor seam used unexpectedly.")};
    }
};

// --------------------------------------------------------------------------
// Deterministic tool handler with exact side-effect prescriptions
// --------------------------------------------------------------------------

class FixtureHandler final : public IToolHandler {
public:
    QString root;
    QSet<QString> allowedCommands;         // exact authorized run-command strings
    bool testReceiptFails = false;         // deterministic failing-test receipt
    QHash<QString, QString> allowedWrites; // rel path -> exact canonical content
    QString unavailableReasonOverride;     // registered capability missing
    bool blockSlowBuild = false;           // cooperative blocking command flag
    QSemaphore slowBuildStarted{0};
    QSemaphore slowBuildRelease{0};

    QHash<QString, int> counts; // receipt issuance per toolId
    QStringList order;          // execution order receipts
    std::atomic_bool sealed{false};
    int postSealCalls = 0;
    int unauthorizedCalls = 0;

    QString unavailableReason() const override {
        return unavailableReasonOverride;
    }

    void seal() {
        sealed.store(true);
    }

    IToolExecutor::Cancel execute(const ToolExecutionRequest& request, const QString& sessionId,
                                  const QString& toolCallId, IToolExecutor::Output,
                                  IToolExecutor::Completion completion) override {
        Q_UNUSED(sessionId);
        Q_UNUSED(toolCallId);
        if (sealed.load()) {
            ++postSealCalls;
            completion({ToolExecutionStatus::Blocked, QStringLiteral("Fixture sealed.")});
            return IToolExecutor::Cancel{};
        }
        const auto& invocation = request.plan.invocations.first();
        const auto toolId = invocation.toolId;
        const auto arg = [&invocation](const QString& id) {
            for (const auto& item : invocation.arguments)
                if (item.id == id)
                    return item.value;
            return QString{};
        };

        if (invocation.arguments.isEmpty())
            ++unauthorizedCalls; // no-argument calls are never prescribed

        if (toolId == QLatin1String("run-command")) {
            const QString command = arg(QStringLiteral("command"));
            if (!allowedCommands.contains(command)) {
                ++unauthorizedCalls;
                completion({ToolExecutionStatus::Blocked,
                            QStringLiteral("Command not in fixture allowlist: %1").arg(command)});
                return IToolExecutor::Cancel{};
            }
            order.append(QStringLiteral("run-command(%1)").arg(command));
        } else {
            order.append(QStringLiteral("%1(%2)").arg(toolId, arg(QStringLiteral("path"))));
        }

        auto observation = std::make_shared<StructuredObservation>();
        observation->data = {{"complete", true}, {"truncated", false}, {"cancelled", false}};
        ToolExecutionResult result{ToolExecutionStatus::Succeeded, QStringLiteral("fixture ok")};
        counts[toolId]++;

        if (toolId == QLatin1String("write-file") || toolId == QLatin1String("edit-file")) {
            const QString path = resolved(arg(QStringLiteral("path")));
            const QString content = arg(QStringLiteral("content"));
            if (!authorizedWrite(path, content)) {
                ++unauthorizedCalls;
                completion({ToolExecutionStatus::Blocked,
                            QStringLiteral("Fixture write outside authorized set: %1").arg(path)});
                return IToolExecutor::Cancel{};
            }
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly) || file.write(content.toUtf8()) < 0) {
                completion({ToolExecutionStatus::Failed, QStringLiteral("fixture write failed")});
                return IToolExecutor::Cancel{};
            }
            result.mutations.append({path});
        } else if (toolId == QLatin1String("read-file") || toolId == QLatin1String("stat-path")) {
            const QString path = resolved(arg(QStringLiteral("path")));
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) {
                completion({ToolExecutionStatus::Failed, QStringLiteral("fixture read failed")});
                return IToolExecutor::Cancel{};
            }
            observation->kind = StructuredObservationKind::FileContent;
            observation->data.insert("path", QFileInfo(path).canonicalFilePath());
            observation->data.insert("content", QString::fromUtf8(file.readAll()));
        } else if (toolId == QLatin1String("list-directory")) {
            observation->kind = StructuredObservationKind::DirectoryListing;
            QJsonArray listing;
            QDirIterator it(resolved(arg(QStringLiteral("path"))), QDir::NoDotAndDotDot);
            while (it.hasNext())
                listing.append(QFileInfo(it.next()).fileName());
            observation->data.insert("entries", listing);
            observation->data.insert("includeHidden", false);
        } else if (toolId == QLatin1String("git-diff") || toolId == QLatin1String("git-status")) {
            observation->kind = StructuredObservationKind::Generic;
            observation->data.insert("stdout", QStringLiteral("- return a - b\n+ return a + b"));
            observation->data.insert("exitCode", 0);
        } else if (toolId == QLatin1String("run-command")) {
            observation->kind = StructuredObservationKind::Generic;
            observation->data.insert("exitCode", testReceiptFails ? 1 : 0);
            observation->data.insert("stdout", testReceiptFails
                                                   ? QStringLiteral("TEST_FAILED")
                                                   : QStringLiteral("ALL_TESTS_PASSED"));
            observation->data.insert("stderr", QString{});
            if (testReceiptFails)
                result.status = ToolExecutionStatus::Failed;
        } else if (toolId == QLatin1String("delete-file") || toolId == QLatin1String("move-file")) {
            ++unauthorizedCalls;
            completion({ToolExecutionStatus::Blocked,
                        QStringLiteral("Fixture forbids destructive receipts.")});
            return IToolExecutor::Cancel{};
        } else {
            ++unauthorizedCalls;
            completion({ToolExecutionStatus::Blocked,
                        QStringLiteral("Tool not prescribed by fixture: %1").arg(toolId)});
            return IToolExecutor::Cancel{};
        }

        if (blockSlowBuild && toolId == QLatin1String("run-command") &&
            arg(QStringLiteral("command")) == QStringLiteral("echo slow-build")) {
            // Slow cooperative receipt: write the declared partial artifact,
            // then wait on the test barrier until the test released it.
            const QString artifact = QDir(root).filePath(QStringLiteral("build/partial.log"));
            QFile file(artifact);
            if (QDir(root).mkpath(QStringLiteral("build")) && file.open(QIODevice::WriteOnly) &&
                file.write("partial fixture output\n") > 0) {
                file.close();
                result.mutations.append({artifact});
                observation->kind = StructuredObservationKind::Generic;
                observation->data.insert("exitCode", 0);
                slowBuildStarted.release();
                if (!slowBuildRelease.tryAcquire(1, kBarrierTimeoutMs)) {
                    completion(
                        {ToolExecutionStatus::Failed, QStringLiteral("fixture barrier timed out")});
                    return IToolExecutor::Cancel{};
                }
                result.summary = QStringLiteral("fixture slow build released");
            } else {
                completion({ToolExecutionStatus::Failed,
                            QStringLiteral("fixture could not write the declared artifact")});
                return IToolExecutor::Cancel{};
            }
        } else if (toolId == QLatin1String("run-command") &&
                   arg(QStringLiteral("command")) == QStringLiteral("echo slow-build")) {
            if (blockSlowBuild) {
                ++unauthorizedCalls;
                completion({ToolExecutionStatus::Blocked,
                            QStringLiteral("slow-build executed without the blocking context")});
                return IToolExecutor::Cancel{};
            }
        }

        result.structuredObservation = observation;
        completion(result);
        return IToolExecutor::Cancel{};
    }

private:
    QString resolved(const QString& raw) const {
        return QDir::cleanPath(
            QDir(root).absoluteFilePath(raw.isEmpty() ? QStringLiteral(".") : raw));
    }

    bool authorizedWrite(const QString& absolutePath, const QString& content) const {
        const auto canonical = QFileInfo(absolutePath).canonicalFilePath();
        const auto target = canonical.isEmpty() ? absolutePath : canonical;
        const auto base = QFileInfo(root).canonicalFilePath();
        if (target != base && !target.startsWith(base + QLatin1Char('/')))
            return false;
        const QString relative = QDir(base).relativeFilePath(target);
        if (!allowedWrites.contains(relative))
            return false;
        return content == allowedWrites.value(relative);
    }
};

bool registerFixtureTools(InMemoryToolRegistry& registry, IToolHandler& handler,
                          const QStringList& ids,
                          const QHash<QString, ToolDescriptor>& overrides = {}) {
    const auto descriptors = BuiltInToolProvider::descriptors();
    for (const auto& id : ids) {
        ToolDescriptor descriptor;
        if (auto overrideIt = overrides.find(id); overrideIt != overrides.end()) {
            descriptor = *overrideIt;
        } else {
            const auto found =
                std::find_if(descriptors.cbegin(), descriptors.cend(),
                             [&id](const ToolDescriptor& item) { return item.id == id; });
            if (found == descriptors.cend())
                return false;
            descriptor = *found;
        }
        if (!registry.registerTool(IToolRegistry::Registration{
                descriptor, std::shared_ptr<IToolHandler>(&handler, [](IToolHandler*) {})}))
            return false;
    }
    return true;
}

// --------------------------------------------------------------------------
// Deterministic chat provider fixture (injected model layer)
// --------------------------------------------------------------------------

class FaultProvider final : public IChatProvider {
public:
    // Observation classification requests (deadlineMs 60000) deterministically
    // yield an empty typed requirement set. Planner requests (deadlineMs
    // 120000) consume the scripted queue; barrier semantics apply only to
    // planner requests.
    QString name() const override {
        return QStringLiteral("fixture-fault-provider");
    }
    ChatProviderStatus status() const override {
        return ChatProviderStatus::Ready;
    }
    ChatProviderReply sendMessage(const QString& message) override {
        Q_UNUSED(message);
        return {true, QStringLiteral("{\"requirements\":[]}"), {}};
    }

    ChatProviderReply sendRequest(const QString& message,
                                  const ChatRequestOptions& options) override {
        Q_UNUSED(message);
        if (options.deadlineMs == 60000)
            return {true, QStringLiteral("{\"requirements\":[]}"), {}};
        ++plannerRequests;
        if (barrierAllRequests) {
            arrived.release();
            if (!release.tryAcquire(1, kBarrierTimeoutMs))
                barrierTimedOut = true;
            if (options.cancellationToken && options.cancellationToken->load()) {
                ++cancelledReports;
                ChatProviderReply cancelledReply;
                cancelledReply.success = false;
                cancelledReply.category = ChatProviderErrorCategory::Cancelled;
                cancelledReply.lifecycle = ChatRequestLifecycle::Cancelled;
                cancelledReply.errorMessage = QStringLiteral("Generation cancelled by user.");
                return cancelledReply;
            }
        }
        if (!plannerReplies.isEmpty())
            return plannerReplies.takeFirst();
        if (faultAfterQueue)
            return *faultAfterQueue;
        ChatProviderReply missing;
        missing.success = false;
        missing.category = ChatProviderErrorCategory::ProviderFailure;
        missing.errorMessage = QStringLiteral("Fixture provider queue exhausted.");
        return missing;
    }

    QList<ChatProviderReply> plannerReplies;
    std::optional<ChatProviderReply> faultAfterQueue;
    bool barrierAllRequests = false;
    QSemaphore arrived{0};
    QSemaphore release{0};
    int plannerRequests = 0;
    int cancelledReports = 0;
    bool barrierTimedOut = false;
};

ModelBinding fixtureBinding() {
    ModelBinding binding;
    binding.providerId = QStringLiteral("fixture-fault-provider");
    binding.modelId = QStringLiteral("fixture-model");
    return binding;
}

std::unique_ptr<LlmAgentRuntime> makeLlmPlanner(FaultProvider& provider,
                                                const QList<ToolDescriptor>& tools) {
    auto planner = std::make_unique<LlmAgentRuntime>(tools, &provider);
    planner->bindModel(fixtureBinding(),
                       std::shared_ptr<IChatProvider>(&provider, [](IChatProvider*) {}));
    return planner;
}

QList<ToolDescriptor> fixtureDescriptors(const QStringList& ids,
                                         const QHash<QString, ToolDescriptor>& overrides = {}) {
    const auto descriptors = BuiltInToolProvider::descriptors();
    QList<ToolDescriptor> result;
    for (const auto& id : ids) {
        if (auto overrideIt = overrides.find(id); overrideIt != overrides.end()) {
            result.append(*overrideIt);
            continue;
        }
        const auto found =
            std::find_if(descriptors.cbegin(), descriptors.cend(),
                         [&id](const ToolDescriptor& item) { return item.id == id; });
        Q_ASSERT(found != descriptors.cend());
        result.append(*found);
    }
    return result;
}

// --------------------------------------------------------------------------
// Shared fixture bundles
// --------------------------------------------------------------------------

void seedFile(const QDir& root, const QString& relative, const QString& content) {
    const QString path = root.filePath(relative);
    QVERIFY2(QDir().mkpath(QFileInfo(path).absolutePath()), "fixture mkdir failed");
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), "fixture seed open failed");
    QVERIFY2(file.write(content.toUtf8()) >= 0, "fixture seed write failed");
    file.close();
}

QString canonicalRoot(const QTemporaryDir& dir) {
    return QFileInfo(dir.path()).canonicalFilePath();
}

const char* loopPhaseName(AgentLoopPhase phase) {
    switch (phase) {
    case AgentLoopPhase::Idle:
        return "Idle";
    case AgentLoopPhase::Running:
        return "Running";
    case AgentLoopPhase::Cancelling:
        return "Cancelling";
    case AgentLoopPhase::AwaitingApproval:
        return "AwaitingApproval";
    case AgentLoopPhase::Completed:
        return "Completed";
    case AgentLoopPhase::Cancelled:
        return "Cancelled";
    case AgentLoopPhase::Failed:
        return "Failed";
    case AgentLoopPhase::Stuck:
        return "Stuck";
    }
    return "Unknown";
}

const char* terminalReasonName(AgentTerminalReason reason) {
    switch (reason) {
    case AgentTerminalReason::None:
        return "None";
    case AgentTerminalReason::Completed:
        return "Completed";
    case AgentTerminalReason::Cancelled:
        return "Cancelled";
    case AgentTerminalReason::ApprovalDenied:
        return "ApprovalDenied";
    case AgentTerminalReason::ProviderFailure:
        return "ProviderFailure";
    case AgentTerminalReason::ToolFailure:
        return "ToolFailure";
    case AgentTerminalReason::CapabilityUnavailable:
        return "CapabilityUnavailable";
    case AgentTerminalReason::SecurityDenied:
        return "SecurityDenied";
    case AgentTerminalReason::IterationLimit:
        return "IterationLimit";
    case AgentTerminalReason::UnableToComplete:
        return "UnableToComplete";
    }
    return "Unknown";
}

// Fixture metadata runtime exposing exactly the fixture descriptors, so
// AgentRuntime reaches the compatibility-handler seam without starting a
// second execution engine.
class FixtureMetadataRuntime final : public IAgentRuntime {
public:
    explicit FixtureMetadataRuntime(QList<ToolDescriptor> tools) : tools_(std::move(tools)) {}
    QString name() const override {
        return QStringLiteral("fixture-metadata");
    }
    AgentStatus status() const override {
        return AgentStatus::Ready;
    }
    QList<AgentCapabilityDescriptor> capabilities() const override {
        return {};
    }
    QList<ToolDescriptor> availableTools() const override {
        return tools_;
    }
    ToolInvocationPlan plan(const AgentRequest&) const override {
        return {};
    }
    AgentResponse execute(const AgentRequest&) override {
        return {};
    }

private:
    QList<ToolDescriptor> tools_;
};

StaticSandboxPolicy permissiveSandbox() {
    return StaticSandboxPolicy(QSet<QString>{QStringLiteral("tool.metadata.read"),
                                             QStringLiteral("tool.risk.medium"),
                                             QStringLiteral("tool.risk.high")});
}

// Loop-level fixture bundle.
struct LoopFixture {
    ScriptedPlanner planner;
    FixtureHandler handler;
    FixtureApprovalPolicy approval;
    StaticSandboxPolicy sandbox = permissiveSandbox();
    NoOpExecutor executor;
    InMemoryToolRegistry registry;
    QStringList knownTools;

    void registerTools(const QStringList& ids,
                       const QHash<QString, ToolDescriptor>& overrides = {}) {
        QVERIFY2(registerFixtureTools(registry, handler, ids, overrides),
                 "fixture tool registration failed");
        knownTools = ids;
    }
};

// Runtime-level fixture bundle driving the AgentRuntime session API.
struct RuntimeFixture {
    FixtureHandler handler;
    FixtureApprovalPolicy approval;
    StaticSandboxPolicy sandbox = permissiveSandbox();
    NoOpExecutor executor;
    ScriptedPlanner planner;
    std::unique_ptr<AgentRuntime> runtime;
    QString session;
    QStringList toolIds;

    void create(const QStringList& toolIds_) {
        toolIds = toolIds_;
        runtime = std::make_unique<AgentRuntime>(
            std::make_unique<FixtureMetadataRuntime>(fixtureDescriptors(toolIds)), &planner,
            executor, approval, sandbox);
        // Replace each compatibility-handler registration with the
        // deterministic fixture handler.
        for (const auto& descriptor : fixtureDescriptors(toolIds)) {
            QVERIFY2(runtime->toolRegistry().unregisterTool(descriptor.id),
                     qPrintable(QStringLiteral("unregister failed for %1").arg(descriptor.id)));
            QVERIFY2(
                runtime->toolRegistry().registerTool(IToolRegistry::Registration{
                    descriptor, std::shared_ptr<IToolHandler>(&handler, [](IToolHandler*) {})}),
                qPrintable(QStringLiteral("register failed for %1").arg(descriptor.id)));
        }
        session = runtime->createSession();
        QVERIFY(!session.isEmpty());
    }

    void configure(const QString& workspaceRoot, bool autonomous = true,
                   const QStringList& availableTools = {}) {
        handler.root = workspaceRoot;
        AgentSessionOptions options;
        options.autonomousMode = autonomous;
        options.workspaceContext.rootPath = workspaceRoot;
        if (!availableTools.isEmpty()) {
            options.availableToolIds = availableTools;
            options.restrictAvailableTools = true;
        }
        runtime->configureSession(session, std::move(options));
    }
};

// Runs an AgentLoop synchronously or asynchronously with a bounded event-loop
// barrier; returns the terminal state in both modes. Uses no Qt assertion
// macros because this is a value-returning helper.
AgentLoopState runLoop(AgentLoop& loop, QObject* owner, const QString& sessionId, bool async,
                       const QString& goal) {
    AgentLoopState state;
    if (async) {
        int terminals = 0;
        QEventLoop pump;
        QTimer timeout;
        timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &pump, &QEventLoop::quit);
        loop.runAsync(goal, sessionId, owner, [&](const AgentLoopState& result) {
            state = result;
            ++terminals;
            pump.quit();
        });
        timeout.start(kBarrierTimeoutMs);
        pump.exec();
    } else {
        state = loop.run(goal, sessionId);
    }
    return state;
}

void configureLoopFixture(AgentLoop& loop, LoopFixture& fixture, const QString& root) {
    fixture.handler.root = root;
    loop.setToolRegistry(&fixture.registry);
    loop.setResourceScope(root);
    AgentContextInput::WorkspaceContext context;
    context.rootPath = root;
    loop.setWorkspaceContext(context);
}

QString terminalFailureDescription(const AgentLoopState& state) {
    return QStringLiteral("phase=%1 reason=%2 abort=%3")
        .arg(loopPhaseName(state.phase))
        .arg(terminalReasonName(state.terminalReason))
        .arg(state.abortReason);
}

} // namespace

// --------------------------------------------------------------------------
// Harness test object
// --------------------------------------------------------------------------

class AgentReliabilityHarnessTest final : public QObject {
    Q_OBJECT

private slots:
    // Harness self-check: the verification oracle must reject a planted effect
    // outside the allowlist (independence from tool outputs).
    void harnessVerifierRejectsSideEffectsOutsideAllowlist() {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a - b; }\n"));
        WorkspaceVerifier verifier(canonicalRoot(workspace));

        WorkspaceVerifier::Expected allowed;
        allowed.changed = {QStringLiteral("src/calc.cpp")};
        QString actual;
        // A placeholder success must be rejected while nothing changed yet.
        QVERIFY(!verifier.matches(allowed, &actual));

        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a + b; }\n"));
        QVERIFY(verifier.matches(allowed, &actual));

        seedFile(QDir(workspace.path()), QStringLiteral("build/rogue.log"), QStringLiteral("x"));
        seedFile(QDir(workspace.path()), QStringLiteral("tests/oracle.txt"),
                 QStringLiteral("tampered"));
        QVERIFY(!verifier.matches(allowed, &actual));
        QVERIFY(!verifier.untouched());
    }

    // ------------------------------------------------------------------
    // COR-01: read-only inspection. Grounded scoped claim completes; the
    // insufficient-evidence variant fails boundedly without fabrication.
    // ------------------------------------------------------------------
    void cor01_readOnlyInspection_data() {
        QTest::addColumn<bool>("async");
        QTest::addColumn<bool>("sufficient");
        for (bool async : {false, true})
            for (bool sufficient : {true, false}) {
                QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(
                    async ? QStringLiteral("async") : QStringLiteral("sync"),
                    sufficient ? QStringLiteral("grounded-claim")
                               : QStringLiteral("insufficient-evidence"))))
                    << async << sufficient;
            }
    }

    void cor01_readOnlyInspection() {
        QFETCH(bool, async);
        QFETCH(bool, sufficient);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("README.md"),
                 QStringLiteral("fixture synthetic repository"));
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a + b; }\n"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("read-file"), QStringLiteral("list-directory")});
        const auto readPath = root + QStringLiteral("/src/calc.cpp");

        auto policy = std::make_shared<FixedIntentPolicy>();
        ObservationRequirement readRequirement;
        readRequirement.domain = ObservationDomain::FileSystem;
        readRequirement.resourceHint = readPath;
        readRequirement.freshness = EvidenceFreshness::TurnScoped;
        readRequirement.purpose = ObservationPurpose::Inspect;
        readRequirement.toolId = QStringLiteral("read-file");
        // Both variants classify the same evidence requirement; the negative
        // variant never performs the required read.
        policy->intent.requirements = {readRequirement};

        ScriptedPlanner planner;
        auto list = toolDecision(QStringLiteral("list-directory"));
        appendArg(list, QStringLiteral("path"), root);
        auto read = toolDecision(QStringLiteral("read-file"));
        appendArg(read, QStringLiteral("path"), readPath);
        auto grounded = finalDecision(
            QStringLiteral("src/calc.cpp uses the addition operator per the fresh read receipt."));
        grounded.grounding = GroundingMode::Verified;
        grounded.groundingDeclared = true;
        planner.decisions = sufficient ? QVector<AgentStepDecision>{list, read, grounded}
                                       : QVector<AgentStepDecision>{list, grounded};
        planner.repeatLast = true;
        planner.canonicalFinal = sufficient;
        planner.canonicalIntent = policy->intent;

        AgentLoop::Config config;
        config.maxIterations = 3;
        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools, config);
        configureLoopFixture(loop, fixture, root);
        loop.setObservationIntentPolicy(policy);
        loop.setToolCallIdProvider([](int) { return QStringLiteral("observed"); });
        WorkspaceVerifier verifier(root);

        const auto state =
            runLoop(loop, this, QStringLiteral("cor01"), async,
                    QStringLiteral("Inspect src/calc.cpp read-only and report its operator"));
        QVERIFY(verifier.untouched());

        if (sufficient) {
            QCOMPARE(state.phase, AgentLoopPhase::Completed);
            QVERIFY(state.finalAnswer.contains(QStringLiteral("return a + b")));
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("read-file"), 0), 1);
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("list-directory"), 0), 1);
            QVERIFY(std::any_of(state.evidence.cbegin(), state.evidence.cend(),
                                [](const EvidenceRecord& item) {
                                    return item.outcome == EvidenceOutcome::Verified;
                                }));
        } else {
            QVERIFY2(state.phase == AgentLoopPhase::Failed,
                     qPrintable(terminalFailureDescription(state)));
            QVERIFY2(state.finalAnswer.isEmpty(), qPrintable(state.finalAnswer));
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("read-file"), 0), 0);
        }
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
    }

    // ------------------------------------------------------------------
    // COR-01 negative: hidden-scope claim built from a non-recursive listing
    // is rejected; the universal negative claim never completes.
    // ------------------------------------------------------------------
    void cor01_hiddenScopeClaimNegative() {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("visible.txt"), QStringLiteral("fixture"));
        seedFile(QDir(workspace.path()), QStringLiteral(".hidden.txt"), QStringLiteral("hidden"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("list-directory"), QStringLiteral("read-file")});

        auto policy = std::make_shared<FixedIntentPolicy>();
        ObservationRequirement hiddenClaim;
        hiddenClaim.domain = ObservationDomain::FileSystem;
        hiddenClaim.resourceHint = root;
        hiddenClaim.claimType = ClaimType::HiddenEntriesExist;
        hiddenClaim.claimId = QStringLiteral("hidden");
        hiddenClaim.toolId = QStringLiteral("list-directory");
        policy->intent.requirements = {hiddenClaim};

        ScriptedPlanner planner;
        auto list = toolDecision(QStringLiteral("list-directory"));
        appendArg(list, QStringLiteral("path"), root);
        auto claim = finalDecision(QStringLiteral("No hidden files exist."));
        claim.grounding = GroundingMode::Verified;
        claim.groundingDeclared = true;
        planner.decisions = {list, claim};
        planner.repeatLast = true;

        AgentLoop::Config config;
        config.maxIterations = 3;
        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools, config);
        configureLoopFixture(loop, fixture, root);
        loop.setObservationIntentPolicy(policy);
        loop.setToolCallIdProvider([](int) { return QStringLiteral("observed"); });
        WorkspaceVerifier verifier(root);

        const auto state = runLoop(loop, this, QStringLiteral("cor01-hidden"), false,
                                   QStringLiteral("Are there no hidden entries here?"));
        QVERIFY2(state.phase == AgentLoopPhase::Failed,
                 qPrintable(terminalFailureDescription(state)));
        QVERIFY(state.finalAnswer.isEmpty());
        QCOMPARE(fixture.handler.counts.value(QStringLiteral("read-file"), 0), 0);
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        QVERIFY(verifier.untouched());
    }

    // ------------------------------------------------------------------
    // COR-02: single-file C++ repair with a planted operator bug; the
    // verifier requires exactly one changed file and only authorized
    // build/test commands. A claimed-verified final over a failing receipt
    // must fail.
    // ------------------------------------------------------------------
    void cor02_singleFileRepair_data() {
        QTest::addColumn<bool>("async");
        QTest::addColumn<bool>("grounded");
        for (bool async : {false, true})
            for (bool grounded : {true, false}) {
                QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(
                    async ? QStringLiteral("async") : QStringLiteral("sync"),
                    grounded ? QStringLiteral("repair-ok") : QStringLiteral("fabricated-claim"))))
                    << async << grounded;
            }
    }

    void cor02_singleFileRepair() {
        QFETCH(bool, async);
        QFETCH(bool, grounded);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a - b; }\n"));
        seedFile(QDir(workspace.path()), QStringLiteral("tests/oracle.txt"),
                 QStringLiteral("trusted fixture baseline"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("read-file"), QStringLiteral("write-file"),
                               QStringLiteral("run-command"), QStringLiteral("git-diff")});
        fixture.handler.allowedCommands = {QStringLiteral("cmake --build build"),
                                           QStringLiteral("ctest --test-dir build")};
        fixture.handler.testReceiptFails = !grounded;
        fixture.handler.allowedWrites.insert(
            QStringLiteral("src/calc.cpp"),
            QStringLiteral("int add(int a,int b){ return a + b; }\n"));
        const auto calcPath = root + QStringLiteral("/src/calc.cpp");

        auto policy = std::make_shared<FixedIntentPolicy>();
        ObservationRequirement readRequirement;
        readRequirement.domain = ObservationDomain::FileSystem;
        readRequirement.resourceHint = calcPath;
        readRequirement.freshness = EvidenceFreshness::TurnScoped;
        readRequirement.purpose = ObservationPurpose::Inspect;
        readRequirement.toolId = QStringLiteral("read-file");
        policy->intent.requirements.append(readRequirement);
        ObservationRequirement diffRequirement;
        diffRequirement.domain = ObservationDomain::Workspace;
        diffRequirement.resourceHint = root;
        diffRequirement.freshness = EvidenceFreshness::Live;
        diffRequirement.purpose = ObservationPurpose::Inspect;
        diffRequirement.toolId = QStringLiteral("git-diff");
        policy->intent.requirements.append(diffRequirement);
        ObservationRequirement buildRequirement;
        buildRequirement.domain = ObservationDomain::ProcessExecution;
        buildRequirement.resourceHint = QStringLiteral("cmake --build build");
        buildRequirement.freshness = EvidenceFreshness::TurnScoped;
        buildRequirement.purpose = ObservationPurpose::Operate;
        policy->intent.requirements.append(buildRequirement);
        ObservationRequirement testRequirement;
        testRequirement.domain = ObservationDomain::ProcessExecution;
        testRequirement.resourceHint = QStringLiteral("ctest --test-dir build");
        testRequirement.freshness = EvidenceFreshness::TurnScoped;
        testRequirement.purpose = ObservationPurpose::Operate;
        policy->intent.requirements.append(testRequirement);

        ScriptedPlanner planner;
        auto initialRead = toolDecision(QStringLiteral("read-file"));
        appendArg(initialRead, QStringLiteral("path"), calcPath);
        auto write = toolDecision(QStringLiteral("write-file"));
        appendArg(write, QStringLiteral("path"), calcPath);
        appendArg(write, QStringLiteral("content"),
                  QStringLiteral("int add(int a,int b){ return a + b; }\n"));
        auto freshRead = toolDecision(QStringLiteral("read-file"));
        appendArg(freshRead, QStringLiteral("path"), calcPath);
        auto build = toolDecision(QStringLiteral("run-command"));
        appendArg(build, QStringLiteral("command"), QStringLiteral("cmake --build build"));
        auto test = toolDecision(QStringLiteral("run-command"));
        appendArg(test, QStringLiteral("command"), QStringLiteral("ctest --test-dir build"));
        auto diff = toolDecision(QStringLiteral("git-diff"));
        appendArg(diff, QStringLiteral("path"), root);
        planner.decisions = {initialRead, write, freshRead, build, test, diff};
        auto verified = finalDecision(QStringLiteral(
            "Replaced the subtraction, tests return ALL_TESTS_PASSED, and the git-diff receipt "
            "shows - return a - b\\n+ return a + b."));
        verified.grounding = GroundingMode::Verified;
        verified.groundingDeclared = true;
        planner.decisions.append(verified);
        planner.repeatLast = true;
        planner.canonicalFinal = grounded;
        planner.canonicalIntent = policy->intent;

        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools);
        configureLoopFixture(loop, fixture, root);
        loop.setObservationIntentPolicy(policy);
        loop.setToolCallIdProvider([](int) { return QStringLiteral("observed"); });
        WorkspaceVerifier verifier(root);

        const auto state = runLoop(loop, this, QStringLiteral("cor02"), async,
                                   QStringLiteral("Fix the planted operator bug in src/calc.cpp"));

        WorkspaceVerifier::Expected expected;
        expected.changed = {QStringLiteral("src/calc.cpp")};
        QString actual;
        if (grounded) {
            QCOMPARE(state.phase, AgentLoopPhase::Completed);
            QVERIFY2(verifier.matches(expected, &actual), qPrintable(actual));
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("write-file"), 0), 1);
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("run-command"), 0), 2);
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("git-diff"), 0), 1);
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("read-file"), 0), 3);
            QVERIFY(state.finalAnswer.contains(QStringLiteral("return a + b")));
            QVERIFY(std::any_of(state.evidence.cbegin(), state.evidence.cend(),
                                [](const EvidenceRecord& item) {
                                    return item.scope == EvidenceScope::Operation &&
                                           item.outcome == EvidenceOutcome::Verified;
                                }));
        } else {
            // The receipt failed; a claimed-verified final is rejected and the
            // run terminates boundedly (Failed or repeat-fenced Stuck) instead
            // of fabricating a completion.
            QVERIFY2(state.phase == AgentLoopPhase::Failed || state.phase == AgentLoopPhase::Stuck,
                     qPrintable(terminalFailureDescription(state)));
            QVERIFY(state.finalAnswer.isEmpty());
            QVERIFY2(verifier.matches(expected, &actual), qPrintable(actual));
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("run-command"), 0), 2);
        }
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        QCOMPARE(verifier.fileContent(QStringLiteral("src/calc.cpp")),
                 QStringLiteral("int add(int a,int b){ return a + b; }\n"));
    }

    // ------------------------------------------------------------------
    // COR-03: dependent multi-file repair; exactly two changes, written in
    // interface-then-implementation order; trusted fixture files untouched.
    // ------------------------------------------------------------------
    void cor03_multiFileDependentRepair() {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("include/calc.h"),
                 QStringLiteral("int add(int,int);\n"));
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("#include \"calc.h\"\nint add(int a,int b){ return a - b; }\n"));
        seedFile(QDir(workspace.path()), QStringLiteral("tests/test_calc.txt"),
                 QStringLiteral("trusted fixture baseline"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("write-file"), QStringLiteral("run-command")});
        fixture.handler.allowedCommands = {QStringLiteral("cmake --build build")};
        fixture.handler.allowedWrites.insert(QStringLiteral("include/calc.h"),
                                             QStringLiteral("int add(int,int,int);\n"));
        fixture.handler.allowedWrites.insert(
            QStringLiteral("src/calc.cpp"),
            QStringLiteral(
                "#include \"calc.h\"\nint add(int a,int b,int c){ return a + b + c; }\n"));

        ScriptedPlanner planner;
        auto writeHeader = toolDecision(QStringLiteral("write-file"));
        appendArg(writeHeader, QStringLiteral("path"), root + QStringLiteral("/include/calc.h"));
        appendArg(writeHeader, QStringLiteral("content"),
                  QStringLiteral("int add(int,int,int);\n"));
        auto writeSource = toolDecision(QStringLiteral("write-file"));
        appendArg(writeSource, QStringLiteral("path"), root + QStringLiteral("/src/calc.cpp"));
        appendArg(writeSource, QStringLiteral("content"),
                  QStringLiteral(
                      "#include \"calc.h\"\nint add(int a,int b,int c){ return a + b + c; }\n"));
        auto build = toolDecision(QStringLiteral("run-command"));
        appendArg(build, QStringLiteral("command"), QStringLiteral("cmake --build build"));
        planner.decisions = {writeHeader, writeSource, build};
        auto finish = finalDecision(QStringLiteral(
            "Both dependent files updated and the build receipt completed ALL_TESTS_PASSED."));
        finish.grounding = GroundingMode::Context;
        finish.groundingDeclared = true;
        planner.decisions.append(finish);
        planner.repeatLast = false;

        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools);
        configureLoopFixture(loop, fixture, root);
        WorkspaceVerifier verifier(root);

        const auto state = runLoop(loop, this, QStringLiteral("cor03"), false,
                                   QStringLiteral("Update the dependent header and source"));
        QCOMPARE(state.phase, AgentLoopPhase::Completed);
        WorkspaceVerifier::Expected expected;
        expected.changed = {QStringLiteral("include/calc.h"), QStringLiteral("src/calc.cpp")};
        QString actual;
        QVERIFY2(verifier.matches(expected, &actual), qPrintable(actual));
        QCOMPARE(verifier.fileContent(QStringLiteral("tests/test_calc.txt")),
                 QStringLiteral("trusted fixture baseline"));
        QCOMPARE(fixture.handler.counts.value(QStringLiteral("write-file"), 0), 2);
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        const int headerIndex = fixture.handler.order.indexOf(
            QStringLiteral("write-file(%1)").arg(root + QStringLiteral("/include/calc.h")));
        const int sourceIndex = fixture.handler.order.indexOf(
            QStringLiteral("write-file(%1)").arg(root + QStringLiteral("/src/calc.cpp")));
        QVERIFY(headerIndex >= 0 && sourceIndex > headerIndex);
        QCOMPARE(verifier.fileContent(QStringLiteral("include/calc.h")),
                 QStringLiteral("int add(int,int,int);\n"));
        QCOMPARE(verifier.fileContent(QStringLiteral("src/calc.cpp")),
                 QStringLiteral(
                     "#include \"calc.h\"\nint add(int a,int b,int c){ return a + b + c; }\n"));
    }

    // ------------------------------------------------------------------
    // COR-04: fixed build/test invocation success vs a deliberately
    // unfixable failing receipt that must terminate without looping.
    // ------------------------------------------------------------------
    void cor04_buildTestInvocation_data() {
        QTest::addColumn<bool>("testFails");
        QTest::newRow("authorized-invocation-succeeds") << false;
        QTest::newRow("deliberately-unfixable") << true;
    }

    void cor04_buildTestInvocation() {
        QFETCH(bool, testFails);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a - b; }\n"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("write-file"), QStringLiteral("run-command")});
        fixture.handler.allowedCommands = {QStringLiteral("cmake --build build"),
                                           QStringLiteral("ctest --test-dir build")};
        fixture.handler.testReceiptFails = testFails;
        fixture.handler.allowedWrites.insert(
            QStringLiteral("src/calc.cpp"),
            QStringLiteral("int add(int a,int b){ return a + b; }\n"));

        ScriptedPlanner planner;
        auto write = toolDecision(QStringLiteral("write-file"));
        appendArg(write, QStringLiteral("path"), root + QStringLiteral("/src/calc.cpp"));
        appendArg(write, QStringLiteral("content"),
                  QStringLiteral("int add(int a,int b){ return a + b; }\n"));
        auto build = toolDecision(QStringLiteral("run-command"));
        appendArg(build, QStringLiteral("command"), QStringLiteral("cmake --build build"));
        auto test = toolDecision(QStringLiteral("run-command"));
        appendArg(test, QStringLiteral("command"), QStringLiteral("ctest --test-dir build"));
        planner.decisions = {write, build, test};
        if (testFails) {
            planner.decisions.append(giveUpDecision(
                QStringLiteral("Fixture test oracle keeps failing; stopping bounded.")));
        } else {
            auto finish = finalDecision(
                QStringLiteral("Authorized command sequence completed with ALL_TESTS_PASSED."));
            finish.grounding = GroundingMode::Context;
            finish.groundingDeclared = true;
            planner.decisions.append(finish);
        }
        planner.repeatLast = false;

        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools);
        configureLoopFixture(loop, fixture, root);
        WorkspaceVerifier verifier(root);

        const auto state = runLoop(loop, this, QStringLiteral("cor04"), false,
                                   testFails ? QStringLiteral("Fix and test (unfixable variant)")
                                             : QStringLiteral("Fix and run the authorized tests"));
        if (testFails) {
            QCOMPARE(state.phase, AgentLoopPhase::Failed);
            QCOMPARE(state.terminalReason, AgentTerminalReason::UnableToComplete);
            QVERIFY(state.finalAnswer.isEmpty());
            // The failing test ran exactly once: no retry-until-green.
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("run-command"), 0), 2);
        } else {
            QCOMPARE(state.phase, AgentLoopPhase::Completed);
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("run-command"), 0), 2);
        }
        WorkspaceVerifier::Expected expected;
        expected.changed = {QStringLiteral("src/calc.cpp")};
        QString actual;
        QVERIFY2(verifier.matches(expected, &actual), qPrintable(actual));
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
    }

    // ------------------------------------------------------------------
    // COR-05: unknown tool / missing required argument / wrong JSON type /
    // extra property. The handler must never execute; workspace untouched.
    // ------------------------------------------------------------------
    void cor05_invalidToolInvocation_data() {
        QTest::addColumn<QString>("variant");
        QTest::newRow("unknown-tool") << QStringLiteral("unknown-tool");
        QTest::newRow("missing-required") << QStringLiteral("missing-required");
        QTest::newRow("wrong-type") << QStringLiteral("wrong-type");
        QTest::newRow("extra-property") << QStringLiteral("extra-property");
    }

    void cor05_invalidToolInvocation() {
        QFETCH(QString, variant);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a + b; }\n"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("read-file"), QStringLiteral("write-file")});

        ScriptedPlanner planner;
        if (variant == QStringLiteral("unknown-tool")) {
            planner.decisions = {toolDecision(QStringLiteral("warp-drive")),
                                 toolDecision(QStringLiteral("warp-drive"))};
        } else if (variant == QStringLiteral("missing-required")) {
            planner.decisions = {toolDecision(QStringLiteral("read-file")),
                                 toolDecision(QStringLiteral("read-file"))};
        } else if (variant == QStringLiteral("wrong-type")) {
            auto invalid = toolDecision(QStringLiteral("read-file"));
            ToolInvocationArgument numberArg;
            numberArg.id = QStringLiteral("path");
            numberArg.value = QStringLiteral("42");
            numberArg.jsonValue = QJsonValue(42.0);
            invalid.arguments.append(numberArg);
            planner.decisions = {invalid, invalid};
        } else {
            auto invalid = toolDecision(QStringLiteral("read-file"));
            appendArg(invalid, QStringLiteral("path"), root + QStringLiteral("/src/calc.cpp"));
            appendArg(invalid, QStringLiteral("debug"), QStringLiteral("true"));
            planner.decisions = {invalid, invalid};
        }
        planner.repeatLast = false;

        AgentLoop::Config config;
        config.maxIterations = 4;
        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools, config);
        configureLoopFixture(loop, fixture, root);
        WorkspaceVerifier verifier(root);

        const auto state = runLoop(loop, this, QStringLiteral("cor05"), false,
                                   QStringLiteral("Attempt an invalid tool invocation"));
        QVERIFY2(state.phase == AgentLoopPhase::Failed || state.phase == AgentLoopPhase::Stuck,
                 qPrintable(loopPhaseName(state.phase)));
        QVERIFY(state.finalAnswer.isEmpty());
        QCOMPARE(fixture.handler.counts.value(QStringLiteral("read-file"), 0), 0);
        QCOMPARE(fixture.handler.counts.value(QStringLiteral("write-file"), 0), 0);
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        QVERIFY(verifier.untouched());
        if (variant == QStringLiteral("unknown-tool")) {
            QVERIFY(std::any_of(
                state.steps.cbegin(), state.steps.cend(), [](const AgentStepRecord& record) {
                    return record.observation.contains(QStringLiteral("Unknown tool"));
                }));
        } else {
            QVERIFY(std::any_of(
                state.steps.cbegin(), state.steps.cend(), [](const AgentStepRecord& record) {
                    return record.observation.contains(QStringLiteral("invalid arguments")) ||
                           record.observation.contains(QStringLiteral("rejected")) ||
                           record.observation.contains(QStringLiteral("Invalid"));
                }));
        }
    }

    // ------------------------------------------------------------------
    // COR-06: unsupported platform, unavailable capability runtime and a
    // disabled registry entry; none may fall back to another capability.
    // ------------------------------------------------------------------
    void cor06_capabilityUnavailable_data() {
        QTest::addColumn<QString>("variant");
        QTest::newRow("platform-unsupported") << QStringLiteral("platform-unsupported");
        QTest::newRow("handler-unavailable") << QStringLiteral("handler-unavailable");
        QTest::newRow("disabled-entry") << QStringLiteral("disabled");
    }

    void cor06_capabilityUnavailable() {
        QFETCH(QString, variant);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a + b; }\n"));

        LoopFixture fixture;
        QHash<QString, ToolDescriptor> overrides;
        const auto descriptors = BuiltInToolProvider::descriptors();
        const auto source =
            std::find_if(descriptors.cbegin(), descriptors.cend(), [](const ToolDescriptor& tool) {
                return tool.id == QLatin1String("stat-path");
            });
        QVERIFY(source != descriptors.cend());
        ToolDescriptor probe = *source;
        probe.id = QStringLiteral("nightvision-probe");
        probe.name = QStringLiteral("Night Vision Probe");
        // Explicit authorization contract for the renamed fixture descriptor.
        probe.requiredPermissionDomain = QStringLiteral("workspace-access");
        probe.authorizationRequirements = {{SecurityDomain::FileSystem,
                                            AccessMode::Read,
                                            AuthorizationResourceKind::FileSystemPath,
                                            QStringLiteral("path"),
                                            {}}};
        ToolDescriptor disabled = probe;
        disabled.enabled = false;

        QStringList requested{QStringLiteral("stat-path")};
        QString requestedId = QStringLiteral("nightvision-probe");
        QString unavailableReason;
        if (variant == QStringLiteral("platform-unsupported")) {
            probe.platforms.linux = ToolPlatformSupport::Unsupported;
            overrides.insert(probe.id, probe);
            requested.prepend(probe.id);
        } else if (variant == QStringLiteral("handler-unavailable")) {
            unavailableReason = QStringLiteral("Fixture capability runtime is unavailable.");
            overrides.insert(probe.id, probe);
            requested.prepend(probe.id);
            requestedId = probe.id;
        } else {
            overrides.insert(disabled.id, disabled);
            requested.prepend(disabled.id);
            requestedId = disabled.id;
        }
        fixture.handler.unavailableReasonOverride = unavailableReason;
        fixture.registerTools(requested, overrides);

        ScriptedPlanner planner;
        planner.decisions = {toolDecision(requestedId), toolDecision(requestedId)};
        planner.repeatLast = false;

        AgentLoop::Config config;
        config.maxIterations = 8;
        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools, config);
        configureLoopFixture(loop, fixture, root);
        WorkspaceVerifier verifier(root);

        const auto state = runLoop(loop, this, QStringLiteral("cor06"), false,
                                   QStringLiteral("Use the unavailable capability"));
        QVERIFY2(state.phase == AgentLoopPhase::Failed || state.phase == AgentLoopPhase::Stuck,
                 qPrintable(loopPhaseName(state.phase)));
        QVERIFY(state.finalAnswer.isEmpty());
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        QVERIFY(verifier.untouched());
        // The capability receipt never executed (zero other receipts issued).
        QCOMPARE(fixture.handler.counts.value(QStringLiteral("stat-path"), 0), 0);
        if (variant == QStringLiteral("handler-unavailable")) {
            QVERIFY(std::any_of(
                state.steps.cbegin(), state.steps.cend(), [](const AgentStepRecord& record) {
                    return record.observation.contains(QStringLiteral("runtime is unavailable"));
                }));
        } else {
            QVERIFY(std::any_of(
                state.steps.cbegin(), state.steps.cend(), [](const AgentStepRecord& record) {
                    return record.observation.contains(
                               QStringLiteral("unsupported on this platform")) ||
                           record.observation.contains(QStringLiteral("Tool unavailable")) ||
                           record.observation.contains(QStringLiteral("Unknown tool"));
                }));
        }
    }

    // ------------------------------------------------------------------
    // COR-07: denying the required filesystem/process action never executes
    // the handler and creates no persistent permission grant.
    // ------------------------------------------------------------------
    void cor07_deniedRequiredActionBlocked() {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/victim.txt"),
                 QStringLiteral("unchanged baseline"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("run-command"), QStringLiteral("write-file")});
        // Nothing is added to allowedCommands / allowedWrites: the denial gate
        // must keep every effect off the workspace.
        fixture.approval.mode = FixtureApprovalPolicy::Mode::RequireApproval;

        ScriptedPlanner planner;
        auto risky = toolDecision(QStringLiteral("run-command"), ToolRiskLevel::High);
        appendArg(risky, QStringLiteral("command"), QStringLiteral("rm -rf victim"));
        planner.decisions = {risky};
        planner.repeatLast = false;

        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools);
        configureLoopFixture(loop, fixture, root);
        WorkspaceVerifier verifier(root);

        auto state = runLoop(loop, this, QStringLiteral("cor07"), false,
                             QStringLiteral("Delete fixture victim"));
        QCOMPARE(state.phase, AgentLoopPhase::AwaitingApproval);
        QCOMPARE(fixture.handler.counts.value(QStringLiteral("run-command"), 0), 0);

        AgentLoop resumed(planner, fixture.executor, fixture.approval, fixture.sandbox,
                          fixture.knownTools);
        configureLoopFixture(resumed, fixture, root);
        state = resumed.resume(state, false);
        QCOMPARE(state.phase, AgentLoopPhase::Failed);
        QCOMPARE(state.terminalReason, AgentTerminalReason::ApprovalDenied);
        QVERIFY(state.finalAnswer.isEmpty());
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        QVERIFY(verifier.untouched());
    }

    // ------------------------------------------------------------------
    // COR-08: explicit allow-once approval; exactly one invocation; replay
    // and foreign approvals rejected afterwards; no permanent grant stored.
    // ------------------------------------------------------------------
    void cor08_allowOnceApproval_data() {
        QTest::addColumn<bool>("runtime");
        QTest::newRow("loop-resume") << false;
        QTest::newRow("runtime-approve") << true;
    }

    void cor08_allowOnceApproval() {
        QFETCH(bool, runtime);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        WorkspaceVerifier verifier(root);

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("run-command")});
        fixture.handler.allowedCommands = {QStringLiteral("echo approved-once")};
        fixture.approval.mode = FixtureApprovalPolicy::Mode::RequireApproval;
        fixture.planner.decisions = {
            toolDecision(QStringLiteral("run-command"), ToolRiskLevel::High)};
        appendArg(fixture.planner.decisions.first(), QStringLiteral("command"),
                  QStringLiteral("echo approved-once"));
        auto finish = finalDecision(QStringLiteral("approved-once receipt completed."));
        finish.grounding = GroundingMode::Context;
        finish.groundingDeclared = true;
        fixture.planner.decisions.append(finish);
        fixture.planner.repeatLast = false;

        QString session;
        if (!runtime) {
            AgentLoop loop(fixture.planner, fixture.executor, fixture.approval, fixture.sandbox,
                           fixture.knownTools);
            configureLoopFixture(loop, fixture, root);
            auto state = runLoop(loop, this, QStringLiteral("cor08"), false,
                                 QStringLiteral("approved once"));
            QCOMPARE(state.phase, AgentLoopPhase::AwaitingApproval);
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("run-command"), 0), 0);
            AgentLoop resumed(fixture.planner, fixture.executor, fixture.approval, fixture.sandbox,
                              fixture.knownTools);
            configureLoopFixture(resumed, fixture, root);
            state = resumed.resume(state, true);
            QCOMPARE(state.phase, AgentLoopPhase::Completed);
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("run-command"), 0), 1);
        } else {
            RuntimeFixture fixtureRuntime;
            fixtureRuntime.create({QStringLiteral("run-command")});
            fixtureRuntime.configure(root, /*autonomous=*/false);
            fixtureRuntime.handler.allowedCommands = {QStringLiteral("echo approved-once")};
            fixtureRuntime.approval.mode = FixtureApprovalPolicy::Mode::RequireApproval;
            fixtureRuntime.planner.decisions = {
                toolDecision(QStringLiteral("run-command"), ToolRiskLevel::High)};
            appendArg(fixtureRuntime.planner.decisions.first(), QStringLiteral("command"),
                      QStringLiteral("echo approved-once"));
            auto runtimeFinish = finalDecision(QStringLiteral("approved-once receipt completed."));
            runtimeFinish.grounding = GroundingMode::Context;
            runtimeFinish.groundingDeclared = true;
            fixtureRuntime.planner.decisions.append(runtimeFinish);
            fixtureRuntime.planner.repeatLast = false;
            session = fixtureRuntime.session;
            QVERIFY(fixtureRuntime.runtime->start(session, QStringLiteral("approved once")));
            QTRY_COMPARE_WITH_TIMEOUT(fixtureRuntime.runtime->sessionState(session).phase,
                                      AgentLoopPhase::AwaitingApproval, kBarrierTimeoutMs);
            // Allow-once: never a permanent grant for the.FileSystem action.
            QVERIFY(fixtureRuntime.runtime->approve(session, false));
            QVERIFY(fixtureRuntime.runtime->continueSession(session, true));
            QTRY_COMPARE_WITH_TIMEOUT(fixtureRuntime.runtime->sessionState(session).phase,
                                      AgentLoopPhase::Completed, kBarrierTimeoutMs);
            QCOMPARE(fixtureRuntime.handler.counts.value(QStringLiteral("run-command"), 0), 1);
            // No persistent grant may exist after allow-once.
            QVERIFY(fixtureRuntime.runtime->persistentPermissionGrants().isEmpty());
            TerminalAudit audit;
            audit.scan(fixtureRuntime.runtime->eventHistory(session));
            QVERIFY2(audit.singleTerminal(), qPrintable(audit.describe()));
            // Replays of the consumed approval and foreign-session approvals
            // are rejected; no second run is created.
            QVERIFY(!fixtureRuntime.runtime->approve(session, false));
            QVERIFY(!fixtureRuntime.runtime->approve(session, true));
            QVERIFY(!fixtureRuntime.runtime->approve(QStringLiteral("foreign-session"), false));
            runtimeHolderList.push_back(std::move(fixtureRuntime.runtime));
        }
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        QVERIFY(verifier.untouched());
    }

    // ------------------------------------------------------------------
    // COR-09: provider disconnect during required work. Transient recovery
    // exhausts its budget; a fatal failure terminates with ProviderFailure;
    // in both variants no stale completion may pass the terminal fence.
    // ------------------------------------------------------------------
    void cor09_providerDisconnect_data() {
        QTest::addColumn<QString>("variant");
        QTest::newRow("transient-recovery-exhausted") << QStringLiteral("transient");
        QTest::newRow("immediate-fatal") << QStringLiteral("fatal");
    }

    void cor09_providerDisconnect() {
        QFETCH(QString, variant);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a + b; }\n"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("read-file")});

        FaultProvider provider;
        ChatProviderReply fatal;
        fatal.success = false;
        fatal.category = ChatProviderErrorCategory::ConnectionFailed;
        fatal.httpStatus = 500; // deliberately non-transient classification
        fatal.attempts = 3;
        fatal.errorMessage = QStringLiteral("fixture connection lost");
        if (variant == QStringLiteral("transient")) {
            ChatProviderReply transientReply;
            transientReply.success = false;
            transientReply.category = ChatProviderErrorCategory::ProviderUnavailable;
            transientReply.httpStatus = 503; // transient; production recovery path
            transientReply.attempts = 1;
            transientReply.errorMessage = QStringLiteral("fixture overload");
            provider.plannerReplies = {transientReply, transientReply};
            provider.faultAfterQueue = fatal;
        } else {
            fatal.attempts = 1;
            provider.faultAfterQueue = fatal;
        }

        auto planner = makeLlmPlanner(provider, fixtureDescriptors(fixture.knownTools));
        AgentLoop loop(*planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools);
        configureLoopFixture(loop, fixture, root);
        WorkspaceVerifier verifier(root);

        const auto state = runLoop(loop, this, QStringLiteral("cor09"), false,
                                   QStringLiteral("Improve the fixture using the provider"));
        QCOMPARE(state.phase, AgentLoopPhase::Failed);
        if (state.terminalReason != AgentTerminalReason::ProviderFailure)
            QFAIL(
                qPrintable(QStringLiteral("expected ProviderFailure, got %1 (%2)")
                               .arg(terminalReasonName(state.terminalReason), state.abortReason)));
        QVERIFY(state.finalAnswer.isEmpty());
        // No stale completed event may exist; the production policy retries a
        // transient 503-class planner failure exactly once per planner call.
        QCOMPARE(provider.plannerRequests, variant == QStringLiteral("transient") ? 2 : 1);
        QCOMPARE(state.providerRecoveryAttempts, variant == QStringLiteral("transient") ? 1 : 0);
        // No tool side effect may be dispatched from a failed planning stage.
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        QVERIFY(verifier.untouched());
    }

    // ------------------------------------------------------------------
    // COR-10: cancel at the classification/planning barrier; no tool or
    // model dispatch afterwards; a fresh task still succeeds.
    // ------------------------------------------------------------------
    void cor10_cancelAtClassificationBarrier() {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a + b; }\n"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("read-file"), QStringLiteral("write-file")});

        ScriptedPlanner planner;
        planner.decisions = {toolDecision(QStringLiteral("read-file"))};
        planner.repeatLast = false;

        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools);
        configureLoopFixture(loop, fixture, root);
        auto policy = std::make_shared<CallbackIntentPolicy>();
        policy->onClassify = [&] { loop.cancelAsync(); };
        loop.setObservationIntentPolicy(policy);
        WorkspaceVerifier verifier(root);

        int terminals = 0;
        loop.runAsync(QStringLiteral("Cancelled at classification"), QStringLiteral("cor10"), this,
                      [&terminals](const AgentLoopState& state) {
                          QCOMPARE(state.phase, AgentLoopPhase::Cancelled);
                          ++terminals;
                      });
        QTRY_COMPARE_WITH_TIMEOUT(terminals, 1, kBarrierTimeoutMs);
        QCOMPARE(planner.calls, 0);
        QCOMPARE(fixture.handler.counts.value(QStringLiteral("read-file"), 0), 0);
        QCOMPARE(fixture.executor.calls, 0);

        // Next fresh task succeeds; no residual queued tool effect.
        LoopFixture fresh;
        fresh.registerTools({QStringLiteral("read-file"), QStringLiteral("write-file")});
        fresh.planner.decisions = {toolDecision(QStringLiteral("read-file")),
                                   finalDecision(QStringLiteral("observed receipt"))};
        appendArg(fresh.planner.decisions.first(), QStringLiteral("path"),
                  QDir(root).filePath(QStringLiteral("src/calc.cpp")));
        fresh.planner.repeatLast = false;
        AgentLoop loop2(fresh.planner, fresh.executor, fresh.approval, fresh.sandbox,
                        fresh.knownTools);
        configureLoopFixture(loop2, fresh, root);
        auto emptyIntent = std::make_shared<FixedIntentPolicy>();
        loop2.setObservationIntentPolicy(emptyIntent);
        // The loop recomputes the canonical scoped representation from the
        // (empty) classified intent; mirror that computation in the planner.
        fresh.planner.canonicalFinal = true;
        fresh.planner.canonicalIntent = emptyIntent->intent;
        const auto state2 = runLoop(loop2, this, QStringLiteral("cor10-next"), false,
                                    QStringLiteral("Read the fixture file"));
        QCOMPARE(state2.phase, AgentLoopPhase::Completed);
        QCOMPARE(fresh.handler.counts.value(QStringLiteral("read-file"), 0), 1);
        QVERIFY(verifier.untouched());
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        QCOMPARE(fresh.handler.unauthorizedCalls, 0);
    }

    // ------------------------------------------------------------------
    // COR-11: cancellation during generation; late provider output arrives
    // after cancellation and must be ignored without completing anything.
    // ------------------------------------------------------------------
    void cor11_cancelDuringGeneration() {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a + b; }\n"));

        RuntimeFixture fixture;
        fixture.create({QStringLiteral("write-file"), QStringLiteral("read-file")});

        // Injected provider blocks generation on a deterministic barrier and
        // holds a late final reply that must never be consumed.
        auto provider = std::make_unique<FaultProvider>();
        provider->barrierAllRequests = true;
        ChatProviderReply lateFinal;
        lateFinal.success = true;
        lateFinal.message =
            QStringLiteral("{\"action\":\"final\",\"grounding\":\"verified\",\"answer\":\"late\"}");
        provider->plannerReplies = {lateFinal};
        auto llmPlanner = makeLlmPlanner(
            *provider,
            fixtureDescriptors({QStringLiteral("write-file"), QStringLiteral("read-file")}));

        auto runtime = std::make_unique<AgentRuntime>(
            std::make_unique<FixtureMetadataRuntime>(
                fixtureDescriptors({QStringLiteral("write-file"), QStringLiteral("read-file")})),
            llmPlanner.get(), fixture.executor, fixture.approval, fixture.sandbox);
        for (const auto& descriptor :
             fixtureDescriptors({QStringLiteral("write-file"), QStringLiteral("read-file")})) {
            QVERIFY(runtime->toolRegistry().unregisterTool(descriptor.id));
            QVERIFY(runtime->toolRegistry().registerTool(IToolRegistry::Registration{
                descriptor,
                std::shared_ptr<IToolHandler>(&fixture.handler, [](IToolHandler*) {})}));
        }
        const QString session = runtime->createSession();
        QVERIFY(!session.isEmpty());
        AgentSessionOptions options;
        options.autonomousMode = true;
        options.workspaceContext.rootPath = root;
        runtime->configureSession(session, std::move(options));

        QVERIFY(runtime->start(session, QStringLiteral("generate a grounded answer")));
        // Barrier: the planner request entered the provider (generation in
        // flight). Request the cancellation, then release the barrier.
        QVERIFY2(provider->arrived.tryAcquire(1, kBarrierTimeoutMs),
                 "provider generation never started");
        QVERIFY(runtime->cancel(session));
        provider->release.release(); // "late response" arrives after cancel

        QTRY_COMPARE_WITH_TIMEOUT(runtime->sessionState(session).phase, AgentLoopPhase::Cancelled,
                                  kBarrierTimeoutMs);
        QCOMPARE(provider->cancelledReports, 1);
        QCOMPARE(provider->plannerRequests, 1);
        TerminalAudit audit;
        audit.scan(runtime->eventHistory(session));
        QVERIFY2(audit.singleTerminal() && audit.cancelled == 1, qPrintable(audit.describe()));
        const auto frozenEvents = runtime->eventHistory(session);
        // Terminal fencing: late approvals and duplicate cancels are rejected;
        // the frozen event history is unchanged.
        QVERIFY(!runtime->approve(session, false));
        QVERIFY(!runtime->approve(session, true));
        QCOMPARE(runtime->cancel(session), false);
        QCOMPARE(runtime->eventHistory(session).size(), frozenEvents.size());
        // Nothing may have been executed or written by the cancelled run.
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        WorkspaceVerifier verifier(root);
        QVERIFY(verifier.untouched());
        providerHolderList.push_back(std::move(provider));
        runtimeHolderList.push_back(std::move(runtime));
        llmPlannerHolderList.push_back(std::move(llmPlanner));
    }

    // ------------------------------------------------------------------
    // COR-12: cancellation during cooperative tool execution; the in-flight
    // receipt may commit its declared partial artifact; no further effect
    // runs after the seal and the next task succeeds.
    // ------------------------------------------------------------------
    void cor12_cancelDuringCooperativeTool() {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);

        RuntimeFixture fixture;
        fixture.handler.allowedCommands = {QStringLiteral("echo slow-build")};
        fixture.handler.blockSlowBuild = true;
        fixture.create({QStringLiteral("run-command"), QStringLiteral("read-file")});
        {
            AgentSessionOptions options;
            options.autonomousMode = true;
            options.workspaceContext.rootPath = root;
            fixture.handler.root = root;
            fixture.runtime->configureSession(fixture.session, std::move(options));
        }
        fixture.planner.decisions = {
            toolDecision(QStringLiteral("run-command"), ToolRiskLevel::High)};
        auto finishPlan = finalDecision(QStringLiteral("Cannot complete; cancelled in build."));
        finishPlan.grounding = GroundingMode::Context;
        finishPlan.groundingDeclared = true;
        appendArg(fixture.planner.decisions.first(), QStringLiteral("command"),
                  QStringLiteral("echo slow-build"));
        fixture.planner.decisions.append(finishPlan);
        fixture.planner.repeatLast = false;
        // Snapshot before the run so the cancelled run's committed partial
        // artifact shows up as an addition.
        WorkspaceVerifier verifier(root);
        WorkspaceVerifier::Expected expected;
        expected.added = {QStringLiteral("build/partial.log")};

        QVERIFY(fixture.runtime->start(fixture.session,
                                       QStringLiteral("run the slow authorized build")));
        if (!fixture.handler.slowBuildStarted.tryAcquire(1, kBarrierTimeoutMs))
            QVERIFY2(false, "slow build never started");
        QVERIFY(fixture.runtime->cancel(fixture.session));
        fixture.handler.slowBuildRelease.release();

        QTRY_COMPARE_WITH_TIMEOUT(fixture.runtime->sessionState(fixture.session).phase,
                                  AgentLoopPhase::Cancelled, kBarrierTimeoutMs);
        // Exactly one invocation; nothing may run after the cancel seal.
        fixture.handler.seal();
        QCOMPARE(fixture.handler.counts.value(QStringLiteral("run-command"), 0), 1);
        QCOMPARE(fixture.handler.postSealCalls, 0);
        QString actual;
        QVERIFY2(verifier.matches(expected, &actual), qPrintable(actual));
        TerminalAudit audit;
        audit.scan(fixture.runtime->eventHistory(fixture.session));
        QVERIFY2(audit.singleTerminal() && audit.cancelled == 1, qPrintable(audit.describe()));
        QVERIFY(!fixture.runtime->approve(fixture.session, false));

        // Next fresh task works cooperatively after the cancelled one.
        const auto nextSession = fixture.runtime->createSession();
        QVERIFY(!nextSession.isEmpty());
        AgentSessionOptions options;
        options.autonomousMode = true;
        options.workspaceContext.rootPath = root;
        fixture.handler.root = root;
        fixture.runtime->configureSession(nextSession, std::move(options));
        fixture.planner.resetPlanningIndex();
        fixture.planner.decisions = {toolDecision(QStringLiteral("read-file")),
                                     finalDecision(QStringLiteral("clean receipt"))};
        appendArg(fixture.planner.decisions.first(), QStringLiteral("path"),
                  QDir(root).filePath(QStringLiteral("src/calc.cpp")));
        QVERIFY(fixture.runtime->start(nextSession, QStringLiteral("read the fixture")));
        QTRY_COMPARE_WITH_TIMEOUT(fixture.runtime->sessionState(nextSession).phase,
                                  AgentLoopPhase::Completed, kBarrierTimeoutMs);
        QCOMPARE(fixture.handler.counts.value(QStringLiteral("read-file"), 0), 1);
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        // Partial artifact unchanged by the next task.
        QCOMPARE(verifier.fileContent(QStringLiteral("build/partial.log")),
                 QStringLiteral("partial fixture output\n"));
        runtimeHolderList.push_back(std::move(fixture.runtime));
    }

    // ------------------------------------------------------------------
    // COR-13: cancellation while awaiting approval; late approvals are
    // rejected, the executor never runs, grants stay cleared.
    // ------------------------------------------------------------------
    void cor13_cancelWhileAwaitingApproval() {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/victim.txt"),
                 QStringLiteral("unchanged"));

        RuntimeFixture fixture;
        fixture.configure(root, true);
        fixture.approval.mode = FixtureApprovalPolicy::Mode::RequireApproval;
        fixture.planner.decisions = {
            toolDecision(QStringLiteral("run-command"), ToolRiskLevel::High)};
        auto finishDecision = finalDecision(QStringLiteral("must never finish"));
        finishDecision.grounding = GroundingMode::Context;
        finishDecision.groundingDeclared = true;
        appendArg(fixture.planner.decisions.first(), QStringLiteral("command"),
                  QStringLiteral("echo approved-once"));
        fixture.planner.decisions.append(finishDecision);
        fixture.planner.repeatLast = false;
        fixture.create({QStringLiteral("run-command")});

        QVERIFY(fixture.runtime->start(fixture.session, QStringLiteral("approved action")));
        QTRY_COMPARE_WITH_TIMEOUT(fixture.runtime->sessionState(fixture.session).phase,
                                  AgentLoopPhase::AwaitingApproval, kBarrierTimeoutMs);
        QVERIFY(fixture.runtime->cancel(fixture.session));
        QTRY_COMPARE_WITH_TIMEOUT(fixture.runtime->sessionState(fixture.session).phase,
                                  AgentLoopPhase::Cancelled, kBarrierTimeoutMs);
        QCOMPARE(fixture.handler.counts.value(QStringLiteral("run-command"), 0), 0);
        // Late approvals are rejected.
        QVERIFY(!fixture.runtime->approve(fixture.session, false));
        QVERIFY(!fixture.runtime->approve(fixture.session, true));
        // No persistent grant may exist after cancellation.
        QVERIFY(fixture.runtime->persistentPermissionGrants().isEmpty());
        TerminalAudit audit;
        audit.scan(fixture.runtime->eventHistory(fixture.session));
        QVERIFY2(audit.singleTerminal() && audit.cancelled == 1, qPrintable(audit.describe()));
        WorkspaceVerifier verifier(root);
        QVERIFY(verifier.untouched());
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        runtimeHolderList.push_back(std::move(fixture.runtime));
    }

    // ------------------------------------------------------------------
    // COR-14: repeated ineffective reads recover once; repeated mutation
    // terminates Stuck with a strictly bounded write effect.
    // ------------------------------------------------------------------
    void cor14_ineffectiveRepetition_data() {
        QTest::addColumn<QString>("variant");
        QTest::newRow("read-repair-recovers") << QStringLiteral("read");
        QTest::newRow("mutation-terminates-stuck") << QStringLiteral("mutation");
    }

    void cor14_ineffectiveRepetition() {
        QFETCH(QString, variant);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a + b; }\n"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("read-file"), QStringLiteral("write-file")});
        const auto path = root + QStringLiteral("/src/calc.cpp");

        ScriptedPlanner planner;
        if (variant == QStringLiteral("read")) {
            auto read = toolDecision(QStringLiteral("read-file"));
            appendArg(read, QStringLiteral("path"), path);
            planner.decisions = {read, read, finalDecision(QStringLiteral("observed result"))};
        } else {
            auto write = toolDecision(QStringLiteral("write-file"));
            appendArg(write, QStringLiteral("path"), path);
            appendArg(write, QStringLiteral("content"),
                      QStringLiteral("int add(int a,int b){ return a + b; }\n"));
            planner.decisions = {write, write};
        }
        planner.repeatLast = false;

        AgentLoop::Config config;
        config.maxIterations = 6;
        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools, config);
        configureLoopFixture(loop, fixture, root);
        WorkspaceVerifier verifier(root);

        const auto state = runLoop(loop, this, QStringLiteral("cor14"), false,
                                   QStringLiteral("Repeat the ineffective action"));
        if (variant == QStringLiteral("read")) {
            QCOMPARE(state.phase, AgentLoopPhase::Completed);
            QCOMPARE(state.repeatedReadRepairs, 1);
            // The second identical read is intercepted; one receipt only.
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("read-file"), 0), 1);
        } else {
            QCOMPARE(state.phase, AgentLoopPhase::Stuck);
            QCOMPARE(state.repeatedReadRepairs, 0);
            QVERIFY(fixture.handler.counts.value(QStringLiteral("write-file"), 0) <= 1);
            QCOMPARE(verifier.fileContent(QStringLiteral("src/calc.cpp")),
                     QStringLiteral("int add(int a,int b){ return a + b; }\n"));
        }
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        QVERIFY(verifier.untouched());
    }

    // ------------------------------------------------------------------
    // COR-15: grounded final accepted only through verified observations,
    // an independent build/test receipt and a diff; a receipt-only final
    // cannot complete the run.
    // ------------------------------------------------------------------
    void cor15_groundedFinalEvidence_data() {
        QTest::addColumn<bool>("async");
        QTest::addColumn<bool>("withVerification");
        for (bool async : {false, true})
            for (bool withVerification : {true, false}) {
                QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(
                    async ? QStringLiteral("async") : QStringLiteral("sync"),
                    withVerification ? QStringLiteral("grounded")
                                     : QStringLiteral("receipt-only-final"))))
                    << async << withVerification;
            }
    }

    void cor15_groundedFinalEvidence() {
        QFETCH(bool, async);
        QFETCH(bool, withVerification);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a - b; }\n"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("read-file"), QStringLiteral("write-file"),
                               QStringLiteral("run-command"), QStringLiteral("git-diff")});
        fixture.handler.allowedCommands = {QStringLiteral("ctest --test-dir build")};
        fixture.handler.allowedWrites.insert(
            QStringLiteral("src/calc.cpp"),
            QStringLiteral("int add(int a,int b){ return a + b; }\n"));
        const auto calcPath = root + QStringLiteral("/src/calc.cpp");

        auto policy = std::make_shared<FixedIntentPolicy>();
        ObservationRequirement readRequirement;
        readRequirement.domain = ObservationDomain::FileSystem;
        readRequirement.resourceHint = calcPath;
        readRequirement.freshness = EvidenceFreshness::TurnScoped;
        readRequirement.purpose = ObservationPurpose::Inspect;
        readRequirement.toolId = QStringLiteral("read-file");
        policy->intent.requirements.append(readRequirement);
        ObservationRequirement testRequirement;
        testRequirement.domain = ObservationDomain::ProcessExecution;
        testRequirement.resourceHint = QStringLiteral("ctest --test-dir build");
        testRequirement.freshness = EvidenceFreshness::TurnScoped;
        testRequirement.purpose = ObservationPurpose::Operate;
        policy->intent.requirements.append(testRequirement);
        ObservationRequirement diffRequirement;
        diffRequirement.domain = ObservationDomain::Workspace;
        diffRequirement.resourceHint = root;
        diffRequirement.freshness = EvidenceFreshness::Live;
        diffRequirement.purpose = ObservationPurpose::Inspect;
        diffRequirement.toolId = QStringLiteral("git-diff");
        policy->intent.requirements.append(diffRequirement);

        ScriptedPlanner planner;
        auto initialRead = toolDecision(QStringLiteral("read-file"));
        appendArg(initialRead, QStringLiteral("path"), calcPath);
        auto write = toolDecision(QStringLiteral("write-file"));
        appendArg(write, QStringLiteral("path"), calcPath);
        appendArg(write, QStringLiteral("content"),
                  QStringLiteral("int add(int a,int b){ return a + b; }\n"));
        auto freshRead = toolDecision(QStringLiteral("read-file"));
        appendArg(freshRead, QStringLiteral("path"), calcPath);
        auto test = toolDecision(QStringLiteral("run-command"));
        appendArg(test, QStringLiteral("command"), QStringLiteral("ctest --test-dir build"));
        auto diff = toolDecision(QStringLiteral("git-diff"));
        appendArg(diff, QStringLiteral("path"), root);
        planner.decisions =
            withVerification ? QVector<AgentStepDecision>{initialRead, write, freshRead, test, diff}
                             : QVector<AgentStepDecision>{initialRead, write, freshRead, test};
        auto finish = finalDecision(QStringLiteral(
            "Fix complete. ctest --test-dir build returns ALL_TESTS_PASSED and the git-diff "
            "receipt shows - return a - b\\n+ return a + b."));
        finish.grounding = GroundingMode::Verified;
        finish.groundingDeclared = true;
        planner.decisions.append(finish);
        planner.repeatLast = true;
        planner.canonicalFinal = withVerification;

        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools);
        configureLoopFixture(loop, fixture, root);
        loop.setObservationIntentPolicy(policy);
        loop.setToolCallIdProvider([](int) { return QStringLiteral("observed"); });
        WorkspaceVerifier verifier(root);

        const auto state =
            runLoop(loop, this, QStringLiteral("cor15"), async,
                    QStringLiteral("Repair the fixture and finish only with verified evidence"));
        WorkspaceVerifier::Expected expected;
        expected.changed = {QStringLiteral("src/calc.cpp")};
        QString actual;
        QVERIFY2(verifier.matches(expected, &actual), qPrintable(actual));
        if (withVerification) {
            QCOMPARE(state.phase, AgentLoopPhase::Completed);
            QVERIFY(state.finalAnswer.contains(QStringLiteral("ALL_TESTS_PASSED")));
            QVERIFY(state.finalAnswer.contains(QStringLiteral("return a + b")));
            QVERIFY(std::any_of(state.evidence.cbegin(), state.evidence.cend(),
                                [](const EvidenceRecord& item) {
                                    return item.scope == EvidenceScope::Operation &&
                                           item.outcome == EvidenceOutcome::Verified;
                                }));
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("git-diff"), 0), 1);
        } else {
            // Tool success alone does not complete the run when the fresh diff
            // requirement is still unmet.
            QVERIFY2(state.phase == AgentLoopPhase::Failed,
                     qPrintable(terminalFailureDescription(state)));
            QVERIFY(state.finalAnswer.isEmpty());
        }
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        QCOMPARE(verifier.fileContent(QStringLiteral("src/calc.cpp")),
                 QStringLiteral("int add(int a,int b){ return a + b; }\n"));
    }

    // ------------------------------------------------------------------
    // COR-16: fabricated success claims are rejected; bounded inability
    // remains truthful without inventing a passing outcome.
    // ------------------------------------------------------------------
    void cor16_fabricatedClaims_data() {
        QTest::addColumn<QString>("variant");
        QTest::newRow("fabricated-test-success") << QStringLiteral("fabricated-success");
        QTest::newRow("no-observation-final") << QStringLiteral("no-observation");
    }

    void cor16_fabricatedClaims() {
        QFETCH(QString, variant);
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a - b; }\n"));

        LoopFixture fixture;
        fixture.registerTools({QStringLiteral("write-file"), QStringLiteral("run-command")});
        fixture.handler.testReceiptFails = true;
        const auto calcPath = root + QStringLiteral("/src/calc.cpp");

        ScriptedPlanner planner;
        if (variant == QStringLiteral("fabricated-success")) {
            auto write = toolDecision(QStringLiteral("write-file"));
            appendArg(write, QStringLiteral("path"), calcPath);
            appendArg(write, QStringLiteral("content"),
                      QStringLiteral("int add(int a,int b){ return a + b; }\n"));
            auto test = toolDecision(QStringLiteral("run-command"), ToolRiskLevel::High);
            appendArg(test, QStringLiteral("command"), QStringLiteral("ctest --test-dir build"));
            auto fabricated = finalDecision(
                QStringLiteral("The authorized test suite passed and the fix is complete."));
            fabricated.grounding = GroundingMode::Verified;
            fabricated.groundingDeclared = true;
            planner.decisions = {write, test, fabricated};
        } else {
            auto fabricated = finalDecision(
                QStringLiteral("src/calc.cpp now contains the fixed addition operator."));
            fabricated.grounding = GroundingMode::Verified;
            fabricated.groundingDeclared = true;
            planner.decisions = {fabricated};
        }
        planner.repeatLast = true;

        AgentLoop::Config config;
        config.maxIterations = 3;
        AgentLoop loop(planner, fixture.executor, fixture.approval, fixture.sandbox,
                       fixture.knownTools, config);
        configureLoopFixture(loop, fixture, root);
        WorkspaceVerifier verifier(root);

        const auto state = runLoop(loop, this, QStringLiteral("cor16"), false,
                                   QStringLiteral("Repair the fixture file"));
        QVERIFY2(state.phase == AgentLoopPhase::Failed || state.phase == AgentLoopPhase::Stuck,
                 qPrintable(terminalFailureDescription(state)));
        QVERIFY(state.finalAnswer.isEmpty());
        if (variant == QStringLiteral("fabricated-success")) {
            // The failing receipt ran once and cannot be repeated into green.
            QCOMPARE(fixture.handler.counts.value(QStringLiteral("run-command"), 0), 1);
            WorkspaceVerifier::Expected expected;
            expected.changed = {QStringLiteral("src/calc.cpp")};
            QString actual;
            QVERIFY2(verifier.matches(expected, &actual), qPrintable(actual));
        } else {
            // No tool executed: no verified observation supports the claim.
            QCOMPARE(fixture.handler.unauthorizedCalls, 0);
            QVERIFY(verifier.untouched());
        }
    }

    // ------------------------------------------------------------------
    // COR-17: the "client" disconnects mid-run by unsubscribing; the run
    // continues, reaches its expected terminal exactly once, and re-attach
    // exposes canonical state with no mutation replay.
    // ------------------------------------------------------------------
    void cor17_disconnectReattachDuringActiveRun() {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);
        seedFile(QDir(workspace.path()), QStringLiteral("src/calc.cpp"),
                 QStringLiteral("int add(int a,int b){ return a - b; }\n"));

        RuntimeFixture fixture;
        fixture.handler.allowedWrites.insert(
            QStringLiteral("src/calc.cpp"),
            QStringLiteral("int add(int a,int b){ return a + b; }\n"));
        fixture.create({QStringLiteral("write-file"), QStringLiteral("read-file")});
        fixture.configure(root);
        fixture.planner.decisions = {toolDecision(QStringLiteral("write-file"))};
        auto finishDecision = finalDecision(QStringLiteral("repair applied"));
        finishDecision.grounding = GroundingMode::Context;
        finishDecision.groundingDeclared = true;
        fixture.planner.decisions.append(finishDecision);
        fixture.planner.repeatLast = false;

        // Client disconnect: subscription goes away while the run is active.
        const QString subscriptionId = fixture.runtime->subscribe([](const AgentEvent&) {});
        fixture.runtime->unsubscribe(subscriptionId);

        QVERIFY(fixture.runtime->start(fixture.session, QStringLiteral("apply repair")));
        QTRY_COMPARE_WITH_TIMEOUT(fixture.runtime->sessionState(fixture.session).phase,
                                  AgentLoopPhase::Completed, kBarrierTimeoutMs);
        // Re-attach: canonical terminal and the full event history remain
        // available, with no cancellation implied.
        TerminalAudit audit;
        audit.scan(fixture.runtime->eventHistory(fixture.session));
        QVERIFY2(audit.singleTerminal() && audit.completed == 1, qPrintable(audit.describe()));
        bool sawCancellation = false;
        for (const auto& event : fixture.runtime->eventHistory(fixture.session))
            sawCancellation |= event.type == AgentEventType::AgentCancelled;
        QVERIFY(!sawCancellation);
        QCOMPARE(fixture.handler.counts.value(QStringLiteral("write-file"), 0), 1);
        WorkspaceVerifier verifier(root);
        WorkspaceVerifier::Expected expected;
        expected.changed = {QStringLiteral("src/calc.cpp")};
        QString actual;
        QVERIFY2(verifier.matches(expected, &actual), qPrintable(actual));
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        runtimeHolderList.push_back(std::move(fixture.runtime));
    }

    // ------------------------------------------------------------------
    // COR-18: stale, duplicate and foreign event identities plus late
    // approvals after terminal are ignored; the first terminal stands and
    // the next fresh task still works.
    // ------------------------------------------------------------------
    void cor18_staleDuplicateForeignEventsIgnored() {
        QTemporaryDir workspace;
        QVERIFY(workspace.isValid());
        const auto root = canonicalRoot(workspace);

        RuntimeFixture fixture;
        fixture.create({QStringLiteral("read-file")});
        fixture.configure(root);
        fixture.planner.decisions = {toolDecision(QStringLiteral("read-file")),
                                     finalDecision(QStringLiteral("observed receipt"))};
        QVERIFY(fixture.runtime->start(fixture.session, QStringLiteral("read the fixture")));
        QTRY_COMPARE_WITH_TIMEOUT(fixture.runtime->sessionState(fixture.session).phase,
                                  AgentLoopPhase::Completed, kBarrierTimeoutMs);

        const auto frozenEvents = fixture.runtime->eventHistory(fixture.session);
        // Duplicate cancel on a terminal run is rejected.
        QVERIFY(!fixture.runtime->cancel(fixture.session));
        // Foreign/unknown session identities are rejected without effect.
        QVERIFY(!fixture.runtime->cancel(QStringLiteral("foreign-session")));
        QVERIFY(!fixture.runtime->approve(QStringLiteral("foreign-session"), false));
        // Late approvals against the stale terminal cannot reopen anything.
        QVERIFY(!fixture.runtime->approve(fixture.session, false));
        QVERIFY(!fixture.runtime->approve(fixture.session, true));
        QCOMPARE(fixture.runtime->sessionState(fixture.session).phase, AgentLoopPhase::Completed);
        QCOMPARE(fixture.runtime->eventHistory(fixture.session).size(), frozenEvents.size());
        TerminalAudit audit;
        audit.scan(fixture.runtime->eventHistory(fixture.session));
        QVERIFY2(audit.singleTerminal() && audit.completed == 1, qPrintable(audit.describe()));

        // The next fresh task still runs; stale state cannot block a new run.
        const auto nextSession = fixture.runtime->createSession();
        QVERIFY(!nextSession.isEmpty());
        AgentSessionOptions options;
        options.autonomousMode = true;
        options.workspaceContext.rootPath = root;
        fixture.runtime->configureSession(nextSession, std::move(options));
        fixture.planner.resetPlanningIndex();
        fixture.planner.decisions = {toolDecision(QStringLiteral("read-file")),
                                     finalDecision(QStringLiteral("next receipt"))};
        QVERIFY(fixture.runtime->start(nextSession, QStringLiteral("read again")));
        QTRY_COMPARE_WITH_TIMEOUT(fixture.runtime->sessionState(nextSession).phase,
                                  AgentLoopPhase::Completed, kBarrierTimeoutMs);
        QCOMPARE(fixture.handler.unauthorizedCalls, 0);
        runtimeHolderList.push_back(std::move(fixture.runtime));
    }

private:
    // Worker threads borrow planner/provider references; keep the holders
    // alive until the slot's threads have joined (AgentRuntime dtor).
    std::vector<std::unique_ptr<FaultProvider>> providerHolderList;
    std::vector<std::unique_ptr<LlmAgentRuntime>> llmPlannerHolderList;
    std::vector<std::unique_ptr<AgentRuntime>> runtimeHolderList;
};

QTEST_MAIN(AgentReliabilityHarnessTest)
#include "test_agent_reliability_harness.moc"
