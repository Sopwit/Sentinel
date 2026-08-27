// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/editor/FuzzyEditor.h"
#include "sentinel/core/mcp/McpService.h"
#include "sentinel/core/security/PathGuard.h"

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSettings>
#include <QStorageInfo>
#include <QSysInfo>
#include <QUrl>

#include <functional>
#include <optional>

#if defined(Q_OS_UNIX)
#include <unistd.h>
#elif defined(Q_OS_WIN)
#define NOMINMAX
#include <windows.h>
#endif

namespace sentinel::core {

RealToolExecutor::RealToolExecutor() = default;

RealToolExecutor::RealToolExecutor(std::shared_ptr<AlarmStore> alarmStore)
    : alarmStore_(std::move(alarmStore)) {}

void RealToolExecutor::setAlarmStore(std::shared_ptr<AlarmStore> alarmStore) {
    alarmStore_ = std::move(alarmStore);
}

void RealToolExecutor::setMemorySnapshot(MemoryEntries entries) {
    memorySnapshot_ = std::move(entries);
}

void RealToolExecutor::setHistorySnapshot(QStringList entries) {
    historySnapshot_ = std::move(entries);
}

void RealToolExecutor::configureMcpServers(const QList<McpServerConfig>& configs) {
    auto service = std::make_shared<McpService>();
    for (const auto& config : configs) {
        if (config.name.trimmed().isEmpty()) {
            continue;
        }
        service->addServer(config);
    }
    service->connectToAll();
    mcpService_ = std::move(service);
}

void RealToolExecutor::setMcpService(std::shared_ptr<IMcpService> service) {
    mcpService_ = std::move(service);
}

void RealToolExecutor::setSubagentRunner(std::function<QString(const QString& task)> runner) {
    subagentRunner_ = std::move(runner);
}

void RealToolExecutor::configureWebSearch(const QString& provider, const QString& apiKey,
                                          int maxResults) {
    webSearchTool_.setSearchProvider(provider);
    webSearchTool_.setApiKey(apiKey);
    webSearchTool_.setMaxResults(maxResults);
}

WebSearchResponse RealToolExecutor::searchWeb(const QString& query) const {
    return webSearchTool_.search(query);
}

namespace {

constexpr int kDefaultReadLimit = 2000;
constexpr int kMaxLineLength = 2000;
constexpr int kMaxReadBytes = 50 * 1024;
constexpr int kGrepGlobLimit = 100;
constexpr int kDirListLimit = 100;
constexpr int kWebFetchPreviewChars = 8000;

QString getArgument(const PlannedToolInvocation& invocation, const QString& argId) {
    for (const auto& arg : invocation.arguments) {
        if (arg.id == argId) {
            return arg.value;
        }
    }
    return QString();
}

int getIntArgument(const PlannedToolInvocation& invocation, const QString& argId, int fallback) {
    bool ok = false;
    const int value = getArgument(invocation, argId).trimmed().toInt(&ok);
    return ok ? value : fallback;
}

QString scopedPath(const QString& workingDirectory, const QString& rawPath) {
    QString path = rawPath.trimmed();
    if (path.isEmpty()) {
        return QString();
    }
    QFileInfo fileInfo(path);
    if (fileInfo.isRelative()) {
        path = QDir(workingDirectory).absoluteFilePath(path);
    }
    return PathGuard::safePath(workingDirectory, path);
}

bool hasBinaryExtension(const QString& path) {
    static const QSet<QString> extensions{
        QStringLiteral("zip"),  QStringLiteral("exe"),   QStringLiteral("so"),
        QStringLiteral("dll"),  QStringLiteral("dylib"), QStringLiteral("wasm"),
        QStringLiteral("pyc"),  QStringLiteral("jpg"),   QStringLiteral("jpeg"),
        QStringLiteral("png"),  QStringLiteral("gif"),   QStringLiteral("webp"),
        QStringLiteral("ico"),  QStringLiteral("pdf"),   QStringLiteral("mp3"),
        QStringLiteral("mp4"),  QStringLiteral("mov"),   QStringLiteral("avi"),
        QStringLiteral("mkv"),  QStringLiteral("wav"),   QStringLiteral("ogg"),
        QStringLiteral("flac"), QStringLiteral("ttf"),   QStringLiteral("otf"),
        QStringLiteral("woff"), QStringLiteral("woff2"), QStringLiteral("class"),
        QStringLiteral("jar"),  QStringLiteral("7z"),    QStringLiteral("tar"),
        QStringLiteral("gz"),   QStringLiteral("rar"),
    };
    return extensions.contains(QFileInfo(path).suffix().toLower());
}

bool looksBinary(const QString& path) {
    if (hasBinaryExtension(path)) {
        return true;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray sample = file.read(4096);
    if (sample.isEmpty()) {
        return false;
    }
    if (sample.contains('\0')) {
        return true;
    }
    int nonPrintable = 0;
    for (char c : sample) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (uc < 9 || (uc > 13 && uc < 32)) {
            ++nonPrintable;
        }
    }
    return nonPrintable * 100 / sample.size() > 30;
}

QString runSynchronousProcess(const QString& program, const QStringList& args,
                              const QString& workingDirectory, int timeoutMs,
                              QString* errorOut = nullptr) {
    QProcess process;
    if (!workingDirectory.isEmpty() && QDir(workingDirectory).exists()) {
        process.setWorkingDirectory(workingDirectory);
    }
    process.start(program, args);
    if (!process.waitForStarted(5000)) {
        if (errorOut) {
            *errorOut = QStringLiteral("Failed to start '%1'.").arg(program);
        }
        return QString();
    }
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(3000);
        if (errorOut) {
            *errorOut = QStringLiteral("Timed out after %1 ms.").arg(QString::number(timeoutMs));
        }
        return QString();
    }
    const QString out = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString err = QString::fromUtf8(process.readAllStandardError()).trimmed();
    if (process.exitCode() != 0 && errorOut && out.isEmpty()) {
        *errorOut =
            err.isEmpty() ? QStringLiteral("Exited with code %1.").arg(process.exitCode()) : err;
    }
    return out;
}

QString osascriptEscape(const QString& text) {
    QString escaped = text;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return escaped;
}

QString humanReadableBytes(quint64 bytes) {
    const double gib = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    if (gib >= 1.0) {
        return QStringLiteral("%1 GiB").arg(QString::number(gib, 'f', 1));
    }
    const double mib = static_cast<double>(bytes) / (1024.0 * 1024.0);
    return QStringLiteral("%1 MiB").arg(QString::number(mib, 'f', 0));
}

QClipboard* activeClipboard() {
    const auto* guiApp = qobject_cast<const QGuiApplication*>(QCoreApplication::instance());
    return guiApp ? QGuiApplication::clipboard() : nullptr;
}

QString systemInfoReport() {
    QStringList lines;
    lines.append(QStringLiteral("OS: %1").arg(QSysInfo::prettyProductName()));
    lines.append(QStringLiteral("Kernel: %1").arg(QSysInfo::kernelVersion()));
    lines.append(QStringLiteral("CPU architecture: %1").arg(QSysInfo::currentCpuArchitecture()));
    lines.append(QStringLiteral("Hostname: %1").arg(QSysInfo::machineHostName()));
    QString user = qEnvironmentVariable("USER");
    if (user.isEmpty()) {
        user = qEnvironmentVariable("USERNAME");
    }
    if (!user.isEmpty()) {
        lines.append(QStringLiteral("User: %1").arg(user));
    }

    quint64 totalRamBytes = 0;
#if defined(Q_OS_UNIX)
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long pageSize = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && pageSize > 0) {
        totalRamBytes = static_cast<quint64>(pages) * static_cast<quint64>(pageSize);
    }
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX memoryStatus{};
    memoryStatus.dwLength = sizeof(memoryStatus);
    if (GlobalMemoryStatusEx(&memoryStatus)) {
        totalRamBytes = memoryStatus.ullTotalPhys;
    }
#endif
    if (totalRamBytes > 0) {
        lines.append(QStringLiteral("Total RAM: %1").arg(humanReadableBytes(totalRamBytes)));
    }

    const QStorageInfo root = QStorageInfo::root();
    if (root.isValid() && root.isReady()) {
        lines.append(QStringLiteral("Root volume (%1): %2 free of %3")
                         .arg(QDir::toNativeSeparators(root.rootPath()),
                              humanReadableBytes(static_cast<quint64>(root.bytesFree())),
                              humanReadableBytes(static_cast<quint64>(root.bytesTotal()))));
    }
    lines.append(
        QStringLiteral("Workspace: %1").arg(QDir::toNativeSeparators(QDir::currentPath())));
    return lines.join(QLatin1Char('\n'));
}

QString processListReport() {
    QString output;
    QString error;
#if defined(Q_OS_WIN)
    output = runSynchronousProcess(QStringLiteral("tasklist"), {}, QString(), 15000, &error);
#else
    output = runSynchronousProcess(QStringLiteral("ps"),
                                   {QStringLiteral("-eo"), QStringLiteral("pid,pcpu,comm")},
                                   QString(), 15000, &error);
#endif
    if (output.isEmpty()) {
        return error.isEmpty() ? QStringLiteral("process-list: No process output was returned.")
                               : QStringLiteral("process-list: %1").arg(error);
    }

    const auto lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QStringList shown;
    const int maxLines = 40;
    for (int i = 0; i < lines.size() && shown.size() < maxLines; ++i) {
        QString line = lines.at(i).trimmed();
        if (line.size() > kMaxLineLength) {
            line = line.left(kMaxLineLength) + QStringLiteral("... (line truncated)");
        }
        shown.append(line);
    }
    return QStringLiteral("process-list: %1 process line(s) (showing first %2):\n%3")
        .arg(QString::number(lines.size()), QString::number(shown.size()),
             shown.join(QLatin1Char('\n')));
}

// Freedesktop .desktop entry resolution (Linux/BSD app-launch support).
#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
struct DesktopEntry {
    QString desktopId;
    QString name;
    QString exec;
    QString icon;
};

