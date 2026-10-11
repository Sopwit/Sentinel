# V4-A.1 evidence

Production release + disposable PTY/IPC and simulated terminal responses; not physical terminal acceptance.

- `signals.py`: real SIGTERM/SIGHUP, startup/idle/playback/overlay/resize/repeat races, actual termios and owned image IDs.
- `compatibility.py`: existing 29 production terminal regressions redirected here.
- `cw045_regression.py`: existing 15 mascot regressions redirected here.
- `performance.py`: existing production measurement helper, four mode/transcript samples.
- Cargo logs: full tests, formatting, Clippy, debug/release builds.

Run wrappers from the repository root. Fixture sockets need permission in restricted environments. `signal-results.json` identifies the exact release SHA-256. `*-operations.json` retains bounded Kitty headers and cleanup counts without PNG payloads.
