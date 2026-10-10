# Sentinel V3.2.2 multi-file coding Agent acceptance

## Scope and baseline

Clean baseline b5e6933, 2026-10-10, macOS 27.0.1 arm64. Personal-Brain CLI unavailable. Architecture/runtime/security/build/testing and the five requested V3.2/V3.2.1 reports were read. The daemon remains authoritative; Rust is the unchanged typed-IPC client. No V3.3, Desktop redesign, permission bypass, new planning/compaction subsystem, provider rewrite, rollback or Git operation was introduced.

## Proven defects and actual changes

A real gateway diagnostic isolates the previously unexplained CMake failure: `cmake --version` fails exit 127, while `/opt/homebrew/bin/cmake --version` fails exit 126 with Operation not permitted. The macOS sandbox cannot read/execute the Homebrew CMake path. Separately `/usr/bin/true; /usr/bin/true` fails exit 128 with fork denied. StaticSandboxPolicy requires process-tree control for run-command, implemented by deny-fork on macOS. Fixing PATH alone cannot enable CMake compiler children. No sandbox restriction was weakened. General safe macOS build execution remains BLOCKED under the current authoritative policy.

Two regression tests independently reproduced loss of a trailing test failure: a 12000-character tool observation ended with TEST_FAILURE, but native continuation kept only the first 2000 characters and planning history kept only its short prefix. Both tests failed before the fix.

- LlmAgentRuntime now retains beginning and end within the same 2000-character observation payload budget, with an explicit excerpt marker and narrower-observation guidance.
- ContextEngine retains beginning and end for bounded run-command observations, preserving exit/status prefix and final test/compiler diagnostics.
- Original observations/evidence remain unchanged. Context items remain untrusted; no excerpt is treated as complete evidence or authorization. No new retry or changed permission policy was added.

This fixes a demonstrated output-handling defect. It does not explain provider empty responses or remove macOS process confinement limits.

## Real acceptance setup

Real release Rust/Ratatui TUI at 120×40 sends natural-language tasks (Ctrl+G to Agent; Enter to send). Private 0700 portable daemon profile, distinct synthetic workspace per task, private Unix socket. Actual native model-selected tools execute through AgentRuntime/ToolExecutionGateway. TUI y/n approval is used only for exact calc.cpp/test.cpp targets and named command digests, each command at most once. No successful tool sequence is scripted.

Explicit binding: LM Studio / nvidia/nemotron-3-nano-4b, actual loaded context 8192 (published 1048576), native output reservation 1024 from existing V3.2.1. Other installed models were catalogued but not selected/loaded. No fallback, server setting change or model download. Full tool catalog is the existing registry, not a task-specific fake list. No actual provider token usage/finish reason is captured in these direct-endpoint runs; they are not inferred from bytes.

Fixtures include header, implementation, test, C++20/CMake definition, two marker files and an unrelated user-change marker. add initially subtracts; its test initially expects4 for add(2,3). Desired behavior is addition with expected 5. Full before/after content/hashes and real text diffs are retained. D's separate host baseline is a real synthetic build/test fixture, explicitly not Agent verification.

See the final [matrix](TUI_V3_2_2_AGENT_ACCEPTANCE_MATRIX.md), [execution evidence](TUI_V3_2_2_EXECUTION_EVIDENCE.md) and [raw artifacts](../reviews/agent-v322-2026-10-10/README.md). Full coding acceptance requires actual authorized edits, build, tests and an accepted truthful final; no partial result is promoted to PASS.

## Additional byte-bound closure

A third regression reproduced overflow of a 512-byte preview with Turkish UTF-8 characters: the old prefix cutter removed continuation bytes but retained a leading byte, producing a replacement character and exceeding the byte budget. ToolOutputTruncator now splits its available byte budget between beginning and end, includes an explicit marker and cuts at valid UTF-8 boundaries. Very small budgets return a bounded ASCII marker. The full original output remains persisted. This also prevents a very long individual line from evicting the final diagnostic at the upstream preview layer.

The live benchmark used the first two excerpt fixes, before this final byte-bound fix. The third fix is regression/build verified, not credited as a repeated live-model success.

## Verdict — IMPROVED — FURTHER FIXES REQUIRED

Real multi-file acceptance did not pass. A failed on an empty native provider response; B changed only the implementation before cancellation; C exhausted context before edits; D changed only the implementation then exhausted context; E timed out with failed discovery calls and was cancelled. None reached accepted final completion. All unrelated-file markers survived. Precise run IDs, timings, approvals and changes are in acceptance-summary.json and live.json.

Remaining highest-value work is authoritative safe process-tree support on macOS (or acceptance on an already-supported platform), measured native context accounting/continuation behavior and recovery from failed edit arguments. Do not enlarge provider limits or automatically retry writes to manufacture success. Empty provider content has no captured finish_reason/token telemetry here; its root cause remains undetermined.

Actual TUI replay inspection confirms visible approval scope, running/failed states and retained task text. It also shows raw resource JSON in approval, stale approval prose in persisted history, and a clipped long notice. These are recorded presentation limitations; no unrelated UI redesign was made.

No commit, push, merge or V3.3 work. Native terminal hosts remain NOT VALIDATED. The isolated benchmark daemon/TUI were shut down by the harness.

## Final verification

- Full CMake tests configure and debug build: PASS. Final offscreen CTest with local test socket/process permission: **122/122 PASS**, 128.71s, including AgentLoop, native continuation, gateway, process sandbox and daemon/Desktop IPC regressions.
- Focused LlmAgentRuntime QtTest suite: **34 PASS**, zero failures (includes setup/cleanup).
- Rust: **73 tests PASS** (48 TUI, 11 IPC, 14 CLI); cargo fmt check and Clippy with warnings denied PASS; debug and release CLI builds PASS. Rust source did not change in this phase.
- Final release sentinel-daemon build PASS. Changed C++ ranges clang-formatted; git diff --check PASS.
- Restricted-host CTest and the intermediate omitted-marker regression are preserved separately; final passing logs are not substituted for live Agent acceptance.

The 122 passing regression targets do not change the live verdict. Recommended next batch: close authoritative safe macOS process execution and measured native context/continuation gaps, then repeat these fresh-workspace A–F journeys. Do not start advanced commands, automatic retries or rollback as a workaround.
