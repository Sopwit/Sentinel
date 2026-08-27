// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/InstructionService.h"
#include <QDir>
#include <QFile>
#include <QTextStream>

namespace sentinel::core {

InstructionService::InstructionService(QObject* parent) : QObject(parent) {}
InstructionService::~InstructionService() = default;

QList<Instruction> InstructionService::discoverInstructions(const QString& startDir) const {
    QList<Instruction> instructions;
    QStringList filenames = {"AGENTS.md", "CLAUDE.md", "CONTEXT.md"};

    QDir dir(startDir);
    while (dir.exists()) {
        for (const auto& filename : filenames) {
            QString path = dir.filePath(filename);
            if (QFile::exists(path)) {
                Instruction instr;
                instr.path = path;
                QFile file(path);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    instr.content = QTextStream(&file).readAll();
                }
                instructions.append(instr);
            }
        }
        if (!dir.cdUp())
            break;
    }

    for (const auto& globalPath : m_globalInstructions) {
        if (QFile::exists(globalPath)) {
            Instruction instr;
            instr.path = globalPath;
            QFile file(globalPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                instr.content = QTextStream(&file).readAll();
            }
            instructions.append(instr);
        }
    }

    for (const auto& url : m_remoteInstructions) {
        Instruction instr;
        instr.path = url;
        instr.isRemote = true;
        instructions.append(instr);
    }

    return instructions;
}

QString InstructionService::loadInstructions(const QString& startDir) const {
    auto instructions = discoverInstructions(startDir);
    QString combined;
    for (const auto& instr : instructions) {
        if (!instr.content.isEmpty()) {
            combined += instr.content + "\n\n";
        }
    }
    return combined.trimmed();
}

void InstructionService::addGlobalInstruction(const QString& path) {
    if (!m_globalInstructions.contains(path)) {
        m_globalInstructions.append(path);
    }
}

void InstructionService::addRemoteInstruction(const QString& url) {
    if (!m_remoteInstructions.contains(url)) {
        m_remoteInstructions.append(url);
    }
}

QStringList InstructionService::globalInstructions() const {
    return m_globalInstructions;
}
QStringList InstructionService::remoteInstructions() const {
    return m_remoteInstructions;
}

} // namespace sentinel::core
