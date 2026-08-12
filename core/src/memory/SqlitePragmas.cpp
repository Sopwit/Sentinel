// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/memory/SqlitePragmas.h"

#include <QSqlQuery>

namespace sentinel::core {

void applySqlitePerformancePragmas(QSqlDatabase& database) {
    if (!database.isOpen()) {
        return;
    }

    QSqlQuery query(database);
    query.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
    query.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));
    query.exec(QStringLiteral("PRAGMA busy_timeout = 5000"));
    query.exec(QStringLiteral("PRAGMA cache_size = -16384"));
    query.exec(QStringLiteral("PRAGMA temp_store = MEMORY"));
}

} // namespace sentinel::core
