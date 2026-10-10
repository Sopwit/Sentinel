# SENTINEL — TUI V3.1 LIVE ACCEPTANCE

Completed 2026-10-10; captures began 2026-10-09. Verdict: **PARTIAL — FURTHER VALIDATION REQUIRED**. Real inference and security-sensitive Agent paths were exercised, but the full referenced-file journey and several failure permutations remain incomplete. No competitor parity or native terminal acceptance is claimed.

## 1. Current baseline

V3 supplied a 43-entry typed registry, slash/palette/help, sessions, authorized references and reused Agent UI, with 66 Rust tests but no successful live inference acceptance. Existing uncommitted changes were preserved. V3.1 adds `/theme` (44 entries), responsive gutters, stable in-flight history and integration regressions. Historical V3 reports remain intact. Architecture/runtime/security/build/testing instructions were consulted; Personal-Brain was unavailable in this environment.

## 2. Isolation and provider readiness

Host: macOS, Rust Ratatui/Crossterm client, copied existing debug daemon 1.0.0-rc.8, IPC 1.1. Each acceptance environment used a private `sentinel-v31-isolated-*` temporary root, copied daemon, portable storage, separate profile name and 0700 socket directory. Synthetic README, CMakeLists (C++20), configuration and source files were the only workspace data. Personal conversations, credentials, Desktop and existing daemon were not accessed or reconfigured. Only test daemons were stopped/restarted.

The user explicitly authorized the complete attached plan and any installed active provider. Read-only discovery found LM Studio models `nvidia/nemotron-3-nano-4b`, `google/gemma-4-e4b`, `qwen/qwen3.5-9b` and Ollama `qwen2.5:3b`. All successful inference tests used LM Studio `http://127.0.0.1:1234`, model `nvidia/nemotron-3-nano-4b`; no silent fallback, download or second model server. Model selection and run binding were daemon-owned. Catalog discovery alone was never accepted as inference readiness.

The inference server remained shared: synthetic requests could affect its latency/cache/logs. No load/unload/configuration was performed. Export used Qt's test-profile AppData path rather than portable storage: `/Users/emir/Library/Application Support/Sopwit/SentinelV31-sentinel-v31-isolated-0_dmd190/exports/`; this contains only the synthetic export. Evidence roots/session/run IDs are recorded in JSON. At final rerun, the LM Studio endpoint refused connection and `model.select` returned `model-unavailable`; the user's service was not restarted or altered.

## 3. Journey A — IMPLEMENTED + VERIFIED (bounded core)

Actual TUI connected, opened Help and model picker, selected the available model, created a session and sent two exact-output Chat prompts. Real responses `FIRST-ANSWER-OK` and `SECOND-ANSWER-OK` arrived and persisted. The pending second-turn capture retains the first response and latest user message; the final capture has both exactly once. Separate contract evidence contains accepted `run.completed` and persisted `SENTINEL-LIVE-OK`. Cancellation is verified separately below. Evidence: `ui-*.ansi`, `ui-environment.json`, `live-contract.json`.

## 4. Journey B — IMPLEMENTED + PARTIALLY VERIFIED

Actual `@` picker searched the synthetic workspace, selected a path and submitted an Agent question. An authorized `read-file` result contained actual CMake content. However that attempt ended in `run.failed: Agent could not determine a grounded next action.` Tool success is not Agent completion. A common-root fuzzy-ranking defect initially selected the same file twice; relative filename matches now outrank full-root matches. The final real picker shows two selected references, and deterministic nested README ranking passes. The final inference rerun could not bind a model because the provider had become unavailable.

Renamed/deleted synthetic files and nested long paths were verified by fresh workspace listings (`session-metadata.json`). Missing/denied/disconnected dispatch and empty-result rendering have deterministic coverage, but not every actual authorization-denied/reference-size permutation was live exercised. Selected references remain quoted paths and require Agent tools; Chat does not receive injected file contents. **The full @file grounded-answer journey is not PASS.** Evidence: `context-ui-attempt1.json`, latest `context-ui.json`, `context-*.ansi`, metadata JSON.

## 5. Journey C — IMPLEMENTED + VERIFIED (bounded cases)

A real read-only Agent autonomously selected `read-file` for CMakeLists, received the C++20 evidence, continued model execution and reached authoritative `run.completed`; persisted final content includes `set(CMAKE_CXX_STANDARD 20)`. An initial 90-second observation expired; a later longer bounded observation completed. This is not proof of a runtime timeout bug.

Three separate real Agent sessions requested new synthetic write-file targets. Before allowing, the harness matched the exact tool/resource/new target. Actual attached TUI keys `y`, `n`, Ctrl+C exercised:

| Decision | Authoritative outcome | Filesystem evidence |
|---|---|---|
| Allow once | `run.completed`, write and readback | `allow.txt` contains exactly `APPROVAL-OK` |
| Deny | `run.failed`, user denied required action | Denied target absent |
| Cancel pending approval | `run.cancelled` | Cancelled target absent |

No blanket grants or persistent permissions. Final Agent presentation can be evidence-like rather than a polished explanation. Approval scope is explicit but resource rendering is still technical JSON. Evidence: `agent-read.json`, `approvals.json`, `approval-*.ansi`.

## 6. Journey D — IMPLEMENTED + PARTIALLY VERIFIED

New session, TUI session picker, real history after daemon restart, direct resume, rename, archive and populated JSON export succeeded. Export boolean result and on-disk contents were checked. In-memory drafts survived expected failure/reconnection; they are not crash-persistent drafts. Session switching has deterministic busy/conflict guards; a full simultaneous active/interrupted/completed multi-session matrix was not exercised live. Evidence: `ui-environment.json`, `continuity-recovery.json`, `session-metadata.json`, `export-verification.json`.

