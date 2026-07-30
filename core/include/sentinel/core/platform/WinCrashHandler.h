// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace sentinel::core {

void installWinCrashHandler(const QString& crashDumpDir);
bool hasPendingCrashDump(const QString& crashDumpDir);
QStringList pendingCrashDumps(const QString& crashDumpDir);
void acknowledgeCrashDump(const QString& dumpPath);

} // namespace sentinel::core
