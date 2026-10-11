# V3.3-A deterministic UX evidence

`before/` and `after/`: identical `ux_evidence_matrix` TestBackend fixture at four sizes × four themes. `after-fallback/`: ASCII/NO_COLOR matrix. PNGs from `render_text.py` are text-snapshot illustrations, not native screenshots or color/style acceptance. Before renderer was the preserved pre-edit Rust source with only the identical evidence test injected temporarily; final implementation was restored immediately afterward.

`pty_check.py`: offline repository-local release CLI checks; no daemon, models or Agent execution. `pty-results.json` records executable path/hash, sizes, clean exits and replay content checks. `pty-*.ansi` are actual PTY output; `.png`/`.txt` replays use the existing bounded renderer at `../agent-v323-2026-10-10/reproduce/render_ansi.py`. Replay is not physical terminal observation. The fallback is opt-in through `NO_COLOR=1 SENTINEL_TUI_ASCII=1`.

Reproduce matrix: create output directory, then `SENTINEL_V33_EVIDENCE=<absolute-output-directory> cargo test --manifest-path cli/Cargo.toml -p sentinel-tui ux_evidence_matrix`. Run PTY checks from this checkout with `python3 docs/reviews/tui-v33a/pty_check.py`. Render PNGs using a Python environment with Pillow. Physical terminal acceptance remains NOT VALIDATED.
