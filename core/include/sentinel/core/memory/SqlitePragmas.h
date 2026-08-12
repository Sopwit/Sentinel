// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSqlDatabase>

namespace sentinel::core {

// Applies local single-writer SQLite performance pragmas to an open connection.
// Safe across Linux, Windows, and macOS; keeps durability crash-safe with WAL.
void applySqlitePerformancePragmas(QSqlDatabase& database);

} // namespace sentinel::core
