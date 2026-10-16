// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Legacy sentinel-cli exit-code contract.
//
// This is the minimal, documented contract for the retained in-process CLI.
// It is intentionally separate from the daemon-backed Rust `sentinel` exit
// table in docs/user/CLI.md; only the shared "success is 0" rule overlaps.
// The full contract, compatibility notes and test seams are documented in
// docs/reference/CLI_REFERENCE.md.
//
//   0  Success: a command completed its work, or usage was printed because no
//      command was supplied.
//   1  Usage/argument error: unknown command or subcommand, unknown key, or a
//      missing/blank required argument. Unchanged from the original CLI.
//   2  Operation failure: provider failure, refused request, model discovery
//      failure, model pull failure, or unusable configuration for the request.
//   3  Deadline timeout: the command's bounded wait expired (chat response
//      deadline, model discovery deadline, or model pull inactivity deadline).
//
// Signals are not caught by the CLI: an interrupted process dies by signal and
// the shell reports the platform convention (e.g. 130 for SIGINT on Unix).
// Failures must never be reported as 0 from synchronous or asynchronous paths.

namespace sentinel::cli {

inline constexpr int kExitSuccess = 0;
inline constexpr int kExitUsage = 1;
inline constexpr int kExitFailure = 2;
inline constexpr int kExitTimeout = 3;

} // namespace sentinel::cli
