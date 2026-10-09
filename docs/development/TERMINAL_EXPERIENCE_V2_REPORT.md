# SENTINEL — TERMINAL EXPERIENCE V2 + QUICK PANEL UX REPORT

Date: 2026-10-09. Verdict: **IMPROVED — FURTHER FIXES REQUIRED**. Implementation is delivered; native terminal and real inference acceptance remain incomplete. No automatic next phase.

## 1. Starting repository state

Branch main, HEAD 102e602. Existing uncommitted Desktop UI Phase 1, provider/controller, navigation and test changes plus audit evidence were preserved. Baseline binary diff saved outside repository before editing. Personal-Brain CLI was unavailable; the user-supplied repository path was used. Architecture, IPC, daemon, Agent, security, CLI/TUI and prior audit/remediation documentation were reviewed. No commit, push, merge, model download or personal-data operation.

## 2. Daemon compatibility findings — Phase A

Read-only inspection: daemon PID 94165 started 17:12; Desktop PID 8246 started 19:14:21. Running daemon hello and current disk executable both report 1.0.0-rc.8; negotiated IPC is 1.1 with chat/agent/approval/session/events/terminal/workspace capabilities. Two simultaneous real Unix socket hello clients received the same server generation. This is actual handshake evidence, not a unit-test substitute.

Socket and runtime directory are owner emir, permissions 0700, socket srwx------. Endpoint locking/ownership, protocol mismatch, reconnect and multi-client behavior are also covered by existing IPC tests. Identical release versions do not prove identical source builds: the daemon predates the recent binary updates. Its exact running commit/build hash is unavailable. Desktop ready state and daemon restart/reconnect were not exercised against the user's application. No process was killed/replaced/restarted.

## 3. Isolation status

Daemon --profile-name changes storage identity before migration; private sockets have independent endpoint locks. However OS credential keys are provider-scoped with fixed Sentinel account, not profile-scoped. Therefore a fully independent production credential boundary is not established. Production inference/Agent testing against personal configuration was not attempted. Deterministic daemon IPC tests and disposable Qt fixtures were used; PTY tests use a nonexistent socket and never attach personal sessions. This is a bounded limitation, not a successful isolated-production E2E claim.

## 4. TUI design changes — Phase C

Borderless compact header/transcript replace repeated bordered regions. Header keeps identity, explicit Chat/Agent, session/state, workspace and actual provider/model readiness. Composer starts at three rows and grows with text, capped at one third of height. Disconnected/no-model startup offers configuration/reconnect commands. Tiny windows show actionable minimum-size guidance. Wide-character cursor and horizontal input scrolling use terminal cell widths. User/Assistant/Agent output and latest safe tool activity are distinct; all terminal outcomes remain daemon-authoritative.

## 5. Keyboard binding contract — Phase B

Enter sends; Ctrl+O inserts newline. Optional Alt/Shift+Enter requires distinct terminal events. Ctrl+P/Tab opens commands; Ctrl+R/F searches transcript; Ctrl+A/E/U/K edits input. /sessions replaces former Ctrl+O session binding, /reconnect replaces former Ctrl+R reconnect, /remove replaces former Ctrl+D reference removal. Ctrl+S is unbound, eliminating Send dependence on XON/XOFF. Full implemented table: [keyboard contract](TUI_KEYBINDING_CONTRACT.md).

## 6. Composer and navigation

Paste never sends. Idle Ctrl+C preserves text and dismisses overlays; only empty idle context exits. Ctrl+D exits only idle/empty/not busy/no references, otherwise deletes a character. Starting-run cancellation is deferred until run ID acknowledgement. Failed/unqueued submission restores pending text and references. Cleanup guard disables bracketed paste and restores Ratatui state on recoverable exit; panic restoration uses Ratatui's hook. PTY tests compare complete terminal attributes and confirm alternate-screen exit at five dimensions. SIGKILL restoration cannot be guaranteed.

## 7. Chat/Agent improvements

Active display includes the submitted user text, Assistant/Agent/state and latest three safe activity records; /activity exposes the bounded existing metadata timeline. Approval remains modal and one-use, with y/n and authoritative acknowledgement. Cancellation never becomes completion. Foreign-generation and late closed-run output deltas are rejected. PgUp begins from the actual followed viewport bottom; new-content-below guidance and Ctrl+B preserve reading position. No hidden reasoning, raw sensitive tool output or duplicate orchestration was introduced.

## 8. Picker and palette improvements — Phase D

Tab/Shift+Tab navigate searchable pickers; filter now searches label, identity and details, enabling session-ID lookup. Selection marker and full identity details remain visible; empty/disconnected choices explain recovery, disabled choices stay disabled. Disconnected mutations are rejected without optimistic selection. Existing commands expose tools, MCP, permissions, context, memory and task diagnostics; unavailable backend operations are not fabricated. Very long selected details still clip to terminal width; a dedicated horizontally scrollable detail inspector remains follow-up.

## 9. Terminal compatibility — Phase E

