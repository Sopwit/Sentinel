// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

namespace sentinel::core {

struct PatchHunk {
    int oldStart{0};
    int oldCount{0};
    int newStart{0};
    int newCount{0};
    QStringList lines;
};

struct PatchFile {
    QString path;
    QString action;
    QList<PatchHunk> hunks;
};

struct Patch {
    QList<PatchFile> files;
};

class PatchParser {
public:
    static Patch parse(const QString& patchText);
    static QString apply(const Patch& patch, const QString& basePath);
    static bool validate(const Patch& patch, const QString& basePath);
};

} // namespace sentinel::core
