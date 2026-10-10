# Sentinel TUI

Launch with `sentinel tui`, `sentinel attach SESSION_ID`, or `sentinel tui --session SESSION_ID`. The canonical CLI launches the `sentinel-tui` library. An interactive terminal is required; the daemon remains authoritative.

The compact header shows **CHAT** or **AGENT**, session title, connection/run state, selected provider/model readiness, and workspace name/root. The main area contains the transcript/output. A multiline composer and footer remain visible at narrow sizes. Searchable pickers and approvals overlay the main area; no permanent extra panes are required. Terminal default backgrounds are preserved. Input recall and per-session drafts are kept only in memory; canonical conversations remain daemon-persisted.

## Keys

| Key | Action |
| --- | --- |
| Enter | Send when idle and non-empty; disconnected drafts are preserved |
| Shift+Enter | Insert newline when the terminal distinguishes it; Alt+Enter / Ctrl+O remain compatibility alternatives |
| Ctrl+G | Explicitly switch Chat/Agent while idle |
| Left/Right, Home/End, Up/Down | Cursor and line editing; Up/Down recall history in a single-line draft |
| Ctrl+Left/Right | Word movement |
| Delete/Backspace | Delete character |
| Alt+Up/Down | History, with unfinished draft restored on leaving history |
| Ctrl+P / Tab | Fuzzy command palette |
| Ctrl+L | Model picker |
| Ctrl+W | Workspace picker |
| `/sessions` | Session picker |
| Ctrl+N | New daemon session |
| `@` / `/files` | Authorized workspace file picker |
| `/remove` | Remove the last selected file reference |
| Ctrl+D | Exit only idle/empty; otherwise delete character |
| PageUp/PageDown | Scroll output |
| Ctrl+B | Follow/jump to bottom |
| Ctrl+F | Search displayed output; Enter finds the first matching line |
| Ctrl+R | Search displayed transcript |
| `/reconnect` | Reconnect the session |
| F1, `?` on an empty composer, `/help` | Keybindings and command list |
| Ctrl+C during a run | Request cancellation once and wait for daemon terminal state |
| Ctrl+C while idle | Preserve draft; exit only when empty |

Bracketed paste inserts text without sending. Native terminal selection/copy remains available; the app does not capture the mouse.

Pickers support case-insensitive subsequence search, Up/Down selection, Enter, and Esc cancel. Model rows expose provider/readiness; unavailable rows cannot be selected. Workspace/session/model/provider changes are refused during active work. Provider selection can trigger authoritative discovery when no cached models exist; select the discovered model explicitly with `/model`. Empty results stay truthful. Routine provider status uses ModelService’s observed snapshot; unobserved cloud credentials are not inspected by opening the TUI. Explicit discovery/preflight remains authoritative. Session IDs remain visible in picker details while titles are primary labels.

## Slash commands

Type `/` for fuzzy discovery. Up/Down selects; Tab completes without execution; Enter completes an incomplete name and executes an exact command only after validation. Escape dismisses discovery and preserves input. Type `//` for literal leading slash text. Pasted slash content stays literal through prompt recall and failed sends. Unknown commands offer suggestions and never become Agent tasks.

Ctrl+P opens the same command catalog. `/help [query]` searches descriptions and actual keyboard bindings; Enter inspects full help without executing the selected command. Disabled rows show their reason.

| Command family | Actual behavior |
|---|---|
| `/new [title]`, `/sessions [query]`, `/resume [id\|last]` | Create, search/switch, resume persisted daemon sessions |
| `/rename <title>`, `/archive` | Existing daemon metadata actions; archive requires no unsent draft/references |
| `/model`, `/provider`, `/workspace` | Existing authoritative pickers; no implicit model fallback |
| `/mode [chat\|agent]`, `/chat`, `/agent` | Explicit mode selection, idle only; no permission changes |
| `/status`, `/doctor`, `/reconnect` | Real status/diagnostics and safe reconnect |
| `/details`, `/activity`, `/notices` | Toggle safe timeline details; inspect bounded activity or system notices |
| `/tools`, `/permissions`, `/mcp`, `/tasks`, `/context`, `/memory` | Fresh read-only safe daemon projections; no grant mutation or memory payload dump |
| `/history [query]`, `/search [query]` | Current-session history refresh/search, or displayed text search |
| `/export [markdown\|json\|text]` | Existing controlled daemon export action; actual result reported |
| `/files`, `/references`, `/remove [index]` | Authorized paths and selected-reference removal; no eager file contents |
| `/diff`, `/review` | Existing Applied workspace evidence; no autonomous review or rollback |
| `/settings`, `/theme [name]` | Inspect preferences / choose terminal, obsidian, glacier or porcelain for this process |
| `/cancel`, `/exit` | Request authoritative cancellation; exit only idle with no unsent content |
| `/plan`, `/compact`, `/undo`, `/redo`, `/skills`, `/init`, `/grill-me`, `/editor` | Explicitly unavailable; explain required backend/safe lifecycle contracts and do nothing |

Aliases and exact availability are in the [complete V3 registry](../development/TUI_V3_COMMAND_SPEC.md). `/clear` is an alias for a new session, not deletion; `/agents` inspects task summaries, not spawning. `/quit` and `/q` alias safe exit. Session switching retains up to 32 memory-only drafts with cursor/mode/reference state and refuses overflow rather than dropping drafts. Active/busy operations block session/model/workspace changes. `/resume last` uses the latest cached daemon list; `/sessions` refreshes it.

