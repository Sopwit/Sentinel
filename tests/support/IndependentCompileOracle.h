// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/runtime/ProcessExecutor.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QTemporaryDir>
#include <QTest>

namespace sentinel::test {
// Trusted test authority: no tool receipt, model command or model-supplied argv enters here.
class IndependentCompileOracle final {
public:
    enum class Mode { Normal, TestFailure, Nonzero, Timeout, BuildOnly };
    struct Step {
        QString stage;
        core::ProcessRecord process;
        QByteArray output;
        bool drained = false;
        bool succeeded() const {
            return drained && !process.timedOut && process.state == core::ProcessState::Exited &&
                   process.exitCode == 0 && process.systemPid > 0;
        }
    };
    struct Result {
        QList<Step> steps;
        QByteArray expectedOutput;
        QString workspaceRoot;
        QHash<QString, QByteArray> inputs;
        bool matchesSources(const QString& root) const {
            if (QFileInfo(root).canonicalFilePath() != workspaceRoot || inputs.isEmpty())
                return false;
            for (auto it = inputs.cbegin(); it != inputs.cend(); ++it) {
                QFile file(root + '/' + it.key());
                if (!file.open(QIODevice::ReadOnly) ||
                    QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256) !=
                        it.value())
                    return false;
            }
            return true;
        }
        bool passed() const {
            if (steps.size() != 4 || steps.last().stage != QStringLiteral("test"))
                return false;
            const QStringList stages{"compile-source", "compile-trusted-test", "link", "test"};
            for (int i = 0; i < steps.size(); ++i)
                if (!steps[i].succeeded() || steps[i].stage != stages[i])
                    return false;
            return steps.last().output == expectedOutput;
        }
        QString summary() const {
            QStringList rows;
            for (const auto& step : steps)
                rows.append(QStringLiteral("%1: exit=%2 timeout=%3 drained=%4")
                                .arg(step.stage)
                                .arg(step.process.exitCode)
                                .arg(step.process.timedOut)
                                .arg(step.drained));
            return rows.join(QStringLiteral("; "));
        }
    };
    static QString unavailableReason() {
#ifndef SENTINEL_ORACLE_COMPILER
        return QStringLiteral("CMake compiler identity was not supplied to this test build");
#else
        if (!QFileInfo::exists(QString::fromUtf8(SENTINEL_ORACLE_COMPILER)))
            return QStringLiteral("Configured compiler is no longer installed");
        if (!QFileInfo::exists(QString::fromUtf8(SENTINEL_ORACLE_GIT)))
            return QStringLiteral("Git is unavailable for disposable repository initialization");
        const auto id = QString::fromUtf8(SENTINEL_ORACLE_COMPILER_ID);
        if (id != "GNU" && id != "Clang" && id != "AppleClang" && id != "MSVC")
            return QStringLiteral("Fixed oracle argv is not implemented for this compiler family");
        return {};
#endif
    }
    static Step initializeRepository(const QString& root) {
#ifdef SENTINEL_ORACLE_GIT
        return run("git-init", QString::fromUtf8(SENTINEL_ORACLE_GIT),
                   {"-c", "init.defaultBranch=fixture", "init", "--quiet", "--template="}, root);
#else
        return {};
#endif
    }
    Result verify(const QString& workspace, bool multiFile, Mode mode = Mode::Normal) const {
        Result result;
#ifdef SENTINEL_ORACLE_COMPILER
        QTemporaryDir outputs;
        if (!outputs.isValid() || !unavailableReason().isEmpty())
            return result;
        result.workspaceRoot = QFileInfo(workspace).canonicalFilePath();
        QStringList inputs{QStringLiteral("src/calc.cpp")};
        if (multiFile)
            inputs.append(QStringLiteral("include/calc.h"));
        for (const auto& name : inputs) {
            QFile file(workspace + '/' + name);
            if (!file.open(QIODevice::ReadOnly))
                return result;
            result.inputs.insert(
                name, QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256));
        }
        const bool msvc = QString::fromUtf8(SENTINEL_ORACLE_COMPILER_ID) == "MSVC";
        const auto compiler = QString::fromUtf8(SENTINEL_ORACLE_COMPILER);
        const auto source = workspace + "/src/calc.cpp";
        const auto object = outputs.filePath(msvc ? "calc.obj" : "calc.o");
        const auto driverObject = outputs.filePath(msvc ? "driver.obj" : "driver.o");
        const auto driverPath = outputs.filePath("trusted_driver.cpp");
        const auto executable = outputs.filePath("trusted_test.exe");
        const int expected = multiFile ? 6 : 5;
        result.expectedOutput = QByteArray("RESULT=") + QByteArray::number(expected) + '\n';
        const auto declaration = multiFile ? QByteArray("#include \"calc.h\"\n")
                                           : QByteArray("extern int add(int,int);\n");
        const auto call = multiFile ? QByteArray("add(1,2,3)") : QByteArray("add(2,3)");
        QByteArray driver = "#include <cstdio>\n" + declaration + "int main(){ int value=" + call +
                            "; std::printf(\"RESULT=%d\\n\",value); return value==" +
                            QByteArray::number(expected + (mode == Mode::TestFailure ? 1 : 0)) +
                            " ? 0 : 7; }\n";
        if (mode == Mode::Nonzero)
            driver = "#include <cstdio>\nint main(){std::puts(\"ALL_TESTS_PASSED\");return 23;}\n";
        QFile file(driverPath);
        if (!file.open(QIODevice::WriteOnly) || file.write(driver) != driver.size())
            return result;
        file.close();
        const auto compile = [&](const QString& input, const QString& output) {
            if (msvc)
                return QStringList{"/nologo",     "/std:c++20", "/EHsc",
                                   "/c",          input,        "/I" + workspace + "/include",
                                   "/Fo" + output};
            return QStringList{"-std=c++20", "-I",  workspace + "/include", "-c", input,
                               "-o",         output};
        };
        result.steps.append(
            run("compile-source", compiler, compile(source, object), outputs.path()));
        if (!result.steps.last().succeeded())
            return result;
        result.steps.append(run("compile-trusted-test", compiler, compile(driverPath, driverObject),
                                outputs.path()));
        if (!result.steps.last().succeeded())
            return result;
        const QStringList link =
            msvc ? QStringList{"/nologo", object, driverObject, "/Fe" + executable}
                 : QStringList{object, driverObject, "-o", executable};
        result.steps.append(run("link", compiler, link, outputs.path()));
        if (!result.steps.last().succeeded() || mode == Mode::BuildOnly)
            return result;
        result.steps.append(mode == Mode::Timeout
                                ? run("test", QCoreApplication::applicationFilePath(),
                                      {"--oracle-timeout-child"}, outputs.path(), 1000)
                                : run("test", executable, {}, outputs.path()));
#endif
        return result;
    }

