# SENTINEL — TUI V3.4 terminal compatibility

> SEC-P1.1 privacy edit (2026-10-12): machine-specific paths are shown as role placeholders; recorded measurements and acceptance results are unchanged. Command examples use `SENTINEL_REPO`; set `export SENTINEL_REPO="$(git rev-parse --show-toplevel)"` from a checkout first. This edited report has a new file digest; original raw evidence is unchanged.

2026-10-11. Terminal compatibility audit and one demonstrated Unix input correction. No graphical protocol is forced from a terminal name. C++ daemon, AgentRuntime, Desktop, providers, Blender and icons were not changed. Existing dirty work was preserved. No commit, push, merge or V4 work.

## macOS finding and evidence limits

The user clarified that Apple Terminal.app differs only by showing the text mascot fallback, rather than waiting for a key. This is consistent with Sentinel's supported-protocol policy, and is not evidence of a presentation failure. Sentinel requires an affirmative Kitty or Sixel response plus a valid cell size; absence, refusal or invalid dimensions selects text. It does not implement iTerm's proprietary inline-image protocol or attempt images everywhere.

The computer-use tool refused access to `com.apple.Terminal` for safety reasons. No alternative automation was used to bypass that restriction. Consequently this phase has **no captured physical Terminal.app capability bytes** and no full physical Terminal.app acceptance. The Apple-like simulated profile below is an explicit fixture, not a capture of Terminal.app. The user's text-only observation is recorded separately from the simulated checks.

The earlier user-confirmed CW-04.5 physical Ghostty initial-image behavior is PASS for that earlier binary/phase. This phase changes input polling, so the latest release still needs a physical Ghostty smoke test. No new physical Ghostty PASS is claimed.

## Confirmed defect and correction

A delayed cell-size reply after the existing 300 ms capability budget reproduced a separate input problem. Isolation showed late DA1 alone passed, while late `CSI 6;16;8t` prevented a delayed conversation update from becoming visible before input. This is a deterministic PTY defect, not asserted to be the user's Terminal.app symptom.

Crossterm 0.28.1's Unix Mio source drains its TTY in an inner read loop. The numeric CSI cell-size report is not a recognized keyboard event. After dropping it, the source attempts another read on the blocking descriptor; the outer poll timeout cannot interrupt that read. The isolated Rust control requested 50 ms, received only the cell-size report, and returned after **599 ms**, when the harness supplied a recovery key. With the correction it returned **51 ms**, false, without a key. See [control results](../reviews/tui-v34/poll-control-results.json) and the before-fix [isolation results](../reviews/tui-v34/late-isolation-results.json).

`poll_terminal_event` now temporarily enables nonblocking stdin reads only during Crossterm polling on Unix, then restores the original flags before rendering or reading the queued event. A drop guard retries restoration on error/unwind. Restoring before output is essential because stdin and stdout can share an open file description. Leaving the terminal globally nonblocking could break image writes. Windows retains its existing native Crossterm path. This adapter is used by both production TUI and the independent playback example.

No new timer, probe, input thread, busy loop, synthetic key, redraw or screen clear was added. The 50 ms input bound and eligible 8 FPS scheduling remain. Capability negotiation still happens once, before input reading, with bounded time/bytes, actual protocol/cell checks and unchanged safety gates. The trailing **CSI 5n remains removed**. Cache, asynchronous loading, image IDs, placement, suppression and owned-image deletion remain unchanged.

## Compatibility matrix

“Supported path” describes Sentinel's policy; it does not assert physical testing of that emulator/version.

