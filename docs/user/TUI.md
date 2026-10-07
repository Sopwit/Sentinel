# Sentinel TUI

Launch with `sentinel tui`, `sentinel attach SESSION_ID`, or `sentinel tui --session SESSION_ID`. The canonical CLI launches the `sentinel-tui` library. An interactive terminal is required; the daemon remains authoritative.

The compact header shows **CHAT** or **AGENT**, session title, connection/run state, selected provider/model readiness, and workspace name/root. The main area contains the transcript/output. A multiline composer and footer remain visible at narrow sizes. Searchable pickers and approvals overlay the main area; no permanent extra panes are required. Terminal default backgrounds are preserved. Input/history/drafts are kept only in memory.

## Keys

| Key | Action |
| --- | --- |
| Enter | Insert newline; never sends |
| Ctrl+S | Send composer contents or execute a slash command |
| Ctrl+G | Explicitly switch Chat/Agent while idle |
| Left/Right, Home/End, Up/Down | Cursor and line editing; Up/Down recall history in a single-line draft |
| Ctrl+Left/Right | Word movement |
| Delete/Backspace | Delete character |
| Alt+Up/Down | History, with unfinished draft restored on leaving history |
| Ctrl+P / Tab | Fuzzy command palette |
| Ctrl+L | Model picker |
| Ctrl+W | Workspace picker |
| Ctrl+O | Session picker |
| Ctrl+N | New daemon session |
| `@` / `/files` | Authorized workspace file picker |
| Ctrl+D | Remove the last selected file reference |
| PageUp/PageDown | Scroll output |
| Ctrl+B | Follow/jump to bottom |
| Ctrl+F | Search displayed output; Enter finds the first matching line |
| Ctrl+R | Reconnect the session |
| F1, `?` on an empty composer, `/help` | Keybindings and command list |
| Ctrl+C during a run | Request cancellation once and wait for daemon terminal state |
| Ctrl+C while idle | Exit TUI |

Bracketed paste inserts text without sending. Native terminal selection/copy remains available; the app does not capture the mouse.

Pickers support case-insensitive subsequence search, Up/Down selection, Enter, and Esc cancel. Model rows expose provider/readiness; unavailable rows cannot be selected. Workspace/session/model/provider changes are refused during active work. Provider selection can trigger authoritative discovery when no cached models exist; select the discovered model explicitly with `/model`. Empty results stay truthful. Routine provider status uses ModelService’s observed snapshot; unobserved cloud credentials are not inspected by opening the TUI. Explicit discovery/preflight remains authoritative. Session IDs remain visible in picker details while titles are primary labels.

## Slash commands

Type a command and press Ctrl+S, or choose it through Ctrl+P.

| Command | Behavior |
| --- | --- |
| `/model` | Model picker |
| `/provider` | Provider readiness/picker and discovered models |
| `/workspace` | Workspace picker; create/attach custom roots with CLI workspace commands |
| `/sessions`, `/new` | Attach/create daemon conversations |
| `/chat`, `/agent` | Explicit mode switch only; `/agent task` is not interpreted as execution |
| `/tools` | Read-only authoritative registry state |
| `/mcp` | Read-only safe MCP connection state |
| `/permissions` | Permission service/policy diagnostics; no persistent grant mutation |
| `/context` | Backend context status; no invented token precision |
| `/memory` | Read-only memory availability/count; no memory-content dump |
| `/tasks` | Backend task/runtime summaries |
| `/status`, `/doctor` | Daemon status / refreshed safe diagnostics |
| `/compact` | Explains that no authoritative compaction operation is available; changes no history |
| `/help` | Commands and keys |
| `/activity` | Inspect bounded safe structured run/tool/subagent activity |
| `/files` | File picker through registered filesystem tool and gateway |
| `/diff` | Changed-file picker and unified whole-file diffs, labelled Applied |

Unknown slash commands are not sent to Agent or shell. Model/provider configuration uses existing authority; no selection or fallback is invented.

## Files and changes

The picker obtains only authorized paths from the daemon's `glob` tool. File names with spaces are preserved and encoded as quoted references. Selected quoted paths are visible in the footer (clipped to terminal width), can be removed with Ctrl+D, and are appended explicitly to an Agent request for observation through its normal tools. Rust does not read file contents. Chat sends with selected file references are blocked until the user explicitly switches mode. Literal/quoted `@path` text can also be pasted as part of an Agent task; it remains a request for authorized observation, not an eager prompt dump.

Workspace roots come from WorkspaceService, never arbitrary terminal CWD. Sensitive files, symlink escapes, authorization denials, disabled workspace tools and hidden traversal rules remain governed by the backend. Files require a custom workspace with an attached root. Built-in workspace roots cannot be changed.

`/diff` uses a bounded in-memory daemon baseline captured before an Agent run. Select a changed file to inspect its unified diff; use `/diff` again to navigate to another file. Edits are already Applied. No accept or rollback is offered. Missing baselines, workspace mismatch, truncation and omitted resources are explicit. This is not Git status, does not infer deletions from unreadable resources, may include concurrent external edits, and does not survive restart. See [CLI](CLI.md) for bounds.

## Runs, approvals and recovery

Only accepted AgentLoop finals complete Agent work. The UI displays safe runtime transitions and tool requested/running/completed/failed events, with timestamps/duration when supplied. Hidden reasoning and raw tool arguments/results are not displayed. Subagent state is shown only for typed events the backend actually publishes.

Approvals show operation, resource, risk, run/session identities, and the pending-operation scope. `y` is **Allow Once**; `n` is **Deny**. Esc leaves the modal visible. The modal stays pending while a response is in flight; stale/duplicate decisions are rejected by daemon identity checks. It clears only after accepted response or authoritative terminal state. Persistent grants are not offered.

IPC runs on a background worker. The rendering loop does not enumerate providers or synchronously wait for daemon replies. On connection loss, the worker retries and reattaches the same session, replacing the snapshot and using sequence/generation metadata to avoid replay duplication. Pending approval comes from the authoritative attach snapshot. Disconnect never cancels or replays an Agent mutation. Daemon restart recovers persisted conversations but does not restart interrupted runs or retain change baselines.
