# SENTINEL — TUI V3.3-B interaction and settings completion

> SEC-P1.1 privacy edit (2026-10-12): machine-specific paths are shown as role placeholders; recorded measurements and acceptance results are unchanged. Command examples use `SENTINEL_REPO`; set `export SENTINEL_REPO="$(git rev-parse --show-toplevel)"` from a checkout first. This edited report has a new file digest; original raw evidence is unchanged.

2026-10-11. Bounded Rust client improvements implemented. Deterministic tests and release PTY journeys pass. This phase has no new physical terminal acceptance; the user's earlier CW-04.5 initial Ghostty display confirmation remains separate.

## Implemented changes

| Area | Change and reason |
| --- | --- |
| Slash commands | Single-line slash input uses one local dispatch path, including paste. Registered commands cannot become inference; unknown names produce local feedback and preserve text. Multiline slash prose and explicit `//` escaping remain literal. Slash arrows, Tab completion, Enter exact execution and Esc draft retention use the existing registry. |
| Command palette | PTY reproduced `sessions` selecting `/commands`: fuzzy matching searched verbose help prose common to many commands. Palette/help now use the same registry matching and ranking as slash discovery; keyboard-help rows remain searchable. No duplicate command handlers were added. |
| Sessions | Picker marks Active before the title, exposes full session identity in details, and prioritizes exact identities over fuzzy description matches. Existing create/list/attach/resume remain backend-owned. History with a missing or different session identity is ignored. Existing bounded per-session drafts survive switches and views; snapshots clear the previous conversation before new history is applied. |
| Archive | Existing backend archive action now requires a session-scoped y confirmation. n/Esc cancels without IPC mutation; disconnected/changed/active/pending state prevents confirmation. Draft/reference guards remain. Archive modal owns focus, suppresses mascot placement, rejects paste and shows its own footer hints. No deletion feature was invented. |
| Models | Real model.list entries retain backend identifiers, health, capabilities and reasons. Full model IDs rank before incidental fuzzy matches in health/description text. Selected markers refresh with authoritative terminal.state; selection acknowledgment does not optimistically claim readiness. Failures give /model retry and /doctor guidance while retaining draft and prior selection. |
| Loading and errors | Model/session request state is bounded by existing requests; empty pickers distinguish loading, no entries, no matches and disconnected state. Cached entries are labeled when disconnected, refreshed lists show a refresh hint, and selection guards remain enforced. No new polling mechanism was added. |
| Settings | /settings displays effective preference values and built-in/environment/temporary-override sources. /settings theme delegates to /theme; /settings details delegates to /details; mascot changes retain the existing mode handler. Reduced motion, ASCII and NO_COLOR launch controls and keyboard alternatives are explained. All changes remain process-local; no new storage or configuration writes. |
| Interaction | Centered transcript/composer widths, themes, Markdown and multiline fallbacks are preserved. Long picker rows retain existing cell-width elision; full identities and provider reasons are available in wrapped details. Modal escape and draft continuity are covered at all four target sizes. |

## Files changed in this phase

- `cli/crates/sentinel-tui/src/lib.rs`: request/loading state, local dispatch, session/model presentation, authoritative refresh, settings inspection/delegation, archive confirmation/focus, help text and regression tests.
- `cli/crates/sentinel-tui/src/picker.rs`: registry-driven command/help ranking and exact model/session identity priority; regression test.
- `cli/crates/sentinel-tui/src/commands.rs`: synchronized settings/archive metadata and help usage.
- `cli/crates/sentinel-tui/src/mascot.rs`: archive-overlay suppression; removed the obsolete mascot-only slash exception now covered by common dispatch.
- `docs/development/TUI_V3_COMMAND_SPEC.md`: current slash/paste contract, archive confirmation, theme/settings and session/model behavior.
- This report and `docs/reviews/tui-v33b/`: deterministic harness, output captures, text replays, IPC fixture records and verification logs. Reusing the CW-04.5 harness regenerated its existing fixture artifacts; its source was unchanged, and the current results were copied into this phase's evidence directory.

No AgentRuntime, observation policy, daemon, inference provider, Desktop, Blender asset or FA-3 icon changes were made by this phase. Other uncommitted work was preserved. Shared mascot capability code, including the CW-04.5 trailing-status-query fix, was not edited. No commit, push or merge.

## Supported and unavailable operations

Supported operations reuse the current contract: session create/list/attach/resume, finite rename/archive/export actions, discovered-model selection, provider inspection and local appearance commands. Archive confirmation is a client interaction guard; the daemon remains authoritative about acceptance and actual result. This phase tests archive cancellation through PTY and confirmation dispatch through Rust tests; it does not claim live archive persistence verification.

Unavailable: session deletion, new providers, verified inference readiness inferred from catalog health, new inference routing, persistent TUI customization, runtime mutation of environment-only reduced motion/ASCII/NO_COLOR, and unsupported registry features such as snapshots/undo/compaction/Plan. Their existing refusal behavior remains. No capability was added merely to fill a UI row.

