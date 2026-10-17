// SPDX-License-Identifier: GPL-3.0-or-later
#include "service/LocalModelRuntime.h"
#include <QFile>
#include <QNetworkReply>
#include <QPointer>
#include <QTemporaryDir>
#include <QtTest>
#include <cstring>

using namespace sentinel::core;
using sentinel::daemon::LocalModelRuntime;

namespace {
class HealthReply final : public QNetworkReply {
public:
    explicit HealthReply(const QNetworkRequest& request, QObject* parent) : QNetworkReply(parent) {
        setRequest(request);
        setUrl(request.url());
        open(QIODevice::ReadOnly);
    }
    void finish(int code, NetworkError error = NoError,
                QByteArray body = R"({"model":"foreign-alias"})") {
        body_ = std::move(body);
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, code);
        if (error != NoError) {
            setError(error, QStringLiteral("synthetic health failure"));
        }
        setFinished(true);
        emit finished();
    }
    bool finishOnAbort = false;
    int aborts = 0;
    void abort() override {
        ++aborts;
        if (finishOnAbort) {
            finish(0, OperationCanceledError);
        }
    }
    qint64 bytesAvailable() const override {
        return body_.size() + QNetworkReply::bytesAvailable();
    }

protected:
    qint64 readData(char* target, qint64 maxSize) override {
        const auto size = qMin(maxSize, qint64(body_.size()));
        if (size == 0) {
            return -1;
        }
        std::memcpy(target, body_.constData(), size_t(size));
        body_.remove(0, size);
        return size;
    }

private:
    QByteArray body_;
};
struct Fixture {
    QTemporaryDir workspace;
    QObject replies;
    LocalModelRuntime::Selection selected{"llama-cpp-server", "http://127.0.0.1:18080", "chosen",
                                          false};
    QList<ModelLibraryEntry> entries;
    QList<QPointer<HealthReply>> probes;
    QList<ProcessRequest> starts;
    QStringList terminated;
    ProcessExecutor::StateCallback onProcessState;
    ProcessRecord process;
    QString published;
    int refreshed = 0;
    std::unique_ptr<LocalModelRuntime> runtime;
    Fixture() {
        QFile file(workspace.filePath("synthetic.gguf"));
        if (!file.open(QIODevice::WriteOnly)) {
            qFatal("Cannot create disposable fixture");
        }
        file.write("GGUF synthetic fixture; never loaded");
        file.close();
        ModelLibraryEntry entry;
        entry.format = "GGUF";
        entry.localFile = file.fileName();
        entry.nativeModelId = "chosen";
        entries.append(entry);
        LocalModelRuntime::Dependencies dependencies;
        dependencies.automaticPolling = false;
        dependencies.selection = [this] { return selected; };
        dependencies.entries = [this] { return entries; };
        dependencies.publishStatus = [this](const QString& status) { published = status; };
        dependencies.refreshDiscovery = [this] { ++refreshed; };
        dependencies.get = [this](const QNetworkRequest& request) {
            auto* reply = new HealthReply(request, &replies);
            probes.append(reply);
            return reply;
        };
        dependencies.findExecutable = [] { return QStringLiteral("synthetic-llama-server"); };
        dependencies.start = [this](const ProcessRequest& request,
                                    ProcessExecutor::StateCallback callback) {
            starts.append(request);
            onProcessState = std::move(callback);
            process.processId = "owned-fixture-handle";
            if (process.state != ProcessState::Failed) {
                process.state = ProcessState::Running;
            }
            return process.processId;
        };
        dependencies.record = [this](const QString& id) {
            if (id != process.processId) {
                qFatal("Unexpected process handle");
            }
            return process;
        };
        dependencies.terminate = [this](const QString& id) {
            if (id != process.processId) {
                qFatal("External process authority attempted");
            }
            terminated.append(id);
            return true;
        };
        runtime = std::make_unique<LocalModelRuntime>(std::move(dependencies));
    }
    ~Fixture() {
        runtime.reset();
        QFile file(workspace.filePath("synthetic.gguf"));
        const bool intact = file.open(QIODevice::ReadOnly) &&
                            file.readAll() == "GGUF synthetic fixture; never loaded";
        QTest::qVerify(intact, "Synthetic GGUF remains unchanged", "", __FILE__, __LINE__);
        QTest::qVerify(QDir(workspace.path()).entryList(QDir::Files | QDir::Hidden) ==
                           QStringList{"synthetic.gguf"},
                       "Only the allowlisted fixture artifact exists", "", __FILE__, __LINE__);
    }
    void health(int code, QNetworkReply::NetworkError error = QNetworkReply::NoError) {
        runtime->ensureRunning();
        if (probes.isEmpty() || !probes.last()) {
            qFatal("Expected a pending health probe");
        }
        probes.last()->finish(code, error);
    }
    void start() {
        health(0, QNetworkReply::ConnectionRefusedError);
    }
    void ready() {
        health(200);
        if (probes.last()->url().path() != "/props") {
            qFatal("Expected runtime identity probe");
        }
        probes.last()->finish(200, QNetworkReply::NoError,
                              R"({"model_alias":"chosen","is_sleeping":false})");
    }
};
} // namespace

