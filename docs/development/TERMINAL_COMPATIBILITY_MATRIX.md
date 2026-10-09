# Terminal compatibility — V2

| Environment | Actual result |
|---|---|
| macOS PTY, TERM=xterm-256color | INTEGRATION PASS: actual binary, disconnected draft/newline/send/cancel/edit/exit, terminal attributes and alternate-screen restoration |
| Ratatui TestBackend | UNIT PASS: 80×24, 100×30, 120×40, 160×48, 40×12 and 10×4; picker/approval layouts and Unicode |
| Ghostty | Native UI observation BLOCKED by computer-use tool safety restriction; installed/running is not PASS |
| macOS Terminal | Native UI observation BLOCKED by computer-use tool safety restriction |
| iTerm2 | NOT VALIDATED |
| Linux terminals | NOT VALIDATED |
| Windows Terminal | NOT VALIDATED; Rust IPC transport remains unsupported on Windows |
| SSH / tmux / screen | NOT VALIDATED |

PTY evidence additionally exercises 20×6 fallback. PTY is not a Ghostty/Terminal screenshot, real daemon inference, terminal keyboard-sequence certification or cross-platform acceptance. Shift/Alt+Enter depends on event reporting; Ctrl+O is the reliable alternative. NO_COLOR and explicit light/dark header styles are implemented; all terminal color-depth combinations and full ASCII fallback remain unvalidated/unimplemented; SENTINEL_TUI_ASCII=1 provides ASCII borders only. Resize is processed as a dirty event; fixed-size layout tests alone do not certify a terminal emulator's native resize behavior.