## 7. Journey E — IMPLEMENTED + PARTIALLY VERIFIED

Only the disposable daemon was stopped and restarted; the TUI reconnected and restored history/draft. Only the disposable profile endpoint was changed to closed `127.0.0.1:1`: accepted Chat subsequently emitted `run.failed(ConnectionFailed)`. Restoring endpoint 1234 enabled discovery. Invalid model selection was rejected. Streaming cancellation produced actual `run.cancelled`; an early harness waited after consuming the terminal event while awaiting its cancel acknowledgement, so it reported a harness timeout despite recording cancellation. That observer was corrected, not misreported as a product defect.

A forced real IPC timeout and abrupt interruption mid-stream/restart were not fully exercised; deterministic stale-generation, terminal-event and request-failure tests cover relevant boundaries. No mutating Agent operation is automatically retried. Final provider unavailability was reported truthfully. Evidence: continuity/UI/context JSON and recordings.

## 8. Command registry validation

[Complete 44-command table](TUI_V3_1_COMMAND_VALIDATION.md) distinguishes live core operations, live reused backend contracts, deterministic-only UI actions and unavailable commands. All aliases have deterministic resolution tests; every alias was not individually exercised live. Plan/compact/rollback/skills/init/guided questions/editor remain disabled with reasons. No decorative working claims.

## 9. Integration defects fixed

[Fix details](TUI_V3_1_INTEGRATION_FIXES.md): preserved transcript across pending/stream/final; responsive 180-cell maximum shared grid; queue rejection rollback; empty-draft paste-provenance reset; accepted-invocation/boolean-result distinction; readable inference failure categories; filename-first context ranking. Themes and keyboard changes are presentation-only.

## 10. Security and Agent authority

C++ daemon, AgentRuntime, ModelBinding, ToolExecutionGateway and filesystem/process boundaries remain authoritative. No C++/IPC/schema/Qt/QML changes. No private reasoning, unrestricted shell, unsafe undo, persistent auto-approval, Git mutation or personal-data tests. Run completion follows daemon terminal events, not tool-result arrival. Failed grounding remains failure. References are untrusted paths, not eager local disk reads.

## 11. PTY/native terminal results

Actual release-binary ANSI PTY output recorded before/after at 80×24, 100×30, 120×40 and 160×48. Deterministic states cover empty connected/disconnected, populated, long identities, Agent running/approval, focused/unfocused composer, command popup/help, Unicode and fallback. Real live interactions cover paste, protocol-injected Shift+Enter, slash discovery, model/session/reference selection, streaming, approval, cancellation and reconnection. All four size captures exited 0 and recorded enhancement push/pop, bracketed-paste disable and alternate-screen restoration.

Themes: terminal, obsidian, glacier, porcelain. `NO_COLOR` deliberately suppresses colors; colored captures explicitly remove it. Viewed replay PNGs confirm light-theme contrast, wide shared alignment and approval scope. Compared with supplied screenshots and recorded baseline, large side gutters are removed without new panels; prior messages stay visible while waiting. The calm welcome composition is retained. PNGs are bounded VT replays of actual output, **not native screenshots**. User screenshots are baseline references. Ghostty, macOS Terminal, iTerm2, Linux terminals and tmux/SSH: **NOT VALIDATED**. Physical Shift+Enter cannot be guaranteed on hosts sending plain Enter; Alt+Enter/Ctrl+O remain fallback.

## 12. Performance

80×24 idle sample over 5.019 seconds: process CPU time remained `0:00.04` and RSS 8240 KiB at the measurement's coarse resolution. This is not precise zero-CPU proof. Debug TestBackend rendered 100 incremental 160×48 frames in 5.132 seconds, retaining 180,000 bytes (approximately 51 ms/frame in this synthetic long-output test). It is not native release rendering latency and suggests long-output rendering merits profiling. Existing bounded output/event queues and stale-session/generation guards were retained; no performance parity claim. Model/server memory was not measured.

## 13. Tests

- `cargo fmt --all -- --check`: PASS.
- `cargo clippy --workspace --all-targets -- -D warnings`: PASS.
- `cargo test --workspace`: PASS, **72 tests** (CLI 14, IPC 11, TUI 47).
- `cargo build --release --workspace`: PASS.
- ASCII + NO_COLOR TUI suite: PASS, 47 tests.
- Existing C++ `test_daemon_ipc` and `test_desktop_ipc`: PASS, 2/2. No C++ source modified; broader CMake rebuild was not required for this Rust-only change.

## 14. Evidence locations

[Evidence index](../reviews/terminal-v31-2026-10-09/README.md) links raw ANSI, event JSON, replay images and test logs. Fixtures are explicitly separated from real inference evidence. Temporary test profiles are synthetic and isolated. No secrets were intentionally captured.

## 15. Remaining blockers

Full successful @file→grounded final flow after picker fix; native physical keyboard acceptance; forced timeout/interrupted-stream matrix; every command's actual UI error permutations; smoother long-transcript rendering; human-readable approval resources; richer authoritative attachment/tool-detail contracts. Native key selection/copy and crash-persistent drafts remain deferred. Themes currently persist only by launch environment, not an interactive saved preference.

## 16. Final verdict

**PARTIAL — FURTHER VALIDATION REQUIRED.** The reported visual/history defects are fixed and regression-tested; real Chat and Agent/approval/persistence evidence substantially improves the V3 baseline. Essential full reference acceptance and native-host validation are not complete. Recommended V3.2: first repeat the grounded reference journey with an available provider, then native Shift+Enter/resize/focus acceptance and failure recovery, followed by focused rendering profiling. Do not begin new backend semantics or V3.2 automatically.
