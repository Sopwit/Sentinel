// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/ProcessSandbox.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSet>
#if defined(Q_OS_MACOS)
#include <mach-o/dyld.h>
#endif
#if defined(Q_OS_WIN)
#include "WindowsProcessSandbox.h"
#endif

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
#if defined(Q_OS_WIN)
    for (const auto& key : {"SystemRoot", "WINDIR"}) {
        const auto name = QString::fromLatin1(key);
        if (source.contains(name)) result.insert(name, source.value(name));
    }
#endif
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
    static const QString launcher = QStandardPaths::findExecutable(QStringLiteral("sandbox-exec"));
    launcher_ = launcher;
#elif defined(Q_OS_LINUX)
    static const QString launcher = QStandardPaths::findExecutable(QStringLiteral("bwrap"));
    launcher_ = launcher;
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
#if !defined(Q_OS_WIN)
    if (launcher_.isEmpty()) {
        launch.result.failureCategory = QStringLiteral("BackendUnavailable");
        return launch;
    }
#endif
    const QFileInfo workdir(plan.workingDirectory);
    if (!workdir.exists() || !workdir.isDir() || workdir.canonicalFilePath().isEmpty()) {
        launch.result.enforcement = SandboxEnforcement::Failed;
        launch.result.failureCategory = QStringLiteral("InvalidWorkingDirectory");
        return launch;
    }
    const auto canonicalWorkdir = workdir.canonicalFilePath();
    const auto home = QFileInfo(QDir::homePath()).canonicalFilePath();
    const auto pathCase =
#if defined(Q_OS_WIN)
        Qt::CaseInsensitive;
#else
        Qt::CaseSensitive;
#endif
    if (QDir(canonicalWorkdir).isRoot() ||
        (!home.isEmpty() &&
         (canonicalWorkdir.compare(home, pathCase) == 0 ||
          home.startsWith(canonicalWorkdir + QLatin1Char('/'), pathCase)))) {
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
    const auto temporaryRoot = QFileInfo(QDir::tempPath()).canonicalFilePath();
    const auto temporaryPath = QFileInfo(plan.temporaryDirectory).canonicalFilePath();
    if (temporaryRoot.isEmpty() || temporaryPath.isEmpty() ||
        !temporaryPath.startsWith(temporaryRoot + QLatin1Char('/'), pathCase)) {
        launch.result.enforcement = SandboxEnforcement::Failed;
        launch.result.failureCategory = QStringLiteral("InvalidTemporaryDirectory");
        return launch;
    }
#if defined(Q_OS_WIN)
    launch.program = program;
    launch.arguments = arguments;
    launch.result.environmentRestricted = plan.restrictedEnvironment;
    auto native = std::make_shared<WindowsSandboxState>();
    HANDLE currentToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY |
                                                 TOKEN_ASSIGN_PRIMARY, &currentToken)) {
        launch.result.enforcement = SandboxEnforcement::Failed;
        launch.result.failureCategory = QStringLiteral("TokenUnavailable");
        return launch;
    }
    BYTE administratorsSid[SECURITY_MAX_SID_SIZE];
    DWORD sidBytes = sizeof(administratorsSid);
    if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, administratorsSid,
                            &sidBytes)) {
        CloseHandle(currentToken);
        launch.result.enforcement = SandboxEnforcement::Failed;
        launch.result.failureCategory = QStringLiteral("AdministratorsSidUnavailable");
        return launch;
    }
    SID_AND_ATTRIBUTES disabledAdmin{administratorsSid, 0};
    const BOOL restricted = CreateRestrictedToken(currentToken, DISABLE_MAX_PRIVILEGE,
                                                  1, &disabledAdmin, 0, nullptr, 0, nullptr,
                                                  &native->token);
    CloseHandle(currentToken);
    if (!restricted) {
        launch.result.enforcement = SandboxEnforcement::Failed;
        launch.result.failureCategory = QStringLiteral("RestrictedTokenUnavailable");
        return launch;
    }
    launch.result.restrictedToken = true;
    native->job = CreateJobObjectW(nullptr, nullptr);
    if (!native->job) {
        launch.result.enforcement = SandboxEnforcement::Failed;
        launch.result.failureCategory = QStringLiteral("JobUnavailable");
        return launch;
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(native->job, JobObjectExtendedLimitInformation,
                                 &limits, sizeof(limits))) {
        launch.result.enforcement = SandboxEnforcement::Failed;
        launch.result.failureCategory = QStringLiteral("JobConfigurationFailed");
        return launch;
    }
    launch.result.processTreeControlled = true;
    launch.result.filesystemRestricted = false;
    launch.result.networkDenied = false;
    launch.result.enforcement = SandboxEnforcement::PartiallyEnforced;
    launch.result.failureCategory = !plan.networkAllowed
        ? QStringLiteral("NetworkDenialUnavailable")
        : QStringLiteral("FilesystemConfinementUnavailable");
    if (plan.requireEnforcement) return launch;
    launch.windows = std::move(native);
    launch.permitted = true;
    return launch;
