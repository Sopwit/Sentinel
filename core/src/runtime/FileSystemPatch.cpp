// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/runtime/FileSystemPatch.h"
#include "sentinel/core/security/PathGuard.h"
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <utility>
namespace sentinel::core {
namespace {
struct PatchHunk {
    int oldStart{0};
    int oldCount{0};
    QStringList oldLines;
    QStringList newLines;
};

struct PatchFile {
    QString action; // "update", "add", "delete"
    QString path;
    QString oldPath;
    QString newPath;
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
bool parseUnifiedDiff(const QString& patch, QList<PatchFile>& files, QString& error,
                      const FileSystemOperationContext& context, bool& cancelled) {
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
        if ((i & 127) == 0 && context.isCancelled()) { cancelled = true; return false; }
        const QString& line = lines.at(i);
        if (line.startsWith(QStringLiteral("--- "))) {
            finishFile();
            const QString oldPath = stripDiffPrefix(line.mid(4).trimmed());
            current.oldPath = oldPath;
            current.action = oldPath == QStringLiteral("/dev/null") ? QStringLiteral("add")
                                                                    : QStringLiteral("update");
            current.path = oldPath;
            continue;
        }
        if (line.startsWith(QStringLiteral("+++ "))) {
            const QString newPath = stripDiffPrefix(line.mid(4).trimmed());
            current.newPath = newPath;
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
    if (files.size() > 128) {
        error = QStringLiteral("Patch exceeds the 128-file authorization limit.");
        return false;
    }
    for (const auto& file : files) {
        const auto parts = file.path.split(QLatin1Char('/'));
        if (file.oldPath.isEmpty() || file.newPath.isEmpty() ||
            (file.oldPath == QLatin1String("/dev/null") &&
             file.newPath == QLatin1String("/dev/null")) ||
            (file.action == QLatin1String("update") && file.oldPath != file.newPath) ||
            parts.contains(QStringLiteral("..")) || file.path.contains(QLatin1Char('\0'))) {
            error = QStringLiteral("Patch contains an unsupported or unsafe target path.");
            return false;
        }
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
                       int& appliedAt, const FileSystemOperationContext& context, bool& cancelled) {
    const int claimed = claimedStart - 1;
    const int maxOffset = fileLines.size();
    for (int offset = 0; offset <= maxOffset; ++offset) {
        if ((offset & 255) == 0 && context.isCancelled()) { cancelled = true; return false; }
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

 } // namespace
bool inspectPatchTargets(const QString& patch, QList<PatchTarget>& targets, QString& error,
                         const FileSystemOperationContext& context) {
    QList<PatchFile> files;
    bool cancelled = false;
    if (!parseUnifiedDiff(patch, files, error, context, cancelled)) {
        if (cancelled) error = QStringLiteral("Patch inspection cancelled.");
        return false;
    }
    for (const auto& file : files)
        targets.append({file.path, file.action});
    return true;
}

ToolExecutionResult applyPatchWithFileSystem(const QString& patch, const QString& cwd,
                                              const IFileSystemService& service,
                                              const FileSystemOperationContext& context,
                                              const ResourceAuthorizationSnapshot* authorization) {
    QList<PatchFile> files;
    QString parseError;
    bool cancelled = false;
    if (!parseUnifiedDiff(patch, files, parseError, context, cancelled))
        return cancelled ? ToolExecutionResult{ToolExecutionStatus::Cancelled,
                                               QStringLiteral("Patch cancelled before applying files.")}
                         : ToolExecutionResult{ToolExecutionStatus::InvalidArguments,
                                               QStringLiteral("apply-patch: %1").arg(parseError)};
    if (!authorization || !authorization->authorized ||
        authorization->workingDirectory != PathGuard::canonicalPath(cwd))
        return {ToolExecutionStatus::Blocked,
                QStringLiteral("apply-patch: filesystem authorization is unavailable.")};
    for (const auto& file : files) {
        const bool needsRead = file.action == QLatin1String("update");
        const AccessMode writeMode = file.action == QLatin1String("delete")
                                         ? AccessMode::Delete : AccessMode::Write;
        bool readAuthorized = !needsRead;
        bool writeAuthorized = false;
        for (const auto& resource : authorization->files) {
            if (resource.patchAction != file.action || resource.argument != file.path)
                continue;
            if (resource.access == AccessMode::Read) readAuthorized = true;
            if (resource.access == writeMode) writeAuthorized = true;
        }
        if (!readAuthorized || !writeAuthorized)
            return {ToolExecutionStatus::Blocked,
                    QStringLiteral("apply-patch: a patch target is not authorized.")};
    }
    QJsonArray outcomes;
    QList<PatchPathOutcome> typedOutcomes;
    QStringList messages;
    QList<FileMutation> committed;
    bool anyFailed = false;
    for (const auto& file : files) {
        if (context.isCancelled()) { cancelled = true; break; }
        FileSystemResult<AuthorizedPath> authorized;
        for (const auto& resource : authorization->files) {
            if (resource.argument == file.path && resource.patchAction == file.action &&
                resource.access == (file.action == QLatin1String("delete")
                                        ? AccessMode::Delete : AccessMode::Write)) {
                authorized = service.revalidateAuthorized(resource.path, cwd);
                break;
            }
        }
        QString resource = authorized.resource;
        FileSystemFailure reason = authorized.failure;
        QString message;
        QList<FileMutation> pathMutations;
        if (authorized.ok()) {
            resource = authorized.value->canonicalPath;
            if (file.action == QLatin1String("add")) {
                const auto existing = service.stat(*authorized.value);
                if (existing.ok()) reason = FileSystemFailure::AlreadyExists;
                else if (existing.failure != FileSystemFailure::NotFound) reason = existing.failure;
                else {
                    QStringList lines;
                    for (const auto& hunk : file.hunks) {
                        if (context.isCancelled()) { cancelled = true; break; }
                        lines.append(hunk.newLines);
                    }
                    if (cancelled) break;
                    const auto written = service.writeFile(*authorized.value, lines.join(QLatin1Char('\n')).toUtf8(), true);
                    reason = written.failure;
                    pathMutations = written.mutations;
                }
            } else if (file.action == QLatin1String("delete")) {
                const auto deleted = service.deleteFile(*authorized.value);
                reason = deleted.failure;
                pathMutations = deleted.mutations;
            } else {
                const auto read = service.readFile(*authorized.value, 16 * 1024 * 1024);
                reason = read.failure;
                if (read.ok() && !read.value->complete) reason = FileSystemFailure::ReadFailed;
                if (reason == FileSystemFailure::None) {
                    QStringList lines = QString::fromUtf8(read.value->content).split(QLatin1Char('\n'));
                    if (lines.size() > 1 && lines.last().isEmpty()) lines.removeLast();
                    bool matches = true;
                    for (const auto& hunk : file.hunks) {
                        if (context.isCancelled()) { cancelled = true; break; }
                        int appliedAt = -1;
                        if (!applyHunkWithFuzz(lines, hunk.oldStart, hunk, appliedAt,
                                               context, cancelled)) {
                            matches = false;
                            break;
                        }
                    }
                    if (cancelled) break;
                    if (!matches) {
                        reason = FileSystemFailure::WriteFailed;
                        message = QStringLiteral("Patch hunk did not match file context");
                    } else {
                        if (context.isCancelled()) { cancelled = true; break; }
                        const auto written = service.writeFile(*authorized.value, lines.join(QLatin1Char('\n')).toUtf8());
                        reason = written.failure;
                        pathMutations = written.mutations;
                    }
                }
            }
        }
        if (reason != FileSystemFailure::None) anyFailed = true;
        else committed.append(pathMutations);
        typedOutcomes.append({resource, file.action, reason, reason == FileSystemFailure::None});
        outcomes.append(QJsonObject{{QStringLiteral("path"), resource},
            {QStringLiteral("operation"), file.action}, {QStringLiteral("success"), reason == FileSystemFailure::None},
            {QStringLiteral("failure"), static_cast<int>(reason)},
            {QStringLiteral("diagnostic"), message}});
        messages.append(QStringLiteral("%1: %2").arg(resource,
            reason == FileSystemFailure::None ? QStringLiteral("applied") :
                message.isEmpty() ? QStringLiteral("failed (%1)").arg(static_cast<int>(reason)) : message));
    }
    const QJsonObject data{{QStringLiteral("paths"), outcomes},
                           {QStringLiteral("partial"), (anyFailed || cancelled) && !committed.isEmpty()},
                           {QStringLiteral("complete"), !anyFailed && !cancelled},
                           {QStringLiteral("truncated"), false},
                           {QStringLiteral("cancelled"), cancelled}};
    auto observation = std::make_shared<StructuredObservation>(StructuredObservation{StructuredObservationKind::PatchResult, data});
    observation->patchPaths = std::move(typedOutcomes);
    return {cancelled ? ToolExecutionStatus::Cancelled
                      : anyFailed ? ToolExecutionStatus::Failed : ToolExecutionStatus::Succeeded,
            cancelled ? QStringLiteral("Patch stopped after %1 committed file(s).\n%2")
                            .arg(committed.size()).arg(messages.join(QLatin1Char('\n')))
                      : QStringLiteral("apply-patch: %1").arg(messages.join(QLatin1Char('\n'))),
            observation, committed};
}

} // namespace sentinel::core