| Environment | Supported path / decision | Evidence / physical status |
| --- | --- | --- |
| Ghostty | Kitty only after ACK + valid cell; text otherwise | Prior user CW-04.5 physical initial display PASS; final V3.4 smoke pending. Fragmented positive and name-only negative fixtures pass. [Official graphics feature](https://ghostty.org/docs/features). |
| Apple Terminal.app | Text when no supported protocol/cell is confirmed | User reports only text fallback. No actual response capture; full physical acceptance pending due tool restriction. No name-based exception added. |
| Kitty | Confirmed Kitty + cell; text on refusal/absence | Simulated response path passes, including a `TERM=xterm-kitty` refusal. Physical pending. [Protocol specification](https://sw.kovidgoyal.net/kitty/graphics-protocol/). |
| iTerm2 | Confirmed Sixel/Kitty if returned; otherwise text | Proprietary inline-image protocol is outside Sentinel's implemented paths. Physical pending. [Official inline-image documentation](https://iterm2.com/documentation-images.html). |
| Linux terminals (e.g. GNOME Terminal, Konsole, foot, xterm, Alacritty) | Per-instance response determines Kitty/Sixel/text; no family whitelist | Fragmented Sixel and negative/timeout fixtures pass on macOS. No Linux executable build or physical test here; support can vary by version/configuration. |
| Native Windows Terminal | Existing non-Unix detector deliberately chooses text | No Windows cross target installed; no Windows build/physical PASS. Host graphics capabilities are not inferred to override this limitation. WSL follows the Unix build and safety gates; not tested physically. |
| SSH | Text; no graphics probe | SSH safety gate tested; both environment keys audited. Remote/emulator passthrough is not assumed. Physical pending. |
| tmux / screen | Text; no graphics probe | Existing TMUX/STY/TERM/TERM_PROGRAM gates retained. tmux and screen simulated paths pass. Physical pending. |
| NO_COLOR / ASCII | Readable text; no graphics probe | Six-size interaction matrix plus capability gate cases pass. |
| Reduced motion | Confirmed image remains static; fallback remains text | Existing player tests pass; does not bypass safety or protocol checks. |
| Non-TTY | No image probing | Pure safety-gate regression passes; interactive TUI still requires a usable terminal. |

The test deliberately confirms Kitty while `TERM_PROGRAM=Apple_Terminal`, and refuses graphics with Ghostty/Kitty names but no affirmative response. These fixtures prove names do not grant capabilities; they do not describe those emulators' actual responses.

## Input and lifecycle

Enter/local slash dispatch, bracketed multiline paste, Turkish UTF-8, Ctrl+O, Ctrl+P, F1, Esc, idle Ctrl+C draft preservation, Ctrl+D draft refusal, resize and clean exit are exercised in production PTYs. Existing Rust tests cover Enter send, Shift/Alt+Enter newline, active cancellation, modal precedence, drafts, stale session responses and settings. No universal Shift+Enter claim: the host must encode it distinctly; Alt+Enter and Ctrl+O remain alternatives. Physical focus restoration, OS shortcut interception and keyboard encodings remain acceptance items.

Ratatui raw/alternate-screen initialization, flush boundaries and cleanup guard were audited. The adapter restores descriptor blocking state before each draw; production journeys verify termios and alternate-screen restoration. The isolated control verifies raw mode and O_NONBLOCK restoration. Existing terminal cleanup handles recoverable errors/early returns, while the input guard covers its own error/unwind path. SIGKILL/process crash cannot promise terminal restoration. No platform-specific daemon behavior was introduced.

## Files changed

- `cli/crates/sentinel-mascot/src/terminal_input.rs`: scoped Unix polling adapter; native passthrough elsewhere.
- `cli/crates/sentinel-mascot/src/lib.rs`: export shared adapter.
- `cli/crates/sentinel-mascot/Cargo.toml`: declare `fs` and `termios` on the existing Unix rustix dependency; no new package/version.
- `cli/crates/sentinel-mascot/src/tests.rs`: safety/cell bounds and fragmented negative-Kitty/Sixel regressions.
- `cli/crates/sentinel-mascot/examples/playback.rs`: use adapter.
- `cli/crates/sentinel-mascot/examples/input_poll.rs`: minimal upstream-versus-adapter reproduction.
- `cli/crates/sentinel-tui/src/lib.rs`: replace only the poll call with shared adapter.
- `docs/reviews/tui-v34/`: fixtures, logs and evidence; this report.

## Validation and performance

- **Automated PASS:** 119 Rust workspace tests (14 CLI, 11 IPC, 21 mascot, 73 TUI); formatting, Clippy all targets with `-D warnings`, debug/release builds and release examples.
- **Simulated terminal PASS:** 29 final production compatibility journeys: 14 response/safety profiles at 60×20 and 120×40, each resized to 100×30, plus disconnected startup. These include absent/negative protocol, missing/invalid cell, fragmented Kitty/Sixel, near-budget fragments and late DA/cell replies after the budget. Delayed IPC text appears before any keyboard input in every case, within a three-second bounded observation window. The window is a fixture assertion, not a production delay.
- **Simulated PTY PASS:** 14 existing interaction journeys across 60×20, 80×24, 100×30, 120×40, 160×48 and 180×48, normal and ASCII/NO_COLOR, plus narrow/large disconnected journeys. Every journey restores termios/alternate screen; local commands do not request inference.
- **Simulated Core Warden PASS:** 15 production regressions for modes, cold/warm startup, connected/disconnected, overlays, populated conversations, resize, missing assets, timeout, SSH/tmux/NO_COLOR and owned cleanup. Assets remain the existing manifest/132 PNGs. No CSI 5n; image bytes/placement before input are verified where eligible. Sixel protocol classification/normal interaction is tested with synthetic responses; physical Sixel rendering is not claimed.
- **Isolated Rust control PASS:** unadapted polling requires the recovery key; adapted polling returns at 50–51 ms. Both initially blocking and initially nonblocking stdin preserve their O_NONBLOCK state, with raw-mode restoration.

Evidence: [Rust](../reviews/tui-v34/tests.log), [compatibility](../reviews/tui-v34/compatibility-results.json), [interaction](../reviews/tui-v34/interaction-results.json), [mascot](../reviews/tui-v34/mascot-results.json), [poll control](../reviews/tui-v34/poll-control-results.json). Initial disconnected-fixture wording expected a label that was obscured by a more specific connection-error notice; the assertion now checks the actual Disconnected header. One early concurrent run's 1.3-second observation ended before fixture history arrived; isolated retry passed and final runs use a bounded three-second no-input deadline. Neither adjustment adds production waiting.

Four final production samples use the V3.3-C macOS harness after builds and other PTY runs finish: 1.5-second warm-up, four-second steady window, Mach-timebase-corrected process CPU cross-checked against wait4, peak RSS, input bytes, resize output and a later warmed upload check. The long case contains 1,500 lines and eight mode/overlay cycles. Terminal-emulator work is excluded.

| Final workload | CPU % of one core | Peak RSS MiB | Max input receipt ms | Max resize draw receipt ms |
| --- | ---: | ---: | ---: | ---: |
| Empty / Off | 0.56 | 9.30 | 1.80 | 3.59 |
| Empty / Static | 0.68 | 12.16 | 1.70 | 3.17 |
| Empty / Animated | 1.87 | 18.44 | 1.65 | 2.53 |
| Long / Animated | 2.17 | 21.47 | 2.96 | 9.41 |

Off/Static emitted zero steady draws/bytes. Both Animated samples emitted 32 draws/four seconds, retaining 8 FPS. All four had zero steady full-screen clears and zero warmed uploads; no PNG decode path changed. Eight long-case mode/overlay cycles reached a sampled RSS plateau of 21.45 MiB; this bounded sample is not an indefinite leak proof.

The recorded V3.3-C long Animated sample was 1.95% CPU / 21.34 MiB; this run is 2.17% / 21.47 MiB. Empty C means were 0.43/0.43/1.61% versus this single sample's 0.56/0.68/1.87%. These are historical, not fresh paired measurements; small differences include scheduling noise and the adapter's bounded fcntl/isatty syscall overhead. The earlier 16.40% long-transcript bottleneck remains eliminated. Input/resize receipt is a PTY proxy, not physical latency. No claim of zero overhead or physical performance PASS.

[Final performance data](../reviews/tui-v34/performance-results.json), [recorded C data](../reviews/tui-v33c/after/performance.json).

The host has only `aarch64-apple-darwin` installed. Platform-independent decision tests run here; neither a Linux nor Windows build/physical result is implied. No unsupported graphical fallback or protocol expansion was needed. Personal-Brain was unavailable; the supplied repository path and local instructions were used.

## Exact physical acceptance commands

Executable: `<repository-root>/cli/target/release/sentinel`.
SHA-256: `f72a3d1ff11fbc13b521981860a32cf3ff0ccc72a70121c81dacd1001afa4b15`.

In **Terminal.app**, run:

```sh
cd "$SENTINEL_REPO" && SENTINEL_TUI_MASCOT=auto SENTINEL_TUI_MASCOT_DIAGNOSTICS=/tmp/sentinel-v34-terminal.jsonl ./cli/target/release/sentinel tui
```

In **Ghostty**, run:

```sh
cd "$SENTINEL_REPO" && SENTINEL_TUI_MASCOT=animated SENTINEL_TUI_MASCOT_DIAGNOSTICS=/tmp/sentinel-v34-ghostty.jsonl ./cli/target/release/sentinel tui
```

1. Touch no keys for five seconds. Terminal.app text fallback is acceptable; Ghostty should retain automatic Core Warden presentation. Verify connected/disconnected labels update without input. A disconnected mascot stays static; only connected eligible animation establishes playback.
2. Inspect 60×20, 80×24, 120×40 and 180×48. Resize empty and populated views. Check draft/scroll retention and overlays without stale images.
3. Type Turkish text; paste multiple lines with `/paths`. Use Ctrl+O or Alt+Enter; check Shift+Enter only if that host supports a distinct encoding. Open Ctrl+P and F1; navigate and Esc back without losing draft.
4. Paste `/settings mascot static`, `/settings mascot auto`, and an invalid slash command on separate tests. Enter must execute locally. Check session/model/settings behavior and archive cancellation. Idle Ctrl+C preserves a draft; Ctrl+D must refuse drafts/active work and exit when idle/empty.
5. Exit normally. Verify shell echo, cursor and scrolling. Inspect the bounded diagnostics for `protocol`, `capability_reason` and `probe_ms`; the Terminal.app trace will establish its actual response/fallback reason. Logs contain safe terminal/presentation metadata, not keystroke contents.
6. Repeat with `NO_COLOR=1 SENTINEL_TUI_ASCII=1`, and separately `SENTINEL_TUI_REDUCED_MOTION=1`. SSH/tmux/screen should retain text fallback rather than forced graphics.

Physical macOS Terminal acceptance: **pending**. Latest-release Ghostty smoke: **pending**; previous CW-04.5 physical display: **user-confirmed PASS**. Linux/Windows physical verification: **pending/unavailable**. Stop at V3.4; no V4 work started.