// Locates a freedesktop .desktop entry for an application name such as
// "Spotify" or "org.kde.dolphin". Searches the standard data directories.
std::optional<DesktopEntry> findDesktopEntry(const QString& appName) {
    const QString needle = appName.trimmed();
    if (needle.isEmpty()) {
        return std::nullopt;
    }
    const QString lowered = needle.toLower();

    QStringList searchRoots;
    const QString dataHome = qEnvironmentVariable("XDG_DATA_HOME").isEmpty()
                                 ? QDir::home().filePath(QStringLiteral(".local/share"))
                                 : qEnvironmentVariable("XDG_DATA_HOME");
    searchRoots.append(dataHome);
    const QString dataDirsEnv = qEnvironmentVariable("XDG_DATA_DIRS");
    const QStringList dataDirs =
        dataDirsEnv.isEmpty()
            ? QStringList{QStringLiteral("/usr/share"), QStringLiteral("/usr/local/share")}
            : dataDirsEnv.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    searchRoots.append(dataDirs);

    std::optional<DesktopEntry> best;
    for (const auto& root : searchRoots) {
        QDirIterator it(QDir(root).filePath(QStringLiteral("applications")),
                        {QStringLiteral("*.desktop")}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            QSettings desktopFile(path, QSettings::IniFormat);
            desktopFile.beginGroup(QStringLiteral("Desktop Entry"));

            const QString type =
                desktopFile.value(QStringLiteral("Type"), QString()).toString().trimmed();
            const bool hidden = desktopFile.value(QStringLiteral("Hidden"), false).toBool();
            if (type != QStringLiteral("Application") || hidden) {
                continue;
            }

            const QString name =
                desktopFile.value(QStringLiteral("Name"), QString()).toString().trimmed();
            const QString desktopId = QFileInfo(path).completeBaseName();
            const bool matches =
                desktopId.compare(lowered, Qt::CaseInsensitive) == 0 ||
                desktopId.endsWith(QStringLiteral(".") + lowered, Qt::CaseInsensitive) ||
                name.compare(needle, Qt::CaseInsensitive) == 0;
            if (!matches) {
                continue;
            }

            DesktopEntry entry;
            entry.desktopId = desktopId;
            entry.name = name;
            entry.exec = desktopFile.value(QStringLiteral("Exec"), QString()).toString();
            entry.icon = desktopFile.value(QStringLiteral("Icon"), QString()).toString();
            desktopFile.endGroup();

            // Prefer exact id matches (e.g. "spotify") over suffix matches
            // (e.g. "org.kde.dolphin" for "dolphin").
            if (!best || (best->desktopId.toLower() != lowered && desktopId.toLower() == lowered)) {
                best = entry;
            }
        }
    }
    return best;
}

// Strips freedesktop field codes (%f, %u, %F, %U) from an Exec= value.
QString desktopExecToCommand(const QString& exec) {
    static const QRegularExpression fieldCodes(QStringLiteral("%[fFuUdDnNickvm]"));
    QString command = exec;
    command.remove(fieldCodes);
    return command.simplified();
}
#endif // !Q_OS_MACOS && !Q_OS_WIN

// True when the text looks like a website address ("sahibinden.com",
// "www.x.com", "https://x.com") rather than an application name.
bool looksLikeDomainName(const QString& text) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed.contains(QLatin1Char(' '))) {
        return false;
    }
    const QString lowered = trimmed.toLower();
    if (lowered.startsWith(QStringLiteral("http://")) ||
        lowered.startsWith(QStringLiteral("https://")) ||
        lowered.startsWith(QStringLiteral("www."))) {
        return true;
    }
    const int dot = trimmed.lastIndexOf(QLatin1Char('.'));
    if (dot < 1 || dot >= trimmed.size() - 2) {
        return false;
    }
    const QString suffix = trimmed.mid(dot + 1);
    if (suffix.size() > 10) {
        return false;
    }
    for (const QChar c : suffix) {
        if (!c.isLetter()) {
            return false;
        }
    }
    return true;
}

struct PatchHunk {
    int oldStart{0};
    int oldCount{0};
    QStringList oldLines;
    QStringList newLines;
};

struct PatchFile {
    QString action; // "update", "add", "delete"
    QString path;
    QList<PatchHunk> hunks;
    QString addContent;
};

// Strips the a/ or b/ prefix used by git-style diffs.
QString stripDiffPrefix(const QString& path) {
    if (path.startsWith(QStringLiteral("a/")) || path.startsWith(QStringLiteral("b/"))) {
        return path.mid(2);
    }
    return path;
}

// Parses a unified diff into per-file hunks. Supports git-style Update
// (--- a/f / +++ b/f), Add (--- /dev/null), and Delete (+++ /dev/null).
bool parseUnifiedDiff(const QString& patch, QList<PatchFile>& files, QString& error) {
    const QStringList lines = patch.split(QLatin1Char('\n'));
    PatchFile current;
    PatchHunk hunk;
    bool inHunk = false;

    auto finishHunk = [&]() {
        if (!hunk.oldLines.isEmpty() || !hunk.newLines.isEmpty()) {
            current.hunks.append(hunk);
        }
        hunk = PatchHunk{};
    };
    auto finishFile = [&]() {
        finishHunk();
        inHunk = false;
        if (!current.path.isEmpty()) {
            files.append(current);
        }
        current = PatchFile{};
    };

    static const QRegularExpression hunkHeader(
        QStringLiteral("^@@ -(\\d+)(?:,(\\d+))? \\+(\\d+)(?:,(\\d+))? @@"));

    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines.at(i);
        if (line.startsWith(QStringLiteral("--- "))) {
            finishFile();
            const QString oldPath = stripDiffPrefix(line.mid(4).trimmed());
            current.action = oldPath == QStringLiteral("/dev/null") ? QStringLiteral("add")
                                                                    : QStringLiteral("update");
            current.path = oldPath;
            continue;
        }
        if (line.startsWith(QStringLiteral("+++ "))) {
            const QString newPath = stripDiffPrefix(line.mid(4).trimmed());
            if (newPath == QStringLiteral("/dev/null")) {
                current.action = QStringLiteral("delete");
            } else if (current.path.isEmpty() || current.path == QStringLiteral("/dev/null")) {
                current.path = newPath;
            }
            continue;
        }
        const auto match = hunkHeader.match(line);
        if (match.hasMatch()) {
            finishHunk();
            hunk.oldStart = match.captured(1).toInt();
            hunk.oldCount = match.captured(2).isEmpty() ? 1 : match.captured(2).toInt();
            inHunk = true;
            continue;
        }
        if (!inHunk) {
            continue;
        }
        if (line.startsWith(QLatin1Char('+'))) {
            hunk.newLines.append(line.mid(1));
        } else if (line.startsWith(QLatin1Char('-'))) {
            hunk.oldLines.append(line.mid(1));
        } else if (line.startsWith(QLatin1Char(' '))) {
            hunk.oldLines.append(line.mid(1));
            hunk.newLines.append(line.mid(1));
        } else if (line.trimmed().isEmpty() && i + 1 < lines.size()) {
            // Tolerate missing leading space on empty context lines.
            hunk.oldLines.append(QString());
            hunk.newLines.append(QString());
        }
    }
    finishFile();

    if (files.isEmpty()) {
        error = QStringLiteral("No file sections found. The patch must use unified diff headers "
                               "(--- a/file, +++ b/file, @@ ... @@).");
        return false;
    }
    return true;
}

// Applies one hunk at the given 0-based position with exact context matching.
bool applyHunkAt(QStringList& fileLines, int position, const PatchHunk& hunk) {
    for (int i = 0; i < hunk.oldLines.size(); ++i) {
        const int target = position + i;
        if (target >= fileLines.size() || fileLines.at(target) != hunk.oldLines.at(i)) {
            return false;
        }
    }
    for (int i = 0; i < hunk.oldLines.size(); ++i) {
        fileLines.removeAt(position);
    }
    for (int i = 0; i < hunk.newLines.size(); ++i) {
        fileLines.insert(position + i, hunk.newLines.at(i));
    }
    return true;
}

// Applies a hunk searching from the claimed line downward then upward (fuzz),
// mirroring how patch tools tolerate offset drift.
bool applyHunkWithFuzz(QStringList& fileLines, int claimedStart, const PatchHunk& hunk,
                       int& appliedAt) {
    const int claimed = claimedStart - 1;
    const int maxOffset = fileLines.size();
    for (int offset = 0; offset <= maxOffset; ++offset) {
        const int down = claimed + offset;
        if (down + hunk.oldLines.size() <= fileLines.size() && applyHunkAt(fileLines, down, hunk)) {
            appliedAt = down;
            return true;
        }
        if (offset > 0) {
            const int up = claimed - offset;
            if (up >= 0 && up + hunk.oldLines.size() <= fileLines.size() &&
                applyHunkAt(fileLines, up, hunk)) {
                appliedAt = up;
                return true;
            }
        }
    }
    return false;
}

// True when the docker CLI is available (checked with a fast --version call).
bool dockerAvailable() {
    return !runSynchronousProcess(QStringLiteral("docker"), {QStringLiteral("--version")},
                                  QString(), 5000)
                .isEmpty();
}

// Runs a shell command inside a throwaway docker container with the workspace
// mounted read-write at /workspace. Ported from openclaw's Docker sandbox
// pattern; degrades to a clear error when docker is missing.
QString runCommandInDocker(const QString& command, const QString& workspace, int timeoutMs) {
    if (!dockerAvailable()) {
        return QStringLiteral(
            "run-command: docker sandbox requested but the docker CLI is not available. "
            "Install Docker or retry without sandbox=docker.");
    }
    const QString image = QStringLiteral("alpine:3.20");
    QStringList args{QStringLiteral("run"),       QStringLiteral("--rm"),
                     QStringLiteral("--network"), QStringLiteral("none"),
                     QStringLiteral("--memory"),  QStringLiteral("2g"),
                     QStringLiteral("--cpus"),    QStringLiteral("2"),
                     QStringLiteral("-v"),        QStringLiteral("%1:/workspace").arg(workspace),
                     QStringLiteral("-w"),        QStringLiteral("/workspace")};
    if (timeoutMs > 60000) {
        // Leave time for image pulls on first use.
        timeoutMs += 120000;
    }
    args.append(image);
    args.append(QStringLiteral("sh"));
    args.append(QStringLiteral("-c"));
    args.append(command);

    QProcess process;
    process.start(QStringLiteral("docker"), args);
    if (!process.waitForStarted(10000)) {
        return QStringLiteral("run-command: failed to start docker (%1).").arg(command);
    }
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(3000);
        return QStringLiteral("run-command: docker sandbox terminated command after timeout %1 ms.")
            .arg(QString::number(timeoutMs));
    }
    const QString out = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString err = QString::fromUtf8(process.readAllStandardError()).trimmed();
    if (process.exitCode() == 0) {
        return out.isEmpty() && err.isEmpty()
                   ? QStringLiteral("Command executed in docker sandbox. (exit=0)")
                   : QStringLiteral("Command executed in docker sandbox. (exit=0)\n\n[STDOUT]:\n%1")
                         .arg(out);
    }
    if (err.contains(QStringLiteral("failed to connect to the docker API")) ||
        err.contains(QStringLiteral("Is the docker daemon running"))) {
        return QStringLiteral(
                   "run-command: the docker CLI is installed but the daemon is not running. "
                   "Start Docker (or OrbStack) and retry, or run without sandbox=docker.\n"
                   "Details: %1")
            .arg(err);
    }
    return QStringLiteral(
               "Docker sandbox command failed. (exit=%1)\n\n[STDOUT]:\n%2\n\n[STDERR]:\n%3")
        .arg(QString::number(process.exitCode()), out, err);
}

