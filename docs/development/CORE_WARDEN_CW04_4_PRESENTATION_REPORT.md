# CW-04.4 — direct Kitty presentation adapter

> SEC-P1.1 privacy edit (2026-10-12): machine-specific paths are shown as role placeholders; recorded measurements and acceptance results are unchanged. Command examples use `SENTINEL_REPO`; set `export SENTINEL_REPO="$(git rev-parse --show-toplevel)"` from a checkout first. This edited report has a new file digest; original raw evidence is unchanged.

Status: implemented and verified at the protocol/output boundary. **Physical Ghostty acceptance is pending user confirmation.**

## Finding and limits of the diagnosis

The user physically confirmed that a standalone Python direct Kitty image appears without input, remains visible for five seconds and disappears on explicit deletion. Both consumers of the previous mascot Player instead used ratatui-image's virtual placement (`a=T,U=1`) and U+10EEEE Unicode placeholders. Prior actual-write/flush measurements excluded a missing loader notification or an upload waiting for keyboard input.

This isolates the failing presentation path to virtual/Unicode placement in the reported environment. It does **not** prove an internal Ghostty GPU/glyph invalidation bug, nor explain exactly why its keypress-triggered repaint resolves the old path. The correction bypasses that dependency using the directly placed image mechanism that the user physically established works. No speculative timer or screen invalidation was added.

## Minimal A/B control

`kitty_first_paint` now runs both paths through the same raw mode, alternate screen, confirmed Kitty negotiation, background PNG decode/resize, Ratatui terminal, 16×8 rectangle at x=5/y=5, five-second no-keyboard lifetime, owned image deletion and terminal restoration. The only presentation branch is `--direct`.

Captured headers, with numeric image IDs omitted here:

| Boundary | A: old Unicode path | B: direct control / production adapter |
| --- | --- | --- |
| First upload | `q=2,i=ID,a=T,U=1,f=32,t=d,s=256,v=272,m=1` | `q=2,i=ID,a=t,f=32,t=d,s=256,v=272,m=1` |
| Placement | 128 U+10EEEE placeholders inside Ratatui cell symbols | `a=p,i=ID,p=1,c=16,r=8,C=1,z=1,q=2` after draw/flush |
| Position | Per-row cursor/SGR/diacritic placeholder sequences | Save cursor, `CSI 6;6 H`, explicit placement, restore cursor |
| Exit | `a=d,d=I,i=ID,q=2` | same owned-ID deletion |

Both use identical upstream raw RGBA encoding and chunking; the direct path changes the upload action and creates a real placement rather than a virtual one. The existing Python control's source was not supplied, so byte-for-byte equivalence with that script is not claimed. Direct placement, cursor policy and targeted deletion follow the [official Kitty graphics protocol](https://sw.kovidgoyal.net/kitty/graphics-protocol/).

PTY A/B captures confirm one upload each, A=128 placeholders/zero direct placements and B=zero placeholders/one direct placement. The first upload arrived approximately 7.2ms and 4.0ms respectively in the final captures. These timing differences are not a performance benchmark or evidence of physical visibility. Both explicitly delete their one owned image after five seconds. Raw-mode settings restore; the A/B harness excludes macOS's kernel-maintained PENDIN flag when comparing termios.

Artifacts: `docs/reviews/core-warden-cw044/A-unicode-results.json`, `B-direct-results.json`, and `ab_pty.py`. Header records contain no image payload.

## Correction

The shared Player prepares a bounded direct Kitty frame cache in its background loader. It reuses ratatui-image 8.0.2's encoder and generated image IDs, extracts its upload at a validated boundary, converts only `a=T,U=1,` to upload-only `a=t,`, and drops the original protocol object. Unexpected upstream output fails closed to the existing load-error/text fallback. No new dependency was added.

During `Player::render`, Ratatui owns the selected layout rectangle and draws ordinary cells; the Player queues the selected image/rectangle without putting escape sequences into cell symbols. Each consumer calls `Player::present` **after** `Terminal::draw` returns and flushes. Production uses the same instrumented Crossterm backend writer for graphics, so the actual graphics write and final flush are measured.

`present` uploads a cached frame only once, then places it at the Ratatui-selected cursor anchor with explicit columns/rows, fixed per-image placement ID and cursor preservation. An unchanged frame/rectangle emits nothing. When changing frames it uploads a new frame before removing the old placement; soft deletion removes only the previous owned image's placement and retains cached data for the next animation loop. Movement replaces the placement without uploading again. Partially clipped rectangles suppress placement and remove the previous placement instead of stretching or painting outside the layout. Hide/off/state changes/resize cache replacement release only owned IDs. Overlays have priority because slot selection hides/releases the mascot before drawing them.

