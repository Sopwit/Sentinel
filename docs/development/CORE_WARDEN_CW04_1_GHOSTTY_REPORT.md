# Core Warden CW-04.1 — Production release display diagnosis

## Proven causes

1. **The invoked release executable was stale.** Before this task, `cli/target/release/sentinel` was dated 2026-10-11 00:43:59 local time, while the integrated debug executable was dated 01:11:07. The old release's strings contained the old settings inspection but neither `SENTINEL_TUI_MASCOT` nor `/settings mascot`. Its SHA-256 was `8c894c995fa29186513375222770cff72828bd0aa848670766f29a5d4680f986`. Consequently the exact release command used by the user could not execute the new mascot path. CW-04 had built the debug workspace, not release. This is a delivery defect, rather than evidence that Ghostty rejected Kitty.
2. **Bracketed-pasted slash input was intentionally marked literal Chat by the existing editor.** `App::send` bypassed local command dispatch whenever `editor.literal` was true. That general behavior incorrectly included the newly introduced `/settings mascot …` preference. A regression test now sends the same settings through real Enter handling in typed and pasted forms, including invalid modes, and asserts zero outgoing IPC requests.

The physical Ghostty shell's environment/capability reply was not captured in the user's earlier run. It cannot be reconstructed from the agent shell. No claim is made that its response timed out, that assets were missing, or that welcome text concealed the image. Those paths are now observable; no unsupported protocol was forced and no terminal safety gate was removed.

## Corrected source and delivery

- `cli/crates/sentinel-tui/src/lib.rs`: route `/settings mascot` locally before the literal-Chat branch; invalid mascot commands remain local errors. Add post-draw diagnostic observation and document the narrow pasted-command exception in help. Other pasted slash text and explicit `//` literal input preserve existing behavior.
- `cli/crates/sentinel-tui/src/mascot.rs`: opt-in startup/presentation diagnostics with asset root/result, mode, environment gate flags, negotiated protocol/cell size, state, placement, selected resolution, loaded-frame count, decoded bytes, loader status and render invocation. Transition records are deduplicated; at most 64 records / 64 KiB per launch. The file must be new; create-new prevents overwriting existing files, Unix mode is 0600, and an unavailable destination disables logging. Prompts, model output, arbitrary keyboard bytes and IPC payloads are not recorded.
- `cli/crates/sentinel-mascot/src/capability.rs`: explain the observed parsed Kitty/Sixel acknowledgement, cell response, response byte count, 300ms budget, measured elapsed time and bounded exit decision. Protocol selection and timeout behavior remain unchanged.
- `cli/crates/sentinel-mascot/README.md`: diagnostic usage and local pasted mascot preference behavior.
- Actual optimized executable: `<LOCAL_HOME>/Desktop/Projects/Sentinel/cli/target/release/sentinel`.
- Current release SHA-256: `04ac8203350fe9c585fdda151decb753d5bacb750ab76f6a2801aef223c2241c` (also saved in `docs/reviews/core-warden-cw041/release-identity.json`).

All existing work was preserved. C++ daemon, AgentRuntime, Blender models, animation frames and FA-3 icon were not modified in CW-04.1. No commit, push, merge or system-wide binary installation occurred. Personal-Brain `brain` was unavailable on PATH; the supplied workspace and repository instructions were used.

## Assets and capability results

The normal development root resolves from the compile-time TUI crate directory to `<LOCAL_HOME>/Desktop/Projects/Sentinel/assets/core-warden`. Explicit `SENTINEL_CORE_WARDEN_ASSETS` takes priority; an existing executable-relative `../share/sentinel/core-warden` directory is the installed candidate. No recursive search or asset copies were added.

The contract/manifest and all 132 required PNG paths validate. The Rust test `all_132_pngs_decode_as_expected_rgba` decodes every source PNG at 96/128/192px. Production retains only the chosen clip/resolution. Confirmed 120×40 empty presentation: `[x=52,y=13,w=16,h=8]`, source 128px, 16 Idle frames, 1,048,576 retained source RGBA bytes. Populated presentation: reserved header rectangle `[94,0,6,3]`, 96px source, 16 frames, 589,824 source RGBA bytes. These exclude encoded payloads and terminal texture memory.

