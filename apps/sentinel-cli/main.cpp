// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commands/ChatCommand.h"
#include "commands/ConfigCommand.h"
#include "commands/ModelCommand.h"
#include "commands/StatusCommand.h"

#include <QCoreApplication>

#include <iostream>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    const QStringList args = QCoreApplication::arguments();

    if (args.size() < 2) {
        std::cout << "Sentinel CLI Tool v1.0.0" << std::endl;
        std::cout << "Usage: sentinel-cli <command> [args...]" << std::endl;
        std::cout << "\nAvailable Commands:" << std::endl;
        std::cout << "  chat <prompt>          Send a prompt and receive response" << std::endl;
        std::cout << "  model [list|pull]      List or pull Ollama inference models" << std::endl;
        std::cout << "  status                 Display system and runtime status" << std::endl;
        std::cout << "  config [get|set]       View or update settings" << std::endl;
        return 0;
    }

    const QString command = args.at(1);
    const QStringList commandArgs = args.mid(2);

    if (command == QStringLiteral("chat")) {
        return sentinel::cli::executeChatCommand(commandArgs);
    } else if (command == QStringLiteral("model")) {
        return sentinel::cli::executeModelCommand(commandArgs);
    } else if (command == QStringLiteral("status")) {
        return sentinel::cli::executeStatusCommand(commandArgs);
    } else if (command == QStringLiteral("config")) {
        return sentinel::cli::executeConfigCommand(commandArgs);
    }

    std::cerr << "Unknown command: " << command.toStdString() << std::endl;
    std::cerr << "Run 'sentinel-cli' without arguments for usage details." << std::endl;
    return 1;
}