#elif defined(Q_OS_MACOS)
    // system.sb provides the macOS runtime baseline needed for a child to
    // start (including its inherited stdio descriptors).  Without it even a
    // simple stdio process aborts before exec, which makes the MCP transport
    // fail during its initialize handshake.
    QString profile = QStringLiteral("(version 1)(deny default)(import \"system.sb\")"
                                     "(allow process-exec)(allow process-fork)"
                                     "(allow signal (target self))"
                                     "(allow file-read* (subpath \"/System\")"
                                     "(subpath \"/usr\") (subpath \"/bin\")"
                                     "(subpath \"/sbin\") (subpath \"/Library\")"
                                     "(literal \"/dev/null\"))");
    // This backend has no kill-on-close process-tree primitive. For a host which
    // forbids detached children, enforce the stronger invariant: no children
    // can be created at all. Brokered processes are launched by the desktop,
    // never by plugin code. Deny overrides the runtime baseline's fork allow.
    if (plan.forbidDetachedChildren)
        profile += QStringLiteral("(deny process-fork)");
    profile += QStringLiteral("(allow file-read* (literal %1))").arg(profileLiteral(program));
    // Homebrew is the supported Qt distribution on macOS.  A Qt-based local
    // MCP server can load its framework dependencies from this immutable
    // prefix; denying it makes the child exit before the stdio handshake.
    const QString homebrewQtAlias = QStringLiteral("/opt/homebrew/opt/qtbase");
    const QString homebrewQtBase = QFileInfo(homebrewQtAlias).canonicalFilePath();
    if (!homebrewQtBase.isEmpty()) {
        profile += QStringLiteral("(allow file-read* file-map-executable (subpath %1))")
                       .arg(profileLiteral(homebrewQtBase));
        // dyld also inspects the install-name alias before resolving the Cellar
        // target. Both names refer to the same read-only runtime dependency.
        profile += QStringLiteral("(allow file-read* file-map-executable (subpath %1))")
                       .arg(profileLiteral(homebrewQtAlias));
    }
    // Permit library directories of already-loaded Homebrew runtime images
    // (for example Qt's ICU dependency), not the entire package-manager tree. dyld may check
    // both an install-name symlink and its canonical target.
    QSet<QString> runtimeImages;
    for (uint32_t i = 0; i < _dyld_image_count(); ++i) {
        const auto* imageName = _dyld_get_image_name(i);
        if (!imageName)
            continue;
        const QString image = QString::fromUtf8(imageName);
        if (!image.startsWith(QStringLiteral("/opt/homebrew/")))
            continue;
        runtimeImages.insert(image);
        const auto canonical = QFileInfo(image).canonicalFilePath();
        if (!canonical.isEmpty())
            runtimeImages.insert(canonical);
        const QString cellar = QStringLiteral("/opt/homebrew/Cellar/");
        if (canonical.startsWith(cellar)) {
            const auto parts = canonical.mid(cellar.size()).split(QLatin1Char('/'));
            if (parts.size() > 2) {
                const QString alias = QStringLiteral("/opt/homebrew/opt/%1/%2")
                                          .arg(parts.first(), parts.mid(2).join(QLatin1Char('/')));
                if (QFileInfo(alias).canonicalFilePath() == canonical)
                    runtimeImages.insert(alias);
                // ABI install names can be additional symlinks (78 -> 78.3).
                const QDir aliases(QFileInfo(alias).absolutePath());
                for (const auto& candidate : aliases.entryInfoList(QDir::Files))
                    if (candidate.canonicalFilePath() == canonical)
                        runtimeImages.insert(candidate.absoluteFilePath());
            }
        }
    }
    QSet<QString> runtimeDirectories;
    for (const auto& image : runtimeImages) {
        profile += QStringLiteral("(allow file-read* file-map-executable (subpath %1))")
                       .arg(profileLiteral(QFileInfo(image).absolutePath()));
        profile += QStringLiteral("(allow file-read* file-map-executable (literal %1))")
                       .arg(profileLiteral(image));
        QString directory = QFileInfo(image).absolutePath();
        while (directory.startsWith(QStringLiteral("/opt/homebrew/"))) {
            runtimeDirectories.insert(directory);
            directory = QFileInfo(directory).absolutePath();
        }
    }
    for (const auto& directory : runtimeDirectories)
        profile += QStringLiteral("(allow file-read-metadata (literal %1))")
                       .arg(profileLiteral(directory));
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
    launch.result.processTreeControlled = plan.forbidDetachedChildren;
#elif defined(Q_OS_LINUX)
    launch.program = launcher_;
    launch.arguments = {QStringLiteral("--die-with-parent"), QStringLiteral("--unshare-user"),
                        QStringLiteral("--unshare-pid"), QStringLiteral("--unshare-ipc"),
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
        const QFileInfo systemPath(QString::fromLatin1(path));
        if (!systemPath.exists()) continue;
        if (systemPath.isSymLink())
            launch.arguments.append({QStringLiteral("--symlink"), systemPath.symLinkTarget(),
                                     QString::fromLatin1(path)});
        else
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
    launch.result.environmentRestricted = plan.restrictedEnvironment;
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