Production PTY success fixture sends actual parser input `ESC _Gi=31;OK ESC \`, `ESC [6;16;8t`, DA and status responses. Diagnostics report `kitty_ack=true`, `cell=Some((8,16))`, protocol `kitty`; the byte count includes all supplied responses. The upstream parser accepts the exact matching ACK, not TERM alone. Failure fixture supplies no response: ~300ms, zero response bytes, no valid ACK/cell, protocol `text`, zero PNG cache. NO_COLOR, SSH and tmux suppress negotiation and graphics. A missing explicit asset directory reports its filesystem error and retains text fallback despite confirmed Kitty.

`render_invoked` means Player's render method was called in a valid slot; it can render fallback text while frames are loading or missing. `frames_loaded > 0` plus Kitty transmissions establishes that the image path executed in the fixture. It does not prove physical visual output.

## Verification

- `cargo test --manifest-path cli/Cargo.toml --workspace --offline`: 98 passed (CLI 14, IPC 11, mascot 14, TUI 59). Test socket binding requires sandbox escalation.
- `cargo fmt --manifest-path cli/Cargo.toml --all --check`: passed.
- `cargo clippy --manifest-path cli/Cargo.toml --workspace --all-targets --offline -- -D warnings`: passed.
- `cargo build --manifest-path cli/Cargo.toml --release --workspace --offline`: passed.
- Production release PTY matrix: Animated, Static, Off, Auto; populated layout; capability timeout; missing assets; NO_COLOR; SSH; tmux; and combined mode/overlay/resize/keyboard scenario. All 11 cases exit cleanly and restore alternate screen and terminal attributes. Animated/Auto cache 16 frames, Static one, Off/unsupported/missing zero. The scenario exercises bracket-pasted settings for all four modes, command palette open/close, 60×20 suppression, 120×40 restoration and Unicode draft echo. No `chat.send` or `agent.start` is observed in any case. Owned Kitty image deletion commands occur on mode changes, hiding/resizing and exit; no global delete-all operation is introduced.
- Existing production TestBackend tests cover empty/populated layouts, welcome/composer text, connection/readiness reservation, Unicode draft, search hiding and frame-cache release at 80×24, 100×30, 120×40 and 160×48. Existing transcript/keyboard/focus tests pass.

Evidence and reproduction script: `docs/reviews/core-warden-cw041/production_pty.py`, `production-pty-results.json`, per-case `*-diagnostics.jsonl`, `workspace-tests.log`, `clippy.log`, `release-build.log` and `release-identity.json`. These use deterministic terminal and IPC fixtures; they do not access a live model or benchmark AgentRuntime.

## Physical Ghostty acceptance

Physical Ghostty PASS is **not claimed**. Earlier computer-use access to Ghostty was denied; it was not bypassed. Run this in the real Ghostty window, with enough room (at least 80×24; 120×40 recommended):

```sh
cd <LOCAL_HOME>/Desktop/Projects/Sentinel
SENTINEL_TUI_MASCOT=animated \
SENTINEL_TUI_MASCOT_DIAGNOSTICS="/tmp/sentinel-cw041-$(date +%s).jsonl" \
./cli/target/release/sentinel tui
```

A fresh diagnostic filename is needed each launch. Existing NO_COLOR, ASCII, SSH/tmux policies remain respected; the diagnostic startup record reports those flags rather than silently overriding them. Confirm moving Core Warden above welcome, compact header presentation in a populated conversation when wide enough, local `/settings mascot static|off|auto|animated`, overlay priority, resize behavior and clean exit. If the image is absent, the JSONL explains asset validation, capability decision, placement suppression and cache/render status. The physical terminal's actual result is the remaining acceptance step. Stop after reporting; no further application surfaces are integrated.
