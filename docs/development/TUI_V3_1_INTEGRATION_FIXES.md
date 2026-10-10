# TUI V3.1 integration and visual fixes

Dates: 2026-10-09–10. This phase builds on the uncommitted V3 implementation. Original V3 reports remain historical. No C++ daemon/protocol/Qt/QML source was changed; no commit, push or merge was performed.

The supplied native screenshots show a narrow central column in a wide terminal and previous messages disappearing during a new response. The user's direct request authorizes fixing those defects, adding themes and making Shift+Enter the primary newline affordance. The attached V3.1 acceptance plan was initially treated as reference material; the user subsequently explicitly authorized the complete plan and existing active providers.

## Fixed defects

| Defect | Cause | Change | Verification |
|---|---|---|---|
| Previous conversation disappears while waiting/streaming | `output` held both canonical history and a current response; start acknowledgement cleared it, and active rendering showed only the latest turn | Preserve the prior canonical transcript separately for the in-flight turn; append the pending user/assistant block immediately; keep history through deltas and terminal events until canonical messages replace it once | Regression covers send → acknowledgement → stream → terminal → history refresh, no duplication; actual two-response PTY recording |
| Large horizontal gutters | Fixed 92-cell outer / 88-cell reading cap used only about half the wide native terminal | Responsive centered grid fills available width with two outer cells, up to 180 outer cells; transcript, composer and footer retain shared alignment | 80×24, 100×30, 120×40, 160×48 and 200×48 deterministic renders; actual before/after recordings at requested sizes |
| Queue rejection could lose visible history | Queue failure restored input but the new split transcript also needed rollback | Restore prior transcript alongside draft/reference/literal provenance; keep rejected prompt out of accepted history | Closed queue regression |
| Fresh commands after clearing pasted text stayed literal | Paste provenance outlived the complete draft | Reset provenance when inserting into an empty draft; retain it when recalling/editing a nonempty pasted command | Editor regression and actual paste → clear → slash commands → exit 0 |
| Export/session action could report false success | `accepted` describes finite-action invocation; boolean `result.value` is the operation result | Require both invocation acceptance and `result.value == true` before reporting saved/changed or opening the post-archive picker | Explicit accepted-true/result-false regression; actual export/rename/archive contracts |
| Weak inference error explanation | Provider connection/timeouts were raw task labels | Explain known daemon `ConnectionFailed`, `Timeout`, `RateLimited`, `CapabilityUnsupported` categories in TUI language; do not mark the daemon disconnected for a failed inference | Error classification regression; actual isolated closed-endpoint `run.failed(ConnectionFailed)` |

| File picker chooses a common-root match ahead of a requested filename | Full-path and explanatory prose fuzzy matches competed with filename matches | Rank fuzzy relative filenames first; allow exact full-path substring as secondary; exclude explanatory prose from file matching | Nested README/common-root regression; live picker rerun recorded separately |

## Keyboard and themes

Shift+Enter is the primary displayed newline shortcut; Enter still sends. Crossterm's standard keyboard disambiguation protocol is enabled and popped during cleanup, allowing supporting terminals to distinguish Shift+Enter from Enter. Legacy Alt+Enter and Ctrl+O remain accepted compatibility alternatives, documented in Help. There is no universal way to distinguish Shift+Enter if a terminal sends the same byte as Enter; physical native-host acceptance is not claimed. Existing Ctrl+P, cancellation, line editing and bracketed paste behavior remain intact.

`/theme [terminal|obsidian|glacier|porcelain]` is one new typed registry command, also available in palette/help. Bare `/theme` opens a picker. Terminal preserves emulator colors; Obsidian is dark neutral, Glacier dark blue, Porcelain light. Surfaces, borders, secondary text and accents share the palette, including popups. `dark` and `light` launch aliases remain compatible. `NO_COLOR` wins and the selection notice explains that override. Theme selection is process-local; `SENTINEL_TUI_THEME` is the existing launch default. No competing config file, daemon setting or permission mutation was introduced.

## Boundaries retained

Model/provider/workspace and session bindings remain daemon-owned. `@` references are authorized path requests, not eager file content. Tools still use AgentRuntime/ToolExecutionGateway; only the accepted final establishes completion. Approval tests permit only an explicitly requested new synthetic target after exact resource/tool matching. Unknown scope is denied. No local shell execution, unsafe rollback, model download, private conversation access or persistent permission grant was added.

See [live acceptance](TUI_V3_1_LIVE_ACCEPTANCE.md) and [command validation](TUI_V3_1_COMMAND_VALIDATION.md) for feature-level evidence and limits.
