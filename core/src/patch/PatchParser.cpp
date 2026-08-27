// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/patch/PatchParser.h"
#include <QDir>
#include <QFile>
#include <QTextStream>

namespace sentinel::core {

Patch PatchParser::parse(const QString& patchText) {
    Patch patch;
    QStringList lines = patchText.split('\n');

    PatchFile* currentFile = nullptr;
    PatchHunk* currentHunk = nullptr;

    for (const auto& line : lines) {
        if (line.startsWith("--- ")) {
            // File add
            PatchFile file;
            file.action = "add";
            currentFile = &file;
        } else if (line.startsWith("+++ ")) {
            if (currentFile) {
                currentFile->path = line.mid(4).trimmed();
            }
        } else if (line.startsWith("@@ ")) {
            if (currentFile) {
                PatchHunk hunk;
                // Parse @@ -oldStart,oldCount +newStart,newCount @@
                currentFile->hunks.append(hunk);
                currentHunk = &currentFile->hunks.last();
            }
        } else if (line.startsWith("-")) {
            if (currentHunk) {
                currentHunk->lines.append(line);
            }
        } else if (line.startsWith("+")) {
            if (currentHunk) {
                currentHunk->lines.append(line);
            }
        } else if (line.startsWith(" ")) {
            if (currentHunk) {
                currentHunk->lines.append(line);
            }
        }

        if (line.startsWith("--- ") && currentFile && !currentFile->path.isEmpty()) {
            patch.files.append(*currentFile);
            currentFile = nullptr;
            currentHunk = nullptr;
        }
    }

    if (currentFile && !currentFile->path.isEmpty()) {
        patch.files.append(*currentFile);
    }

    return patch;
}

QString PatchParser::apply(const Patch& patch, const QString& basePath) {
    for (const auto& file : patch.files) {
        QString fullPath = QDir(basePath).filePath(file.path);

        if (file.action == "add") {
            QFile f(fullPath);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                for (const auto& hunk : file.hunks) {
                    for (const auto& line : hunk.lines) {
                        if (line.startsWith('+')) {
                            f.write(line.mid(1).toUtf8());
                            f.write("\n");
                        }
                    }
                }
            }
        } else if (file.action == "delete") {
            QFile::remove(fullPath);
        }
    }
    return "Patch applied successfully";
}

bool PatchParser::validate(const Patch& patch, const QString& basePath) {
    for (const auto& file : patch.files) {
        QString fullPath = QDir(basePath).filePath(file.path);
        if (file.action == "delete" && !QFile::exists(fullPath)) {
            return false;
        }
    }
    return true;
}

} // namespace sentinel::core
