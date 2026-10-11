# SENTINEL — CORE WARDEN CW-04 INTEGRATION REPORT

## Delivery status and concurrency boundary

CW-04 is now connected to the production `sentinel tui` render loop, filtered daemon events and existing `/settings` registry. The mascot crate is a workspace member and TUI dependency. Concurrent TUI work was preserved; `editor.rs` was not changed by this integration. No AgentRuntime, daemon, Desktop, icons, Blender or animation frames were changed by this integration. No commit, push or merge was performed.
The existing repository was inspected: the production Rust TUI uses Ratatui 0.29 and Crossterm 0.28, an IPC worker, an elapsed event/render loop with a 50ms IPC polling ceiling, bounded transcript/activity state, centered welcome/composer grid, modal pickers/approval dialogs and process-local environment-based preferences. Existing events are filtered by session, run, server generation and sequence before projection. Those filtering and Agent-authority boundaries must remain in place.

Personal-Brain's `brain` executable was unavailable on PATH; the explicitly supplied repository and local architecture/runtime/security/build/test instructions were used. Existing uncommitted and concurrent changes were preserved. No AgentRuntime, ObservationIntentPolicy, daemon, Desktop/Quick Panel, icons, Blender master, approved animation manifest or source frame was modified by this work. No commit, push or merge was performed.

## Architecture

Shared workspace crate: `cli/crates/sentinel-mascot`.

- `Assets`: bounded canonical-root metadata/manifest validation and selected-frame decoding.
- `Projection`: presentation-only mapping of already authenticated and filtered real events. It has no IPC client, model, permissions, task dispatch or execution authority.
- `Player`: elapsed-time playback, a replaceable background load request, generation cancellation and one active clip cache. PNG decode, pixel resize and protocol encoding happen on the loader thread, not inside render callbacks or animation ticks.
- `Capabilities`: one bounded startup query using ratatui-image's public query/parser; actual Kitty/Sixel and cell-size responses determine support.
- `DisplayMode`: Off, Static, Animated and Auto, launch-configured through `SENTINEL_TUI_MASCOT`, with process-local changes through `/settings mascot off|static|animated|auto`.
- `placement`: centered empty-screen placement, optional compact header placement, narrow-screen suppression and unconditional overlay suppression. It never reserves a right sidebar or narrows the transcript column.

The crate uses the main workspace lockfile without upgrading production Ratatui or Crossterm. Dependencies are Ratatui 0.29.0, Crossterm 0.28.1, ratatui-image 8.0.2 and image 0.25.10 with PNG enabled. Ratatui-image also enables its own JPEG dependency; no new media pipeline or runtime 3D library is introduced. Toolchain measured: rustc/cargo 1.98.1; declared MSRV 1.94.

## Renderer and capability selection

