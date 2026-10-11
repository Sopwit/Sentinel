# Core Warden CW-04.2 — startup and empty-screen placement

## Evidence and cause

The user's physical CW-04.1 diagnostic files were available locally and are preserved in `docs/reviews/core-warden-cw042/prior-physical-*.jsonl`. They show confirmed Kitty, actual cell size **16×34**, successful manifest/PNG discovery, `connected=false`, state `error`, one loaded static frame (36,864 source RGBA bytes), and a render invocation. Thus the offline path already accepted background results and attempted rendering; an absent asset or failed capability probe is not the demonstrated cause. These old records have no input timestamps, so they cannot independently establish the exact keyboard-to-pixel timing.

Three code-level issues explain the fragile presentation:

1. A static/offline cache had no scheduled paint after its initial Kitty upload. The upstream stateful protocol includes image transmission only on first render; later renders are placement-only. Once static cache readiness was processed, `next_wake()` returned None. The next placement-only pass could therefore depend on unrelated IPC/input. A bounded explicit placement refresh closes this gap. This is the lifecycle correction for the reported symptom; its effect on physical Ghostty requires the new user test below. We do not claim to have proven a Ghostty internals defect.
2. Static/disconnected source selection is the canonical **96px neutral**. Empty placement used 96/128 source pixels as display pixels, and `Resize::Fit` never enlarged smaller sources. On the real font that is only 6×3 cells (or a 96px image inside a nominal 8×4 empty slot), rather than prominent welcome artwork. This was an empty-size calculation defect, not accidental use of the populated branch: the physical diagnostics confirm a centered empty slot, not y=0 header placement.
3. Fallback labels used an unwrapped Paragraph. `Unavailable` cannot fit in six cells, causing the reported partial word.

Also, worker pre-encoding used the requested rectangle while UI `StatefulImage` could recompute a different fitted rectangle and encode again on the interactive thread. Worker and render sizing now agree, and render directly invokes the maintained protocol's cached render method. No graphics codec or capability bypass was introduced.

## Changes

- `cli/crates/sentinel-mascot/src/playback.rs`: explicit dirty result for selection changes and cache-ready/load-failed states; one 16ms deadline after the first Kitty paint of each cache; cancellation of the deadline on release/hide/mode/state changes. `next_wake` returns the earlier of this one-shot deadline and an actual animation frame deadline. The second paint uses the same owned image ID without retransmission. It does not loop or animate disconnected state.
- The existing production loop continues collecting load results within its bounded **50ms IPC/input wait ceiling**. Cache completion returns dirty automatically; no new steady polling or input dependency is introduced. The worker still waits on its condition variable when idle. A test-only loader delay checks a completion later than the initial draw; no production delay setting was added.
- Empty display targets: 128px at 80×24, 192px at 100×30, and at most 256px for roomy screens. Available reading cells constrain the target and leave five welcome rows plus one gap. Sources remain the existing 96/128px assets; static fallback remains canonical neutral 96px. Populated artwork remains compact, approximately 48px in a reserved header slot. Reading/composer width, header and overlays are preserved.
- Pixel scaling and encoding use `Resize::Scale` with Triangle filtering on the worker. The UI only renders pre-encoded protocols. On the observed 180×50, 16×34-cell terminal, the empty slot is now **[82,17,16,8]**, approximately 256×272 cell pixels, instead of [86,20,8,4] containing a non-upscaled 96px image. Cell padding can add transparent pixels; 256px is the target graphic size, not an exact nontransparent silhouette bound.
- Fallback shows a loading label while pending if it fits. A state label is shown only when its complete width and second row fit; otherwise it is omitted. A small symbol remains and normal daemon/readiness text is preserved. No partial status words are drawn.
- `cli/crates/sentinel-mascot/src/tests.rs`: no-input cold/repeated cache-start regression for Static/Animated/Auto offline, scaled large-font placement, whole-label checks. Private playback tests additionally cover 150ms delayed loading, pre-encoded sizing, exact 125ms connected frame deadlines, disconnect replacement and owned cleanup.
- `cli/crates/sentinel-tui/src/mascot.rs`: extend existing bounded diagnostics with loading, cache-ready, redraw-request/reason, selected frame index and one-shot placement refresh status. Startup capability/asset results remain. Limits remain 64 records / 64 KiB, no prompts or model output.
- `cli/crates/sentinel-mascot/README.md`: lifecycle and sizing documentation.

