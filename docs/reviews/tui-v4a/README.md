# V4-A release evaluation evidence

Actual arm64 production executable, disposable IPC fixtures and PTYs. No real daemon/inference launched. Physical terminal acceptance is separate. The project report lists blockers explicitly; forced SIGTERM is a negative result, not PASS.

Reproduce from the repository root after the locked Rust release build:

```sh
./tools/terminal/stage-release.sh /absolute/new/output
python3 docs/reviews/tui-v4a/startup.py
python3 docs/reviews/tui-v4a/compatibility.py
python3 docs/reviews/tui-v4a/interaction.py
python3 docs/reviews/tui-v4a/cw045_regression.py
python3 docs/reviews/tui-v4a/relocation.py
python3 docs/reviews/tui-v4a/recovery.py
python3 docs/reviews/tui-v4a/performance.py
```

Relocation uses the named local `rc-stage` produced during evaluation, moves a copy to a disposable directory, and checks installed, relocated, bare binary, missing override and checkout launch outside repository CWD. The final executable hash must match stage-inventory.json. Local staging directories and binaries are Git-ignored; nothing is globally installed.

Run performance after builds and other PTY runs finish. Methodology comes from the V3.3-C harness; comparison with V3.4 is historical, not a fresh paired benchmark. `startup-error-before.json` documents the prior raw-mode cleanup defect. `forced-exit-results.json` records SIGTERM leaving the terminal unrestored; its fixture restores its own PTY afterward. The startup failure test masks only macOS PENDIN, a transient kernel retype flag; all other termios settings must match.

CI/package policy is audited from local files, not a remote CI run. Windows named-pipe backend remains unavailable. Formal distribution-license and signing gates remain incomplete.
