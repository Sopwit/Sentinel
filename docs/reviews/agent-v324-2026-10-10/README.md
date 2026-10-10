# V3.2.4 evidence — 2026-10-10

This directory separates deterministic regression from live reasoning acceptance. Final verdict: IMPROVED — FURTHER FIXES REQUIRED.

- `summary.json`: actual run IDs, terminal events, classification metadata, elapsed times, tool counts and fixture-preservation checks for ten real TUI journeys.
- `qwen/live.json`, `nemotron/live.json`: full typed IPC event/history records and synthetic before/after fixture snapshots. Every model chooses its own actions; there are no scripted successful live tool decisions. All fixtures are disposable, not user-project data.
- `*/diagnostics.jsonl`: allowlisted production metadata, no prompt bodies, private reasoning or credentials. Timeout records have no observed response usage/finish reason.
- `*-loaded.json`: independently reported actual 8192 context/four-slot instance configurations. `models-before.json` and `models-after.json` establish empty loaded state before/after task-owned instances.
- `binary-hashes.json`, `*/daemon.sha256`: identical frozen daemon binary in both isolated profiles and the recorded release build. SHA256: `33653fb1d034ff10959af30781570b526dc73163b47b522e3758051f0e6b24b8`.
- `*-final.ansi`: actual 120×40 PTY captures. Qwen A and Nemotron C PNG/TXT files are bounded VT replays, visually inspected. They are not physical terminal screenshots. The later TUI wrapper/ellipsis fixes have deterministic coverage only.
- `ctest-initial.txt`: initial 120/122 result; `ctest-final.txt`: corrected final 122/122 result. `focused-final.txt`: final focused six-executable pass. Fixtures were corrected to use the actual canonical source path and explicitly implement the bounded provider contract; production checks were not bypassed.
- `rust-complete.txt`, `clippy-complete.txt`, Rust debug/release build logs: final 75 Rust tests and toolchain gates. `rust-presentation-initial.txt` preserves the notice-width test failure before correcting its expectation.
- `format-final.txt`, `release-complete.txt`, `tests-complete.txt`: build/format evidence. Logs are named .txt so repository log-ignore rules do not hide the deliverable.
- `cleanup.json`: both task-owned daemons exited, no loaded task instance remains. Portable profiles were independent of user settings; no personal Keychain data or permission was changed.

## Reproduce

From the repository root, build the C++ release daemon and release Rust client, then run `sh docs/reviews/agent-v324-2026-10-10/reproduce/run.sh` only when isolated testing with the two already-installed local models is authorized. It refuses an initially loaded user model, uses task-owned 8192/four-slot instances sequentially, copies one daemon binary, creates isolated portable profiles/synthetic workspaces, invokes the real release TUI and restores loaded state. The harness reuses the V3.2.3 isolated-profile helper; no shell/build commands are authorized in the read journeys.

Deterministic verification: `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure --timeout 45`. Focused targets are local_inference, observation_policy, agent_loop, agent_runtime, llm_agent_runtime and upgrade. Rust gates use the cli workspace. Local HTTP/IPC fixtures need local socket access; sandbox EPERM is not a provider failure.

Source provenance: the checkout initially contained uncommitted V3.2.3 work. External commits appeared while this task was running, ending at observed 144ef090. This agent issued no commit/push/merge commands and preserved that state. Later changes concern corrected regression fixtures, limited TUI presentation and these reports. No daemon behavior change followed the live binary freeze.
