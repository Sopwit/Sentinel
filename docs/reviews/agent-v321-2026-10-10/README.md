# V3.2.1 acceptance evidence — 2026-10-10

Real isolated daemon + actual installed local LM Studio model, macOS arm64. Source baseline 18bc2a9 plus uncommitted V3.2.1 fixes. No personal profile/data or running user daemon was a target. No commits/pushes/merges occurred.

- `loaded-context.json`: read-only local provider loaded-window metadata (8192 vs published 1048576).
- `live.json`: initial A–H natural-model benchmark, actual public IPC events, approvals, history, provider usage where returned. `*.diff`: before/after synthetic source evidence. Filter events by run_id; late previous-run events can be in a case's collection.
- `extended-live.json`: fresh direct-endpoint longer-bound B/C/D follow-up, without proxy token metrics.
- `cancellation-pty.json` / `owned-process-cleanup.json`: cmake launch failed, no PID; not process cleanup acceptance.
- `cancellation-pty-sleep.json` / `owned-process-cleanup-sleep.json`: confirmed owned /bin/sleep PID before actual release TUI Ctrl+C; no descendant afterward; authoritative cancelled state.
- `recovery-live.json`: separate streaming/unavailable probes; assess actual terminal/event data rather than harness intent.
- `process-inspection.json`: redacted authoritative history of the failed cmake probe, not private reasoning.
- `agent-c-*`: actual release TUI transcript of the pre-fix old-cancellation/new-session race. `session-race-before.log`: deterministic failing reproduction. Final CTest includes passing fixed regression.
- `*.ansi`: raw PTY output. `*.ansi.png` / `.txt`: deterministic VT replays, visually inspected where the report says so. These are not native-host screenshots.
- `cpp-ctest.log`: final 122/122 C++ suite. Build, Rust tests and Clippy logs are separate.

Harness scripts reproduce disposable fixtures and exact resource/digest approvals. They require the repository cwd, built daemon/TUI and explicitly selected installed model. They use temporary directories, private portable profiles and socket access; do not aim them at personal workspaces. `sentinel_v321_accept.py` is a support module. The harnesses import it from their own directory. Metadata proxy component sizes are re-encoded JSON and not additive wire-token accounting. It does not propagate upstream disconnect while blocked reading; direct probes establish separate cancellation evidence. Observer timeout is never a completed task.

Private model reasoning is not collected. Public synthetic outputs, tool names, events and numeric usage are recorded. No native Ghostty/macOS Terminal/iTerm2/Linux acceptance is implied.

`binary-manifest.json` distinguishes copied daemon snapshots. Initial live/old-session captures precede the cancellation/session fix. The extended benchmark uses the earlier race-fix build; final tests and sleep/recovery probes use the final build, whose reset additionally preserves identity while a run remains active. Context/evidence changes are present in all benchmark copies. This avoids claiming every live capture exercised the exact final binary.

`benchmark-summary.json` gives run-attributed counts and final observer outcomes. `extended-workspace-manifest.json` records all final fixture paths/sizes/hashes; no generated build tree was present. The direct D configured command was requested twice and individually approved; a later unlisted digest was denied. Neither source edit nor build/test success is established by that run.
