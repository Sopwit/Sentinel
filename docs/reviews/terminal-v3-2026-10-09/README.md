# TUI V3 terminal and runtime evidence

Date: 2026-10-09. No personal daemon data, provider credentials or workspace contents were used. No model was downloaded and no tool task was executed.

## Actual terminal recordings

`before-*.ansi` uses the saved pre-V3 V2.3 release binary; `after-*.ansi` uses the final V3 release binary. These are actual disconnected PTY captures at 80×24, 100×30, 120×40 and 160×48 with TERM=xterm-256color and the same dark-theme environment. Both point to an unused temporary socket. Raw recordings contain ANSI terminal controls, not executable shell scripts.

Idle composition is intentionally preserved: compact header/model row, centered welcome, capped aligned composer and minimal footer. The V3 work adds contextual command UI rather than redesigning the idle screen. Replayed before/after wide-terminal images and decoded text were inspected: content grid and welcome alignment remain consistent. The approved image attachment was not available in this execution context, so exact pixel/reference comparison was impossible; the written composition requirements and existing approved V2.3 layout were used.

`live-*.ansi` records one actual 100×30 PTY process attached to a disposable real daemon:

1. Connected idle screen with actual Personal workspace and unselected model.
2. Typing `/` opens completion, without execution.
3. `/help` opens searchable registry help; selected metadata shows the active shortcut.
4. `/model` shows an empty real catalog.
5. Sending a harmless prompt is refused/preserved because no model is discovered.
6. Stop daemon: disconnected error, draft preserved.
7. Restart same temporary portable profile: Connected and “Connection restored; draft preserved,” same draft.
8. Explicitly clear draft and `/exit`: process exits 0.

These observations establish UI/IPC recovery, not successful inference. The separate direct daemon check observed `model-unavailable` for chat and `permission-denied` for files.

## Replayed images and fixtures

`*.ansi.png` and `*.ansi.txt` are bounded VT replays of actual captured bytes, rendered using Menlo and a simple terminal decoder. They are convenient visual evidence, **not native terminal screenshots**. Decoder styling/cell metrics may differ from a real emulator. `live-slash.ansi.png`, `live-help.ansi.png`, `live-recovered.ansi.png` and before/after wide views were inspected. Full recording bytes remain the authoritative captured evidence.

`fixture-plan-disabled-*.txt` are synthetic Ratatui TestBackend renders, explicitly not live runtime evidence. Tests also render slash/help states at all four required sizes plus 24×8. Existing V2.3 tests cover empty connected/disconnected, populated content, long model/workspace names, Agent running/approval and focused/unfocused composer. Run fixtures with `SENTINEL_V3_EVIDENCE` to regenerate.

## Real daemon contract check

`isolated-daemon.json` contains actual replies from a copied daemon running `--portable` under a unique profile with private temporary storage/socket. Create/attach/rename/list succeeded; restart changed server generation while preserving the session; resume and archive succeeded. Files were denied, catalog empty, chat unavailable. These are accepted session operations, not fabricated user/assistant messages. Default daemon-provided agent/task summaries are metadata, not evidence of executed work.

## Checks

`tests.log`: final workspace run, 14 CLI + 11 IPC + 41 TUI = 66 passing tests. `clippy.log`: workspace all-targets, warnings denied. `release.log`: optimized workspace build. `fallback.log`: ASCII/no-color TUI run. Formatting and `git diff --check` also pass. Socket integration checks required the approved sandbox escape; data remained disposable.

Ghostty, macOS Terminal, iTerm2, Linux emulator and tmux/SSH: **NOT VALIDATED**. A PTY is not a native host acceptance test. No full inference, coding Agent approval/tool/result, grounded file response or populated transcript export journey is marked PASS. See [implementation report](../../development/TUI_V3_IMPLEMENTATION_REPORT.md) and [remaining gaps](../../development/TUI_V3_REMAINING_GAPS.md).