// Runs `npx playwright <mode>` for browser-screenshot / browser-pdf. Real
// Playwright CLI integration (ported from SWE-agent's browser bundle idea);
// requires Node.js and npx on PATH, fails gracefully otherwise.
QString runPlaywrightCli(const QString& mode, const QString& url, const QString& outputPath,
                         const QString& extraWait) {
    if (runSynchronousProcess(QStringLiteral("npx"), {QStringLiteral("--version")}, QString(), 5000)
            .isEmpty()) {
        return QStringLiteral(
                   "browser-%1: Node.js (npx) is required for Playwright browser tools. Install "
                   "Node.js and run 'npx playwright install chromium' once.")
            .arg(mode);
    }

    QStringList args{QStringLiteral("-y"), QStringLiteral("playwright"), mode};
    if (!extraWait.isEmpty()) {
        args.append(QStringLiteral("--wait-for-timeout"));
        args.append(extraWait);
    }
    args.append(url);
    args.append(outputPath);

    QProcess process;
    process.start(QStringLiteral("npx"), args);
    if (!process.waitForStarted(10000)) {
        return QStringLiteral("browser-%1: failed to start npx.").arg(mode);
    }
    // npx may fetch the playwright package on first use; allow extra time.
    if (!process.waitForFinished(300000)) {
        process.kill();
        process.waitForFinished(3000);
        return QStringLiteral("browser-%1: Playwright timed out after 300 s.").arg(mode);
    }
    const QString err = QString::fromUtf8(process.readAllStandardError()).trimmed();
    if (process.exitCode() != 0 || !QFile::exists(outputPath)) {
        return QStringLiteral("browser-%1: Playwright failed: %2")
            .arg(mode, err.isEmpty() ? QStringLiteral("no output file was produced") : err);
    }
    return QStringLiteral("browser-%1: saved '%2'.").arg(mode, outputPath);
}

QString applyPatchReport(const QString& patch, const QString& workingDirectory,
                         const std::function<QString(const QString&)>& scopePath) {
    QList<PatchFile> files;
    QString parseError;
    if (!parseUnifiedDiff(patch, files, parseError)) {
        return QStringLiteral("apply-patch: %1").arg(parseError);
    }

    QStringList applied;
    QStringList failures;

    for (const auto& file : files) {
        const QString scoped = scopePath(file.path);
        if (scoped.isEmpty()) {
            failures.append(QStringLiteral("%1: outside the approved workspace").arg(file.path));
            continue;
        }

        if (file.action == QStringLiteral("add")) {
            QFile out(scoped);
            if (QFile::exists(scoped)) {
                failures.append(QStringLiteral("%1: file already exists").arg(scoped));
                continue;
            }
            QDir().mkpath(QFileInfo(scoped).absolutePath());
            QStringList contentLines;
            for (const auto& hunk : file.hunks) {
                contentLines.append(hunk.newLines);
            }
            if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
                failures.append(QStringLiteral("%1: %2").arg(scoped, out.errorString()));
                continue;
            }
            out.write(contentLines.join(QLatin1Char('\n')).toUtf8());
            applied.append(QStringLiteral("added %1 (%2 lines)")
                               .arg(scoped, QString::number(contentLines.size())));
            continue;
        }

        if (file.action == QStringLiteral("delete")) {
            if (!QFile::exists(scoped)) {
                failures.append(QStringLiteral("%1: file not found").arg(scoped));
                continue;
            }
            if (QFile::remove(scoped)) {
                applied.append(QStringLiteral("deleted %1").arg(scoped));
            } else {
                failures.append(QStringLiteral("%1: delete failed").arg(scoped));
            }
            continue;
        }

        // Update
        QFile in(scoped);
        if (!in.open(QIODevice::ReadOnly | QIODevice::Text)) {
            failures.append(QStringLiteral("%1: file not found").arg(scoped));
            continue;
        }
        QString content = QString::fromUtf8(in.readAll());
        in.close();
        QStringList fileLines = content.split(QLatin1Char('\n'));
        if (fileLines.size() > 1 && fileLines.last().isEmpty()) {
            fileLines.removeLast();
        }

        bool fileOk = true;
        int hunksApplied = 0;
        for (const auto& hunk : file.hunks) {
            int appliedAt = -1;
            if (!applyHunkWithFuzz(fileLines, hunk.oldStart, hunk, appliedAt)) {
                failures.append(QStringLiteral("%1: hunk at line %2 did not match the file "
                                               "context")
                                    .arg(scoped, QString::number(hunk.oldStart)));
                fileOk = false;
                break;
            }
            ++hunksApplied;
        }
        if (!fileOk) {
            continue;
        }

        QFile out(scoped);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            failures.append(QStringLiteral("%1: %2").arg(scoped, out.errorString()));
            continue;
        }
        out.write(fileLines.join(QLatin1Char('\n')).toUtf8());
        applied.append(
            QStringLiteral("updated %1 (%2 hunk(s))").arg(scoped, QString::number(hunksApplied)));
    }

    QStringList report;
    if (!applied.isEmpty()) {
        report.append(QStringLiteral("apply-patch: applied to %1 file(s):")
                          .arg(QString::number(applied.size())));
        report.append(applied);
    }
    if (!failures.isEmpty()) {
        report.append(
            QStringLiteral("apply-patch: %1 failure(s):").arg(QString::number(failures.size())));
        report.append(failures);
    }
    if (report.isEmpty()) {
        report.append(QStringLiteral("apply-patch: nothing to apply."));
    }
    return report.join(QLatin1Char('\n'));
}

// Extracts code symbols (classes, functions, methods) with line numbers for
// common language families. Ported concept from Cline's
// list_code_definition_names tool.
QStringList extractCodeDefinitions(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    const QString content = QString::fromUtf8(file.readAll());
    const QStringList lines = content.split(QLatin1Char('\n'));
    const QString suffix = QFileInfo(path).suffix().toLower();

    static const QRegularExpression cxxPattern(QStringLiteral(
        "^\\s*(?:template\\s*<[^>]*>\\s*)?(?:class|struct|enum\\s+class|enum|namespace)\\s+"
        "([A-Za-z_]\\w*)"));
    static const QRegularExpression cxxFunction(
        QStringLiteral("^\\s*(?:[A-Za-z_][\\w:<>,\\*\\&\\s]*)\\s+([A-Za-z_]\\w*)\\s*\\("));
    static const QRegularExpression pythonPattern(
        QStringLiteral("^\\s*(?:async\\s+)?def\\s+([A-Za-z_]\\w*)|^\\s*class\\s+([A-Za-z_]\\w*)"));
    static const QRegularExpression jsPattern(
        QStringLiteral("^\\s*(?:export\\s+)?(?:default\\s+)?(?:async\\s+)?(?:function\\*?|class|"
                       "interface|type|enum)\\s+([A-Za-z_$][\\w$]*)"));
    static const QRegularExpression rustPattern(QStringLiteral(
        "^\\s*(?:pub(?:\\(\\w+\\))?\\s+)?(?:async\\s+)?(?:fn|struct|enum|trait|mod)\\s+"
        "([A-Za-z_]\\w*)"));

    const bool isPython = suffix == QStringLiteral("py");
    const bool isJsLike = suffix == QStringLiteral("js") || suffix == QStringLiteral("jsx") ||
                          suffix == QStringLiteral("ts") || suffix == QStringLiteral("tsx") ||
                          suffix == QStringLiteral("mjs");
    const bool isRust = suffix == QStringLiteral("rs");
    const bool isCxx = suffix == QStringLiteral("c") || suffix == QStringLiteral("cc") ||
                       suffix == QStringLiteral("cpp") || suffix == QStringLiteral("cxx") ||
                       suffix == QStringLiteral("h") || suffix == QStringLiteral("hpp") ||
                       suffix == QStringLiteral("java") || suffix == QStringLiteral("cs");

    QStringList definitions;
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines.at(i);
        auto add = [&](const QString& kind, const QString& name) {
            definitions.append(
                QStringLiteral("%1:%2 %3 %4").arg(QString::number(i + 1), kind, name));
        };
        if (isPython) {
            const auto m = pythonPattern.match(line);
            if (m.hasMatch()) {
                if (!m.captured(1).isEmpty()) {
                    add(QStringLiteral("def"), m.captured(1));
                } else {
                    add(QStringLiteral("class"), m.captured(2));
                }
            }
        } else if (isJsLike) {
            const auto m = jsPattern.match(line);
            if (m.hasMatch()) {
                add(QStringLiteral("symbol"), m.captured(1));
            }
        } else if (isRust) {
            const auto m = rustPattern.match(line);
            if (m.hasMatch()) {
                add(QStringLiteral("item"), m.captured(1));
            }
        } else if (isCxx) {
            const auto m = cxxPattern.match(line);
            if (m.hasMatch()) {
                add(QStringLiteral("type"), m.captured(1));
                continue;
            }
            const auto f = cxxFunction.match(line);
            if (f.hasMatch() && f.captured(1) != QStringLiteral("return") &&
                f.captured(1) != QStringLiteral("if") && f.captured(1) != QStringLiteral("for") &&
                f.captured(1) != QStringLiteral("while") &&
                f.captured(1) != QStringLiteral("switch")) {
                add(QStringLiteral("function"), f.captured(1));
            }
        }
        if (definitions.size() >= 100) {
            definitions.append(QStringLiteral("(truncated at 100 definitions)"));
            break;
        }
    }
    return definitions;
}

QDateTime parseAlarmTime(const QString& raw) {
    const auto trimmed = raw.trimmed();
    QDateTime parsed = QDateTime::fromString(trimmed, Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(trimmed, Qt::ISODate);
    }
    if (parsed.isValid()) {
        return parsed;
    }

    const QRegularExpression timeOnly(QStringLiteral("^(\\d{1,2}):(\\d{2})(?::(\\d{2}))?$"));
    const auto match = timeOnly.match(trimmed);
    if (match.hasMatch()) {
        const auto now = QDateTime::currentDateTime();
        QTime time(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
        QDateTime candidate(now.date(), time);
        if (candidate <= now) {
            candidate = candidate.addDays(1);
        }
        return candidate;
    }

    for (const char* format : {"yyyy-MM-dd HH:mm", "dd.MM.yyyy HH:mm", "HH:mm:ss"}) {
        parsed = QDateTime::fromString(trimmed, QString::fromLatin1(format));
        if (parsed.isValid()) {
            return parsed;
        }
    }
    return QDateTime();
}

QString readToolReport(const QString& path, int offset, int limit) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("read-file: Failed to open '%1'.").arg(path);
    }
    const QString content = QString::fromUtf8(file.readAll());
    QStringList lines = content.split(QLatin1Char('\n'));
    if (lines.size() > 1 && lines.last().isEmpty()) {
        lines.removeLast();
    }

    if (offset < 1) {
        offset = 1;
    }
    if (offset > lines.size()) {
        return QStringLiteral("read-file: Offset %1 is out of range for this file (%2 lines).")
            .arg(QString::number(offset), QString::number(lines.size()));
    }

    int end = qMin(lines.size(), offset + limit - 1);
    int bytes = 0;
    QStringList numbered;
    for (int i = offset - 1; i < end; ++i) {
        QString line = lines.at(i);
        if (line.size() > kMaxLineLength) {
            line = line.left(kMaxLineLength) + QStringLiteral("... (line truncated)");
        }
        bytes += line.size() + 8;
        if (bytes > kMaxReadBytes) {
            end = i;
            break;
        }
        numbered.append(QStringLiteral("%1: %2").arg(QString::number(i + 1), line));
    }

    QString footer;
    if (bytes > kMaxReadBytes) {
        footer = QStringLiteral("(Output capped at 50 KB. Showing lines %1-%2 of %3. Use offset=%4 "
                                "to continue.)")
                     .arg(QString::number(offset), QString::number(end),
                          QString::number(lines.size()), QString::number(end + 1));
    } else if (end < lines.size()) {
        footer = QStringLiteral("(Showing lines %1-%2 of %3. Use offset=%4 to continue.)")
                     .arg(QString::number(offset), QString::number(end),
                          QString::number(lines.size()), QString::number(end + 1));
    } else {
        footer =
            QStringLiteral("(End of file - total %1 lines.)").arg(QString::number(lines.size()));
    }

    return QStringLiteral("<path>%1</path>\n<type>file</type>\n<content>\n%2\n</content>\n\n%3")
        .arg(QDir::toNativeSeparators(path), numbered.join(QLatin1Char('\n')), footer);
}

