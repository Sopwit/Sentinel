// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENTINEL_DESKTOP_LOGGINGINITIALIZER_H
#define SENTINEL_DESKTOP_LOGGINGINITIALIZER_H

#include <QString>

namespace sentinel::desktop {

void configureLogging(bool verbose, bool quiet, const QString& logDir);

} // namespace sentinel::desktop

#endif // SENTINEL_DESKTOP_LOGGINGINITIALIZER_H
