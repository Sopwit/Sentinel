# TUI V3.4 evidence

Production release + disposable typed IPC/PTY fixtures. No real daemon, private conversations or physical terminal-emulator claim. Diagnostics are bounded by existing code. Terminal profiles explicitly supply responses; Apple-like is not a physical Terminal.app capture.

From the repository root:

```sh
python3 docs/reviews/tui-v34/compatibility.py
python3 docs/reviews/tui-v34/interaction.py
python3 docs/reviews/tui-v34/cw045_regression.py
python3 docs/reviews/tui-v34/poll_control.py
python3 docs/reviews/tui-v34/performance.py
```

Build `cargo build --release --workspace --examples` in `cli` for the isolated control; build `cargo build --release --workspace` for the production binary. The control sends an unknown cell reply then supplies a recovery key at 600 ms only for a blocked upstream poll. Its 50 ms timeout returns on time with the adapter; the production app does not receive synthetic keys from its implementation.

Run performance after other tests/builds finish. It imports the unchanged V3.3-C methodology and writes only this phase's artifacts. Comparison is against recorded C data, not a newly rerun C binary. CPU uses Mach conversion and wait4 cross-checks.

`late-isolation-results.json` and failure snapshots document exploratory pre-adapter cases. Final `compatibility-results.json`, `interaction-results.json`, `mascot-results.json`, `poll-control-results.json` and `performance-results.json` supersede exploratory runs. No native Linux/Windows target was installed or used. Physical commands and limitations are in the development report.
