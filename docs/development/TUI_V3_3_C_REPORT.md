# SENTINEL — TUI V3.3-C stabilization

2026-10-11. Focused Rust client stabilization. Automated tests and simulated production PTY checks are distinct from physical Ghostty acceptance, which remains pending for V3.3-B/C UX. The user's earlier physical CW-04.5 initial-display confirmation remains valid evidence for that earlier phase only.

## Changes and demonstrated causes

| Issue | Evidence and correction |
| --- | --- |
| Paused transcript becomes blank after widening | The B source retained the previous wrapped row offset beyond the resized content bottom. A realistic 60×20 → 80×24 regression failed against the preserved source. Display offsets now clamp to the current viewport bottom; PgUp/PgDn use the same bound. Only visible wrapped rows enter Paragraph. Drafts and paused-follow state remain intact. |
| Long transcript costs CPU on each animation frame | The actual B release repeatedly parsed and wrapped unchanged Markdown during 8 FPS mascot draws. A one-entry cache now reuses styled wrapped rows for unchanged text, width, theme and empty/conversation presentation. New text, session content, width or theme replaces the entry. No prior transcript history is accumulated. This preserves existing Markdown rendering. |
| Narrow settings opens at its end | A production 60×20 PTY journey exposed inspection text inheriting conversation follow-latest. Inspection views now start at row zero, save the conversation scroll/follow state and restore it on Esc or return to conversation. Commands no longer unconditionally enable follow-latest. Session switches explicitly reset the selected conversation position. Starting a real run discards the saved inspection position. |
| Narrow footer clips controls | A 60×20 replay showed a partial `F1 he`. Narrow footers now use complete shorter existing hints and width-bound fitting. Full keyboard help remains available. No shortcuts were added. |

No event-loop timers, polling intervals, terminal negotiation, image lifecycle, image assets or IPC contracts changed. The existing bounded input wait remains at most 50 ms; redraws remain driven by updates, input or eligible animation frames. CW-04.5's absence of trailing CSI 5n is preserved.

## Files changed in this phase

- `cli/crates/sentinel-tui/src/lib.rs`: cache use, viewport clamping, inspection navigation, compact footer and three regressions.
- `cli/crates/sentinel-tui/src/transcript_cache.rs`: bounded current-presentation cache and invalidation/replacement regression.
- `docs/development/TUI_V3_COMMAND_SPEC.md`: inspection navigation and responsive footer documentation.
- `docs/development/TUI_V3_3_C_REPORT.md`: this report.
- `docs/reviews/tui-v33c/`: reproducible production PTY/performance adapters, logs, JSON results and 48 text snapshots. The locally preserved B executable is ignored by Git.

The repository already contained substantial uncommitted work, including C++/AgentRuntime changes. Those files were not edited by this phase. No daemon, provider, Desktop, Blender or FA-3 icon changes; no commit, push or merge. Personal-Brain was unavailable, so the explicit repository path and repository-local architecture/security/build/testing instructions were used.

## Production performance methodology

The preserved B release and final release run sequentially under the same macOS PTY harness, with disposable typed IPC fixtures and confirmed simulated Kitty responses. No real daemon or inference is launched. Connected idle fixture state makes animation legitimate within the fixture; disconnected production cases are separately checked and remain static.

Each binary has nine runs: two repetitions of empty Off/Static/Animated, then one per mode with a 1,500-line ~192 kB UTF-8 Markdown transcript, within the existing IPC frame limit. The harness verifies the final transcript row actually rendered. After 1.5 seconds warming, a four-second steady window measures process CPU, actual draw boundaries, transmitted bytes and graphics commands. A later half-second window checks that warmed frames no longer upload. Five input updates, all six resizes and four long-transcript page scrolls are timed. Eight static↔animated plus overlay cycles sample RSS in the long Animated run.

CPU uses `proc_pid_rusage`, converted with `mach_timebase_info` (125/3 on this host), and cumulative values are cross-checked against `wait4` user+system CPU. Percent is percent of one core. Peak RSS includes `wait4` process peak; the parent harness and terminal-emulator/GPU work are excluded. Initial exploratory CPU values omitted the Mach timebase conversion and were discarded. Final JSON records both CPU counters and the conversion.

Input latency is first matching editor-byte receipt in the parent. Resize measures completion output after the resize clear and following cursor/draw boundary. Scroll measures the next completed draw; an animation tick can coincide, so it is a responsiveness proxy, not a physical pixel measurement. Short runs describe this workload, not a long-term leak proof or universal benchmark. PNG decoding stays in the existing background bounded cache; no new decode path was added.