[ratatui-image's renderer and picker](https://docs.rs/crate/ratatui-image/8.0.2/source/src/picker.rs) supply the maintained protocol abstraction. The implementation deliberately avoids the upstream stdio timeout helper: its audited 8.0.2 implementation starts a detached blocking stdin reader which can outlive a timeout. It also avoids upstream tmux environment inference and passthrough mutation. The CW-04 adapter uses the upstream query bytes/parser with a synchronous Unix poll/read loop bounded to **300ms and 4096 response bytes**, after raw alternate-screen entry and before another input reader starts. No reader thread survives negotiation.

- Confirmed Kitty plus valid cell size: Kitty, prioritized over Sixel. This is the intended Ghostty path; TERM/TERM_PROGRAM alone cannot enable it.
- Confirmed Sixel plus valid cell size: Sixel. Auto uses one static frame conservatively; explicitly Animated can use cached frames. A released Sixel cache requests a full terminal clear/buffer invalidation through `take_full_redraw()` before the next draw.
- Failed/partial negotiation, invalid font size, non-TTY, SSH, tmux/screen, ASCII or NO_COLOR: text. SSH/multiplexer policy suppresses probes, not just image rendering. No tmux options are changed.
- iTerm2 environment guesses are not accepted as capability proof. Non-Unix negotiation is currently text-only. The data/playback API remains portable; Windows physical image support is unverified and not enabled by assumption.
- Static and reduced-motion retain a single neutral frame when a confirmed renderer exists. Off performs no capability probe in the preview and draws no mascot. The caller keeps normal application status text available.

Startup negotiation happens before interactive draft handling and may consume bytes typed during its short startup window. It is not a recurring background probe. A future platform-specific query adapter should preserve startup user bytes if that behavior is required. Do not start the normal keyboard reader concurrently with negotiation.

## Asset loading and cache lifetime

The canonical source remains `assets/core-warden`; `state-contract.json` is the only new asset-side contract. No application-specific PNG copies or Blender modifications were made. All 132 required PNG paths were checked and decoded in validation tests; production loading reads metadata for the manifest, then decodes only the chosen state/resolution.

Normal source size is 96px; roomy empty screens (at least 120×40) use 128px. Compact populated placement decodes 96px source frames and fits them into an approximately 48px header image; it does not expand the conversation column. 192px playback is deliberately disabled. A normal clip cache contains 16 Idle, 16 Thinking or 12 Tool Running frames. Static/missing-state/disconnected presentations retain one neutral 96px frame.

The loader stores source RGBA and pre-encoded renderer data in the selected clip's StatefulProtocols. Generation checks stop stale requests between frames; no queue of animation ticks exists. Hidden, disabled, overlay-covered and narrow presentations cancel loading and release the cache. A state/size change replaces it. A transient in-flight decode or ready result can overlap cancellation, so the retained source-byte counter is not a complete allocator/RSS ceiling. Peak process RSS is measured separately below. Protocol payloads, resized copies and terminal-owned caches also consume memory.

Kitty images transmit once per cached frame, then use upstream Unicode-placeholder placement. Repeated loops reuse those terminal images. On release/exit, targeted Kitty deletion commands free only this player's image IDs; the implementation does not send a global delete-all command. This small cleanup adapter is needed because the selected upstream version exposes no public deletion lifecycle API. Placement/render encoding remains upstream. Sixel has no equivalent retained image ID; its release requests terminal clear/invalidation. Callers must consume that request. Raw/alternate-screen restoration belongs to the existing terminal guard, after the player drops.

## Observable state mapping

The current contract covers Idle, Listening, Thinking, Processing, Tool Running, Approval, Speaking, Success, Warning and Error. Only the first-phase Idle/Thinking/Tool Running assets animate. All other states explicitly use the existing neutral frame plus a truthful label, or text if images are unavailable. See `CORE_WARDEN_SHARED_ASSET_CONTRACT.md`.

- Connected/ready → Idle.
- Actual model submission pending / authoritative run.started → Thinking.
- Already-filtered tool.running during an active presentation → Tool Running.
- tool.result → Thinking, never completion.
- Approval → static/label mapping, with the graphic hidden under the modal.
- Disconnection → static unavailable / text.
- Failed/cancelled → Error/Warning static mapping.
- run.completed → Idle. No accepted Agent Success is inferred by this library.

The fixture's numbered keys feed explicitly labeled synthetic presentation events; it never connects to a daemon or runs an Agent. Production must reuse the existing daemon events and filtering, not fixture timers or model prose.

## Placement and visual evidence

`examples/playback.rs` is a real interactive Ratatui/Crossterm playback executable, independent of a working Agent. It demonstrates all three clips, populated and empty presentation, a focus-priority overlay, resize suppression, local keyboard draft input, scroll keys, display modes and reduced motion. It is clearly labeled **isolated playback fixture**, not the production app.

`examples/layout_evidence.rs` records actual TestBackend cell buffers and placement decisions for 80×24, 100×30, 120×40, 160×48 and narrow 60×20 screens. `tools/render_evidence.py` combines those buffers with the corresponding canonical source frame for review. These PNGs are explicitly labeled reconstructions, not terminal screenshots. They demonstrate the library/fixture layout; the production integration additionally has direct TestBackend tests for welcome/composer preservation, search suppression, header reservations, narrow sizing and validated settings.

Representative files under `docs/reviews/core-warden-cw04/layouts/`:

- `idle-80x24.png`, `idle-100x30.png`, `idle-120x40.png`, `idle-160x48.png`.
- `thinking-120x40.png`, `tool_running-120x40.png`.
- `populated-120x40.png`: compact header mascot; full-width transcript.
- `overlay-120x40.png`: graphic suppressed, dialog visible.
- `fallback-80x24.png`, `idle-60x20.png`: readable text fallback.

No permanent sidebar or production UI redesign is included.

## Production integration

`sentinel-tui/src/mascot.rs` resolves assets and calculates placement from the real composer/content layout. Startup negotiates once after terminal initialization, before input reads. Off skips negotiation; if launched Off, switching modes retains text until restart. Other mode changes use retained capabilities. Reduced motion, NO_COLOR, ASCII, SSH and multiplexer policies remain in effect.

Asset discovery uses `SENTINEL_CORE_WARDEN_ASSETS` when supplied; otherwise an existing `../share/sentinel/core-warden` directory relative to the executable, then the development source directory. Installed distributions must package the canonical asset directory at that share path or configure the explicit root. No packaging installer was added in this phase. Missing/invalid assets fall back safely.

The App feeds accepted snapshots and session/run/generation/sequence-filtered events into the presentation projection. Only actual submissions imply Thinking. Tool Running requires accepted `tool.running`; neither output deltas nor generic busy metadata infer tool activity. Normal run labels and authority remain with the existing TUI.

Before each dirty draw, Player updates nonblockingly and requests at most its elapsed frame deadline under the existing 50ms IPC polling ceiling. Sixel cache release clears the terminal buffer. Player drops before terminal restoration. Empty screens place the image above welcome text; populated wide screens reserve a compact header gap beside readiness. Transcript width and composer remain unchanged. Pickers, approval, search, slash discovery and inspection hide/release artwork. Narrow terminals retain text. Tool running remains visible as footer text when artwork cannot be placed.

Process-local preference: `/settings mascot off|static|animated|auto`. Launch default: `SENTINEL_TUI_MASCOT`. `/settings` lists effective mode and its source without creating another settings file.

## Production validation (2026-10-11)

- Workspace tests: 96 passed (CLI 14, IPC 11, mascot 14, TUI 57). Unix-socket tests required sandbox escalation; the restricted attempt failed to bind test sockets. The final TUI-only run also passed all 57 after adding populated-render checks.
- Workspace formatting, Clippy `--all-targets -- -D warnings`, and workspace debug build passed.
- Direct production render tests cover 80×24, 100×30, 120×40 and 160×48. They wait for the real background image loader, retain Unicode draft and welcome/transcript text, reserve readiness/connection header cells, and release frame cache on search. Settings validation, narrow suppression and foreign/stale event rejection are covered.
- Actual built `cli/target/debug/sentinel --socket <absent socket> tui` ran in a 100×30 PTY with NO_COLOR. `/settings mascot static`, `/settings`, inspection dismissal and idle exit succeeded; terminal attributes and alternate screen restored, with zero Kitty transmissions. This verifies text fallback without a daemon; it does not certify graphics in Ghostty.
- Logs, eight production TestBackend cell dumps and PTY result: `docs/reviews/core-warden-cw04/production/`. Cell dumps contain Kitty placeholders, so they are diagnostic text rather than physical screenshots.
- Existing editor/core/daemon changes were preserved. No new application packaging installer or release deployment was performed. The executable built here is `cli/target/debug/sentinel`.

## Reproducible commands

```sh
cargo test --manifest-path cli/crates/sentinel-mascot/Cargo.toml
cargo fmt --manifest-path cli/crates/sentinel-mascot/Cargo.toml --check
cargo clippy --manifest-path cli/crates/sentinel-mascot/Cargo.toml --all-targets -- -D warnings
cargo build --release --examples --manifest-path cli/crates/sentinel-mascot/Cargo.toml
cargo run --release --manifest-path cli/crates/sentinel-mascot/Cargo.toml --example playback -- --fixture --seconds 24
python3 cli/crates/sentinel-mascot/tools/measure_playback.py
python3 cli/crates/sentinel-mascot/tools/measure_playback.py --steady
cargo run --release --manifest-path cli/crates/sentinel-mascot/Cargo.toml --example layout_evidence
python3 cli/crates/sentinel-mascot/tools/render_evidence.py
```

The last helper requires Pillow and the macOS Menlo font; it is development QA tooling, not a Python application backend. PTY tooling is Unix-only. Run the interactive example in an actual terminal for visual motion review. `--assets` accepts an explicit canonical root; `--metrics` records fixture metrics. No Blender installation is required for playback.

## Actual measurements

Evidence: `docs/reviews/core-warden-cw04/pty-matrix.json`, `steady-idle.json` and individual JSON records. Release binaries ran in real Unix PTYs, with **simulated Kitty/cell-size capability responses**, not a physical Ghostty window. The PTY reader drained output promptly. Child CPU/RSS comes from `wait4`; CPU is percent of one core over the whole child run, including startup and one cache load. Output bytes include protocol payloads, placeholders, ordinary TUI text, cleanup and restoration. Terminal emulator decode/render/GPU costs are excluded.

Three-second Idle samples (one injected UTF-8 draft character adds one redraw):

| Canvas | Protocol evidence | Draws | Child CPU | Peak process RSS | Retained source RGBA | Total backend bytes | Input→visible echo |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 80×24 | Kitty / simulated reply | 26 | 1.57% | 5.44 MiB | 0.5625 MiB | 841,410 | 0.91ms |
| 100×30 | Kitty / simulated reply | 26 | 1.99% | 5.72 MiB | 0.5625 MiB | 855,600 | 1.39ms |
| 120×40 | Kitty / simulated reply | 26 | 2.67% | 6.78 MiB | 1.0000 MiB | 1,500,137 | 1.33ms |
| 160×48 | Kitty / simulated reply | 26 | 2.89% | 6.95 MiB | 1.0000 MiB | 1,535,187 | 1.93ms |

26 total draws comprise initialization/load, 24 intended animation samples and the keyboard redraw; it is not a 26/3 animation rate. Exactly 16 Kitty source images were transmitted in each Idle sample, then reused; repeated loops did not retransmit all pixel payloads. Raw RGBA/base64 protocol uploads cost considerably more than compressed PNG disk size. The initial 96px loop accounts for roughly 0.8 MB of total output; 128px roughly 1.5 MB. This is a real cost, especially on remote links, which is why SSH defaults to text.

Twelve-second 80×24 Idle samples, including startup/load and a single injected character:

| Mode | Protocol | Draws | Child CPU | Peak process RSS | Total backend bytes |
| --- | --- | ---: | ---: | ---: | ---: |
| Off | text | 2 | 0.171% | 2.77 MiB | 1,273 |
| Animated | kitty | 98 | 0.759% | 5.72 MiB | 901,256 |

Animated cost was about 0.59 percentage points above the isolated Off fixture in this sample. This does not establish the production TUI's idle regression or Ghostty CPU use. 98 draws in the animated sample comprise 96 intended frame updates plus initialization/input work. No allocation-count profiler was used; upstream placeholder/buffer rendering still allocates, while PNG decoding/resizing/encoding is absent from ordinary animation ticks.

Fallback measurements:

| Scenario | Selected protocol | Draws / 3s | Source RGBA retained | Capability query |
| --- | --- | ---: | ---: | --- |
| off-80x24 | text | 2 | 0 bytes | 0.00ms |
| static-80x24 | kitty | 3 | 36,864 bytes | 0.11ms |
| probe-timeout | text | 2 | 0 bytes | 300.08ms |
| no_color | text | 2 | 0 bytes | 0.00ms |
| sentinel_tui_ascii | text | 2 | 0 bytes | 0.00ms |
| ssh_tty | text | 2 | 0 bytes | 0.00ms |
| tmux | text | 2 | 0 bytes | 0.00ms |
| sentinel_tui_reduced_motion | kitty | 3 | 36,864 bytes | 0.05ms |

The five-second overlay/state/resize case used Kitty with simulated responses: **44 draws**, **3.49% child CPU**, **10.91 MiB peak process RSS**, **2,882,970 backend bytes**, and **1.74ms input→visible echo**. Its state/cache logs show Idle's 16-frame cache, Thinking's 16-frame cache and Tool Running's 12-frame cache, release during the overlay, a compact populated header, release on 60×20 resize and reload on restoration. Those transitions legitimately create extra draws/cache uploads beyond a pure 8 FPS loop.

Every matrix case exited successfully, restored the PTY's original termios flags, emitted alternate-screen restoration and freed its owned Kitty IDs when present. Unsupported cases retained zero decoded PNG bytes and transmitted no Kitty image payloads. These are protocol/PTY lifecycle checks, **not visual proof of no ghost images** on a physical terminal.

## Tests and acceptance

- **14 new crate tests passed**: complete manifest/file validation, all 132 PNGs decode as correctly sized RGBA, frame timing and loop boundaries, truthful state mapping, missing-asset/text fallback, capability-result rejection, four target layouts plus narrow/overlay suppression, cache replacement/lifetime, resize, disabled/static/reduced-motion/disconnected behavior, Sixel Auto static/full-clear lifecycle, bounded loader shutdown and targeted Kitty cleanup.
- New crate formatting check, Clippy `--all-targets -- -D warnings`, and release example builds passed.
- Existing Rust workspace tests passed: **14 CLI + 11 IPC + 54 TUI = 79 tests**. The first restricted-shell attempt failed to bind fixture Unix sockets; rerunning with the required local-socket access passed. Logs are retained as `existing-rust-workspace-tests.log`. No unrelated live Agent benchmark was run.
- Current scope is a separate Rust/presentation asset foundation; no C++ shared/runtime code changed, so the broad CMake suite was not run.
- Physical Ghostty was installed, but the computer-use tool **denied access to `com.mitchellh.ghostty` for safety reasons**. No alternate launch/control method was used to bypass that denial. Therefore no physical Ghostty visual/performance acceptance is claimed.

## Known limitations and readiness

1. Production hooks are implemented and covered by Rust tests. Physical terminal visual acceptance remains outstanding; existing PTY performance figures below describe the standalone fixture, not the production TUI.
2. Ghostty and Sixel terminals require physical visual acceptance for flicker, transparent edges, resizing and ghost-image cleanup. PTY recordings and TestBackend reconstructions cannot establish those properties. Sixel may flatten transparency/background color; Auto conservatively stays static there.
3. Windows negotiation and iTerm2-specific detection are not implemented. They safely retain text unless a supported Unix response-based protocol is confirmed.
4. PNG/source bytes, encoded cache bytes, peak process RSS and terminal-owned image memory are distinct. RSS measurements cover the fixture process only; no terminal GPU/texture cache measurement is available. No production network/model latency or terminal FPS claim is made.
5. Short startup queries can consume startup keystrokes. Normal keyboard/IPC event processing must begin after detection, and detection must never recur on animation ticks.
6. Compact header artwork is intentionally small (about 48px fitted from a 96px source). The state text remains authoritative/readable; production review should confirm this size is useful.

Production wiring is complete; physical visual/performance review remains. CW-03B can add assets through an explicit contract/validator update; it has not been rendered. Desktop, Quick Panel, Onboarding and Voice integration guidance is provided, but those clients are not implemented. FA-3 remains the application/tray icon. Stop here for review.

## CW-04.1 follow-up

The release binary was rebuilt after discovering the prior release predated integration. Pasted mascot settings now remain local, and bounded opt-in production diagnostics are available. See [CW-04.1 diagnosis and release verification](CORE_WARDEN_CW04_1_GHOSTTY_REPORT.md). Physical Ghostty acceptance remains unclaimed.


## CW-04.5 verified terminal input fix

The isolated physical comparison identified a delayed `CSI 0n` reply to the trailing `CSI 5n` capability query as the source of the keyboard-dependent input stall. The shared capability layer now strips only that unnecessary final request, preserving real graphics acknowledgment, cell-size validation, response limits and safety fallbacks. No rendering timer or layout change was introduced.

The current release passed 105 Rust tests, workspace formatting and Clippy, release build, 15 fragmented-response production PTY cases and 3 no-input/key/resize comparisons. The no-input case emitted direct placement at approximately 241 ms. Physical production Ghostty acceptance is pending user confirmation. See [CW-04.5 changes, evidence and acceptance command](../reviews/core-warden-cw045/REPORT.md).