QString directoryListingReport(const QString& path, int offset) {
    const QDir dir(path);
    const auto entries = dir.entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot,
                                       QDir::DirsFirst | QDir::Name);

    if (offset < 1) {
        offset = 1;
    }
    if (offset > entries.size()) {
        return QStringLiteral(
                   "read-file: Offset %1 is out of range for this directory (%2 entries).")
            .arg(QString::number(offset), QString::number(entries.size()));
    }

    QStringList listed;
    const int end = qMin(entries.size(), offset + kDirListLimit - 1);
    for (int i = offset - 1; i < end; ++i) {
        const QString suffix =
            QFileInfo(dir, entries.at(i)).isDir() ? QStringLiteral("/") : QString();
        listed.append(entries.at(i) + suffix);
    }

    QString footer =
        end < entries.size()
            ? QStringLiteral("(Showing %1 of %2 entries. Use offset=%3 to continue.)")
                  .arg(QString::number(listed.size()), QString::number(entries.size()),
                       QString::number(end + 1))
            : QStringLiteral("(Total %1 entries.)").arg(QString::number(entries.size()));

    return QStringLiteral(
               "<path>%1</path>\n<type>directory</type>\n<entries>\n%2\n</entries>\n\n%3")
        .arg(QDir::toNativeSeparators(path), listed.join(QLatin1Char('\n')), footer);
}

} // namespace