## Validation

- **113 Rust tests passed**: CLI 14, IPC 11, mascot 19, TUI 69, plus doc tests. Eight added regressions cover palette ranking/slash arrows, registered and invalid paste versus multiline text, session identities/drafts/stale history, authoritative model selection/failure, settings delegation/source labels, archive confirmation, picker loading/error/focus at four sizes, and exact model-ID priority. Existing fixtures were updated to include required session identity and the revised single-line slash contract.
- Workspace `cargo fmt --check`: passed.
- Workspace Clippy with all targets and `-D warnings`: passed.
- Workspace debug and release builds: passed.
- **9 production interaction PTY journeys passed**: 80×24, 100×30, 120×40 and 160×48, each normal and ASCII/NO_COLOR, plus one disconnected fallback journey. They use disposable IPC fixtures and the actual release executable. Screen replays assert slash discovery/Esc, active identities, switch-away stale-content clearing, switch-back multiline draft retention, Ctrl+O, pasted settings, model failure then selection success, authoritative Selected marker, archive cancellation, session creation, typed/pasted invalid-command feedback and exit. Every journey restored termios and alternate screen; no chat.send or agent.start was issued.
- **15 CW-04.5 production PTY regressions passed** again: cold/repeated disconnected starts, Static/Animated/Auto/Off, connected and populated fixtures, capability timeout, missing assets, NO_COLOR/SSH/tmux gates, local settings, overlays, resize and cleanup. All assert absence of CSI 5n and retain real fragmented Kitty/cell-response negotiation. Visible cases upload/place before keyboard input; assets validate 132 required PNGs.
- Loading/empty/error states, stale reply rejection and modal focus use deterministic Rust/TestBackend fixtures. PTY stale-content checks verify clearing on switch; deliberately delayed stale history is covered by Rust tests, not a live daemon. Existing four-theme and responsive tests remain passing; physical contrast and host keyboard encoding are not inferred from those tests.

Initial restricted-shell workspace tests could not bind local Unix fixture sockets; rerunning with the required local-socket access passed. During implementation, PTY exposed the palette/model matching issues described above. A fixture session-create response initially omitted required type/created_at fields and was corrected to the existing contract; no protocol changes were made. The final complete runs passed. Personal-Brain executable was unavailable; the explicitly supplied repository path and local instructions were used.

Evidence: [PTY results](../reviews/tui-v33b/pty-results.json), [Rust tests](../reviews/tui-v33b/tests.log), [CW-04.5 revalidation](../reviews/tui-v33b/cw045-results.json), and [reproduction harness](../reviews/tui-v33b/pty_journeys.py). Text snapshots replay terminal output; they are not native screenshots.

## Remaining UX limits and physical verification

TUI settings and up to 32 session drafts remain local and nonpersistent. Environment-only preferences require relaunch; starting with mascot off intentionally skips graphics negotiation, so enabling graphics later may require a restart. Catalog health and model selection do not verify inference readiness. Long editor lines still use horizontal scrolling; grapheme editing and richer Markdown remain outside this phase. Backend reconnect notices may replace transient footer feedback; /notices exposes longer feedback. Shift+Enter depends on host keyboard support; Alt+Enter and Ctrl+O remain alternatives.

Release executable: `<repository-root>/cli/target/release/sentinel`.

SHA-256: `fa91ea3b0b02d56b2550d7fe30d755e55c28c7356d7b37a74ff7da15455d83ff`.

In Ghostty, with the existing daemon connection available, run:

```sh
cd "$SENTINEL_REPO" && SENTINEL_TUI_MASCOT=animated ./cli/target/release/sentinel tui
```

1. Touch no keys for five seconds; confirm the already accepted Core Warden initial-display behavior remains. A disconnected mascot is static and does not establish animated playback.
2. Type /, navigate with arrows, complete with Tab/Enter and cancel with Esc. Paste /settings mascot static and /settings theme glacier, then press Enter; verify local feedback. Try an invalid command and inspect /notices.
3. Enter a multiline draft with Shift+Enter or Ctrl+O. Open /sessions through Ctrl+P without replacing the draft, switch sessions, return and verify the original draft and correct conversation. Cancel /archive confirmation and check that the session remains.
4. Inspect /model, verify actual identities and Selected marker after a supported selection. Distinguish returned availability from inference readiness; verify actionable error if the backend refuses selection.
5. Inspect /settings, switch themes, open/close overlays, resize through the four sizes and check clipping, focus, contrast and image remnants. Exit normally. Relaunch with NO_COLOR=1 SENTINEL_TUI_ASCII=1 to inspect fallback; SENTINEL_TUI_REDUCED_MOTION=1 should keep graphics static where available.

These steps are pending physical V3.3-B acceptance. Fixture/PTY success is not a physical Ghostty PASS or a live daemon/inference integration claim. Work stops at V3.3-B; no V3.3-C or AgentRuntime work was started.
