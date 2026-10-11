# Sentinel mascot playback foundation

Shared CW-04 playback crate, integrated into the production Rust TUI render loop, filtered daemon events and `/settings mascot off|static|animated|auto`. Agent authority remains in the daemon; source Blender and animation assets are unchanged.

```sh
cargo test --manifest-path cli/crates/sentinel-mascot/Cargo.toml
cargo fmt --manifest-path cli/crates/sentinel-mascot/Cargo.toml --check
cargo clippy --manifest-path cli/crates/sentinel-mascot/Cargo.toml --all-targets -- -D warnings
cargo build --release --examples --manifest-path cli/crates/sentinel-mascot/Cargo.toml
cargo run --release --manifest-path cli/crates/sentinel-mascot/Cargo.toml --example playback -- --fixture --seconds 24
```

Run the last command in a real terminal. `SENTINEL_TUI_MASCOT=off|static|animated|auto` sets the launch preference. `SENTINEL_TUI_REDUCED_MOTION=1` uses a static frame. The fixture is clearly labeled and never connects to an Agent or daemon. Controls: 1 Idle, 2 Thinking, 3 Tool Running, 4 disconnected, m overlay, p populated conversation, s Static, a Animated, o Off, u Auto, r reduced motion, Esc dismiss overlay, q exit. Other characters enter a local draft. No real session data is involved.

`--assets /explicit/canonical/root` overrides the development asset root. For installation, supply the installed shared asset directory. `--metrics /path.json` records fixture measurements. Input to this fixture is deterministic; tool labels do not claim real execution.

`tools/measure_playback.py` runs a Unix PTY matrix with simulated capability replies. It records child CPU, peak process RSS, actual backend output bytes, source RGBA cache bytes, input echo latency and terminal restoration. These results are **not physical Ghostty acceptance**. `examples/layout_evidence.rs` and `tools/render_evidence.py` produce explicitly labeled TestBackend reconstructions; the rendering helper requires Pillow and the current macOS Menlo font.

Production lifecycle and integration hooks are documented in `docs/development/CORE_WARDEN_CW04_TUI_INTEGRATION.md`; shared asset ownership and future Qt/QML guidance are in `docs/development/CORE_WARDEN_SHARED_ASSET_CONTRACT.md`.

Production diagnostics: set `SENTINEL_TUI_MASCOT_DIAGNOSTICS` to a new writable file path before launching `cli/target/release/sentinel tui`. Logging is opt-in, capped at 64 transition records / 64 KiB, and records capability decisions, asset resolution, placement and cache/render status. It excludes prompts and model output. Existing files are never overwritten; unavailable destinations disable logging. Use a fresh path per launch. Capability negotiation remains bounded to 300ms and never forces Kitty from TERM hints.

Single-line slash commands, including bracket-pasted `/settings mascot off|static|animated|auto`, execute locally; invalid names show local feedback. Multiline slash prose remains literal chat text.

Production empty views reserve display space independently of the 96/128px source. PNG decoding/scaling and encoding stay in the bounded background cache. Confirmed Kitty uses owned direct placements after the terminal draw/flush boundary; static/hidden/off states do not schedule animation wakes. No one-shot placement timer is required. CW-04.5 removes the trailing CSI 5n; V3.4 scopes Unix nonblocking input to polling to prevent late cell replies from blocking redraws.

Production asset lookup is runtime-only: explicit `SENTINEL_CORE_WARDEN_ASSETS`, executable-relative `../share/sentinel/core-warden`, then the bounded standard checkout layout. No compiled developer-home lookup. See `docs/interfaces/CLI.md` for local RC staging; missing assets select text fallback.