The ineffective CW-04.2 16ms follow-up was removed. Static/off/hidden presentation does not schedule an animation wake. The existing 8 FPS clock, background loading, bounded cache, reduced motion, disconnected neutral presentation, text fallback and capability safety gates remain. Non-Kitty graphics retain the upstream renderer. The encoder remains bounded by the same clip count, source resolution and display rectangle limits; no duplicate encoded frame cache is retained. Tests verify upload reuse across a full 16-frame loop. Idle static presentation generates no repeated placement bytes. No new CPU benchmark is claimed.

## Modified source

- `cli/crates/sentinel-mascot/src/playback.rs`: direct cache and post-draw adapter; removes 16ms workaround; targeted lifecycle and clipping tests.
- `cli/crates/sentinel-mascot/src/tests.rs`: replaces workaround-specific test with input-free direct-placement and unchanged-frame regression.
- `cli/crates/sentinel-mascot/examples/playback.rs`: uses shared post-draw adapter.
- `cli/crates/sentinel-mascot/examples/kitty_first_paint.rs`: same-lifecycle A/B control and owned cleanup.
- `cli/crates/sentinel-tui/src/lib.rs`: calls the shared adapter after terminal draw/flush.
- `cli/crates/sentinel-tui/src/mascot.rs`: diagnostics identify direct presentation, encoded cache bytes and empty/populated state without conversation content.
- `cli/crates/sentinel-tui/src/mascot_output.rs`: counts actual direct placement commands; regression proves placement occurs after backend flush without keyboard input.
- `docs/reviews/core-warden-cw044/`: A/B and production fixture scripts, results and validation logs.

Existing V3.3 and unrelated uncommitted changes preserved. No Desktop, daemon, AgentRuntime, icons, Blender models or animation assets modified. No commit, push or merge.

## Validation

- Focused Rust tests: **19 mascot + 61 TUI = 80 passed**.
- Formatting check: passed.
- Clippy for both crates, all targets, `-D warnings`: passed.
- Production workspace release build and mascot examples build: passed.
- 15 production release PTY fixture scenarios: passed. Static/Animated/Auto/Off; connected/disconnected; cold/warm startup; empty/populated layouts; missing assets; negative capability response; NO_COLOR/SSH/tmux safety; local settings commands; overlay open/close; narrow/restore resize; terminal restoration and owned deletion. Existing editor/layout tests cover keyboard and Unicode behavior; PTY captures are not screenshots.
- Production disconnected Static capture: at monotonic 186,091µs, one upload and one direct placement, zero Unicode placeholders/virtual placements, seven completed backend flush calls. No keyboard event had occurred. This is output evidence, not physical Ghostty acceptance.
- Regression: upload and direct placement after background load with no input across Static/Animated/Auto and connected/disconnected; 8 FPS frame selection; full loop reuses 16 uploads; unchanged frame emits no bytes; reposition retains upload; clipping hides; no global clear; owned cleanup and hidden state produce no new placement.

The populated fixture checks actual populated application state rather than searching for contiguous transcript text inside an incremental terminal diff. Physical absence of ghost images and visible overlay priority still require visual acceptance.

Actual release executable: `<repository-root>/cli/target/release/sentinel`.

## Physical Ghostty verification

Optional same-lifecycle comparison, run each separately in Ghostty without touching keys:

```sh
"$SENTINEL_REPO/cli/target/release/examples/kitty_first_paint"
```

```sh
"$SENTINEL_REPO/cli/target/release/examples/kitty_first_paint" --direct
```

A is the previous virtual presentation. B uses explicit direct placement, remains for five seconds and explicitly deletes its image. Physical results of these new controls have not been observed by the agent.

Final production command:

```sh
SENTINEL_TUI_MASCOT=animated SENTINEL_TUI_MASCOT_DIAGNOSTICS="/tmp/sentinel-cw044-$(date +%s).jsonl" "$SENTINEL_REPO/cli/target/release/sentinel" tui
```

Touch no keys for five seconds and verify automatic appearance. With a connected legitimate animated state, verify animation without input. Disconnected neutral display does not prove animation. Open Ctrl+P and close with Esc; inspect for image remnants. Resize and inspect placement. With an empty composer, Ctrl+D exits; inspect normal terminal restoration. The new diagnostics expose `graphics_presented`, `direct_placements`, image IDs, flush timestamps and the first keyboard-event timestamp without recording key contents.

**Do not mark physical Ghostty PASS until the user confirms the new direct control and production result.**