The production `lib.rs` and `editor.rs` were not changed in CW-04.2, preserving the ongoing V3.3 work. No daemon, AgentRuntime, Desktop, Blender/animation assets or FA-3 icons were modified. No commit, push or merge was performed. `brain` was unavailable on PATH; repository-local instructions and the supplied workspace were used.

## Validation

- Focused Rust tests: **77 passed** (mascot 18, TUI 59), including the explicit sequence startup → delayed background result → dirty draw → placement-only redraw → no further static wake, with no user event injected.
- Formatting check, workspace Clippy all targets with `-D warnings`, and workspace release build passed.
- **15 production release PTY scenarios passed**, using deterministic terminal responses with the observed 16×34 cell geometry and deterministic IPC where connected. Cases include fresh offline process, repeated offline process, offline Static/Auto, connected Animated/Static/Off/Auto, populated layout, negotiation timeout, missing assets, NO_COLOR, SSH, tmux, and combined preference/overlay/resize/Unicode input scenario. All restore terminal attributes and alternate screen. No inference request is sent by preferences. Existing owned-image deletion checks and matrix cleanup pass. Missing/unsupported presentations load no frames.
- Fresh offline process transmitted its first image before any keyboard event at 58.6 ms; repeated offline process at 58.6 ms. Both contain the automatic `kitty_placement_refresh` diagnostic with exactly one static cache frame. These are PTY transmission timings, not physical pixel-display measurements. “Cold” here means first fresh test process/cache; the OS page cache was not forcibly purged.
- Connected animation has 16 cached frames and 125ms elapsed frame intervals (8 FPS); the fake-clock test verifies no advance at 124ms and advance at 125ms. Disconnected artwork remains one neutral static frame even in Animated/Auto. It is **not proof of animated playback**.

Evidence and reproducible fixture: `docs/reviews/core-warden-cw042/production_pty.py`, `production-pty-results.json`, per-case diagnostics, `focused-tests.log`, `clippy.log`, `release-build.log`, `release-identity.json`. No live Agent benchmark was used. The screenshot-like output of a PTY is not treated as physical Ghostty acceptance.

Release executable: `<LOCAL_HOME>/Desktop/Projects/Sentinel/cli/target/release/sentinel`.
SHA-256: `4ac9cc2d89a4c94be3f3578f63ced78af424aefec0eb6ad0008141ec7900fdc3`.

## New physical Ghostty check

Run inside a real Ghostty window (120×40 or larger is recommended):

```sh
cd <LOCAL_HOME>/Desktop/Projects/Sentinel
SENTINEL_TUI_MASCOT=animated \
SENTINEL_TUI_MASCOT_DIAGNOSTICS="/tmp/sentinel-cw042-$(date +%s).jsonl" \
./cli/target/release/sentinel tui
```

1. Do not press any key for at least two seconds. With the daemon disconnected, a clearly visible neutral Core Warden should appear automatically above welcome. It should remain static; the normal disconnected status remains truthful.
2. Confirm welcome/composer alignment and absence of clipped `Unavailable` text. Change locally through `/settings mascot static`, `off`, `auto`, `animated`. Off removes it. Offline Auto/Animated still show static artwork.
3. Open/close the command palette, resize below 80×24 then restore. Artwork must hide/reappear with correct layering and leave the draft intact. Exit idle with Ctrl+D; the terminal must restore normally.
4. With the daemon already connected through the usual application workflow, verify legitimate Idle/Thinking playback without keyboard input and compact populated presentation. Do not infer animation from the offline test.
5. Read the fresh JSONL path for `cache_ready`, `redraw_requested`, `kitty_placement_refresh`, placement, selected frame and mode if behavior differs. Existing accessibility/SSH/multiplexer gates remain respected.

The user has physically observed graphics from the preceding release. **CW-04.2 physical startup/placement acceptance remains pending a new user-observed test.** No physical PASS is claimed here. Stop after reporting.
