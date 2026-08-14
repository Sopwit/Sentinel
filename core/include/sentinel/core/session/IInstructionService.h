// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>

namespace sentinel::core {

struct Instruction {
    QString path;
    QString content;
    bool isRemote{false};
};

class IInstructionService {
public:
    virtual ~IInstructionService() = default;

    virtual QList<Instruction> discoverInstructions(const QString& startDir) const = 0;
    virtual QString loadInstructions(const QString& startDir) const = 0;
    virtual void addGlobalInstruction(const QString& path) = 0;
    virtual void addRemoteInstruction(const QString& url) = 0;
    virtual QStringList globalInstructions() const = 0;
    virtual QStringList remoteInstructions() const = 0;
};

} // namespace sentinel::core