| Workload / mode | B CPU % | C CPU % | B → C peak RSS MiB | B → C max input receipt ms | B → C max resize draw ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Empty / Off | 0.45 | 0.43 | 9.22 → 9.36 | 1.52 → 1.72 | 3.49 → 3.51 |
| Empty / Static | 0.42 | 0.43 | 12.19 → 12.05 | 1.57 → 1.61 | 3.43 → 3.73 |
| Empty / Animated | 1.49 | 1.61 | 18.20 → 18.42 | 1.57 → 1.51 | 3.37 → 3.71 |
| Long / Off | 0.42 | 0.41 | 12.59 → 13.45 | 21.32 → 2.69 | 22.80 → 10.81 |
| Long / Static | 0.44 | 0.41 | 13.95 → 14.92 | 23.10 → 3.16 | 24.04 → 10.95 |
| Long / Animated | 16.40 | 1.95 | 20.03 → 21.34 | 15.61 → 2.62 | 24.64 → 10.46 |

Empty CPU is the mean of two runs; RSS and latency are the maxima across those runs. Long results have one run per mode. The long Animated workload used about 88% less process CPU, with ~1.31 MiB extra peak RSS for the current transcript cache. Empty-screen differences are small and not treated as a confirmed performance regression or improvement. Resize still reparses at the new width.

Off and Static emitted **zero steady draws/bytes**. Candidate Animated emitted **32 draws in four seconds** in every run, retaining the 8 FPS target (B long Animated had 33). All 18 runs had zero steady full-screen clears and zero late uploads. During the earlier steady window Animated finished uploading the remaining first-cycle frames (candidate 4–5 uploads); warmed cycles reused images. Static uploaded its image once during initialization, with no steady retransmission. Frame PNG decoding remains the existing loader/cache behavior, corroborated by its passing bounded-cache tests.

Long-transcript page-scroll completion proxies improved from 17.11–21.61 ms across B modes to 1.51–2.93 ms across C modes. Candidate long Animated RSS across eight mode/overlay cycles was 21.17, 21.20, 21.23, 21.25, 21.28, 21.28, 21.30, 21.30 MiB: a small allocator/cache warm-up followed by a plateau in this bounded run, not proof of indefinite leak freedom. Every performance process restored termios/alternate screen and issued no inference.

Canonical measurements: [B JSON](../reviews/tui-v33c/baseline/performance.json), [C JSON](../reviews/tui-v33c/after/performance.json). B SHA-256: `fa91ea3b0b02d56b2550d7fe30d755e55c28c7356d7b37a74ff7da15455d83ff`; C SHA-256 is listed below. These are production-process measurements with simulated backend/terminal responses, not physical Ghostty benchmarks.

## Validation

- **Automated PASS:** full Rust workspace, **117 tests** (14 CLI, 11 IPC, 19 mascot, 73 TUI), no failures. Four new regressions cover cache invalidation/replacement, resize scroll bounds, inspection position restoration and the six-size overlay/draft matrix.
- **Automated PASS:** `cargo fmt --all --check`, `cargo clippy --workspace --all-targets -- -D warnings`, debug and release workspace builds.
- **Simulated PTY PASS:** **14 production interaction journeys**, all six sizes normal and ASCII/NO_COLOR plus disconnected 60×20 and 180×48. Slash discovery/navigation/cancellation, pasted local settings, invalid typed/pasted commands, session identity/switch/draft restoration, model selection failure/success/authoritative marker, archive cancellation and session creation passed. No inference was requested. Every process exited cleanly and restored termios/alternate screen.
- **Simulated PTY PASS:** **15 production Core Warden regressions**: cold/repeated disconnected startup, Off/Static/Animated/Auto, connected/populated fixture, timeout, missing assets, NO_COLOR/SSH/tmux fallback, overlays, resize, local settings and cleanup. Fragmented actual protocol-response parsing remains exercised. Assets validate the manifest and 132 required PNGs; visible cases upload/place before input. No CSI 5n is sent.
- **Automated responsive audit:** 48 TestBackend snapshots: welcome, transcript, slash, sessions, models, help, settings and archive at 60×20, 80×24, 100×30, 120×40, 160×48 and 180×48. Text wrapping, composer draft retention and bounded overlay placement pass. The 60-column welcome uses the existing narrow text fallback; larger eligible rectangles retain the existing mascot scaling. Populated compact placement follows existing size gates. No sidebar or dock added.
- Existing tests cover typed IPC stale generation/session rejection, disconnect/reconnect, modal precedence, Enter/newline/flow-control compatibility, Turkish Unicode editing, archive cancellation, settings scope, four themes, overlay suppression and owned-resource cleanup. No live backend readiness or cancellation reliability is inferred from fixtures.

