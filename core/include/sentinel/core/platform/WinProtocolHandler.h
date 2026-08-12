// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

namespace sentinel::core {

// Registers sentinel:// protocol handler in the Windows registry (HKCU, no admin).
void registerSentinelProtocol();

// Checks if any argument in args starts with "sentinel://" and returns the full URL.
// Returns empty string if none found.
QString extractSentinelUrl(const QStringList& args);

// Name used for QLocalServer / QLocalSocket IPC between instances.
constexpr auto sentinelIpcServerName = "SentinelDesktopIPC";

} // namespace sentinel::core