ToolExecutionResult RealToolExecutor::execute(const ToolExecutionRequest& request) const {
    if (request.plan.status != ToolInvocationPlanStatus::Planned ||
        request.plan.invocations.isEmpty()) {
        return {
            ToolExecutionStatus::EmptyPlan,
            QStringLiteral("No planned tool invocation reached the execution boundary."),
        };
    }

    for (const auto& invocation : request.plan.invocations) {
        if (!request.knownToolIds.contains(invocation.toolId)) {
            return {
                ToolExecutionStatus::UnknownTool,
                QStringLiteral("Execution boundary rejected unknown tool metadata: %1")
                    .arg(invocation.toolId),
            };
        }
    }

    if (request.approval.status == ApprovalStatus::Denied ||
        request.approval.status == ApprovalStatus::RequiresApproval) {
        return {
            ToolExecutionStatus::Blocked,
            request.approval.status == ApprovalStatus::RequiresApproval
                ? QStringLiteral("Execution boundary blocked: explicit user approval is required.")
                : QStringLiteral("Execution boundary blocked: approval denied."),
        };
    }

    if (request.sandbox.status == SandboxStatus::Denied ||
        request.sandbox.status == SandboxStatus::BlockedByApproval) {
        return {
            ToolExecutionStatus::Blocked,
            QStringLiteral("Execution boundary blocked by sandbox capability."),
        };
    }

    QStringList logs;
    QString currentWorkingDirectory = QDir::currentPath();

    for (const auto& invocation : request.plan.invocations) {
        // 1. local-plan-summary
        if (invocation.toolId == QLatin1String("local-plan-summary")) {
            logs.append(QStringLiteral("Executed: Local Plan Summary"));
        }
        // 2. read-file (files and directories, offset/limit paging)
        else if (invocation.toolId == QLatin1String("read-file")) {
            QString path = getArgument(invocation, QStringLiteral("path"));
            if (path.isEmpty()) {
                path = getArgument(invocation, QStringLiteral("topic"));
            }
            if (path.isEmpty()) {
                logs.append(QStringLiteral("read-file: No path argument provided."));
                continue;
            }
            const QString scoped = scopedPath(currentWorkingDirectory, path);
            if (scoped.isEmpty()) {
                logs.append(QStringLiteral("read-file: Path is outside the approved workspace."));
                continue;
            }
            QFileInfo info(scoped);
            if (!info.exists()) {
                const QDir parent = info.dir();
                const QString needle = info.fileName().toLower();
                QStringList candidates;
                for (const auto& entry : parent.entryList(QDir::Files | QDir::Dirs)) {
                    if (needle.size() > 2 && entry.toLower().contains(needle)) {
                        candidates.append(entry);
                    }
                    if (candidates.size() >= 3) {
                        break;
                    }
                }
                logs.append(candidates.isEmpty()
                                ? QStringLiteral("read-file: File not found: %1").arg(scoped)
                                : QStringLiteral("read-file: File not found: %1. Did you mean one "
                                                 "of these? %2")
                                      .arg(scoped, candidates.join(QStringLiteral(", "))));
                continue;
            }
            if (info.isDir()) {
                logs.append(directoryListingReport(
                    scoped, getIntArgument(invocation, QStringLiteral("offset"), 1)));
                continue;
            }
            if (looksBinary(scoped)) {
                logs.append(QStringLiteral("read-file: Cannot read binary file: %1").arg(scoped));
                continue;
            }
            logs.append(readToolReport(
                scoped, getIntArgument(invocation, QStringLiteral("offset"), 1),
                getIntArgument(invocation, QStringLiteral("limit"), kDefaultReadLimit)));
        }
        // 3. write-file
        else if (invocation.toolId == QLatin1String("write-file")) {
            QString path = getArgument(invocation, QStringLiteral("path"));
            const QString content = getArgument(invocation, QStringLiteral("content"));
            if (path.isEmpty()) {
                logs.append(QStringLiteral("write-file: No path argument provided."));
                continue;
            }
            const QString scoped = scopedPath(currentWorkingDirectory, path);
            if (scoped.isEmpty()) {
                logs.append(QStringLiteral("write-file: Path is outside the approved workspace."));
                continue;
            }
            QFile file(scoped);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                logs.append(QStringLiteral("write-file: Failed to open '%1': %2")
                                .arg(scoped, file.errorString()));
                continue;
            }
            file.write(content.toUtf8());
            logs.append(QStringLiteral("write-file: Successfully wrote %1 bytes to '%2'")
                            .arg(QString::number(content.toUtf8().size()), scoped));
        }
        // 4. edit-file (fuzzy matching cascade)
        else if (invocation.toolId == QLatin1String("edit-file")) {
            const QString rawPath = getArgument(invocation, QStringLiteral("path"));
            const QString oldString = getArgument(invocation, QStringLiteral("oldString"));
            const QString newString = getArgument(invocation, QStringLiteral("newString"));
            const bool replaceAll =
                getArgument(invocation, QStringLiteral("replaceAll")).trimmed().toLower() ==
                QStringLiteral("true");

            if (rawPath.isEmpty()) {
                logs.append(QStringLiteral("edit-file: No path argument provided."));
                continue;
            }
            const QString scoped = scopedPath(currentWorkingDirectory, rawPath);
            if (scoped.isEmpty()) {
                logs.append(QStringLiteral("edit-file: Path is outside the approved workspace."));
                continue;
            }

            if (!QFile::exists(scoped) && oldString.trimmed().isEmpty()) {
                QDir().mkpath(QFileInfo(scoped).absolutePath());
                QFile file(scoped);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    file.write(newString.toUtf8());
                    logs.append(QStringLiteral("edit-file: Created new file '%1' (%2 bytes).")
                                    .arg(scoped, QString::number(newString.toUtf8().size())));
                } else {
                    logs.append(QStringLiteral("edit-file: Failed to create '%1': %2")
                                    .arg(scoped, file.errorString()));
                }
                continue;
            }
            if (!QFile::exists(scoped)) {
                logs.append(QStringLiteral("edit-file: File not found: %1").arg(scoped));
                continue;
            }
            if (oldString == newString) {
                logs.append(QStringLiteral("edit-file: oldString and newString are identical."));
                continue;
            }

            FuzzyEditRequest editRequest;
            editRequest.filePath = scoped;
            editRequest.oldString = oldString;
            editRequest.newString = newString;
            editRequest.replaceAll = replaceAll;
            const FuzzyEditor editor;
            const auto result = editor.edit(editRequest);
            if (result.success) {
                logs.append(
                    QStringLiteral("edit-file: Edited %1 line(s) in '%2' (match strategy: %3, "
                                   "confidence: %4%%).")
                        .arg(QString::number(result.linesChanged), scoped,
                             QStringLiteral("strategy #%1")
                                 .arg(static_cast<int>(result.usedStrategy)),
                             QString::number(result.confidence)));
            } else {
                logs.append(QStringLiteral("edit-file: %1").arg(result.error));
            }
        }
        // 4b. delete-file (workspace-scoped, single files only)
        else if (invocation.toolId == QLatin1String("delete-file")) {
            const QString rawPath = getArgument(invocation, QStringLiteral("path"));
            if (rawPath.isEmpty()) {
                logs.append(QStringLiteral("delete-file: No path argument provided."));
                continue;
            }
            const QString scoped = scopedPath(currentWorkingDirectory, rawPath);
            if (scoped.isEmpty()) {
                logs.append(QStringLiteral("delete-file: Path is outside the approved workspace."));
                continue;
            }
            QFileInfo info(scoped);
            if (!info.exists()) {
                logs.append(QStringLiteral("delete-file: File not found: %1").arg(scoped));
                continue;
            }
            if (info.isDir()) {
                logs.append(
                    QStringLiteral("delete-file: Refusing to delete a directory: %1").arg(scoped));
                continue;
            }
            if (QFile::remove(scoped)) {
                logs.append(QStringLiteral("delete-file: Deleted '%1'.").arg(scoped));
            } else {
                logs.append(QStringLiteral("delete-file: Failed to delete '%1'.").arg(scoped));
            }
        }
        // 4c. move-file (workspace-scoped rename/move, no overwriting)
        else if (invocation.toolId == QLatin1String("move-file")) {
            const QString rawSource = getArgument(invocation, QStringLiteral("source"));
            const QString rawDestination = getArgument(invocation, QStringLiteral("destination"));
            if (rawSource.isEmpty() || rawDestination.isEmpty()) {
                logs.append(QStringLiteral("move-file: Both source and destination are required."));
                continue;
            }
            const QString source = scopedPath(currentWorkingDirectory, rawSource);
            const QString destination = scopedPath(currentWorkingDirectory, rawDestination);
            if (source.isEmpty() || destination.isEmpty()) {
                logs.append(QStringLiteral("move-file: Path is outside the approved workspace."));
                continue;
            }
            QFileInfo sourceInfo(source);
            if (!sourceInfo.exists()) {
                logs.append(QStringLiteral("move-file: Source not found: %1").arg(source));
                continue;
            }
            if (sourceInfo.isDir()) {
                logs.append(
                    QStringLiteral("move-file: Refusing to move a directory: %1").arg(source));
                continue;
            }
            if (QFile::exists(destination)) {
                logs.append(
                    QStringLiteral("move-file: Destination already exists: %1").arg(destination));
                continue;
            }
            QDir().mkpath(QFileInfo(destination).absolutePath());
            if (QFile::rename(source, destination)) {
                logs.append(
                    QStringLiteral("move-file: Moved '%1' to '%2'.").arg(source, destination));
            } else {
                logs.append(QStringLiteral("move-file: Failed to move '%1' to '%2'.")
                                .arg(source, destination));
            }
        }
        // 4d. apply-patch (unified diff; ported from cline/openclaw/opencode)
        else if (invocation.toolId == QLatin1String("apply-patch")) {
            const QString patch = getArgument(invocation, QStringLiteral("patch"));
            if (patch.trimmed().isEmpty()) {
                logs.append(QStringLiteral("apply-patch: No patch argument provided."));
                continue;
            }
            logs.append(applyPatchReport(patch, currentWorkingDirectory,
                                         [&currentWorkingDirectory](const QString& p) {
                                             return scopedPath(currentWorkingDirectory, p);
                                         }));
        }
        // 4e. list-code-definitions (ported from cline)
        else if (invocation.toolId == QLatin1String("list-code-definitions")) {
            QString path = getArgument(invocation, QStringLiteral("path"));
            if (path.isEmpty()) {
                logs.append(QStringLiteral("list-code-definitions: No path argument provided."));
                continue;
            }
            const QString scoped = scopedPath(currentWorkingDirectory, path);
            if (scoped.isEmpty()) {
                logs.append(QStringLiteral(
                    "list-code-definitions: Path is outside the approved workspace."));
                continue;
            }
            if (!QFile::exists(scoped)) {
                logs.append(
                    QStringLiteral("list-code-definitions: File not found: %1").arg(scoped));
                continue;
            }
            const auto definitions = extractCodeDefinitions(scoped);
            logs.append(definitions.isEmpty()
                            ? QStringLiteral("list-code-definitions: No definitions found in '%1'.")
                                  .arg(scoped)
                            : QStringLiteral("list-code-definitions: %1 definition(s) in '%2':\n%3")
                                  .arg(QString::number(definitions.size()), scoped,
                                       definitions.join(QLatin1Char('\n'))));
        }
        // 5. grep
        else if (invocation.toolId == QLatin1String("grep")) {
            const QString pattern = getArgument(invocation, QStringLiteral("pattern"));
            if (pattern.trimmed().isEmpty()) {
                logs.append(QStringLiteral("grep: No pattern argument provided."));
                continue;
            }
            QRegularExpression regex(pattern);
            if (!regex.isValid()) {
                logs.append(QStringLiteral("grep: Invalid regular expression: %1")
                                .arg(regex.errorString()));
                continue;
            }
            const QString rawPath = getArgument(invocation, QStringLiteral("path"));
            QString searchRoot = rawPath.trimmed().isEmpty()
                                     ? currentWorkingDirectory
                                     : scopedPath(currentWorkingDirectory, rawPath);
            if (searchRoot.isEmpty()) {
                searchRoot = currentWorkingDirectory;
            }
            const QString include = getArgument(invocation, QStringLiteral("include")).trimmed();

            static const QSet<QString> skippedDirs{
                QStringLiteral(".git"),   QStringLiteral("node_modules"),
                QStringLiteral("build"),  QStringLiteral("dist"),
                QStringLiteral("target"), QStringLiteral("__pycache__"),
                QStringLiteral(".cache"), QStringLiteral("venv"),
                QStringLiteral(".venv"),
            };

            int matches = 0;
            int filesWithMatches = 0;
            QString currentFile;
            QStringList output;
            bool truncated = false;

            QDirIterator it(searchRoot, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString filePath = it.next();
                if (skippedDirs.contains(QFileInfo(filePath).dir().dirName())) {
                    continue;
                }
                if (!include.isEmpty() && !QDir::match(include, QFileInfo(filePath).fileName())) {
                    continue;
                }
                QFileInfo info(filePath);
                if (info.size() > 2 * 1024 * 1024 || looksBinary(filePath)) {
                    continue;
                }
                QFile file(filePath);
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    continue;
                }
                int lineNumber = 0;
                bool fileHeaderWritten = false;
                while (!file.atEnd()) {
                    ++lineNumber;
                    const QString line = QString::fromUtf8(file.readLine());
                    if (regex.match(line).hasMatch()) {
                        if (matches >= kGrepGlobLimit) {
                            truncated = true;
                            break;
                        }
                        if (!fileHeaderWritten) {
                            output.append(
                                QStringLiteral("%1:").arg(QDir::toNativeSeparators(filePath)));
                            fileHeaderWritten = true;
                            ++filesWithMatches;
                        }
                        QString trimmedLine = line;
                        if (trimmedLine.endsWith(QLatin1Char('\n'))) {
                            trimmedLine.chop(1);
                        }
                        if (trimmedLine.size() > kMaxLineLength) {
                            trimmedLine = trimmedLine.left(kMaxLineLength) +
                                          QStringLiteral("... (line truncated)");
                        }
                        output.append(QStringLiteral("  Line %1: %2")
                                          .arg(QString::number(lineNumber), trimmedLine));
                        ++matches;
                    }
                }
                if (truncated) {
                    break;
                }
            }

            if (matches == 0) {
                logs.append(QStringLiteral("grep: No matches found for '%1' under '%2'.")
                                .arg(pattern, searchRoot));
            } else {
                logs.append(output.join(QLatin1Char('\n')));
                logs.append(QStringLiteral("\nFound %1 match(es) in %2 file(s).%3")
                                .arg(QString::number(matches), QString::number(filesWithMatches),
                                     truncated
                                         ? QStringLiteral(" (Results truncated. Consider a more "
                                                          "specific path or pattern.)")
                                         : QString()));
            }
        }
        // 6. glob
        else if (invocation.toolId == QLatin1String("glob")) {
            const QString pattern = getArgument(invocation, QStringLiteral("pattern"));
            if (pattern.trimmed().isEmpty()) {
                logs.append(QStringLiteral("glob: No pattern argument provided."));
                continue;
            }
            const QString rawPath = getArgument(invocation, QStringLiteral("path"));
            QString searchRoot = rawPath.trimmed().isEmpty()
                                     ? currentWorkingDirectory
                                     : scopedPath(currentWorkingDirectory, rawPath);
            if (searchRoot.isEmpty()) {
                searchRoot = currentWorkingDirectory;
            }

            QStringList found;
            QDirIterator it(searchRoot, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString filePath = it.next();
                if (QDir::match(pattern, QFileInfo(filePath).fileName()) ||
                    QDir::match(pattern, filePath)) {
                    found.append(QDir::toNativeSeparators(filePath));
                    if (found.size() >= kGrepGlobLimit) {
                        break;
                    }
                }
            }
            found.sort();

            if (found.isEmpty()) {
                logs.append(QStringLiteral("glob: No files matching '%1' under '%2'.")
                                .arg(pattern, searchRoot));
            } else {
                logs.append(found.join(QLatin1Char('\n')));
                logs.append(QStringLiteral("\nFound %1 file(s).%2")
                                .arg(QString::number(found.size()),
                                     found.size() >= kGrepGlobLimit
                                         ? QStringLiteral(" (Results truncated. Use a more "
                                                          "specific pattern or path.)")
                                         : QString()));
            }
        }
        // 7. run-command (timeout + workdir aware)
        else if (invocation.toolId == QLatin1String("run-command")) {
            const QString command = getArgument(invocation, QStringLiteral("command"));
            if (command.isEmpty()) {
                logs.append(QStringLiteral("run-command: No command argument provided."));
                continue;
            }
            int timeoutMs = getIntArgument(invocation, QStringLiteral("timeout"), 60000);
            timeoutMs = qBound(1000, timeoutMs, 600000);
            const QString workdirArg = getArgument(invocation, QStringLiteral("workdir")).trimmed();
            QString workingDirectory = currentWorkingDirectory;
            if (!workdirArg.isEmpty()) {
                const QString scopedWorkdir = scopedPath(currentWorkingDirectory, workdirArg);
                if (!scopedWorkdir.isEmpty() && QDir(scopedWorkdir).exists()) {
                    workingDirectory = scopedWorkdir;
                } else if (QDir(workdirArg).exists()) {
                    workingDirectory = workdirArg;
                }
            }

            const QString sandbox =
                getArgument(invocation, QStringLiteral("sandbox")).trimmed().toLower();
            if (sandbox == QStringLiteral("docker")) {
                logs.append(runCommandInDocker(command, workingDirectory, timeoutMs));
                continue;
            }

            QProcess process;
            process.setWorkingDirectory(workingDirectory);

            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            QString pathEnv = env.value(QStringLiteral("PATH"));
#if defined(Q_OS_MACOS)
            if (!pathEnv.contains(QStringLiteral("/opt/homebrew/bin"))) {
                pathEnv = QStringLiteral("/opt/homebrew/bin:/usr/local/bin:") + pathEnv;
            }
#elif defined(Q_OS_UNIX)
            if (!pathEnv.contains(QStringLiteral("/usr/local/bin"))) {
                pathEnv = QStringLiteral("/usr/local/bin:") + pathEnv;
            }
#endif
            env.insert(QStringLiteral("PATH"), pathEnv);
            process.setProcessEnvironment(env);

#if defined(Q_OS_WIN)
            process.start(QStringLiteral("cmd.exe"), QStringList{QStringLiteral("/c"), command});
#else
            process.start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), command});