class LocalModelRuntimeTest final : public QObject {
    Q_OBJECT
private slots:
    void health200DoesNotVerifyAlias() {
        Fixture f;
        f.health(200);
        QCOMPARE(f.probes.size(), 2);
        QCOMPARE(f.probes.first()->url().path(), QString("/health"));
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Unknown);
        QCOMPARE(f.refreshed, 0);
        QVERIFY(f.starts.isEmpty());
        f.probes.last()->finish(200, QNetworkReply::NoError,
                                R"({"model_alias":"foreign","is_sleeping":false})");
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::MissingModel);
        QVERIFY(f.runtime->status() != "llama.cpp is ready.");
        QCOMPARE(f.selected.model, QString("chosen"));
        QVERIFY(f.terminated.isEmpty());
        QCOMPARE(f.refreshed, 0);
    }
    void matchingRuntimeIdentityIsReady() {
        Fixture f;
        f.ready();
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Ready);
        QCOMPARE(f.runtime->status(), QString("llama.cpp is ready."));
        QCOMPARE(f.published, f.runtime->status());
        QCOMPARE(f.refreshed, 1);
        QCOMPARE(f.probes.size(), 2);
        const auto request = f.probes.last()->request();
        QCOMPARE(request.url().path(), QString("/props"));
        QCOMPARE(request.url().query(), QString("autoload=false"));
        QCOMPARE(request.transferTimeout(), 1500);
        QCOMPARE(request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(),
                 int(QNetworkRequest::ManualRedirectPolicy));
        QVERIFY(f.starts.isEmpty());
        QVERIFY(f.terminated.isEmpty());
        QCOMPARE(f.selected.model, QString("chosen"));
    }
    void emptySelectionChoosesFirstGguf_data() {
        QTest::addColumn<QString>("model");
        QTest::newRow("empty") << QString{};
        QTest::newRow("whitespace") << QString("  ");
    }
    void emptySelectionChoosesFirstGguf() {
        QFETCH(QString, model);
        Fixture f;
        f.selected.model = model;
        f.runtime->ensureRunning();
        QCOMPARE(f.starts.size(), 0);
        QCOMPARE(f.probes.size(), 0);
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::MissingModel);
        QCOMPARE(f.selected.model, model);
        QVERIFY(f.terminated.isEmpty());
        // An explicit selection still starts exactly that model with the original permission.
        f.selected.model = "chosen";
        f.start();
        QCOMPARE(f.starts.size(), 1);
        QCOMPARE(f.starts.first().arguments.value(3), QString("chosen"));
        QVERIFY(f.starts.first().unconfinedPermitted);
        QCOMPARE(f.starts.first().timeoutMs, 0);
        QCOMPARE(f.starts.first().program, QString("synthetic-llama-server"));
    }
    void failedProbeRetainsReady_data() {
        QTest::addColumn<bool>("owned");
        QTest::addColumn<int>("code");
        QTest::addColumn<int>("error");
        QTest::newRow("shared-http503") << false << 503 << int(QNetworkReply::NoError);
        QTest::newRow("shared-http500") << false << 500 << int(QNetworkReply::NoError);
        QTest::newRow("owned-connection-refused")
            << true << 0 << int(QNetworkReply::ConnectionRefusedError);
        QTest::newRow("shared-timeout") << false << 0 << int(QNetworkReply::TimeoutError);
        QTest::newRow("owned-timeout") << true << 0 << int(QNetworkReply::TimeoutError);
        QTest::newRow("http200-with-error") << false << 200 << int(QNetworkReply::TimeoutError);
    }
    void failedProbeRetainsReady() {
        QFETCH(bool, owned);
        QFETCH(int, code);
        QFETCH(int, error);
        Fixture f;
        if (owned) {
            f.start();
        }
        f.ready();
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Ready);
        f.health(code, QNetworkReply::NetworkError(error));
        QCOMPARE(f.probes.size(), owned ? 4 : 3);
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Unavailable);
        QVERIFY(f.runtime->status() != "llama.cpp is ready.");
        QCOMPARE(f.selected.model, QString("chosen"));
        QCOMPARE(f.starts.size(), owned ? 1 : 0);
        QVERIFY(f.terminated.isEmpty());
        // A fresh successful observation, rather than an old cached result, restores Ready.
        f.ready();
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Ready);
        QCOMPARE(f.refreshed, 2);
    }
    void providerSwitchRetainsOwnedChild() {
        Fixture f;
        f.start();
        QCOMPARE(f.starts.size(), 1);
        f.selected.provider = "ollama";
        f.runtime->ensureRunning();
        QCOMPARE(f.probes.size(), 1);
        QEXPECT_FAIL("", "P3-02 D4: switching provider returns before owned cleanup", Continue);
        QCOMPARE(f.terminated.size(), 1);
        f.runtime.reset();
    }
    void oldEndpointCanPublishReady() {
        Fixture f;
        f.runtime->ensureRunning();
        f.runtime->ensureRunning();
        QCOMPARE(f.probes.size(), 1);
        f.selected.endpoint = "http://127.0.0.1:18081";
        f.probes.first()->finish(200);
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Unknown);
        QVERIFY(f.runtime->status() != "llama.cpp is ready.");
        QCOMPARE(f.probes.size(), 1); // Old endpoint cannot dispatch identity follow-up.
        QVERIFY(f.starts.isEmpty());
        f.ready();
        QCOMPARE(f.probes.last()->url().port(), 18081);
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Ready);
        QCOMPARE(f.refreshed, 1);
        QVERIFY(f.starts.isEmpty());
        QVERIFY(f.terminated.isEmpty());
    }
    void endpointSwitchDoesNotReconcileOwnedChild() {
        Fixture f;
        f.start();
        f.selected.endpoint = "http://127.0.0.1:18081";
        f.health(0, QNetworkReply::ConnectionRefusedError);
        QCOMPARE(f.probes.last()->url().port(), 18081);
        QCOMPARE(f.starts.size(), 1);
        QEXPECT_FAIL("", "P3-02 D5b: same-model endpoint switch leaves old owned child", Continue);
        QCOMPARE(f.terminated.size(), 1);
    }
    void modelSwitchRespectsBusyAndOwnedHandle() {
        Fixture f;
        f.start();
        f.selected.model = "next";
        f.selected.busy = true;
        f.health(503);
        QVERIFY(f.terminated.isEmpty());
        f.selected.busy = false;
        f.runtime->ensureRunning();
        QCOMPARE(f.terminated, QStringList{"owned-fixture-handle"});
        QCOMPARE(f.starts.size(), 1);
    }
    void processExitIsObservedWithoutImplicitRestart_data() {
        QTest::addColumn<int>("state");
        QTest::newRow("exited") << int(ProcessState::Exited);
        QTest::newRow("failed") << int(ProcessState::Failed);
        QTest::newRow("cancelled") << int(ProcessState::Cancelled);
    }
    void processExitIsObservedWithoutImplicitRestart() {
        QFETCH(int, state);
        Fixture f;
        f.start();
        f.process.state = ProcessState(state);
        f.onProcessState(f.process);
        QCOMPARE(f.runtime->status(), QString("llama.cpp stopped."));
        QCOMPARE(f.refreshed, 1);
        f.health(0, QNetworkReply::ConnectionRefusedError);
        QCOMPARE(f.starts.size(), 1); // attemptedKey suppresses automatic same-key restart.
        QVERIFY(f.terminated.isEmpty());
    }
    void sharedListenerAndLateShutdownNeverStartOrAdopt() {
        Fixture f;
        f.health(503);
        QVERIFY(f.starts.isEmpty());
        f.runtime->ensureRunning();
        f.runtime->beginShutdown();
        f.probes.last()->finish(200);
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Unknown);
        QVERIFY(f.starts.isEmpty());
        QVERIFY(f.terminated.isEmpty());
        f.runtime->ensureRunning();
        QCOMPARE(f.probes.size(), 2);
        f.runtime.reset();
    }
    void unsupportedEndpointHasNoSideEffects_data() {
        QTest::addColumn<QString>("endpoint");
        QTest::newRow("remote") << QString("http://example.invalid:18080");
        QTest::newRow("userinfo") << QString("http://synthetic@127.0.0.1:18080");
        QTest::newRow("https") << QString("https://127.0.0.1:18080");
    }
    void unsupportedEndpointHasNoSideEffects() {
        QFETCH(QString, endpoint);
        Fixture f;
        f.selected.endpoint = endpoint;
        f.runtime->ensureRunning();
        QVERIFY(f.probes.isEmpty());
        QVERIFY(f.starts.isEmpty());
        QVERIFY(f.terminated.isEmpty());
    }
    void supersededSelectionDoesNotPublishReady_data() {
        QTest::addColumn<bool>("providerSwitch");
        QTest::newRow("provider-switch") << true;
        QTest::newRow("model-switch") << false;
    }
    void supersededSelectionDoesNotPublishReady() {
        QFETCH(bool, providerSwitch);
        Fixture f;
        f.runtime->ensureRunning();
        if (providerSwitch) {
            f.selected.provider = "lm-studio";
        } else {
            f.selected.model = "next";
        }
        f.probes.first()->finish(200);
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Unknown);
        QVERIFY(f.starts.isEmpty());
        QCOMPARE(f.refreshed, 0);
    }
    void identityWithoutAuthoritativeEvidence_data() {
        QTest::addColumn<QByteArray>("body");
        QTest::addColumn<int>("code");
        QTest::addColumn<int>("error");
        QTest::addColumn<int>("expected");
        const auto unknown = int(RuntimeReadinessState::Unknown);
        QTest::newRow("malformed") << QByteArray("{") << 200 << 0 << unknown;
        QTest::newRow("array") << QByteArray("[]") << 200 << 0 << unknown;
        QTest::newRow("health-only") << QByteArray(R"({"status":"ok"})") << 200 << 0 << unknown;
        QTest::newRow("catalog-only")
            << QByteArray(R"({"data":[{"id":"chosen"}]})") << 200 << 0 << unknown;
        QTest::newRow("missing-sleep-state")
            << QByteArray(R"({"model_alias":"chosen"})") << 200 << 0 << unknown;
        QTest::newRow("sleep-state-string")
            << QByteArray(R"({"model_alias":"chosen","is_sleeping":"false"})") << 200 << 0
            << unknown;
        QTest::newRow("alias-not-string")
            << QByteArray(R"({"model_alias":17,"is_sleeping":false})") << 200 << 0 << unknown;
        QTest::newRow("empty-alias")
            << QByteArray(R"({"model_alias":"","is_sleeping":false})") << 200 << 0 << unknown;
        const QByteArray identity(R"({"model_alias":"chosen","is_sleeping":false})");
        QTest::newRow("unsupported-props") << identity << 404 << 0 << unknown;
        QTest::newRow("props-timeout")
            << identity << 200 << int(QNetworkReply::TimeoutError) << unknown;
        QTest::newRow("props-redirect") << identity << 302 << 0 << unknown;
        QTest::newRow("sleeping") << QByteArray(R"({"model_alias":"chosen","is_sleeping":true})")
                                  << 200 << 0 << int(RuntimeReadinessState::Unavailable);
    }
    void identityWithoutAuthoritativeEvidence() {
        QFETCH(QByteArray, body);
        QFETCH(int, code);
        QFETCH(int, error);
        QFETCH(int, expected);
        Fixture f;
        f.ready();
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Ready);
        f.health(200);
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Unknown);
        f.probes.last()->finish(code, QNetworkReply::NetworkError(error), body);
        QCOMPARE(int(f.runtime->readiness()), expected);
        QVERIFY(f.runtime->status() != "llama.cpp is ready.");
        QCOMPARE(f.refreshed, 1);
        QCOMPARE(f.selected.model, QString("chosen"));
        QVERIFY(f.starts.isEmpty());
        QVERIFY(f.terminated.isEmpty());
    }
    void oldRepliesCannotOverwriteCurrentObservation_data() {
        QTest::addColumn<bool>("identityPhase");
        QTest::addColumn<bool>("currentReady");
        QTest::newRow("old-health-new-failure") << false << false;
        QTest::newRow("old-identity-new-failure") << true << false;
        QTest::newRow("old-health-new-ready") << false << true;
        QTest::newRow("old-identity-new-ready") << true << true;
    }
    void oldRepliesCannotOverwriteCurrentObservation() {
        QFETCH(bool, identityPhase);
        QFETCH(bool, currentReady);
        Fixture f;
        f.runtime->ensureRunning();
        if (identityPhase) {
            f.probes.last()->finish(200);
        }
        const auto old = f.probes.last();
        f.selected.endpoint = "http://127.0.0.1:18081";
        if (currentReady) {
            f.ready();
        } else {
            f.health(503);
        }
        const auto accepted = f.runtime->status();
        const auto state = f.runtime->readiness();
        const auto count = f.probes.size();
        const auto refreshes = f.refreshed;
        old->finish(200, QNetworkReply::NoError, R"({"model_alias":"chosen","is_sleeping":false})");
        QCOMPARE(f.runtime->status(), accepted);
        QCOMPARE(f.runtime->readiness(), state);
        QCOMPARE(f.probes.size(), count);
        QCOMPARE(f.refreshed, refreshes);
        QCOMPARE(old->aborts, 1);
        QVERIFY(f.starts.isEmpty());
        QVERIFY(f.terminated.isEmpty());
    }
    void sameSelectionOldRequestCannotResurrectReady() {
        Fixture f;
        f.ready();
        const auto oldIdentity = f.probes.last();
        f.health(503);
        const auto failedStatus = f.runtime->status();
        oldIdentity->finish(200, QNetworkReply::NoError,
                            R"({"model_alias":"chosen","is_sleeping":false})");
        QCOMPARE(f.runtime->status(), failedStatus);
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Unavailable);
        QCOMPARE(f.refreshed, 1);
        QCOMPARE(f.probes.size(), 3);
        QVERIFY(f.starts.isEmpty());
    }
    void endpointABAReturnDoesNotAcceptOldGeneration_data() {
        QTest::addColumn<bool>("identityPhase");
        QTest::newRow("health-pending") << false;
        QTest::newRow("identity-pending") << true;
    }
    void endpointABAReturnDoesNotAcceptOldGeneration() {
        QFETCH(bool, identityPhase);
        Fixture f;
        f.runtime->ensureRunning();
        if (identityPhase) {
            f.probes.last()->finish(200);
        }
        const auto oldA = f.probes.last();
        oldA->finishOnAbort = true; // Exercise synchronous finished during Qt abort.
        f.selected.endpoint = "http://127.0.0.1:18081";
        f.runtime->ensureRunning();
        const auto oldB = f.probes.last();
        f.selected.endpoint = "http://127.0.0.1:18080";
        f.runtime->ensureRunning();
        const auto currentA = f.probes.last();
        const auto count = f.probes.size();
        oldA->finish(200, QNetworkReply::NoError,
                     R"({"model_alias":"chosen","is_sleeping":false})");
        oldB->finish(200);
        QCOMPARE(f.probes.size(), count);
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Unknown);
        QCOMPARE(f.refreshed, 0);
        currentA->finish(200);
        f.probes.last()->finish(200, QNetworkReply::NoError,
                                R"({"model_alias":"chosen","is_sleeping":false})");
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Ready);
        QCOMPARE(f.refreshed, 1);
        QVERIFY(f.starts.isEmpty());
        QVERIFY(f.terminated.isEmpty());
    }
    void duplicateFinishedDoesNotRefreshOrStart() {
        Fixture f;
        f.ready();
        f.probes.first()->finish(200);
        f.probes.last()->finish(200, QNetworkReply::NoError,
                                R"({"model_alias":"chosen","is_sleeping":false})");
        QCOMPARE(f.probes.size(), 2);
        QCOMPARE(f.refreshed, 1);
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Ready);
        f.ready(); // Normal periodic recheck must not create a discovery refresh loop.
        QCOMPARE(f.refreshed, 1);
        QVERIFY(f.starts.isEmpty());
        QVERIFY(f.terminated.isEmpty());
    }
    void pendingIdentityAfterShutdownOrDestruction_data() {
        QTest::addColumn<bool>("destroy");
        QTest::newRow("shutdown") << false;
        QTest::newRow("destruction") << true;
    }
    void pendingIdentityAfterShutdownOrDestruction() {
        QFETCH(bool, destroy);
        Fixture f;
        f.health(200);
        const auto pending = f.probes.last();
        pending->finishOnAbort = true;
        if (destroy) {
            f.runtime.reset();
        } else {
            f.runtime->beginShutdown();
        }
        const auto accepted = f.published;
        pending->finish(200, QNetworkReply::NoError,
                        R"({"model_alias":"chosen","is_sleeping":false})");
        QCOMPARE(f.published, accepted);
        QCOMPARE(f.refreshed, 0);
        QCOMPARE(f.probes.size(), 2);
        QVERIFY(f.starts.isEmpty());
        QVERIFY(f.terminated.isEmpty());
    }
    void emptyOrUnavailableSelectionNeverSubstitutesOrStops_data() {
        QTest::addColumn<bool>("owned");
        QTest::newRow("owned-empty") << true;
        QTest::newRow("unavailable-explicit") << false;
    }
    void emptyOrUnavailableSelectionNeverSubstitutesOrStops() {
        QFETCH(bool, owned);
        Fixture f;
        if (owned) {
            f.start();
            f.selected.model.clear();
            f.runtime->ensureRunning();
        } else {
            f.selected.model = "not-registered";
            f.start();
        }
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::MissingModel);
        QCOMPARE(f.starts.size(), owned ? 1 : 0);
        QVERIFY(f.terminated.isEmpty());
        QCOMPARE(f.selected.model, owned ? QString{} : QString("not-registered"));
    }
    void synchronousFailedStartSetsUnavailable() {
        Fixture f;
        f.process.state = ProcessState::Failed;
        f.process.error = "Exec format error";
        f.start();
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Unavailable);
        QCOMPARE(f.starts.size(), 1);
        QVERIFY(f.terminated.isEmpty());
        // Verify subsequent ensureRunning does not restart in a loop due to attemptedKey_
        f.runtime->ensureRunning();
        QCOMPARE(f.starts.size(), 1);
    }
    void repeatedShutdownIsIdempotent() {
        Fixture f;
        f.health(200);
        f.runtime->beginShutdown();
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Unknown);
        f.runtime->beginShutdown();
        QCOMPARE(f.runtime->readiness(), RuntimeReadinessState::Unknown);
        f.runtime.reset();
    }
    void duplicateEnsureRunningWhileProbingDropsExtraRequests() {
        Fixture f;
        f.runtime->ensureRunning();
        QCOMPARE(f.probes.size(), 1);
        f.runtime->ensureRunning();
        QCOMPARE(f.probes.size(), 1);
        f.probes.last()->finish(200);
        QCOMPARE(f.probes.size(), 2);
        f.runtime->ensureRunning();
        QCOMPARE(f.probes.size(), 2);
    }
    void removalDuringActualUseRemainsBlocked() {
        QSKIP("No model-residency lease/removal contract exists; a live provider handle is not "
              "proof of active model use. Source-only D6.");
    }
};
QTEST_GUILESS_MAIN(LocalModelRuntimeTest)
#include "test_local_model_runtime.moc"