Metadata inspection views do not replace a streaming response; Escape returns to the conversation. Native terminal copy/selection remains available. No shell shortcut, external editor, persistent key remapping or guaranteed Plan mode is implemented.

## Files and changes

The picker obtains only authorized paths from the daemon's `glob` tool. File names with spaces are preserved and encoded as quoted references. Selected quoted paths are visible in the footer (clipped to terminal width), can be removed with `/remove`, and are appended explicitly to an Agent request for observation through its normal tools. Rust does not read file contents. Chat sends with selected file references are blocked until the user explicitly switches mode. Literal/quoted `@path` text can also be pasted as part of an Agent task; it remains a request for authorized observation, not an eager prompt dump.

Workspace roots come from WorkspaceService, never arbitrary terminal CWD. Sensitive files, symlink escapes, authorization denials, disabled workspace tools and hidden traversal rules remain governed by the backend. Files require a custom workspace with an attached root. Built-in workspace roots cannot be changed.

`/diff` uses a bounded in-memory daemon baseline captured before an Agent run. Select a changed file to inspect its unified diff; use `/diff` again to navigate to another file. Edits are already Applied. No accept or rollback is offered. Missing baselines, workspace mismatch, truncation and omitted resources are explicit. This is not Git status, does not infer deletions from unreadable resources, may include concurrent external edits, and does not survive restart. See [CLI](CLI.md) for bounds.

## Runs, approvals and recovery

Only accepted AgentLoop finals complete Agent work. The UI displays safe runtime transitions and tool requested/running/completed/failed events, with timestamps/duration when supplied. Hidden reasoning and raw tool arguments/results are not displayed. Subagent state is shown only for typed events the backend actually publishes.

Approvals show operation, resource, risk, run/session identities, and the pending-operation scope. `y` is **Allow Once**; `n` is **Deny**. Esc leaves the modal visible. The modal stays pending while a response is in flight; stale/duplicate decisions are rejected by daemon identity checks. It clears only after accepted response or authoritative terminal state. Persistent grants are not offered.

IPC runs on a background worker. The rendering loop does not enumerate providers or synchronously wait for daemon replies. On connection loss, the worker retries and reattaches the same session, replacing the snapshot and using sequence/generation metadata to avoid replay duplication. Pending approval comes from the authoritative attach snapshot. Disconnect never cancels or replays an Agent mutation. Daemon restart recovers persisted conversations but does not restart interrupted runs or retain change baselines.

See the [V2 keyboard contract](../development/TUI_KEYBINDING_CONTRACT.md) for context precedence and Ctrl+A/E/U/K editing. Ctrl+S is no longer Send.

## V2.3 welcome and alignment

An empty conversation shows a centered Sentinel symbol, welcome text and command/help hints, including while disconnected or without discovered models. Persisted system messages stay separate from conversation messages; the footer points to `/notices` for their full text and runtime notices. Commands such as `/help`, `/doctor` and `/notices` remain inspectable in the viewport.

The transcript and composer share a centered grid, with responsive width and a maximum 180-cell outer grid (V3.1 expands the former 88-cell reading cap) and the input aligned to assistant body text. The composer grows for explicit newlines, including a trailing newline, and retains horizontal scrolling for long input. Picker, approval and search focus quiet the composer border; closing the overlay restores input focus and the draft.

The model row reserves space for a short readiness label rather than the verbose diagnostic explanation. Provider health `Available` is shown as `Inference unverified`: catalog/provider health does not attest to successful inference initialization. Use `/doctor` for the full daemon-supplied readiness summary. Long workspace/provider/model names are bounded with `...` so they cannot overwrite connection/readiness labels; the existing workspace/model pickers retain full selection details.

See the [V3 implementation report](../development/TUI_V3_IMPLEMENTATION_REPORT.md) for measured tests, terminal evidence and live acceptance limits.

## V3.1 streaming and themes

Previous messages remain visible while a new response starts and streams. The current user turn appears immediately below the history; final canonical history replaces this presentation without duplication. A rejected submission restores the draft and history. Provider failure after accepted submission remains a failed persisted turn, available through prompt recall; it is not replayed automatically.

Use `/theme` for the theme picker or `/theme glacier`, `/theme obsidian`, `/theme porcelain`, `/theme terminal`. For the next launch, set the existing `SENTINEL_TUI_THEME` environment variable. `NO_COLOR` disables palette colors even when a theme is selected; remove that variable at launch if color is wanted. The default terminal theme preserves the emulator background. Theme changes never change daemon permissions.

Shift+Enter is the primary newline hint. Enhanced keyboard reporting is requested and restored on exit; terminals that cannot distinguish it should use Alt+Enter or the retained Ctrl+O alternative. Physical Shift+Enter acceptance must be tested in the actual terminal host. See [V3.1 acceptance](../development/TUI_V3_1_LIVE_ACCEPTANCE.md) for actual live workflow evidence and limits.

## Agent continuity and reliability

Reattaching an active session restores its current task and preceding conversation without replacing newer streamed output. System notices remain available separately. Technical detail stays behind `/details`; a reconnect does not replay the complete historical tool timeline.

A successful tool is not a completed Agent task. Completion requires an accepted daemon final answer. A failed run may have already changed an approved file; inspect `/diff` before deciding what to do next. Sentinel does not automatically retry a mutation after interruption. V3.2 live acceptance, including remaining context-capacity and multi-file limitations, is recorded in the [reliability report](../development/TUI_V3_2_AGENT_RELIABILITY_REPORT.md).
