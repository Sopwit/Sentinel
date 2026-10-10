# TUI V3 Agent workflow and authority

Chat sends `chat.send`; Agent sends `agent.start`. The header and composer visibly identify the selected mode. Switching mode is local intent, idle-only, and grants no permissions. The daemon resolves the actual ModelBinding and applies its policy. Plan is **SPECIFIED ONLY**: `/plan` refuses execution because no authoritative no-mutation policy exists. Asking the model to avoid writes would not enforce this boundary.

Execution uses authoritative run IDs, session IDs, generations and sequence numbers. Tool completion does not complete an Agent. Only the runtime’s accepted final/terminal transitions establish results. The default timeline shows a small recent summary; `/details` expands bounded safe metadata. `/activity` inspects the full bounded activity view. Warnings, cancellation, failure and approval remain explicit. No raw logs, private reasoning, arbitrary tool arguments or results are exposed beyond the daemon’s safe projection.

Inspection content is separate from conversation/stream buffers. `/status`, `/doctor`, help, notices and capability views cannot overwrite streamed output. Escape returns to the conversation. System notices remain accessible through `/notices` without impersonating assistant messages. Detail expansion does not change daemon execution.

Approval displays operation, resource, risk and run/session identity. `y` requests Allow Once; `n` requests Deny. Pending state remains until accepted reply or terminal event. Esc does not approve. Ctrl+C requests cancellation; cancellation during start is queued until a real run identity arrives. The client never manufactures successful cancellation or finality. Reconnect restores pending state from the daemon snapshot and never replays a mutation automatically.

Session/model/provider/workspace changes are refused while active or a selection is in flight. A session switch is a viewing/attachment operation supported only while idle; no multi-session execution semantics are invented. Drafts survive switching within the bounded memory cache. Accepted session snapshots restore canonical messages and binding; the TUI never silently selects a fallback model. Catalog discovery is explicitly distinct from successful inference readiness.

## Workspace context and changes

`@` opens the existing `workspace.files` picker. The daemon invokes registered filesystem discovery through ToolExecutionGateway, validating workspace root and policy; Rust reads no local files. Search covers returned names, nested paths and details, not arbitrary disk. Selected paths remain visible, can be removed via `/references` or `/remove [index]`, and are appended as JSON-quoted path requests only to an explicit Agent task. Up to 16 references are allowed. They are untrusted requests for tool observation, **not attachment contents**. Chat with selected references is refused rather than falsely promising grounding. A deleted/renamed path is resolved or rejected by the daemon at tool time; large directories may be truncated by backend bounds. Literal pasted `@path` is ordinary task text, not automatic inclusion. No eager content or hidden expansion occurs.

Existing daemon file listing and read-size/type/path limits remain authoritative. No new byte inclusion budget is invented: the TUI includes zero file-content bytes. IPC frame size remains 262,144 bytes, composer 65,536 bytes. Queue/activity bounds remain 32 requests, 256 updates and 200 timeline entries. Discovery denial/truncation is shown; stale session file replies are ignored.

`/diff` and `/review` show actual bounded Applied workspace evidence, with per-file navigation through the existing picker. The baseline is in memory, may include concurrent edits, omits unreadable resources, and does not survive restart. This is not a proposed patch approval flow, reliable Git status, or rollback system. No automatic Git operation was added. Tests/verification claims are shown only when supplied by the daemon, not inferred from assistant text.

`!command` remains ordinary text. Shell tools, if requested by an Agent, must follow ToolRegistry, ToolExecutionGateway, ProcessExecutor, workspace authorization, approval and cancellation. No local subprocess/editor pathway was introduced.

## Guided questions specification only

A future `/grill-me` needs a daemon-owned interview ID, goal, one pending question, optional choices and free text, persisted answers tied to the session, a bounded plan artifact and explicit approval transition. Questions must be distinguishable from permission requests. Neither opening an interview nor answering it grants tool permissions. Execution must require a separately accepted operation under policy. Until those typed state/transitions exist, the command explains unavailability and sends nothing.
