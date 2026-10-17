// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Deterministic exit-contract tests for the legacy in-process C++ CLI
// (apps/sentinel-cli). Every provider interaction is served by a loopback
// QTcpServer fixture: no real model downloads, cloud API calls, daemon or
// personal settings are used. Each test runs the real sentinel-cli binary as
// a subprocess against an isolated temporary HOME/XDG profile.

#include "commands/ExitCodes.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>
#include <utility>

#if defined(Q_OS_UNIX)
#include <signal.h>
#include <sys/types.h>
#endif

using sentinel::cli::kExitFailure;
using sentinel::cli::kExitSuccess;
using sentinel::cli::kExitTimeout;
using sentinel::cli::kExitUsage;

namespace {

constexpr int kChatTimeoutOverrideMs = 1500;
constexpr int kPullIdleOverrideMs = 1500;

// Minimal loopback stand-in for the Ollama API surface the legacy CLI touches:
// GET /api/version (health), GET /api/tags (discovery), POST /api/generate
// (chat) and POST /api/pull (model pull). Stall flags keep a connection
// accepted but unanswered so bounded-wait contracts can be exercised.
class FakeOllamaServer final : public QObject {
public:
    explicit FakeOllamaServer(QObject* parent = nullptr) : QObject(parent) {
        connect(&server_, &QTcpServer::newConnection, this,
                [this]() { drainPendingConnections(); });
    }

    bool listen() {
        return server_.listen(QHostAddress::LocalHost, 0);
    }

    quint16 port() const {
        return server_.serverPort();
    }

    bool stallAll = false;
    bool stallGenerate = false;
    bool stallPull = false;
    bool pullHttpError = false;
    int pullRequests = 0;
    int generateRequests = 0;
    int tagRequests = 0;

private:
    void drainPendingConnections() {
        while (auto* socket = server_.nextPendingConnection()) {
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                buffer_[socket].append(socket->readAll());
                tryRespond(socket);
            });
        }
    }

    static QByteArray routeResponse(const QString& method, const QString& path) {
        const auto respond = [](int status, const QByteArray& body, const QByteArray& contentType) {
            return "HTTP/1.1 " + QByteArray::number(status) +
                   (status == 200 ? " OK\r\n" : " Error\r\n") + "Content-Type: " + contentType +
                   "\r\nContent-Length: " + QByteArray::number(body.size()) +
                   "\r\nConnection: close\r\n\r\n" + body;
        };
        if (method == QLatin1String("GET") && path == QLatin1String("/api/version")) {
            return respond(200, R"({"version":"0.0.0-fixture"})", "application/json");
        }
        if (method == QLatin1String("GET") && path == QLatin1String("/api/tags")) {
            return respond(
                200,
                R"({"models":[{"name":"fake:latest","size":1073741824,)"
                R"("modified_at":"2026-01-01T00:00:00Z","details":{"family":"fixture"}}]})",
                "application/json");
        }
        if (method == QLatin1String("POST") && path == QLatin1String("/api/generate")) {
            static const QByteArray generateBody =
                "{\"model\":\"fake:latest\",\"response\":\"Hello from the fixture.\","
                "\"done\":false}\n{\"response\":\"\",\"done\":true}\n";
            return respond(200, generateBody, "application/json");
        }
        if (method == QLatin1String("POST") && path == QLatin1String("/api/pull")) {
            static const QByteArray pullBody =
                "{\"status\":\"pulling manifest\"}\n{\"status\":\"success\"}\n";
            return respond(200, pullBody, "application/json");
        }
        return respond(404, R"({"error":"not found"})", "application/json");
    }

    void tryRespond(QTcpSocket* socket) {
        const auto& data = buffer_[socket];
        const auto headerEnd = data.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            return;
        }
        const auto headerBlock = data.left(headerEnd);
        const auto firstLine = headerBlock.left(headerBlock.indexOf("\r\n"));
        const auto parts = firstLine.split(' ');
        if (parts.size() < 2) {
            return;
        }
        const QString method = QString::fromLatin1(parts.at(0));
        const QString path = QString::fromLatin1(parts.at(1));

        qint64 contentLength = 0;
        const auto headerLines = QString::fromLatin1(headerBlock).split(QLatin1String("\r\n"));
        for (const auto& rawLine : headerLines) {
            const auto line = rawLine.trimmed();
            if (line.startsWith(QLatin1String("content-length:"), Qt::CaseInsensitive)) {
                contentLength = line.mid(line.indexOf(QLatin1Char(':')) + 1).trimmed().toLongLong();
            }
        }
        const auto bodyStart = headerEnd + 4;
        if (data.size() - bodyStart < contentLength) {
            return; // wait for the full request body
        }

        if (method == QLatin1String("GET") && path == QLatin1String("/api/tags")) {
            ++tagRequests;
        } else if (path == QLatin1String("/api/generate")) {
            ++generateRequests;
        } else if (path == QLatin1String("/api/pull")) {
            ++pullRequests;
        }

        const bool stall = stallAll || (stallGenerate && path == QLatin1String("/api/generate")) ||
                           (stallPull && path == QLatin1String("/api/pull"));
        if (stall) {
            return; // keep the connection open and never answer
        }

        QByteArray response;
        if (pullHttpError && path == QLatin1String("/api/pull")) {
            const QByteArray body = R"({"error":"manifest not found"})";
            response = "HTTP/1.1 500 Error\r\nContent-Type: application/json\r\n"
                       "Content-Length: " +
                       QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
        } else {
            response = routeResponse(method, path);
        }
        socket->write(response);
        socket->disconnectFromHost();
        buffer_.remove(socket);
    }

    QTcpServer server_;
    QHash<QTcpSocket*, QByteArray> buffer_;
};

} // namespace

