// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/ProcessSandbox.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSet>

namespace sentinel::core {

namespace {

QString profileLiteral(QString path) {
    path.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    path.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QStringLiteral("\"") + path + QStringLiteral("\"");
}

QProcessEnvironment restrictedEnvironment(const QProcessEnvironment& source,
                                          const SandboxExecutionPlan& plan) {
    if (!plan.restrictedEnvironment) return source;
    QProcessEnvironment result;
    for (const auto& key : {"PATH", "LANG", "LC_ALL", "LC_CTYPE", "TERM", "TMPDIR", "TMP", "TEMP"}) {
        const auto name = QString::fromLatin1(key);
        if (source.contains(name)) result.insert(name, source.value(name));
    }
    if (!plan.temporaryDirectory.isEmpty()) {
        result.insert(QStringLiteral("TMPDIR"), plan.temporaryDirectory);
        result.insert(QStringLiteral("TMP"), plan.temporaryDirectory);
        result.insert(QStringLiteral("TEMP"), plan.temporaryDirectory);
    }
    return result;
}

} // namespace

PlatformProcessSandbox::PlatformProcessSandbox() {
#if defined(Q_OS_MACOS)
    launcher_ = QStandardPaths::findExecutable(QStringLiteral("sandbox-exec"));
#elif defined(Q_OS_LINUX)
    launcher_ = QStandardPaths::findExecutable(QStringLiteral("bwrap"));
#endif
}

SandboxLaunch PlatformProcessSandbox::prepare(const SandboxExecutionPlan& plan,
                                              const QString& program,
                                              const QStringList& arguments,
                                              const QProcessEnvironment& environment) const {
    SandboxLaunch launch;
    launch.environment = restrictedEnvironment(environment, plan);
    launch.result.backend =
#if defined(Q_OS_MACOS)
        QStringLiteral("macOS sandbox-exec");
#elif defined(Q_OS_LINUX)
        QStringLiteral("Linux bubblewrap");
#else
        QStringLiteral("Windows restricted process");
#endif
    if (launcher_.isEmpty()) {
        launch.result.failureCategory = QStringLiteral("BackendUnavailable");
        return launch;
    }
    const QFileInfo workdir(plan.workingDirectory);
    if (!workdir.exists() || !workdir.isDir() || workdir.canonicalFilePath().isEmpty()) {
        launch.result.enforcement = SandboxEnforcement::Failed;
        launch.result.failureCategory = QStringLiteral("InvalidWorkingDirectory");
        return launch;
    }
    const auto canonicalWorkdir = workdir.canonicalFilePath();
    const auto home = QFileInfo(QDir::homePath()).canonicalFilePath();
    if (canonicalWorkdir == QLatin1String("/") ||
        (!home.isEmpty() &&
         (canonicalWorkdir == home || home.startsWith(canonicalWorkdir + QLatin1Char('/'))))) {
        launch.result.enforcement = SandboxEnforcement::Failed;
        launch.result.failureCategory = QStringLiteral("WorkingDirectoryTooBroad");
        return launch;
    }
    if (!QFileInfo(program).isAbsolute() || !QFileInfo(program).isExecutable()) {
        launch.result.enforcement = SandboxEnforcement::Failed;
        launch.result.failureCategory = QStringLiteral("ExecutableUnavailable");
        return launch;
    }
    for (const auto& path : plan.readablePaths + plan.writablePaths) {
        if (!QFileInfo(path).isAbsolute()) {
            launch.result.enforcement = SandboxEnforcement::Failed;
            launch.result.failureCategory = QStringLiteral("InvalidAuthorizedPath");
            return launch;
        }
    }
#if defined(Q_OS_MACOS)
    if (plan.forbidDetachedChildren) {
        launch.result.enforcement = SandboxEnforcement::PartiallyEnforced;
        launch.result.failureCategory = QStringLiteral("DetachedProcessControlUnavailable");
        return launch;
    }
    QString profile = QStringLiteral("(version 1)(deny default)"
                                     "(allow process-exec)(allow process-fork)"
                                     "(allow signal (target self))"
                                     "(allow file-read* (subpath \"/System\")"
                                     "(subpath \"/usr\") (subpath \"/bin\")"
                                     "(subpath \"/sbin\") (subpath \"/Library\")"
                                     "(literal \"/dev/null\"))");
    profile += QStringLiteral("(allow file-read* (literal %1))").arg(profileLiteral(program));
    for (const auto& path : plan.readablePaths)
        profile += QStringLiteral("(allow file-read* (subpath %1))").arg(profileLiteral(path));
    for (const auto& path : plan.writablePaths)
        profile += QStringLiteral("(allow file-read* file-write* (subpath %1))")
                       .arg(profileLiteral(path));
    if (!plan.temporaryDirectory.isEmpty())
        profile += QStringLiteral("(allow file-read* file-write* (subpath %1))")
                       .arg(profileLiteral(plan.temporaryDirectory));
    if (plan.networkAllowed)
        profile += QStringLiteral("(allow network*)");
    launch.program = launcher_;
    launch.arguments = {QStringLiteral("-p"), profile, program};
    launch.arguments.append(arguments);
    launch.result.processTreeControlled = false;
#elif defined(Q_OS_LINUX)
    launch.program = launcher_;
    launch.arguments = {QStringLiteral("--die-with-parent"), QStringLiteral("--unshare-pid"),
                        QStringLiteral("--proc"), QStringLiteral("/proc"),
                        QStringLiteral("--dev"), QStringLiteral("/dev")};
    if (!plan.networkAllowed) launch.arguments.append(QStringLiteral("--unshare-net"));
    QSet<QString> created;
    const auto prepareParents = [&](const QString& path) {
        QString parent = QFileInfo(path).absolutePath();
        QStringList missing;
        while (parent != QLatin1String("/") && !created.contains(parent)) {
            missing.prepend(parent);
            parent = QFileInfo(parent).absolutePath();
        }
        for (const auto& item : missing) {
            launch.arguments.append({QStringLiteral("--dir"), item});
            created.insert(item);
        }
    };
    for (const auto& path : {"/usr", "/bin", "/sbin", "/lib", "/lib64"}) {
        if (QFileInfo::exists(QString::fromLatin1(path)))
            launch.arguments.append({QStringLiteral("--ro-bind"), QString::fromLatin1(path),
                                     QString::fromLatin1(path)});
    }
    for (const auto& path : {"/etc/ld.so.cache", "/etc/nsswitch.conf", "/etc/passwd",
                             "/etc/group"}) {
        if (QFileInfo::exists(QString::fromLatin1(path))) {
            prepareParents(QString::fromLatin1(path));
            launch.arguments.append({QStringLiteral("--ro-bind"), QString::fromLatin1(path),
                                     QString::fromLatin1(path)});
        }
    }
    for (const auto& path : plan.readablePaths) {
        prepareParents(path);
        launch.arguments.append({QStringLiteral("--ro-bind"), path, path});
    }
    for (const auto& path : plan.writablePaths) {
        prepareParents(path);
        launch.arguments.append({QStringLiteral("--bind"), path, path});
    }
    if (!plan.temporaryDirectory.isEmpty()) {
        prepareParents(plan.temporaryDirectory);
        launch.arguments.append({QStringLiteral("--bind"), plan.temporaryDirectory,
                                 plan.temporaryDirectory});
    }
    launch.arguments.append({QStringLiteral("--chdir"), workdir.canonicalFilePath(),
                             QStringLiteral("--"), program});
    launch.arguments.append(arguments);
    launch.result.processTreeControlled = true;
#endif
    launch.result.enforcement = SandboxEnforcement::Enforced;
    launch.result.networkDenied = !plan.networkAllowed;
    launch.result.filesystemRestricted = true;
    launch.permitted = true;
    return launch;
}

QString sandboxEnforcementName(SandboxEnforcement enforcement) {
    switch (enforcement) {
    case SandboxEnforcement::Enforced: return QStringLiteral("Enforced");
    case SandboxEnforcement::PartiallyEnforced: return QStringLiteral("Partially Enforced");
    case SandboxEnforcement::Unsupported: return QStringLiteral("Unsupported");
    case SandboxEnforcement::Failed: return QStringLiteral("Failed");
    }
    return QStringLiteral("Unsupported");
}

} // namespace sentinel::core