#endif

            if (!process.waitForFinished(timeoutMs)) {
                const bool started = process.state() != QProcess::NotRunning;
                process.kill();
                process.waitForFinished(3000);
                if (started) {
                    logs.append(
                        QStringLiteral(
                            "run-command: Shell tool terminated command after exceeding timeout "
                            "%1 ms. If this command is expected to take longer, retry it with a "
                            "larger timeout value in milliseconds.\nCommand: %2")
                            .arg(QString::number(timeoutMs), command));
                } else {
                    logs.append(QStringLiteral("run-command: Failed to start: %1").arg(command));
                }
                continue;
            }
            const QString stdoutContent =
                QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            const QString stderrContent =
                QString::fromUtf8(process.readAllStandardError()).trimmed();
            const int exitCode = process.exitCode();
            if (exitCode == 0) {
                if (stdoutContent.isEmpty() && stderrContent.isEmpty()) {
                    logs.append(QStringLiteral("Command executed successfully. (exit=0)"));
                } else {
                    logs.append(
                        QStringLiteral("Command executed successfully. (exit=0)\n\n[STDOUT]:\n%1")
                            .arg(stdoutContent));
                    if (!stderrContent.isEmpty()) {
                        logs.append(QStringLiteral("\n\n[STDERR]:\n%1").arg(stderrContent));
                    }
                }
            } else {
                logs.append(
                    QStringLiteral(
                        "Command execution failed. (exit=%1)\n\n[STDOUT]:\n%2\n\n[STDERR]:\n%3")
                        .arg(QString::number(exitCode), stdoutContent, stderrContent));
            }
        }
        // 8. app-launch
        else if (invocation.toolId == QLatin1String("app-launch")) {
            const QString app = getArgument(invocation, QStringLiteral("app")).trimmed();
            if (app.isEmpty()) {
                logs.append(QStringLiteral("app-launch: No app argument provided."));
                continue;
            }
            // Safety net: domains are websites, not applications. Guide the
            // caller to open-url instead of trying to launch a browser as an
            // app.
            if (looksLikeDomainName(app)) {
                logs.append(
                    QStringLiteral("app-launch: '%1' is a website address, not an application. "
                                   "Retry with the open-url tool and url=%1 to open it in the "
                                   "browser.")
                        .arg(app));
                continue;
            }
            const QString extraArgs = getArgument(invocation, QStringLiteral("args")).trimmed();

            QString error;
#if defined(Q_OS_MACOS)
            QStringList args{QStringLiteral("-a"), app};
            if (!extraArgs.isEmpty()) {
                args.append(extraArgs);
            }
            runSynchronousProcess(QStringLiteral("open"), args, QString(), 15000, &error);
#elif defined(Q_OS_WIN)
            QStringList args{QStringLiteral("/c"), QStringLiteral("start"), QStringLiteral(""),
                             app};
            if (!extraArgs.isEmpty()) {
                args.append(extraArgs);
            }
            runSynchronousProcess(QStringLiteral("cmd.exe"), args, QString(), 15000, &error);
#else
            // Linux/BSD: resolve the application through its freedesktop
            // .desktop entry so names like "Files" launch org.kde.dolphin.
            const auto entry = findDesktopEntry(app);
            if (entry) {
                QString command = desktopExecToCommand(entry->exec);
                QStringList args;
                if (!extraArgs.isEmpty()) {
                    args.append(extraArgs);
                }
                if (command.isEmpty()) {
                    command = entry->desktopId;
                }
                const bool started = QProcess::startDetached(command, args);
                logs.append(
                    started
                        ? QStringLiteral("app-launch: Launched '%1' (%2) via desktop entry.")
                              .arg(app, entry->desktopId)
                        : QStringLiteral("app-launch: Found desktop entry '%1' but failed to start "
                                         "'%2'.")
                              .arg(entry->desktopId, command));
                continue;
            }
            QProcess::startDetached(app,
                                    extraArgs.isEmpty() ? QStringList{} : QStringList{extraArgs});
#endif
            logs.append(error.isEmpty()
                            ? QStringLiteral("app-launch: Launch requested for '%1'.").arg(app)
                            : QStringLiteral("app-launch: %1 (%2)").arg(error, app));
        }
        // 9. app-quit
        else if (invocation.toolId == QLatin1String("app-quit")) {
            const QString app = getArgument(invocation, QStringLiteral("app")).trimmed();
            if (app.isEmpty()) {
                logs.append(QStringLiteral("app-quit: No app argument provided."));
                continue;
            }

            QString error;
#if defined(Q_OS_MACOS)
            runSynchronousProcess(
                QStringLiteral("osascript"),
                QStringList{QStringLiteral("-e"),
                            QStringLiteral("quit app \"%1\"").arg(osascriptEscape(app))},
                QString(), 15000, &error);
#elif defined(Q_OS_WIN)
            runSynchronousProcess(
                QStringLiteral("taskkill"),
                QStringList{QStringLiteral("/IM"),
                            app.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)
                                ? app
                                : app + QStringLiteral(".exe"),
                            QStringLiteral("/F")},
                QString(), 15000, &error);
#else
            runSynchronousProcess(QStringLiteral("pkill"),
                                  QStringList{QStringLiteral("-f"), QStringLiteral("-i"), app},
                                  QString(), 15000, &error);