namespace {

struct CliResult {
    bool started = false;
    bool finished = false;
    bool killedAfterTimeout = false;
    int exitCode = -1;
    QProcess::ExitStatus exitStatus = QProcess::NormalExit;
    QByteArray standardOutput;
    QByteArray standardError;
};

} // namespace

class LegacyCliExitCodesTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void usageWithoutArgsReturnsSuccess();
    void unknownCommandReturnsUsageError();
    void chatWithoutPromptReturnsUsageError();
    void configUnknownKeyReturnsUsageError();
    void modelPullBlankNameReturnsUsageError();

    void statusAndConfigSucceedWithoutConfiguration();
    void modelListSucceedsAgainstFixture();
    void modelListFailsWhenEndpointIsRefused();

    void modelPullSucceedsAgainstFixture();
    void modelPullFailureReturnsNonzero();
    void modelPullHttpErrorReturnsNonzero();
    void modelPullStallReturnsTimeout();

    void chatWithoutConfigurationReturnsNonzero();
    void chatProviderFailureReturnsNonzero();
    void chatSucceedsAgainstFixture();
    void chatStallReturnsTimeout();
    void chatRefusedWithoutConversationStoreSetsFailure();

#if defined(Q_OS_UNIX)
    void interruptedPullExitsAbnormally();
#endif

private:
    CliResult runCli(const QStringList& args, const QHash<QString, QString>& extraEnv = {},
                     int waitMs = 60000);
    QString settingsPath() const;
    void writeSettings(const QJsonObject& values) const;
    void writeEndpointSettings(quint16 port) const;

    std::unique_ptr<QTemporaryDir> tempDir_;
    QProcessEnvironment baseEnv_;
    QString settingsSuffix_; // settings.json path relative to isolated profile root
};

