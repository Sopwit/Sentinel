# CW-04.3 — presentation investigation

Status: **physical defect unresolved; no proven presentation fix**. This change adds measurements and regression coverage. It must not be described as a successful CW-04.3 fix.

## Findings

The disposable production PTY fixture confirms that background decode completion reaches Player, dirties the UI, invokes the upstream cached image protocol, passes Ratatui diffing, writes the Kitty upload and Unicode placement bytes through Crossterm, and completes stdout flush without keyboard input. Existing CW-04.2 physical observations remain authoritative: the mascot was not visible until a key was pressed. The PTY is a byte sink with a simulated capability response, not a Ghostty renderer.

The final no-input capture recorded:

- First completed loading draw: 2,798 writer bytes, no graphics.
- Cache-ready draw at 137,704 monotonic microseconds: 383,012 cumulative writer bytes; one `a=T,U=1` upload; 128 actual U+10EEEE placement characters; six completed backend flush calls; last flush at 136,632 microseconds. Image ID 517436958, slot [82,17,16,8], eight image-bearing Ratatui cells (one encoded row per cell), and 5,738 terminal-buffer diff entries.
- Existing 16ms follow-up: no new upload, another 128 placeholders. This workaround is not accepted as a fix and has not been extended.
- PTY reader independently received upload and placeholders before any input.

No-input, one-key and resize-only runs all emitted graphics before the 2-second comparison action. The key did not cause the first upload. Resize exercises placement reconstruction; it is not evidence of physical visibility. Exact final captures are in `docs/reviews/core-warden-cw043/comparison-results.json` and individual diagnostics/byte-receipt files.

Independent direct Ratatui/ratatui-image example, without Player or IPC: one upload and 128 placeholders before input, approximately 4.4ms to the first upload. This also does not test Ghostty painting. Alternate screen restored; normal termios settings restored excluding macOS's kernel-maintained PENDIN bit.

Installed Ghostty plist reports version 1.3.1. This identifies the installed application, not the version of any particular existing terminal process. No Ghostty internal renderer bug is proven. Capability ACK confirms the tested graphics command, not successful GPU presentation.

## Changes

- `cli/crates/sentinel-tui/src/mascot_output.rs`: opt-in actual successful-write counters, split-write-safe Kitty header/Unicode parsing, completed-flush timestamps and bounded image IDs. Does not store user text or image payload. Adds real Crossterm output regression across Static/Animated/Auto and connected/disconnected startup, with no keyboard event.
- `cli/crates/sentinel-tui/src/mascot.rs`: bounded draw boundary and first-key-event diagnostics; reserves one record for the first key, without recording its contents. Maximum 64 records / 64KiB. Existing state, cache, rectangle, capability and asset diagnostics retained.
- `cli/crates/sentinel-tui/src/lib.rs`: wraps the production Crossterm writer; records draw initiation/completion, output counters and opt-in terminal-buffer diff counts. Normal Ratatui raw-mode initialization and cleanup remain in place. Buffer snapshots exist only in memory while diagnostics are enabled and are never logged.
- `cli/crates/sentinel-mascot/examples/kitty_first_paint.rs`: independent diagnostic example with background decode, one direct image draw and five seconds without keyboard reads.
- `docs/reviews/core-warden-cw043/`: disposable fixture scripts, numeric results and validation logs.

There is **no newly introduced timer, full-screen invalidation, forced graphics capability or speculative renderer workaround**. Existing CW-04.2 scheduling is unchanged. Existing V3.3/uncommitted work preserved. No daemon, AgentRuntime, Desktop, official icon or Blender assets modified by this task. No commit/push/merge.

## Validation

- Focused Rust tests: 18 mascot + 61 TUI = 79 passed.
- Formatting check passed.
- Clippy, both crates, all targets, `-D warnings`: passed.
- Production workspace release build: passed.
- Production PTY mode/fallback/overlay/resize matrix: 15 scenarios passed; see `production-pty-results.json`; simulated capability and IPC, not physical acceptance.
- Local mascot settings are checked by existing typed/pasted dispatch tests, with no inference. Fixture rejects unexpected chat/agent requests.
- No claim of physical animation, overlay cleanup, keyboard responsiveness or Ghostty presentation PASS.

Actual release executable: `<LOCAL_HOME>/Desktop/Projects/Sentinel/cli/target/release/sentinel`.

## Required real Ghostty evidence

Run in Ghostty:

```sh
SENTINEL_TUI_MASCOT=animated SENTINEL_TUI_MASCOT_DIAGNOSTICS="/tmp/sentinel-cw043-$(date +%s).jsonl" <LOCAL_HOME>/Desktop/Projects/Sentinel/cli/target/release/sentinel tui
```

Touch no keys for five seconds. Record whether Core Warden is visible before input; if absent, press one harmless key and record whether it appears. Open/close the command palette with Ctrl+P/Esc and inspect remnants. Connected actual animated activity is required to assess animation; disconnected neutral presentation is not animation proof. Clear the draft, then exit with Ctrl+D and inspect terminal restoration.

The new real-terminal diagnostic needs to be correlated with the user's visual observation before selecting a presentation fix. It distinguishes missing writes/flush from terminal interpretation or painting failure. If upload, placeholders and completed flush precede the first key in the failing Ghostty run, further investigation belongs after the stdout boundary; targeted cell invalidation remains unproven and has not been added.