private:
    static Step run(const QString& stage, const QString& program, const QStringList& arguments,
                    const QString& cwd, int timeout = 10000) {
        Step step;
        step.stage = stage;
        bool terminal = false;
        core::ProcessExecutor executor; // destroyed before callback captures, even on failure
        core::ProcessRequest request;
        request.program = program;
        request.arguments = arguments;
        request.workingDirectory = cwd;
        const auto host = QProcessEnvironment::systemEnvironment();
        for (const auto& name : {"PATH", "SystemRoot", "WINDIR", "TEMP", "TMP", "TMPDIR", "INCLUDE",
                                 "LIB", "LIBPATH", "SDKROOT", "DEVELOPER_DIR"})
            if (host.contains(name))
                request.environment.insert(name, host.value(name));
        request.environment.insert("CCACHE_DISABLE",
                                   "1"); // Execute the compiler behind configured wrappers.
        request.environment.insert("GIT_CONFIG_NOSYSTEM", "1");
        request.environment.insert("GIT_CONFIG_GLOBAL", cwd + "/no-global-config");
        request.environment.insert("LC_ALL", "C"); // Nonempty, explicitly allowlisted environment.
        request.timeoutMs = timeout;
        request.unconfinedPermitted = true;
        const auto id = executor.start(
            request,
            [&](const core::ProcessRecord& record) {
                if (record.state == core::ProcessState::Exited ||
                    record.state == core::ProcessState::Failed ||
                    record.state == core::ProcessState::Cancelled) {
                    step.process = record;
                    terminal = true;
                }
            },
            [&](const QString&, core::ProcessStream, const QByteArray& bytes) {
                step.output += bytes;
            });
        step.drained = QTest::qWaitFor([&] { return terminal; }, timeout + 5000);
        if (!step.drained) {
            executor.kill(id);
            (void)QTest::qWaitFor([&] { return terminal; }, 5000);
        }
        executor.shutdown(); // only this executor's children; no orphan on timeout/failed start
        return step;
    }
};
} // namespace sentinel::test