namespace {

void applyTestEnvironment(QProcessEnvironment& env, const QString& root) {
    env.insert(QStringLiteral("HOME"), root);
    env.insert(QStringLiteral("CFFIXED_USER_HOME"), root);
    env.insert(QStringLiteral("XDG_CONFIG_HOME"), root + QStringLiteral("/config"));
    env.insert(QStringLiteral("XDG_DATA_HOME"), root + QStringLiteral("/share"));
    env.insert(QStringLiteral("XDG_STATE_HOME"), root + QStringLiteral("/state"));
    env.insert(QStringLiteral("XDG_CACHE_HOME"), root + QStringLiteral("/cache"));
    env.insert(QStringLiteral("XDG_DOCUMENTS_HOME"), root + QStringLiteral("/Documents"));
#if defined(Q_OS_WIN)
    env.insert(QStringLiteral("USERPROFILE"), root);
    env.insert(QStringLiteral("APPDATA"), root + QStringLiteral("/AppData/Roaming"));
    env.insert(QStringLiteral("LOCALAPPDATA"), root + QStringLiteral("/AppData/Local"));
#endif
}

} // namespace

QString LegacyCliExitCodesTest::settingsPath() const {
    return tempDir_ ? QDir(tempDir_->path()).filePath(settingsSuffix_) : QString{};
}

void LegacyCliExitCodesTest::writeSettings(const QJsonObject& values) const {
    const QFileInfo info(settingsPath());
    QDir().mkpath(info.absolutePath());
    QFile file(settingsPath());
    QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Truncate),
             qPrintable(QStringLiteral("cannot write %1").arg(settingsPath())));
    file.write(QJsonDocument(values).toJson(QJsonDocument::Indented));
}

void LegacyCliExitCodesTest::writeEndpointSettings(quint16 port) const {
    writeSettings(QJsonObject{
        {QStringLiteral("ollamaEndpoint"), QStringLiteral("http://127.0.0.1:%1").arg(port)},
    });
}

void LegacyCliExitCodesTest::initTestCase() {
    QTemporaryDir probeDir;
    QVERIFY(probeDir.isValid());

    const auto probeRoot = probeDir.path();
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    applyTestEnvironment(env, probeRoot);
    QDir().mkpath(probeRoot + QStringLiteral("/Documents"));
    QDir().mkpath(probeRoot + QStringLiteral("/config"));

    QProcess probe;
    probe.setProcessEnvironment(env);
    probe.start(QStringLiteral(SENTINEL_CLI_BINARY), {QStringLiteral("status")});
    QVERIFY(probe.waitForStarted(15000));
    QVERIFY2(probe.waitForFinished(60000),
             "sentinel-cli status did not finish during test bootstrap");
    QCOMPARE(probe.exitCode(), 0);

    const auto lines = probe.readAllStandardOutput().split('\n');
    QString settingsFile;
    for (const auto& line : lines) {
        if (line.startsWith("Settings File:")) {
            const auto text = QString::fromUtf8(line);
            settingsFile = text.mid(text.indexOf(QLatin1Char(':')) + 1).trimmed();
            break;
        }
    }
    QVERIFY2(!settingsFile.isEmpty(), "status output did not report its settings file path");
    const auto cleanProbeRoot = QDir::fromNativeSeparators(probeRoot);
    const auto cleanSettingsFile = QDir::fromNativeSeparators(settingsFile);
    QVERIFY2(
        cleanSettingsFile.startsWith(cleanProbeRoot),
        "settings file is not rooted in isolated test directory; test isolation would be invalid");
    settingsSuffix_ = cleanSettingsFile.mid(cleanProbeRoot.size() + 1);
    QVERIFY2(QFileInfo(settingsSuffix_).isRelative(), "settings suffix must be relative");
}

void LegacyCliExitCodesTest::init() {
    tempDir_ = std::make_unique<QTemporaryDir>();
    QVERIFY(tempDir_->isValid());

    const auto root = tempDir_->path();
    baseEnv_ = QProcessEnvironment::systemEnvironment();
    applyTestEnvironment(baseEnv_, root);
    baseEnv_.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
    // Deterministic deadlines: never inherit ambient test-seam overrides.
    baseEnv_.remove(QStringLiteral("SENTINEL_CLI_CHAT_TIMEOUT_MS"));
    baseEnv_.remove(QStringLiteral("SENTINEL_CLI_PULL_IDLE_TIMEOUT_MS"));
    QVERIFY(QDir().mkpath(root + QStringLiteral("/Documents")));
    QVERIFY(QDir().mkpath(root + QStringLiteral("/config")));
}

