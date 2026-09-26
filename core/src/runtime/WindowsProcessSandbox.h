// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#if defined(Q_OS_WIN)
#include <windows.h>

namespace sentinel::core {

struct WindowsSandboxState {
    HANDLE token = nullptr;
    HANDLE job = nullptr;
    ~WindowsSandboxState() {
        if (job) CloseHandle(job);
        if (token) CloseHandle(token);
    }
    WindowsSandboxState(const WindowsSandboxState&) = delete;
    WindowsSandboxState& operator=(const WindowsSandboxState&) = delete;
    WindowsSandboxState() = default;
};

} // namespace sentinel::core
#endif
