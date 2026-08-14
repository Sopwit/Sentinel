// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

namespace sentinel::core {

struct ShellCommandInfo {
    QString command;
    QStringList subcommands;
    QStringList filePaths;
    QStringList flags;
    QStringList arguments;
    bool hasExternalAccess{false};
    QStringList externalPaths;
    bool isDestructive{false};
    bool requiresNetwork{false};
};

class ShellAnalyzer {
public:
    ShellCommandInfo analyze(const QString& command) const;
    bool isDestructiveCommand(const QString& command) const;
    QStringList extractFilePaths(const QString& command) const;
    bool accessesExternalDirectory(const QString& command, const QString& workingDir) const;
    bool requiresPermission(const ShellCommandInfo& info) const;

private:
    QString resolvePath(const QString& path, const QString& workingDir) const;
    bool isWithinDirectory(const QString& path, const QString& directory) const;
    QStringList destructiveCommands() const;
    QStringList networkCommands() const;
};

} // namespace sentinel::core
