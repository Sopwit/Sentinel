// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/snapshot/DiffViewerService.h"
#include <QFile>
#include <QTextStream>

namespace sentinel::core {

DiffViewerService::DiffViewerService(QObject* parent) : QObject(parent) {}
DiffViewerService::~DiffViewerService() = default;

Diff DiffViewerService::computeDiff(const QString& oldPath, const QString& newPath) const {
    Diff diff;
    DiffFile file;
    file.path = newPath;
    file.oldPath = oldPath;

    QFile oldFile(oldPath);
    QFile newFile(newPath);

    QStringList oldLines, newLines;
    if (oldFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        oldLines = QTextStream(&oldFile).readAll().split('\n');
    }
    if (newFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        newLines = QTextStream(&newFile).readAll().split('\n');
    }

    int maxLines = qMax(oldLines.size(), newLines.size());
    DiffHunk hunk;
    hunk.oldStart = 1;
    hunk.newStart = 1;

    for (int i = 0; i < maxLines; ++i) {
        DiffLine line;
        if (i >= oldLines.size()) {
            line.content = "+" + newLines[i];
            line.type = '+';
            line.newLineNum = i + 1;
            hunk.lines.append(line);
            file.additions++;
        } else if (i >= newLines.size()) {
            line.content = "-" + oldLines[i];
            line.type = '-';
            line.oldLineNum = i + 1;
            hunk.lines.append(line);
            file.deletions++;
        } else if (oldLines[i] != newLines[i]) {
            DiffLine oldLine;
            oldLine.content = "-" + oldLines[i];
            oldLine.type = '-';
            oldLine.oldLineNum = i + 1;
            hunk.lines.append(oldLine);
            file.deletions++;

            DiffLine newLine;
            newLine.content = "+" + newLines[i];
            newLine.type = '+';
            newLine.newLineNum = i + 1;
            hunk.lines.append(newLine);
            file.additions++;
        }
    }

    file.hunks.append(hunk);
    diff.files.append(file);
    diff.totalAdditions = file.additions;
    diff.totalDeletions = file.deletions;
    return diff;
}

Diff DiffViewerService::computeGitDiff(const QString& directory) const {
    Q_UNUSED(directory)
    return {};
}

QString DiffViewerService::formatDiff(const Diff& diff, DiffViewMode mode) const {
    Q_UNUSED(mode)
    QString result;
    for (const auto& file : diff.files) {
        result += QStringLiteral("--- %1\n+++ %2\n").arg(file.oldPath, file.path);
        for (const auto& hunk : file.hunks) {
            result += QStringLiteral("@@ -%1 +%2 @@\n").arg(hunk.oldStart).arg(hunk.newStart);
            for (const auto& line : hunk.lines) {
                result += line.content + "\n";
            }
        }
    }
    return result;
}

void DiffViewerService::setViewMode(DiffViewMode mode) { m_viewMode = mode; }
DiffViewMode DiffViewerService::viewMode() const { return m_viewMode; }

} // namespace sentinel::core