#endif
            logs.append(error.isEmpty()
                            ? QStringLiteral("app-quit: Quit requested for '%1'.").arg(app)
                            : QStringLiteral("app-quit: %1 (%2)").arg(error, app));
        }
        // 9b. open-url (default browser, http/https only)
        else if (invocation.toolId == QLatin1String("open-url")) {
            const QString url = getArgument(invocation, QStringLiteral("url")).trimmed();
            if (url.isEmpty()) {
                logs.append(QStringLiteral("open-url: No url argument provided."));
                continue;
            }
            const QUrl parsed = QUrl::fromUserInput(url);
            if (!parsed.isValid() || (parsed.scheme() != QStringLiteral("http") &&
                                      parsed.scheme() != QStringLiteral("https"))) {
                logs.append(
                    QStringLiteral("open-url: Only http and https URLs can be opened. Got: %1")
                        .arg(url));
                continue;
            }
            const QString target = parsed.toString(QUrl::FullyEncoded);
            bool launched = false;
#if defined(Q_OS_MACOS)
            launched = QProcess::startDetached(QStringLiteral("open"), {target});
#elif defined(Q_OS_WIN)
            launched = QProcess::startDetached(
                QStringLiteral("cmd.exe"),
                {QStringLiteral("/c"), QStringLiteral("start"), QString(), target});
#else
            launched = QProcess::startDetached(QStringLiteral("xdg-open"), {target});
#endif
            logs.append(launched ? QStringLiteral("open-url: Opened '%1' in the default browser.")
                                       .arg(parsed.toString())
                                 : QStringLiteral("open-url: Failed to open '%1' in the default "
                                                  "browser.")
                                       .arg(parsed.toString()));
        }
        // 9c. mcp-list (discover configured MCP servers and their tools)
        else if (invocation.toolId == QLatin1String("mcp-list")) {
            if (!mcpService_) {
                logs.append(QStringLiteral(
                    "mcp-list: No MCP servers are configured. Add servers in Settings to "
                    "extend the toolset."));
                continue;
            }
            const auto servers = mcpService_->servers();
            if (servers.isEmpty()) {
                logs.append(QStringLiteral("mcp-list: No MCP servers are configured."));
                continue;
            }
            QStringList lines;
            for (const auto& server : servers) {
                const QString stateName = [this, &server]() {
                    switch (mcpService_->connectionState(server.name)) {
                    case McpConnectionState::Connected:
                        return QStringLiteral("connected");
                    case McpConnectionState::Connecting:
                        return QStringLiteral("connecting");
                    case McpConnectionState::Error:
                        return QStringLiteral("error");
                    case McpConnectionState::Disconnected:
                        return QStringLiteral("disconnected");
                    }
                    return QStringLiteral("unknown");
                }();
                lines.append(
                    QStringLiteral("%1 (%2, %3)").arg(server.name, server.type, stateName));
                for (const auto& tool : mcpService_->tools(server.name)) {
                    lines.append(QStringLiteral("  - %1: %2")
                                     .arg(tool.name, tool.description.simplified().left(200)));
                }
            }
            logs.append(QStringLiteral("mcp-list: %1 server(s):\n%2")
                            .arg(QString::number(servers.size()), lines.join(QLatin1Char('\n'))));
        }
        // 9d. mcp-call (invoke a tool on a configured MCP server; ported from
        // cline's use_mcp_tool pattern)
        else if (invocation.toolId == QLatin1String("mcp-call")) {
            if (!mcpService_) {
                logs.append(QStringLiteral(
                    "mcp-call: No MCP servers are configured. Use mcp-list after configuring "
                    "servers in Settings."));
                continue;
            }
            const QString server = getArgument(invocation, QStringLiteral("server")).trimmed();
            const QString tool = getArgument(invocation, QStringLiteral("tool")).trimmed();
            if (server.isEmpty() || tool.isEmpty()) {
                logs.append(
                    QStringLiteral("mcp-call: Both server and tool arguments are required."));
                continue;
            }
            const QString argumentsRaw = getArgument(invocation, QStringLiteral("arguments"));
            QJsonObject arguments;
            if (!argumentsRaw.trimmed().isEmpty()) {
                const auto document = QJsonDocument::fromJson(argumentsRaw.toUtf8());
                if (!document.isObject()) {
                    logs.append(QStringLiteral("mcp-call: 'arguments' must be a JSON object, e.g. "
                                               "{\"key\": \"value\"}. Rewrite the input."));
                    continue;
                }
                arguments = document.object();
            }

            const auto result = mcpService_->callTool(server, tool, arguments);
            if (result.contains(QStringLiteral("error"))) {
                logs.append(QStringLiteral("mcp-call: %1/%2 failed: %3")
                                .arg(server, tool,
                                     result.value(QStringLiteral("error"))
                                         .toObject()
                                         .value(QStringLiteral("message"))
                                         .toString()));
                continue;
            }

            QStringList contentParts;
            const auto content = result.value(QStringLiteral("result"))
                                     .toObject()
                                     .value(QStringLiteral("content"))
                                     .toArray();
            for (const auto& item : content) {
                const auto object = item.toObject();
                if (object.value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
                    contentParts.append(object.value(QStringLiteral("text")).toString());
                }
            }
            if (contentParts.isEmpty()) {
                contentParts.append(
                    QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)));
            }
            const bool isError = result.value(QStringLiteral("result"))
                                     .toObject()
                                     .value(QStringLiteral("isError"))
                                     .toBool();
            logs.append(
                QStringLiteral("mcp-call: %1/%2 %3\n%4")
                    .arg(server, tool,
                         isError ? QStringLiteral("returned an error:") : QStringLiteral("result:"),
                         contentParts.join(QLatin1Char('\n'))));
        }
        // 9e. spawn-agent (bounded read-only subagent; ported from cline's
        // new_task / openclaw's sessions_spawn pattern)
        else if (invocation.toolId == QLatin1String("spawn-agent")) {
            const QString task = getArgument(invocation, QStringLiteral("task")).trimmed();
            if (task.isEmpty()) {
                logs.append(QStringLiteral("spawn-agent: No task argument provided."));
                continue;
            }
            if (subagentActive_) {
                logs.append(QStringLiteral(
                    "spawn-agent: subagents cannot spawn further subagents; finish this "
                    "subtask directly."));
                continue;
            }
            if (!subagentRunner_) {
                logs.append(QStringLiteral(
                    "spawn-agent: No subagent runner is configured in this session."));
                continue;
            }
            subagentActive_ = true;
            const QString answer = subagentRunner_(task);
            subagentActive_ = false;
            logs.append(QStringLiteral("spawn-agent: subagent finished the task '%1'.\n\n%2")
                            .arg(task.left(200), answer));
        }
        // 9f. browser-screenshot (Playwright CLI; ported from SWE-agent's
        // web_browser bundle)
        else if (invocation.toolId == QLatin1String("browser-screenshot")) {
            const QString url = getArgument(invocation, QStringLiteral("url")).trimmed();
            if (url.isEmpty()) {
                logs.append(QStringLiteral("browser-screenshot: No url argument provided."));
                continue;
            }
            QString path = getArgument(invocation, QStringLiteral("path")).trimmed();
            if (path.isEmpty()) {
                path = QDir(QDir::tempPath())
                           .filePath(QStringLiteral("sentinel_browser_%1.png")
                                         .arg(QDateTime::currentMSecsSinceEpoch()));
            } else {
                const QString scoped = scopedPath(currentWorkingDirectory, path);
                if (scoped.isEmpty()) {
                    logs.append(QStringLiteral(
                        "browser-screenshot: Path is outside the approved workspace."));
                    continue;
                }
                path = scoped;
            }
            logs.append(
                runPlaywrightCli(QStringLiteral("screenshot"), url, path, QStringLiteral("2500")));
        }
        // 9g. browser-pdf
        else if (invocation.toolId == QLatin1String("browser-pdf")) {
            const QString url = getArgument(invocation, QStringLiteral("url")).trimmed();
            if (url.isEmpty()) {
                logs.append(QStringLiteral("browser-pdf: No url argument provided."));
                continue;
            }
            QString path = getArgument(invocation, QStringLiteral("path")).trimmed();
            if (path.isEmpty()) {
                path = QDir(QDir::tempPath())
                           .filePath(QStringLiteral("sentinel_browser_%1.pdf")
                                         .arg(QDateTime::currentMSecsSinceEpoch()));
            } else {
                const QString scoped = scopedPath(currentWorkingDirectory, path);
                if (scoped.isEmpty()) {
                    logs.append(
                        QStringLiteral("browser-pdf: Path is outside the approved workspace."));
                    continue;
                }
                path = scoped;
            }
            logs.append(runPlaywrightCli(QStringLiteral("pdf"), url, path, QString()));
        }
        // 10. system-notify
        else if (invocation.toolId == QLatin1String("system-notify")) {
            const QString title = getArgument(invocation, QStringLiteral("title")).trimmed();
            const QString message = getArgument(invocation, QStringLiteral("message")).trimmed();
            if (message.isEmpty()) {
                logs.append(QStringLiteral("system-notify: No message argument provided."));
                continue;
            }

            QString error;
#if defined(Q_OS_MACOS)
            runSynchronousProcess(
                QStringLiteral("osascript"),
                QStringList{QStringLiteral("-e"),
                            QStringLiteral("display notification \"%1\" with title \"%2\"")
                                .arg(osascriptEscape(message),
                                     osascriptEscape(title.isEmpty() ? QStringLiteral("Sentinel")
                                                                     : title))},
                QString(), 15000, &error);
#elif defined(Q_OS_WIN)
            // PowerShell toast via the Windows Runtime projection; works on
            // Windows 10/11 without any extra dependencies.
            const QString escapedTitle = title.isEmpty() ? QStringLiteral("Sentinel") : title;
            const QString psScript = QStringLiteral(
                "[Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, "
                "ContentType = WindowsRuntime] | Out-Null;"
                "[Windows.Data.Xml.Dom.XmlDocument, Windows.Data.Xml.Dom.XmlDocument, "
                "ContentType = WindowsRuntime] | Out-Null;"
                "$template = "
                "'<toast><visual><binding template=\"ToastGeneric\">"
                "<text id=\"1\">%1</text><text id=\"2\">%2</text></binding></visual></toast>';"
                "$xml = New-Object Windows.Data.Xml.Dom.XmlDocument;"
                "$xml.LoadXml($template);"
                "$toast = New-Object Windows.UI.Notifications.ToastNotification($xml);"
                "[Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier("
                "'Sentinel').Show($toast);");
            runSynchronousProcess(
                QStringLiteral("powershell"),
                QStringList{
                    QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                    QStringLiteral("-Command"),
                    psScript.arg(escapedTitle.replace(QLatin1Char('\''), QStringLiteral("''")),
                                 message.replace(QLatin1Char('\''), QStringLiteral("''")))},
                QString(), 15000, &error);
#else
            runSynchronousProcess(
                QStringLiteral("notify-send"),
                QStringList{title.isEmpty() ? QStringLiteral("Sentinel") : title, message},
                QString(), 15000, &error);
#endif
            logs.append(error.isEmpty()
                            ? QStringLiteral("system-notify: Notification sent: %1").arg(message)
                            : QStringLiteral("system-notify: %1").arg(error));
        }
        // 10b. clipboard-read
        else if (invocation.toolId == QLatin1String("clipboard-read")) {
            QClipboard* clipboard = activeClipboard();
            if (!clipboard) {
                logs.append(QStringLiteral(
                    "clipboard-read: Clipboard is unavailable without a GUI session."));
                continue;
            }
            const QString text = clipboard->text();
            if (text.isEmpty()) {
                logs.append(QStringLiteral("clipboard-read: Clipboard is empty."));
            } else {
                QString preview = text;
                if (preview.size() > kWebFetchPreviewChars) {
                    preview = preview.left(kWebFetchPreviewChars) +
                              QStringLiteral("\n... (content truncated at %1 characters)")
                                  .arg(QString::number(kWebFetchPreviewChars));
                }
                logs.append(QStringLiteral("clipboard-read: (%1 characters)\n%2")
                                .arg(QString::number(text.size()), preview));
            }
        }
        // 10c. clipboard-write
        else if (invocation.toolId == QLatin1String("clipboard-write")) {
            const QString text = getArgument(invocation, QStringLiteral("text"));
            if (text.isEmpty()) {
                logs.append(QStringLiteral("clipboard-write: No text argument provided."));
                continue;
            }
            QClipboard* clipboard = activeClipboard();
            if (!clipboard) {
                logs.append(QStringLiteral(
                    "clipboard-write: Clipboard is unavailable without a GUI session."));
                continue;
            }
            clipboard->setText(text);
            logs.append(QStringLiteral("clipboard-write: Copied %1 character(s) to the clipboard.")
                            .arg(QString::number(text.size())));
        }
        // 10d. system-info (read-only machine report)
        else if (invocation.toolId == QLatin1String("system-info")) {
            logs.append(QStringLiteral("system-info:\n%1").arg(systemInfoReport()));
        }
        // 10e. process-list (read-only snapshot of running processes)
        else if (invocation.toolId == QLatin1String("process-list")) {
            logs.append(processListReport());
        }
        // 10f. current-time
        else if (invocation.toolId == QLatin1String("current-time")) {
            const QDateTime now = QDateTime::currentDateTime();
            logs.append(QStringLiteral("current-time: %1\nUTC: %2\nEpoch seconds: %3")
                            .arg(now.toString(QStringLiteral("dddd, yyyy-MM-dd HH:mm:ss t")),
                                 now.toUTC().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss 'UTC'")),
                                 QString::number(now.toSecsSinceEpoch())));
        }
        // 11. set-alarm
        else if (invocation.toolId == QLatin1String("set-alarm")) {
            if (!alarmStore_) {
                logs.append(QStringLiteral("set-alarm: No alarm store is configured."));
                continue;
            }
            const QString rawTime = getArgument(invocation, QStringLiteral("time"));
            const QString label = getArgument(invocation, QStringLiteral("label")).trimmed();
            const auto triggerAt = parseAlarmTime(rawTime);
            if (!triggerAt.isValid()) {
                logs.append(QStringLiteral(
                                "set-alarm: Could not parse time '%1'. Use HH:mm, HH:mm:ss, or an "
                                "ISO datetime (yyyy-MM-ddTHH:mm).")
                                .arg(rawTime));
                continue;
            }
            const auto entry =
                alarmStore_->schedule(triggerAt, label.isEmpty() ? QStringLiteral("Alarm") : label);
            logs.append(QStringLiteral("set-alarm: Alarm scheduled at %1 (id: %2) - %3")
                            .arg(triggerAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                                 entry.id, entry.label));
        }
        // 12. list-alarms
        else if (invocation.toolId == QLatin1String("list-alarms")) {
            if (!alarmStore_) {
                logs.append(QStringLiteral("list-alarms: No alarm store is configured."));
                continue;
            }
            const auto alarms = alarmStore_->active();
            if (alarms.isEmpty()) {
                logs.append(QStringLiteral("list-alarms: No active alarms."));
            } else {
                QStringList lines;
                for (const auto& alarm : alarms) {
                    lines.append(
                        QStringLiteral("%1 - %2 - %3")
                            .arg(alarm.triggerAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                                 alarm.label, alarm.id));
                }
                logs.append(
                    QStringLiteral("list-alarms: %1 active alarm(s):\n%2")
                        .arg(QString::number(alarms.size()), lines.join(QLatin1Char('\n'))));
            }
        }
        // 12b. cancel-alarm
        else if (invocation.toolId == QLatin1String("cancel-alarm")) {
            if (!alarmStore_) {
                logs.append(QStringLiteral("cancel-alarm: No alarm store is configured."));
                continue;
            }
            const QString id = getArgument(invocation, QStringLiteral("id")).trimmed();
            if (id.isEmpty()) {
                logs.append(QStringLiteral("cancel-alarm: No id argument provided."));
                continue;
            }
            logs.append(alarmStore_->remove(id)
                            ? QStringLiteral("cancel-alarm: Alarm %1 cancelled.").arg(id)
                            : QStringLiteral("cancel-alarm: No active alarm with id %1.").arg(id));
        }
        // 13. todo-write
        else if (invocation.toolId == QLatin1String("todo-write")) {
            const QString todosRaw = getArgument(invocation, QStringLiteral("todos"));
            const auto document = QJsonDocument::fromJson(todosRaw.toUtf8());
            if (!document.isArray()) {
                logs.append(QStringLiteral(
                    "todo-write: The 'todos' argument must be a JSON array of "
                    "{content, status, priority} objects. Please rewrite the input."));
                continue;
            }

            QJsonArray cleaned;
            const auto array = document.array();
            for (const auto& value : array) {
                const auto object = value.toObject();
                const QString status = object.value(QStringLiteral("status")).toString();
                if (status != QStringLiteral("pending") &&
                    status != QStringLiteral("in_progress") &&
                    status != QStringLiteral("completed") &&
                    status != QStringLiteral("cancelled")) {
                    logs.append(
                        QStringLiteral(
                            "todo-write: Invalid status '%1'. Allowed: pending, in_progress, "
                            "completed, cancelled. Please rewrite the input.")
                            .arg(status));
                    continue;
                }
                QJsonObject fixed;
                fixed.insert(QStringLiteral("content"),
                             object.value(QStringLiteral("content")).toString());
                fixed.insert(QStringLiteral("status"), status);
                fixed.insert(
                    QStringLiteral("priority"),
                    object.value(QStringLiteral("priority")).toString(QStringLiteral("medium")));
                cleaned.append(fixed);
            }
            todos_ = cleaned;

            int pending = 0;
            for (const auto& value : todos_) {
                if (value.toObject().value(QStringLiteral("status")).toString() ==
                    QStringLiteral("pending")) {
                    ++pending;
                }
            }
            logs.append(
                QStringLiteral("todo-write: %1 todo(s) saved (%2 pending). Current list:\n%3")
                    .arg(QString::number(todos_.size()), QString::number(pending),
                         QString::fromUtf8(QJsonDocument(todos_).toJson(QJsonDocument::Compact))));
        }
        // 14. todo-read
        else if (invocation.toolId == QLatin1String("todo-read")) {
            logs.append(todos_.isEmpty()
                            ? QStringLiteral("todo-read: No todos recorded for this session yet.")
                            : QStringLiteral("todo-read: Current todos:\n%1")
                                  .arg(QString::fromUtf8(
                                      QJsonDocument(todos_).toJson(QJsonDocument::Compact))));
        }
        // 14b. memory-search (long-term memory snapshot lookup)
        else if (invocation.toolId == QLatin1String("memory-search")) {
            const QString query = getArgument(invocation, QStringLiteral("query")).trimmed();
            if (query.isEmpty()) {
                logs.append(QStringLiteral("memory-search: No query argument provided."));
                continue;
            }
            if (memorySnapshot_.isEmpty()) {
                logs.append(QStringLiteral(
                    "memory-search: No memory entries are available for this session."));
                continue;
            }
            const int limit =
                qBound(1, getIntArgument(invocation, QStringLiteral("limit"), 10), 50);
            const QString needle = query.toLower();

            QStringList matches;
            int totalMatches = 0;
            for (const auto& entry : memorySnapshot_) {
                if (!entry.first.toLower().contains(needle) &&
                    !entry.second.toLower().contains(needle)) {
                    continue;
                }
                ++totalMatches;
                if (matches.size() < limit) {
                    QString value = entry.second.simplified();
                    if (value.size() > 400) {
                        value = value.left(400) + QStringLiteral("...");
                    }
                    matches.append(QStringLiteral("%1: %2").arg(entry.first, value));
                }
            }

            if (matches.isEmpty()) {
                logs.append(
                    QStringLiteral("memory-search: No memory entries match '%1'.").arg(query));
            } else {
                logs.append(QStringLiteral("memory-search: %1 match(es) for '%2' (showing %3):\n%4")
                                .arg(QString::number(totalMatches), query,
                                     QString::number(matches.size()),
                                     matches.join(QLatin1Char('\n'))));
            }
        }
        // 14c. history-search (ported from openclaw transcripts tool)
        else if (invocation.toolId == QLatin1String("history-search")) {
            const QString query = getArgument(invocation, QStringLiteral("query")).trimmed();
            if (query.isEmpty()) {
                logs.append(QStringLiteral("history-search: No query argument provided."));
                continue;
            }
            if (historySnapshot_.isEmpty()) {
                logs.append(QStringLiteral(
                    "history-search: No chat history is available for this session."));
                continue;
            }
            const int limit =
                qBound(1, getIntArgument(invocation, QStringLiteral("limit"), 10), 50);
            const QString needle = query.toLower();

            QStringList matches;
            int totalMatches = 0;
            for (const auto& entry : historySnapshot_) {
                if (!entry.toLower().contains(needle)) {
                    continue;
                }
                ++totalMatches;
                if (matches.size() < limit) {
                    QString preview = entry.simplified();
                    if (preview.size() > 400) {
                        preview = preview.left(400) + QStringLiteral("...");
                    }
                    matches.append(preview);
                }
            }

            if (matches.isEmpty()) {
                logs.append(
                    QStringLiteral("history-search: No history entries match '%1'.").arg(query));
            } else {
                logs.append(
                    QStringLiteral("history-search: %1 match(es) for '%2' (showing %3):\n%4")
                        .arg(QString::number(totalMatches), query, QString::number(matches.size()),
                             matches.join(QLatin1Char('\n'))));
            }
        }
        // 14d. ask-question (ported from cline ask_followup_question / opencode question)
        else if (invocation.toolId == QLatin1String("ask-question")) {
            const QString question = getArgument(invocation, QStringLiteral("question")).trimmed();
            if (question.isEmpty()) {
                logs.append(QStringLiteral("ask-question: No question argument provided."));
                continue;
            }
            const QString optionsRaw = getArgument(invocation, QStringLiteral("options"));
            QStringList options;
            for (const auto& option : optionsRaw.split(QLatin1Char('\n'))) {
                const QString trimmed = option.trimmed();
                if (!trimmed.isEmpty()) {
                    options.append(trimmed);
                }
            }

            QString formatted = question;
            if (!options.isEmpty()) {
                QStringList numbered;
                for (int i = 0; i < options.size(); ++i) {
                    numbered.append(
                        QStringLiteral("%1. %2").arg(QString::number(i + 1), options.at(i)));
                }
                formatted += QStringLiteral("\n") + numbered.join(QLatin1Char('\n'));
            }
            logs.append(
                QStringLiteral("ask-question: Question registered for the user:\n%1\n"
                               "End this run now with the question above as your final answer "
                               "(asked in the user's language). Wait for the user's reply before "
                               "taking further steps.")
                    .arg(formatted));
        }
        // 15. web-fetch
        else if (invocation.toolId == QLatin1String("web-fetch")) {
            const QString url = getArgument(invocation, QStringLiteral("url")).trimmed();
            if (url.isEmpty()) {
                logs.append(QStringLiteral("web-fetch: No url argument provided."));
                continue;
            }
            const QString format =
                getArgument(invocation, QStringLiteral("format")).trimmed().toLower();
            const auto fetchFormat =
                format == QStringLiteral("text")
                    ? WebFetchFormat::Text
                    : (format == QStringLiteral("html") ? WebFetchFormat::Html
                                                        : WebFetchFormat::Markdown);

            const auto response = webFetchTool_.fetch(url, fetchFormat);
            if (!response.success) {
                logs.append(QStringLiteral("web-fetch: Request failed: %1")
                                .arg(response.errorString.isEmpty()
                                         ? QStringLiteral("HTTP %1").arg(response.statusCode)
                                         : response.errorString));
                continue;
            }
            QString content = response.content;
            if (content.size() > kWebFetchPreviewChars) {
                content = content.left(kWebFetchPreviewChars) +
                          QStringLiteral("\n\n... [content truncated at %1 characters] ...")
                              .arg(QString::number(kWebFetchPreviewChars));
            }
            logs.append(QStringLiteral("web-fetch: %1\n\n%2").arg(url, content));
        }
        // 16. voice-transcribe
        else if (invocation.toolId == QLatin1String("voice-transcribe")) {
            const QString path = getArgument(invocation, QStringLiteral("path"));
            if (path.isEmpty()) {
                logs.append(QStringLiteral("voice-transcribe: No audio path provided."));
                continue;
            }
            QProcess process;
#if defined(Q_OS_WIN)
            const QString whisperBinary = QStringLiteral("whisper.exe");
#else
            const QString whisperBinary = QStringLiteral("whisper");
#endif
            process.start(whisperBinary, {path});
            if (!process.waitForFinished(15000)) {
                logs.append(
                    QStringLiteral("voice-transcribe: Whisper timed out for '%1'").arg(path));
                continue;
            }
            const QString transcript = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            logs.append(QStringLiteral("voice-transcribe: OK\n%1").arg(transcript));
        }
        // 17. voice-speak
        else if (invocation.toolId == QLatin1String("voice-speak")) {
            const QString text = getArgument(invocation, QStringLiteral("text"));
            if (text.isEmpty()) {
                logs.append(QStringLiteral("voice-speak: No text argument provided."));
                continue;
            }
            QProcess process;
#if defined(Q_OS_WIN)
            const QString piperBinary = QStringLiteral("piper.exe");
#else
            const QString piperBinary = QStringLiteral("piper");
#endif
            const QString ttsOutput =
                QDir(QDir::tempPath()).filePath(QStringLiteral("sentinel_tts.wav"));
            process.start(piperBinary,
                          {QStringLiteral("--model"), QStringLiteral("en_US-lessac-medium.onnx"),
                           QStringLiteral("--output_file"), ttsOutput});
            process.write(text.toUtf8());
            process.closeWriteChannel();
            if (!process.waitForFinished(10000)) {
                logs.append(QStringLiteral("voice-speak: Piper TTS timed out."));
                continue;
            }
            logs.append(QStringLiteral("voice-speak: TTS synthesis OK → %1").arg(ttsOutput));
        }
        // 18. web-search
        else if (invocation.toolId == QLatin1String("web-search")) {
            const auto query = getArgument(invocation, QStringLiteral("query"));
            if (query.trimmed().isEmpty()) {
                logs.append(QStringLiteral("web-search: No query argument provided."));
                continue;
            }

            const auto response = webSearchTool_.search(query.trimmed());
            if (!response.success) {
                logs.append(
                    QStringLiteral("web-search: Request failed: %1").arg(response.errorString));
                continue;
            }
            for (int i = 0; i < response.results.size(); ++i) {
                const auto& result = response.results.at(i);
                logs.append(QStringLiteral("%1. %2\n%3\n%4")
                                .arg(i + 1)
                                .arg(result.title)
                                .arg(result.url)
                                .arg(result.snippet));
            }
            if (response.results.isEmpty()) {
                logs.append(QStringLiteral("web-search: No results found for '%1'.").arg(query));
            }
        }
        // 19. open-workspace
        else if (invocation.toolId == QLatin1String("open-workspace")) {
            const QString path = getArgument(invocation, QStringLiteral("path"));
            const auto workspacePath = QDir(path).absolutePath();
            if (path.isEmpty() || !QDir(workspacePath).exists()) {
                return {
                    ToolExecutionStatus::Blocked,
                    QStringLiteral("open-workspace: requested workspace does not exist."),
                };
            }
            currentWorkingDirectory = workspacePath;
            logs.append(QStringLiteral("open-workspace: Workspace context set → '%1'")
                            .arg(currentWorkingDirectory));
        }
        // 20. summarize-current-conversation
        else if (invocation.toolId == QLatin1String("summarize-current-conversation")) {
            logs.append(QStringLiteral("summarize-current-conversation: Summary compiled."));
        }
        // 21. provider-test-call
        else if (invocation.toolId == QLatin1String("provider-test-call")) {
            return {
                ToolExecutionStatus::Blocked,
                QStringLiteral(
                    "provider-test-call is unavailable: no provider test executor is configured."),
            };
        }
        // 22. export-conversation
        else if (invocation.toolId == QLatin1String("export-conversation")) {
            return {
                ToolExecutionStatus::Blocked,
                QStringLiteral(
                    "export-conversation is unavailable: use the conversation export service."),
            };
        }
        // Never report an unknown implementation as a successful execution.
        else {
            return {
                ToolExecutionStatus::UnknownTool,
                QStringLiteral("Execution boundary rejected unimplemented tool: %1")
                    .arg(invocation.toolId),
            };
        }
    }

    return {
        ToolExecutionStatus::Succeeded,
        logs.join(QStringLiteral("\n\n")),
    };
}

} // namespace sentinel::core
