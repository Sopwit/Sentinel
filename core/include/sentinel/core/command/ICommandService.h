// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <functional>

namespace sentinel::core {

struct Command {
    QString name;
    QString description;
    QString templateText;
    QString agentOverride;
    QString modelOverride;
    bool isSubtask{false};
};

using CommandFunction = std::function<void(const QString& args)>;

class ICommandService {
public:
    virtual ~ICommandService() = default;

    virtual void registerCommand(const Command& cmd) = 0;
    virtual void registerBuiltin(const QString& name, CommandFunction func) = 0;
    virtual bool execute(const QString& name, const QString& args = {}) = 0;
    virtual QList<Command> commands() const = 0;
    virtual std::optional<Command> findCommand(const QString& name) const = 0;
    virtual QString resolveTemplate(const QString& templateText, const QString& args) const = 0;
};

} // namespace sentinel::core
