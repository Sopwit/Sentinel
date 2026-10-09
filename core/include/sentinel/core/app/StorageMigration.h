// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace sentinel::core {

struct StorageMigration {
    // Relocates the per-application QStandardPaths roots that a legacy
    // application name produced onto the roots of the current application name.
    // Existing content wins: only entries missing from the current root are
    // moved, so repeated calls are idempotent and safe to run from more than one
    // process. Absolute paths stored inside moved JSON files are repointed at the
    // new root. Returns how many distinct roots were relocated.
    static int migrateLegacyApplicationStorage(const QString& legacyApplicationName,
                                               const QString& applicationName);
};

} // namespace sentinel::core
