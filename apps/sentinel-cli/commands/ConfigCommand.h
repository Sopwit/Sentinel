// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENTINEL_CLI_CONFIGCOMMAND_H
#define SENTINEL_CLI_CONFIGCOMMAND_H

#include <QStringList>

namespace sentinel::cli {

int executeConfigCommand(const QStringList& args);

} // namespace sentinel::cli

#endif // SENTINEL_CLI_CONFIGCOMMAND_H
