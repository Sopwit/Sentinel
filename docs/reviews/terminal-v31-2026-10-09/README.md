# Sentinel TUI V3.1 evidence

Captured 2026-10-09–10. [Acceptance report](../../development/TUI_V3_1_LIVE_ACCEPTANCE.md) gives exact statuses and limitations. Data is synthetic; inference was real where specified. No native-host acceptance is claimed.

| Evidence | Meaning |
|---|---|
| `before-WxH.ansi` / `after-WxH.ansi` | Actual old/new release PTY output at all four requested sizes |
| `theme-*-WxH.ansi` | Actual four-theme PTY captures, NO_COLOR removed |
| `ui-waiting.ansi`, `ui-chat-two.ansi`, `ui-environment.json` | Real two-turn Chat, history retention, help/model/session/context selection, paste/newline, reconnect and exit |
| `live-contract.json` | Real Chat completion/persistence; initial bounded Agent observation timed out |
| `agent-read.json` | Real autonomous file read, model continuation, authoritative completion |
| `approvals.json`, `approval-*.ansi` | Actual TUI allow-once/deny/cancel with exact synthetic targets and filesystem checks |
| `context-ui-attempt1.json` | Tool read succeeded but final Agent grounding failed; not PASS |
| `context-ui.json`, `context-*.ansi` | Final picker rerun selected two references; inference blocked by actual provider unavailable |
| `continuity-recovery.json` | Restarted persisted history, provider failure/recovery and actual streaming cancellation; observer timeout caveat in report |
| `session-metadata.json`, `export-verification.json` | Real rename/archive/export, invalid model, renamed/deleted/long nested file listing |
| `pty-metrics.json` | Idle sample and exit/restoration checks |
| `rust-tests.log`, `clippy.log`, `release-build.log`, `ascii-no-color-tests.log`, `cpp-ipc-tests.log`, `render-performance.log` | Final verification logs |

`*.ansi.png` and `*.ansi.txt` are bounded VT replays of actual output; replay styling approximates a terminal and is not a screenshot of Ghostty/Terminal. `v31-*` TestBackend fixtures, where present, are synthetic deterministic render evidence and never prove live inference. Temporary harnesses generated these recordings; the production behavior is covered by repository Rust regression tests.

Viewed examples: [wide layout](after-160x48.ansi.png), [Porcelain](theme-porcelain-120x40.ansi.png), [approval scope](approval-allow.ansi.png), [pending history](ui-waiting.ansi.png). Supplied native screenshots remain the original visual references. Final verdict is PARTIAL, not competitor parity.