void LegacyCliExitCodesTest::cleanup() {
    tempDir_.reset();
}

CliResult LegacyCliExitCodesTest::runCli(const QStringList& args,
                                         const QHash<QString, QString>& extraEnv, int waitMs) {
    CliResult result;
    auto env = baseEnv_;
    for (auto it = extraEnv.cbegin(); it != extraEnv.cend(); ++it) {
        env.insert(it.key(), it.value());
    }

    QProcess process;
    process.setProcessEnvironment(env);
    process.start(QStringLiteral(SENTINEL_CLI_BINARY), args);
    result.started = process.waitForStarted(15000);
    if (!result.started) {
        return result;
    }
    // Wait through the Qt event loop instead of QProcess::waitForFinished():
    // the in-process Ollama fixture in this test process must be able to
    // accept and answer connections while the CLI subprocess runs. A blocking
    // waitForFinished() would starve the fixture's event loop, so every
    // fixture interaction would silently time out in the CLI.
    result.finished =
        QTest::qWaitFor([&process]() { return process.state() == QProcess::NotRunning; }, waitMs);
    if (!result.finished) {
        result.killedAfterTimeout = true;
        process.kill();
        process.waitForFinished(5000);
    }
    result.exitCode = process.exitCode();
    result.exitStatus = process.exitStatus();
    result.standardOutput = process.readAllStandardOutput();
    result.standardError = process.readAllStandardError();
    return result;
}

// ── Usage / argument contract (exit 1) ───────────────────────────────────────

void LegacyCliExitCodesTest::usageWithoutArgsReturnsSuccess() {
    const auto result = runCli({});
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY(!result.killedAfterTimeout);
    QCOMPARE(result.exitCode, kExitSuccess);
    QVERIFY(result.standardOutput.contains("Usage: sentinel-cli <command>"));
}

void LegacyCliExitCodesTest::unknownCommandReturnsUsageError() {
    const auto result = runCli({QStringLiteral("frobnicate")});
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY(!result.killedAfterTimeout);
    QCOMPARE(result.exitCode, kExitUsage);
    QVERIFY(result.standardError.contains("Unknown command: frobnicate"));
}

void LegacyCliExitCodesTest::chatWithoutPromptReturnsUsageError() {
    const auto missing = runCli({QStringLiteral("chat")});
    QVERIFY(missing.started);
    QVERIFY(missing.finished);
    QVERIFY(!missing.killedAfterTimeout);
    QCOMPARE(missing.exitCode, kExitUsage);
    QVERIFY(missing.standardError.contains("Usage: sentinel-cli chat <prompt>"));

    // Whitespace-only prompts are the same class of argument error.
    const auto blank = runCli({QStringLiteral("chat"), QStringLiteral("   ")});
    QVERIFY(blank.started);
    QVERIFY(blank.finished);
    QVERIFY(!blank.killedAfterTimeout);
    QCOMPARE(blank.exitCode, kExitUsage);
    QVERIFY(blank.standardError.contains("Usage: sentinel-cli chat <prompt>"));
}

void LegacyCliExitCodesTest::configUnknownKeyReturnsUsageError() {
    const auto result =
        runCli({QStringLiteral("config"), QStringLiteral("get"), QStringLiteral("bogusKey")});
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY(!result.killedAfterTimeout);
    QCOMPARE(result.exitCode, kExitUsage);
    QVERIFY(result.standardError.contains("Unknown configuration key: bogusKey"));
}

void LegacyCliExitCodesTest::modelPullBlankNameReturnsUsageError() {
    // A whitespace-only model id previously reached OllamaModelPuller::pull(),
    // which silently returned without emitting pullFinished: the CLI then waited
    // forever in its event loop. It must be rejected as a usage error instead.
    const auto result =
        runCli({QStringLiteral("model"), QStringLiteral("pull"), QStringLiteral("   ")}, {}, 20000);
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY2(!result.killedAfterTimeout,
             "blank model pull argument must fail fast instead of waiting indefinitely");
    QCOMPARE(result.exitCode, kExitUsage);
    QVERIFY(result.standardError.contains("Usage: sentinel-cli model pull"));
}

