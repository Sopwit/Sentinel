// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/shell/ShellAnalyzer.h"
#include <QDir>
#include <QRegularExpression>

namespace sentinel::core {

ShellCommandInfo ShellAnalyzer::analyze(const QString& command) const {
    ShellCommandInfo info;
    info.command = command;

    QStringList tokens = command.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (tokens.isEmpty())
        return info;

    info.subcommands = tokens.first().split('|');

    info.filePaths = extractFilePaths(command);
    info.isDestructive = isDestructiveCommand(command);
    info.requiresNetwork = networkCommands().contains(tokens.first().toLower());

    QString workingDir = QDir::currentPath();
    for (const auto& path : info.filePaths) {
        if (accessesExternalDirectory(path, workingDir)) {
            info.hasExternalAccess = true;
            info.externalPaths.append(path);
        }
    }

    return info;
}

bool ShellAnalyzer::isDestructiveCommand(const QString& command) const {
    QStringList tokens = command.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (tokens.isEmpty())
        return false;

    QString cmd = tokens.first().toLower();
    cmd = QFileInfo(cmd).fileName();

    return destructiveCommands().contains(cmd);
}

QStringList ShellAnalyzer::extractFilePaths(const QString& command) const {
    QStringList paths;
    QRegularExpression pathRx("[\\/\\.\\-\\w]+(?:\\.[a-z]+)?");
    QRegularExpressionMatchIterator it = pathRx.globalMatch(command);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString captured = match.captured(0);
        if (captured.contains('/') || captured.contains('.') && !captured.startsWith('-')) {
            paths.append(captured);
        }
    }
    return paths;
}

bool ShellAnalyzer::accessesExternalDirectory(const QString& command,
                                              const QString& workingDir) const {
    QStringList paths = extractFilePaths(command);
    for (const auto& path : paths) {
        if (!isWithinDirectory(resolvePath(path, workingDir), workingDir)) {
            return true;
        }
    }
    return false;
}

bool ShellAnalyzer::requiresPermission(const ShellCommandInfo& info) const {
    return info.isDestructive || info.hasExternalAccess || info.requiresNetwork;
}

QString ShellAnalyzer::resolvePath(const QString& path, const QString& workingDir) const {
    if (QDir::isAbsolutePath(path))
        return path;
    return QDir(workingDir).filePath(path);
}

bool ShellAnalyzer::isWithinDirectory(const QString& path, const QString& directory) const {
    QString resolved = QDir(directory).absoluteFilePath(path);
    return resolved.startsWith(directory);
}

QStringList ShellAnalyzer::destructiveCommands() const {
    return {"rm",     "rmdir",   "rmrf",     "unlink", "shred", "dd",       "mkfs",
            "format", "del",     "erase",    "rd",     "chmod", "chown",    "chgrp",
            "kill",   "killall", "shutdown", "reboot", "init",  "systemctl"};
}

QStringList ShellAnalyzer::networkCommands() const {
    return {"curl", "wget",   "ssh",     "scp",     "rsync",  "git", "npm",
            "pip",  "docker", "kubectl", "ansible", "telnet", "nc",  "ncat"};
}

} // namespace sentinel::core
