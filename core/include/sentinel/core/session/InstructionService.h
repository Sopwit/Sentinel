// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/session/IInstructionService.h"
#include <QObject>

namespace sentinel::core {

class InstructionService : public QObject, public IInstructionService {
    Q_OBJECT
public:
    explicit InstructionService(QObject* parent = nullptr);
    ~InstructionService() override;

    QList<Instruction> discoverInstructions(const QString& startDir) const override;
    QString loadInstructions(const QString& startDir) const override;
    void addGlobalInstruction(const QString& path) override;
    void addRemoteInstruction(const QString& url) override;
    QStringList globalInstructions() const override;
    QStringList remoteInstructions() const override;

private:
    QStringList m_globalInstructions;
    QStringList m_remoteInstructions;
};

} // namespace sentinel::core