// ── Success contract (exit 0) ────────────────────────────────────────────────

void LegacyCliExitCodesTest::statusAndConfigSucceedWithoutConfiguration() {
    // Fresh profile: no settings.json exists yet. Informational commands must
    // still succeed with defaults instead of failing or crashing.
    QVERIFY(!QFile::exists(settingsPath()));

    const auto status = runCli({QStringLiteral("status")});
    QVERIFY(status.started);
    QVERIFY(status.finished);
    QVERIFY(!status.killedAfterTimeout);
    QCOMPARE(status.exitCode, kExitSuccess);
    QVERIFY(status.standardOutput.contains("Settings File:"));

    const auto config = runCli({QStringLiteral("config"), QStringLiteral("get")});
    QVERIFY(config.started);
    QVERIFY(config.finished);
    QVERIFY(!config.killedAfterTimeout);
    QCOMPARE(config.exitCode, kExitSuccess);
    QVERIFY(config.standardOutput.contains("ollamaEndpoint = "));
}

void LegacyCliExitCodesTest::modelListSucceedsAgainstFixture() {
    FakeOllamaServer fixture;
    QVERIFY(fixture.listen());
    writeEndpointSettings(fixture.port());

    const auto result = runCli({QStringLiteral("model"), QStringLiteral("list")});
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY(!result.killedAfterTimeout);
    QCOMPARE(result.exitCode, kExitSuccess);
    QVERIFY(result.standardOutput.contains("Installed models:"));
    QVERIFY(result.standardOutput.contains("fake:latest"));
}

void LegacyCliExitCodesTest::modelPullSucceedsAgainstFixture() {
    FakeOllamaServer fixture;
    QVERIFY(fixture.listen());
    writeEndpointSettings(fixture.port());

    const auto result =
        runCli({QStringLiteral("model"), QStringLiteral("pull"), QStringLiteral("fake:latest")});
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY(!result.killedAfterTimeout);
    QCOMPARE(result.exitCode, kExitSuccess);
    QVERIFY(result.standardOutput.contains("Pull completed: fake:latest"));
}

void LegacyCliExitCodesTest::chatSucceedsAgainstFixture() {
    FakeOllamaServer fixture;
    QVERIFY(fixture.listen());
    writeSettings(QJsonObject{
        {QStringLiteral("ollamaEndpoint"),
         QStringLiteral("http://127.0.0.1:%1").arg(fixture.port())},
        {QStringLiteral("selectedRuntimeProvider"), QStringLiteral("ollama")},
        {QStringLiteral("selectedModel.ollama"), QStringLiteral("fake:latest")},
    });

    const auto result = runCli({QStringLiteral("chat"), QStringLiteral("hello")}, {}, 30000);
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY(!result.killedAfterTimeout);
    QCOMPARE(result.exitCode, kExitSuccess);
    QVERIFY(result.standardOutput.contains("Hello from the fixture"));
    QVERIFY(fixture.generateRequests >= 1);
}

// ── Failure contract (exit 2) ────────────────────────────────────────────────

void LegacyCliExitCodesTest::modelListFailsWhenEndpointIsRefused() {
    // Bind then close a loopback port so the endpoint is guaranteed refused.
    quint16 refusedPort = 0;
    {
        QTcpServer closer;
        QVERIFY(closer.listen(QHostAddress::LocalHost, 0));
        refusedPort = closer.serverPort();
    }
    writeEndpointSettings(refusedPort);

    const auto result = runCli({QStringLiteral("model"), QStringLiteral("list")}, {}, 30000);
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY(!result.killedAfterTimeout);
    QCOMPARE(result.exitCode, kExitFailure);
    QVERIFY(result.standardError.contains("Model discovery failed"));
}

