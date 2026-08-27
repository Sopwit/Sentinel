// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/command/CommandService.h"

namespace sentinel::core {

CommandService::CommandService(QObject* parent) : QObject(parent) {}
CommandService::~CommandService() = default;

void CommandService::registerCommand(const Command& cmd) {
    for (int i = 0; i < m_commands.size(); ++i) {
        if (m_commands[i].name == cmd.name) {
            m_commands[i] = cmd;
            return;
        }
    }
    m_commands.append(cmd);
}

void CommandService::registerBuiltin(const QString& name, CommandFunction func) {
    m_builtins[name] = func;
}

bool CommandService::execute(const QString& name, const QString& args) {
    if (m_builtins.contains(name)) {
        m_builtins[name](args);
        return true;
    }

    auto cmd = findCommand(name);
    if (cmd.has_value()) {
        QString resolved = resolveTemplate(cmd->templateText, args);
        Q_UNUSED(resolved)
        return true;
    }
    return false;
}

QList<Command> CommandService::commands() const {
    return m_commands;
}

std::optional<Command> CommandService::findCommand(const QString& name) const {
    for (const auto& cmd : m_commands) {
        if (cmd.name == name)
            return cmd;
    }
    return std::nullopt;
}

QString CommandService::resolveTemplate(const QString& templateText, const QString& args) const {
    QString result = templateText;
    result.replace("$ARGUMENTS", args);
    result.replace("$1", args.section(' ', 0, 0));
    return result;
}

} // namespace sentinel::core
