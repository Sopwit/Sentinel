# TUI V3 implementation report

Date: 2026-10-09. Scope: existing Rust/Ratatui client. No commits, pushes, merges, Desktop/Quick Panel changes, new daemon API or security policy changes. Existing uncommitted V2.2/V2.3 visual work is preserved.

## Original limitations and selected patterns

The baseline already had a centered reading/composer grid, welcome screen, roles, wrapping, safe timeline, approvals and fuzzy pickers. Command metadata/dispatch were separate and undiscoverable inline; help did not explain availability; unknown commands lacked suggestions; metadata inspections could overwrite streamed text; per-session unsent drafts lacked a bounded continuity workflow; pasted slash text could be misinterpreted as a command. File references were gateway path requests, not attachments. No enforceable Plan, snapshots, compaction or typed interview existed.

[Competitor matrix](TUI_V3_COMPETITOR_WORKFLOW_MATRIX.md) records official documentation research and adopted patterns: discovery, centralized commands, searchable help and session continuity. No visual clone, permission bypass, shell shortcut or parity claim was introduced.

## Delivered feature status

| Area | Status | Exact result |
|---|---|---|
| Command registry | IMPLEMENTED + VERIFIED | 43 typed entries, metadata/arguments/availability/security/bindings, exhaustive shared dispatch; alias/name/binding uniqueness tests |
| Slash experience | IMPLEMENTED + VERIFIED | Fuzzy popup, descriptions/usage/examples, disabled reasons, Up/Down, completion-only Tab/partial Enter, exact Enter validation, Esc, suggestions and literal slash/paste tests |
| Core commands | IMPLEMENTED + PARTIALLY VERIFIED | Help/new/sessions/resume/model/provider/workspace/mode/status/doctor/exit; complete table and live boundaries in [command spec](TUI_V3_COMMAND_SPEC.md) |
| Session continuity | IMPLEMENTED + PARTIALLY VERIFIED | Searchable picker, selected/last resume, rename/archive via existing finite actions, up to 32 unsent session drafts, active/busy guards; real disposable persistence tested |
| Model/provider | EXISTING + REUSED | Refresh preserves query/selection and provider filtering; accepted binding authority retained; empty live catalog and rejected send observed, successful inference not observed |
| References | EXISTING + REUSED | Authorized `@` picker, fuzzy returned paths, quoted untrusted names, stale reply guard, removal picker/index; no local file read or automatic contents |
| Agent details | IMPLEMENTED + PARTIALLY VERIFIED | Expand/collapse bounded safe metadata; independent inspection view preserves stream; existing approval/cancel authority tested deterministically, no live tool execution |
| History/export | IMPLEMENTED + PARTIALLY VERIFIED | Current-session transcript search/refresh; finite controlled export dispatch; no populated live export performed |
| Keyboard/composer | IMPLEMENTED + VERIFIED | Shared registry shortcut handlers, paste literal provenance through failure/recall, preserved Ctrl+O/P/K, Unicode, multiline, bounded history and focus precedence |
| Preferences | EXISTING + REUSED | Existing theme/fallback environment; `/settings` read-only inspection; `/details` memory-only preference, no persistent remapping |
| Advanced workflows | SPECIFIED ONLY / DEFERRED | Plan/compact/undo/redo/skills/init/grill/editor unavailable with tested explanation; review reuses existing Applied diff |
| Security/integration | EXISTING + REUSED | Generated IPC schema validated, daemon/tool/model authority unchanged; no automatic permission grant, shell, rollback or Git operations |

## Verification

Rust workspace tests pass: **14 CLI, 11 IPC, 41 TUI = 66 tests**, plus empty doctest suites. Tests cover parser/aliases/arguments, fuzzy/unknown commands, registry canonical dispatch, paste/literal handling, readonly Plan refusal, session drafts/active guards, model picker dispatch, safe metadata disclosure, approval/cancellation, stale events, queue/disconnect errors, narrow rendering and the existing visual/keyboard contracts. No fake model/fixture result is represented as live acceptance.

Checks passed: `cargo fmt --manifest-path cli/Cargo.toml --all -- --check`; `cargo clippy --manifest-path cli/Cargo.toml --workspace --all-targets -- -D warnings`; `cargo test --manifest-path cli/Cargo.toml --workspace`; `cargo build --manifest-path cli/Cargo.toml --workspace --release`. The initial sandbox run could not bind local test sockets; the approved rerun used disposable local sockets and passed. Backend code was not changed, so broad CMake tests were not required for this scoped Rust change.

Rendering covers 80×24, 100×30, 120×40, 160×48 and an additional 24×8 stress size. Existing empty connected/disconnected, populated, long names, running, approval, focused/unfocused and search tests remain. New slash/help/disabled-command fixtures exercise those dimensions. Render measurement remains bounded; this is not a native emulator performance benchmark.

## Real workflow evidence and acceptance boundaries

A copied daemon executable ran with `--portable`, a unique test profile and an owner-only temporary socket/data directory. It created/attached/renamed/listed a session, restarted, resumed its persisted identity and archived it with accepted replies. File discovery returned `permission-denied`; model listing returned empty; chat returned `model-unavailable`. No model download, real provider credentials, private history or workspace mutation occurred. See [isolated daemon evidence](../reviews/terminal-v3-2026-10-09/isolated-daemon.json).

| Journey | Result | Evidence / limitation |
|---|---|---|
| A First use | IMPLEMENTED + PARTIALLY VERIFIED | Real PTY connected/help/model discovery/failed send; no model available, so successful answer BLOCKED |
| B Project context | BLOCKED | Real daemon rejects unauthorized files; deterministic picker/ref dispatch works; grounded response not exercised |
| C Coding Agent | BLOCKED | Approval/cancel/tool event UI tested with explicit fixtures only; no live model, permissions or tool execution |
| D Continuity | IMPLEMENTED + PARTIALLY VERIFIED | Real create/rename/restart/resume/archive; no user/assistant exchange persisted because inference unavailable |
| E Recovery | IMPLEMENTED + PARTIALLY VERIFIED | Real PTY daemon stop/restart/reconnect exercised with failed draft preserved; resumed inference not exercised |

**No complete five-journey PASS or competitor-level parity is claimed.** Native Ghostty, macOS Terminal, iTerm2, Linux terminal and tmux/SSH acceptance: **NOT VALIDATED**. Actual before/after PTY output is available at all four requested sizes. The approved screenshot attachment was unavailable in the execution context; comparison uses the specified minimalist composition and preserves the V2.3 grid. See [terminal evidence](../reviews/terminal-v3-2026-10-09/README.md).

## Remaining problems and next batch

The composer retains horizontal long-line editing rather than new soft wrapping; file context requires explicit Agent observation; detailed tool payloads, selection editing, persistent preferences and crash-safe drafts remain absent. Full live inference/Agent acceptance needs a disposable configured provider/workspace. [Remaining gaps](TUI_V3_REMAINING_GAPS.md) specifies each backend/security requirement. Recommended next batch is isolated live A–E acceptance, then typed attachment/tool-detail contracts, before any new workflow mode.