void LegacyCliExitCodesTest::modelPullFailureReturnsNonzero() {
    quint16 refusedPort = 0;
    {
        QTcpServer closer;
        QVERIFY(closer.listen(QHostAddress::LocalHost, 0));
        refusedPort = closer.serverPort();
    }
    writeEndpointSettings(refusedPort);

    // Defect P0.1/F03: this path printed "Pull failed" but returned 0.
    const auto result =
        runCli({QStringLiteral("model"), QStringLiteral("pull"), QStringLiteral("fake:latest")}, {},
               30000);
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY(!result.killedAfterTimeout);
    QCOMPARE(result.exitCode, kExitFailure);
    QVERIFY(result.standardOutput.contains("Pull failed: fake:latest"));
    QVERIFY(result.standardError.contains("sentinel-cli: model pull failed"));
}

void LegacyCliExitCodesTest::modelPullHttpErrorReturnsNonzero() {
    FakeOllamaServer fixture;
    QVERIFY(fixture.listen());
    fixture.pullHttpError = true;
    writeEndpointSettings(fixture.port());

    const auto result =
        runCli({QStringLiteral("model"), QStringLiteral("pull"), QStringLiteral("fake:latest")}, {},
               30000);
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY(!result.killedAfterTimeout);
    QCOMPARE(result.exitCode, kExitFailure);
    QVERIFY(result.standardOutput.contains("Pull failed: fake:latest"));
}

void LegacyCliExitCodesTest::chatWithoutConfigurationReturnsNonzero() {
    // Fresh profile with the default selection (llama-cpp-server, no configured
    // model): the chat request cannot be routed and must fail, not report 0.
    const auto result = runCli({QStringLiteral("chat"), QStringLiteral("hello")}, {}, 30000);
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY(!result.killedAfterTimeout);
    QCOMPARE(result.exitCode, kExitFailure);
    QVERIFY(result.standardError.contains("sentinel-cli: chat failed"));
}

void LegacyCliExitCodesTest::chatProviderFailureReturnsNonzero() {
    // Configured Ollama provider and model, but the endpoint refuses connections.
    quint16 refusedPort = 0;
    {
        QTcpServer closer;
        QVERIFY(closer.listen(QHostAddress::LocalHost, 0));
        refusedPort = closer.serverPort();
    }
    writeSettings(QJsonObject{
        {QStringLiteral("ollamaEndpoint"), QStringLiteral("http://127.0.0.1:%1").arg(refusedPort)},
        {QStringLiteral("selectedRuntimeProvider"), QStringLiteral("ollama")},
        {QStringLiteral("selectedModel.ollama"), QStringLiteral("fake:latest")},
    });

    const auto result = runCli({QStringLiteral("chat"), QStringLiteral("hello")}, {}, 30000);
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY(!result.killedAfterTimeout);
    QCOMPARE(result.exitCode, kExitFailure);
    QVERIFY(result.standardError.contains("sentinel-cli: chat failed"));
}

void LegacyCliExitCodesTest::chatRefusedWithoutConversationStoreSetsFailure() {
    // Turn the conversation database path into a directory so
    // SQLiteConversationStore cannot open it, the chat conversation store
    // stays unavailable and sendMessage() refuses the request before any
    // provider work starts. This path previously ignored the refusal, waited
    // the full 30 s response deadline, printed the timeout marker and exited 0
    // (baseline evidence: exit 0 after ~30 s). Both possible document roots
    // are blocked because Qt resolves the documents location differently
    // across builds.
    const QStringList documentRoots{
        baseEnv_.value(QStringLiteral("XDG_DOCUMENTS_HOME")),
        baseEnv_.value(QStringLiteral("HOME")) + QStringLiteral("/Documents"),
#if defined(Q_OS_WIN)
        baseEnv_.value(QStringLiteral("USERPROFILE")) + QStringLiteral("/Documents"),
#endif
    };
    for (const auto& root : documentRoots) {
        if (!root.isEmpty()) {
            QVERIFY(QDir().mkpath(root + QStringLiteral("/Sentinel/conversations.sqlite3")));
        }
    }

    const auto result = runCli({QStringLiteral("chat"), QStringLiteral("hello")}, {}, 20000);
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY2(!result.killedAfterTimeout,
             "refused chat request must settle promptly instead of waiting for the deadline");
    QCOMPARE(result.exitCode, kExitFailure);
    QVERIFY(result.standardError.contains("sentinel-cli: chat request refused"));
}