Actual macOS PTY binary tests: 80×24, 100×30, 120×40, 160×48, 20×6. Ratatui TestBackend additionally covers 40×12 and 10×4, picker/approval overlays and UTF-8/wide input. Default terminal foreground/background are preserved; explicit light/dark header style and NO_COLOR supported. SENTINEL_TUI_ASCII provides ASCII panel borders, not full content transliteration. Native Ghostty and macOS Terminal access was denied by the computer-use tool; no alternate GUI-control mechanism was used. iTerm2, Linux, Windows, SSH, tmux/screen and emulator-specific modifier sequences are NOT VALIDATED. Windows Rust transport remains unsupported. See [matrix](TERMINAL_COMPATIBILITY_MATRIX.md).

## 10. Quick Panel — Phase F

Existing native layout retained. Focusable connection details and visible connection text complement status; original errors remain in diagnostics with truthful summaries for recognized IPC timeout/provider/model failures. Prompt/preview heights adapt within bounds. Submission guards protect draft while busy/disconnected. Approval buttons use shared themed controls. Escape closes details before the panel and restores prompt focus; native event-filter handling was corrected to respect that order. No backend timeout thresholds or discovery behavior changed without reproduction. Actual user timeout could not be reproduced by hello-only checks, so it is not declared fixed. See [Quick Panel report](QUICK_PANEL_UX_REMEDIATION.md).

## 11. Performance

Idle rendering is dirty-driven instead of continuously repainting at 20 Hz; updates are coalesced before draw. Request queue is capped at 32 with nonblocking submission; event channels 256, output 262144 bytes, editor 65536 bytes, recall 100 and activity 200. A debug TestBackend measurement of 100 incremental 160×48 renders retaining 180000 bytes took 4.2605 seconds. This is a bounded synthetic renderer measurement, not native streaming throughput or battery certification. Large-transcript rendering remains linear and warrants profiling/caching. Idle CPU, real tool-heavy Agent throughput, native resize latency, startup/battery and concurrent Desktop inference are NOT VALIDATED.

## 12. Tests and build — Phase G

- Rust fmt --check: PASS.
- Clippy --all-targets -- -D warnings: PASS.
- cargo test --locked workspace: PASS, 43 tests (14 CLI integration, 11 IPC, 18 TUI/editor/picker), zero failures.
- C++ tests build and debug desktop/daemon build: PASS.
- Full offscreen CTest: **122/122 PASS, 95.80 seconds**; subsequent native Quick Panel test with final fixture wording also PASS.
- Native Qt fixture: loading/empty/response/error, Chat/Agent Enter, Shift+Enter, focus, detail Escape, busy selection lock, compact 420/320 widths and human timeout/raw diagnostics: PASS.
- Generated IPC contract check: PASS; no schema changes.
- QML lint target: exits successfully with 130 warnings, predominantly existing context/unqualified access; not warning-free. Quick Panel's existing nativeDesktop context warning remains.
- PTY lifecycle: PASS at all five sizes, restored termios and alternate screen, successful idle exit. NOT a real model cancellation test.
- git diff --check: PASS.

Initial sandbox runs could not create local sockets/process/window services; successful test evidence was collected outside that restriction with temporary fixtures. No security setting was weakened.

## 13. Before/after evidence

[Dated evidence and hashes](../reviews/terminal-v2-2026-10-09/README.md): actual baseline 102e602 disconnected PTY recording and new dimension recordings; native light/dark Quick Panel error, idle and synthetic approval captures. ANSI recordings are not native terminal screenshots. No fake live Agent/Chat/approval/cancellation images replace missing runtime evidence. Fresh Quick Panel before capture and native TUI screenshots remain unavailable.

## 14. Real E2E evidence

REAL IPC HANDSHAKE: two simultaneous read-only clients, protocol/version/capabilities/generation verified. REAL PTY PROCESS: actual new binary with disconnected input and shutdown/terminal restoration. NATIVE QT OBSERVATION: actual rendered QML using explicitly synthetic fixture states. **NOT VALIDATED:** real local-model Chat, safe Agent task, live approval allow/deny/cancellation, daemon restart and concurrent Desktop/CLI inference. Deterministic existing IPC tests cover those contracts but do not substitute for production E2E.

## 15. Remaining limitations

Native emulator access restrictions and profile-independent OS credential isolation prevent full acceptance. No user daemon restart, credential access or personal-history operation was used to bypass them. Full ASCII, grapheme editing, long-detail inspection, full translation/accessibility, native tray/global shortcut/multi-monitor/microphone/OS notification behavior and real performance remain follow-ups. Existing Desktop Phase 1 work was left intact except necessary Quick Panel native event handling. No installer or remote/wearable work began.

## 16. Final verdict

**IMPROVED — FURTHER FIXES REQUIRED.** Substantial terminal input/layout/workflow improvements and focused Quick Panel remediation are implemented and tested. Acceptance criteria involving real inference, native terminal platforms and complete native integrations remain unmet. Stop here; do not automatically start another phase or commit.
