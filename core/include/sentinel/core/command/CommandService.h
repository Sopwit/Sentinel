// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/command/ICommandService.h"
#include <QList>
#include <QMap>
#include <QObject>

namespace sentinel::core {

class CommandService : public QObject, public ICommandService {
    Q_OBJECT
public:
    explicit CommandService(QObject* parent = nullptr);
    ~CommandService() override;

    void registerCommand(const Command& cmd) override;
    void registerBuiltin(const QString& name, CommandFunction func) override;
    bool execute(const QString& name, const QString& args = {}) override;
    QList<Command> commands() const override;
    std::optional<Command> findCommand(const QString& name) const override;
    QString resolveTemplate(const QString& templateText, const QString& args) const override;

private:
    QList<Command> m_commands;
    QMap<QString, CommandFunction> m_builtins;
};

} // namespace sentinel::core