Evidence: [Rust log](../reviews/tui-v33c/tests.log), [Clippy](../reviews/tui-v33c/clippy.log), [interaction results](../reviews/tui-v33c/interaction-results.json), [mascot results](../reviews/tui-v33c/mascot-results.json), [layout snapshots](../reviews/tui-v33c/layout/), [reproduction](../reviews/tui-v33c/README.md). An initial optional snapshot export used an incorrect relative directory; absolute-path export passed. The initial narrow interaction failure drove the inspection fix; the complete final matrix passed.

## Remaining limitations and readiness

Physical V3.3-B/C UX, host keyboard encoding, contrast, actual Ghostty latency/CPU and real daemon disconnect/reconnect still require observation. Selected/available models do not establish verified inference readiness. Session persistence and supported operations remain daemon-owned; no unsupported backend features were introduced. Preferences and bounded session drafts remain process-local. Starting with mascot Off skips capability negotiation; later enabling graphics may require relaunch. Shift+Enter depends on terminal support; Alt+Enter/Ctrl+O are existing alternatives. Long editor lines retain horizontal scrolling; grapheme editing is unchanged.

The one-entry transcript cache trades bounded current-content memory for less parsing. Presentation text is still constructed/compared on redraw, and streaming content or a width/theme change still reparses. Scroll offsets remain row-based after reflow; the fix prevents an empty out-of-range viewport, but does not promise exact word anchoring across widths. No speculative further optimization was made.

V3.3 is suitable to enter physical acceptance. V3.4 should be considered after this checklist passes and any observed regressions are resolved. Automated/PTY evidence alone does not close physical acceptance. AgentRuntime reliability remains deferred; no next phase is started.

## Exact physical Ghostty acceptance

Executable: `<LOCAL_HOME>/Desktop/Projects/Sentinel/cli/target/release/sentinel`.

SHA-256: `4e565fcc173d81d9900550b8bb90c097737ccb56e2945fbcfdb33ebbb4b398fc`.

Run from Ghostty:

```sh
cd <LOCAL_HOME>/Desktop/Projects/Sentinel && SENTINEL_TUI_MASCOT=animated ./cli/target/release/sentinel tui
```

1. Start at 180×48. Touch no keys for five seconds. Confirm Core Warden appears automatically; connected idle should animate at the existing 8 FPS target. A disconnected mascot must remain static and truthful.
2. Inspect empty and populated conversations at 60×20, 80×24, 100×30, 120×40, 160×48 and 180×48. Check welcome alignment, composer/transcript widths, wrapped Turkish text and code blocks. Narrow mascot fallback is expected. PgUp a long conversation, resize wider/narrower and verify content remains visible and the draft survives; Ctrl+B returns to latest.
3. Enter a Turkish multiline draft using Shift+Enter where supported, then Alt+Enter or Ctrl+O. Open Ctrl+P and F1, navigate with arrows/Tab/Enter, close with Esc. Verify draft retention and focus. Do not send a draft merely to test overlay navigation.
4. Open `/settings` through the palette while preserving the draft. Verify its first lines are visible, PgUp/PgDn scroll it, and Esc restores the conversation position. Test typed/pasted `/settings mascot static`, `animated`, `auto`, `off` locally. Relaunch with Animated if the process initially started Off. Check settings source/scope and themes.
5. Open session/model pickers; verify active/selected markers, correct conversation after switch and restored session draft. Cancel archive confirmation. Invalid slash commands must stay local. With a real active run, verify Ctrl+C requests cancellation without claiming completion; Ctrl+D must refuse active work/unsent drafts and exit when idle/empty.
6. Open/close overlays and resize repeatedly while connected animation runs. Check suppression, no stale images, no input-dependent animation and no remnants after exit. Test disconnected presentation and `/reconnect` using the existing daemon lifecycle; confirm honest loading/error states.
7. Relaunch with `NO_COLOR=1 SENTINEL_TUI_ASCII=1` and separately `SENTINEL_TUI_REDUCED_MOTION=1`. Verify readable fallback/static behavior. SSH/tmux graphics safety remains gated. Exit normally and confirm cursor, echo, colors and shell scrolling are restored.

Physical acceptance is **pending**, not PASS.
