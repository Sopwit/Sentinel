# Agent V3.2 evidence — 2026-10-10

All live requests use isolated portable daemon profiles and disposable synthetic workspaces, LM Studio at `127.0.0.1:1234`, explicitly selected `nvidia/nemotron-3-nano-4b`. JSON records contain public IPC results/events, not private model reasoning. Test scripts are acceptance harnesses, not a new runtime/backend.

- `context-ui.json`, `context-*.ansi`: real `@` selection, authorized read-file, tool continuation and accepted grounded final. `context-final.ansi.png`/`.txt` are bounded VT replays.
- `natural-tasks.json`, `level*.diff`: unmodified baseline natural levels 2–5. Empty diffs mean no change, not successful fixes. Observer timeouts are not terminal success.
- `post-fix-tasks.json`, `post-fix-level*.diff`: same natural levels 2–3 with updated daemon. Do not infer causal performance gains from single stochastic runs.
- `running-readiness.json`: current active binding versus catalog readiness, captured while running.
- `controlled-recovery.json`, `controlled-*.diff`: live denied run-command approval (explicit failed terminal, no execution); running-tool cancellation probe blocked by pre-tool provider context-size failure.
- `recovery.json`: restart/persisted session, deliberately closed provider endpoint, explicit failure, restored catalog/binding. No successful post-restore inference is claimed.
- `agent-details-80x24.ansi.png` and `.txt`: retained before-fix rendered snapshot showing missing active task. The original before raw ANSI was overwritten by the capture harness; it is not presented as retained evidence.
- `post-fix-agent-*.ansi`: actual release TUI active-session reconnect at 80×24 and 120×40 after correction; current task restored. Rendered `.png`/`.txt` are bounded replays, not native terminal screenshots.
- `native-before.log`: three deterministic native repair regressions fail before implementation. These provider responses are scripted and are not live autonomy evidence.
- Build/test logs: exact verification commands and outcomes are summarized in the matrix. `ctest-sandbox-limited.log` records sandbox restrictions, not an accepted final suite run.

## Reproduction

From the repository root after building `build/tests/apps/sentinel-daemon/sentinel-daemon` and `cli/target/release/sentinel`, copy the harnesses to `/tmp` (their imports resolve there). Run the Python scripts with permission for localhost and Unix sockets. `sentinel-v32-tasks.py` used the baseline debug daemon; to repeat a baseline supply `SENTINEL_V32_DAEMON` pointing at a baseline build. The default module now uses the tests daemon. The controlled runner uses a 120-second observer bound and denies every command unless its exact resource scope is authorized. The post-fix runner uses a fresh synthetic project and a 300-second bound per task. Approval responses are restricted to the named synthetic target file and explicit command resources. No private workspace is supplied.

The context/recovery/observer scripts contain recorded disposable root/session references; regenerate those from a fresh context run instead of assuming temporary roots persist. Review endpoint/model availability before running. No model/server installation or configuration is part of these scripts. The recovery script changes only its own disposable daemon profile.

Physical Ghostty, macOS Terminal, iTerm2, Linux and tmux/SSH acceptance, including physical Shift+Enter: **NOT VALIDATED**. Native malformed-call repair: deterministic only; no live protocol-fault acceptance. Full coding autonomy: not accepted.