// ── Timeout contract (exit 3) ────────────────────────────────────────────────

void LegacyCliExitCodesTest::modelPullStallReturnsTimeout() {
    FakeOllamaServer fixture;
    QVERIFY(fixture.listen());
    fixture.stallPull = true;
    writeEndpointSettings(fixture.port());

    // The pull endpoint accepts the request but never sends a byte: the CLI's
    // inactivity deadline must abort the pull and exit 3 (previously: hung
    // indefinitely in QEventLoop::exec()).
    const auto result =
        runCli({QStringLiteral("model"), QStringLiteral("pull"), QStringLiteral("fake:latest")},
               {{QStringLiteral("SENTINEL_CLI_PULL_IDLE_TIMEOUT_MS"),
                 QString::number(kPullIdleOverrideMs)}},
               30000);
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY2(!result.killedAfterTimeout,
             "stalled model pull must terminate at the inactivity deadline");
    QCOMPARE(result.exitCode, kExitTimeout);
    QVERIFY(result.standardError.contains("sentinel-cli: model pull stalled"));
    QCOMPARE(fixture.pullRequests, 1);
}

void LegacyCliExitCodesTest::chatStallReturnsTimeout() {
    FakeOllamaServer fixture;
    QVERIFY(fixture.listen());
    fixture.stallGenerate = true;
    writeSettings(QJsonObject{
        {QStringLiteral("ollamaEndpoint"),
         QStringLiteral("http://127.0.0.1:%1").arg(fixture.port())},
        {QStringLiteral("selectedRuntimeProvider"), QStringLiteral("ollama")},
        {QStringLiteral("selectedModel.ollama"), QStringLiteral("fake:latest")},
    });

    // Health and discovery succeed, so the chat request is accepted and a
    // provider worker starts; the worker then never delivers a response. The
    // response deadline must exit 3 (previously: exit 0 after printing the
    // timeout marker).
    const auto result = runCli(
        {QStringLiteral("chat"), QStringLiteral("hello")},
        {{QStringLiteral("SENTINEL_CLI_CHAT_TIMEOUT_MS"), QString::number(kChatTimeoutOverrideMs)}},
        30000);
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QVERIFY2(!result.killedAfterTimeout, "stalled chat must terminate at the response deadline");
    QCOMPARE(result.exitCode, kExitTimeout);
    QVERIFY(result.standardOutput.contains("[Timeout waiting for response]"));
    QVERIFY(result.standardError.contains("sentinel-cli: chat timed out"));
}

// ── Interruption (signal death, where supported) ─────────────────────────────

#if defined(Q_OS_UNIX)
void LegacyCliExitCodesTest::interruptedPullExitsAbnormally() {
    // The legacy CLI installs no signal handlers: an interrupted run must die
    // by signal (shell convention 128+SIGINT) and must never convert into a
    // successful exit through an asynchronous callback.
    FakeOllamaServer fixture;
    QVERIFY(fixture.listen());
    fixture.stallPull = true;
    writeEndpointSettings(fixture.port());

    QProcess process;
    process.setProcessEnvironment(baseEnv_);
    process.start(QStringLiteral(SENTINEL_CLI_BINARY),
                  {QStringLiteral("model"), QStringLiteral("pull"), QStringLiteral("fake:latest")});
    QVERIFY(process.waitForStarted(15000));
    QVERIFY(QTest::qWaitFor([&]() { return fixture.pullRequests >= 1; }, 10000));

    ::kill(static_cast<pid_t>(process.processId()), SIGINT);
    QVERIFY(process.waitForFinished(10000));
    QVERIFY2(process.exitStatus() == QProcess::CrashExit || process.exitCode() != 0,
             "interrupted pull must not report a successful exit");
}
#endif

QTEST_GUILESS_MAIN(LegacyCliExitCodesTest)

#include "test_legacy_cli_exit_codes.moc"
