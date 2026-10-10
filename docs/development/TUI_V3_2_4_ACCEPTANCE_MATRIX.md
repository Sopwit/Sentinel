# V3.2.4 acceptance matrix

Verdict: **IMPROVED — FURTHER FIXES REQUIRED**. This is not full phase acceptance.

## Real TUI, real models

Actual release Rust/Ratatui TUI over typed IPC; private portable daemon profiles; new disposable synthetic workspaces per journey; task-owned already-installed provider instances. Both use the same release daemon SHA256 `33653fb1d034ff10959af30781570b526dc73163b47b522e3758051f0e6b24b8`. Local API reports 8192 context and four slots for each instance. Initial/final loaded state is empty. No downloads, user-setting changes, fallback, writes, process approvals or scripted successful tool decisions occurred.

| Journey | Qwen3.5-9B MLX 4bit | Nemotron-3-Nano-4B GGUF Q4_K_M |
|---|---|---|
| A — single-file explanation | NOT ACCEPTED: run.failed at 30.75 s; classification Timeout; zero tools | NOT ACCEPTED: run.failed at 30.68 s; classification Timeout; zero tools |
| B — multi-file relationship + synthetic bugs | NOT ACCEPTED: run.failed at 31.71 s; classification Timeout; zero tools | NOT ACCEPTED: run.failed at 30.65 s; classification Timeout; zero tools |
| C — normal classification | Safe failure alternative: run.failed at 31.10 s; indeterminate Timeout; no tools | PASS for this normal journey: classification HTTP200/stop at 8.506 s; run.completed at 58.11 s with answer `4`; no tools |
| D — deterministic transport stall / cancellation | Core local HTTP and AgentLoop fixtures PASS; complete real-TUI stall/cancel fixture NOT VALIDATED | Same deterministic fixtures; no separate model-dependent success claim |
| E — evidence continuation | NOT ACCEPTED: run.failed at 31.35 s; classification Timeout; never reached reads | NOT ACCEPTED: run.failed at 30.77 s; classification Timeout; never reached reads |
| F — context pressure | Safe failure before context pressure: run.failed at 30.76 s; context-pressure completion NOT VALIDATED | Safe failure before context pressure: run.failed at 30.69 s; context-pressure completion NOT VALIDATED |

A successful classification transport is not enough: Nemotron's C additionally validates the classification category and actual correct answer/completion. Nine timeout journeys have no HTTP response, finish reason or actual token usage; none is invented. All ten before/after workspace snapshots match, including unrelated markers. No read-task completion or real repeated-read recovery was observed. These results do not rank models or engines.

The Rust client captured by the live matrix includes the cancellation-notice fix. The later failure-wrapper/notice-ellipsis presentation corrections have deterministic render/interaction coverage, not a new live-model acceptance run. ANSI PNGs are bounded VT replays, not physical-terminal screenshots. Qwen A and Nemotron C replays were inspected. Ghostty, Terminal, iTerm2, Linux terminals and tmux/SSH remain **NOT VALIDATED**.

## Deterministic and regression gates

| Gate | Result |
|---|---|
| Tests configure + complete debug/tests build | PASS; ccache disabled after its configured external cache was sandbox-denied |
| Release configure + complete C++ build | PASS |
| Focused classifier, inference, AgentLoop/runtime, grounding and upgrade | PASS, six executables |
| Full C++/IPC CTest, final rerun | **122/122 PASS**, finite 45-second per-test watchdog |
| Initial full CTest | 120/122; new canonical-path fixture and legacy bounded-provider fixture failed; fixed and retained in evidence |
| Rust final tests | **75 PASS**: CLI 14, IPC 11, TUI 50 |
| Rust format / Clippy with warnings denied | PASS |
| Rust debug/release builds | PASS |
| C++ changed-file formatting / whitespace | PASS for the checked files; scoped LlmAgentRuntime additions formatted |
| Keychain upgrade encryption | PASS with injected test key and real macOS AES; personal Keychain integration NOT VALIDATED |
| Safe macOS build-tool execution / cancellation during build | BLOCKED, existing deny-fork and containment design unchanged |
| Linux execution | NOT VALIDATED; no new runner used |

Initial sandboxed Rust socket tests failed with EPERM; the authorized local-socket rerun passes. An initial TUI notice-width expectation failed and was corrected; final Rust suite passes. These failures are verification history, not model failures.

## Phase acceptance criteria

1. Finite classification bounds: PASS for the implemented production paths; unsupported providers reject the capability.
2. Stalled classification cannot indefinitely wait: PASS in local HTTP fixtures and nine live timeouts.
3. Truthful cancellation: deterministic PASS; upstream cancellation unconfirmed; full real-TUI deterministic stall/cancel journey remains open.
4. Grounded relevant read-only explanations: deterministic projection PASS; live gate NOT ACCEPTED.
5. No uncontrolled successful-read loops: inventory and existing limits retained; live recovery NOT VALIDATED.
6. Context/tool evidence reliability: scoped validation PASS; exact tokenization/general context-pressure acceptance remains PARTIAL.
7. Permissions/daemon authority: preserved.
8. Relevant regressions: PASS after fixture corrections.
9. Real-model evidence: included, with failures visible.

See [summary](../reviews/agent-v324-2026-10-10/summary.json), [classification](TUI_V3_2_4_CLASSIFICATION_RELIABILITY.md), [grounded completion](TUI_V3_2_4_GROUNDED_COMPLETION.md) and [final report](TUI_V3_2_4_FINAL_REPORT.md).
