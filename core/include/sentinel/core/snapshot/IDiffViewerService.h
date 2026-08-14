// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>

namespace sentinel::core {

enum class DiffViewMode : std::uint8_t {
    Unified,
    Split
};

struct DiffLine {
    QString content;
    int oldLineNum{0};
    int newLineNum{0};
    char type{' '};
};

struct DiffHunk {
    int oldStart{0};
    int newStart{0};
    QList<DiffLine> lines;
};

struct DiffFile {
    QString path;
    QString oldPath;
    bool isBinary{false};
    QList<DiffHunk> hunks;
    int additions{0};
    int deletions{0};
};

struct Diff {
    QList<DiffFile> files;
    int totalAdditions{0};
    int totalDeletions{0};
};

class IDiffViewerService {
public:
    virtual ~IDiffViewerService() = default;

    virtual Diff computeDiff(const QString& oldPath, const QString& newPath) const = 0;
    virtual Diff computeGitDiff(const QString& directory) const = 0;
    virtual QString formatDiff(const Diff& diff, DiffViewMode mode = DiffViewMode::Unified) const = 0;
    virtual void setViewMode(DiffViewMode mode) = 0;
    virtual DiffViewMode viewMode() const = 0;
};

} // namespace sentinel::core
