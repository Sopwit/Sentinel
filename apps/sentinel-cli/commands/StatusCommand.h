// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENTINEL_CLI_STATUSCOMMAND_H
#define SENTINEL_CLI_STATUSCOMMAND_H

#include <QStringList>

namespace sentinel::cli {

int executeStatusCommand(const QStringList& args);

} // namespace sentinel::cli

#endif // SENTINEL_CLI_STATUSCOMMAND_H
