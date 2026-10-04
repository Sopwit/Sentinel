# Local IPC v1

Sentinel's headless daemon composes the existing C++ services with `ApplicationControllerBuilder`. Rust clients never link or reimplement the runtime. Desktop remains an in-process client during Stage A.

## Canonical contract

[`protocol/ipc-v1.json`](../../protocol/ipc-v1.json) is the canonical versioned field/enum definition. Run `python3 tools/ipc/generate.py` after changes; `--check` fails on drift. It generates the C++ request validator/command enum and Rust request, response and event payload enums. Generation is checked by CTest and Rust CI, and does not add a generator dependency to ordinary CMake builds. The Cargo package version is derived from `SENTINEL_APP_VERSION`; protocol version is independent: **1.0**.

Transport is local Unix-domain sockets on macOS/Linux through Qt `QLocalServer` and Rust `UnixStream`. Windows Rust transport currently reports unsupported/unavailable; no Windows runtime certification is claimed.

Each frame is one compact UTF-8 JSON object followed by LF:

```json
{"version":{"major":1,"minor":0},"type":"request","id":"request-1","name":"hello","payload":{"client_id":"example","major":1,"minor":0,"capabilities":[]}}
```

The daemon replies with `type: response`, the same `id` and `name`, and a typed payload. Errors have `type: error`, the correlated identifier when available, and `payload.code`. Events have `type: event`, `name`, and `payload.session_id`/`run_id`. Uncorrelatable malformed frames return an empty identifier. Unknown commands, enum variants, missing fields and wrong field types fail explicitly. Major version mismatch is rejected. Future minor versions may add optional fields/events; clients tolerate unknown future-minor events without treating them as success.

## Commands

| Command | Required payload | Result |
|---|---|---|
| hello | client_id, major, minor, capabilities | version, daemon_version, capabilities |
| daemon.status | none | running, uptime_ms, active_runs, sessions, daemon_version |
| daemon.shutdown | none | shutting_down |
| model.list / model.current | none | discovered models / selected provider and model |
| session.list | none | conversation-backed sessions |
| session.create | title | newly persisted conversation/session |
| session.attach | session_id | snapshot and future event subscription |
| chat.send / agent.start | session_id, text | run_id and session_id |
| run.cancel | run_id | cancellation acknowledgement |
| approval.respond | run_id, approval_id, allow | acknowledgement |

Consult the canonical file for exact response and event fields. Capabilities advertise the implemented surface. There is no IPC provider/model mutation command. Active binding is captured once and core `ModelBinding` remains authoritative.

Events are `run.started`, `output.delta`, `tool.requested`, `approval.requested`, `tool.result`, `run.completed`, `run.failed`, and `run.cancelled`. They map existing Chat/Agent events. Tool success does not produce Agent completion; only core accepted final-answer completion does. Final events include canonical terminal text/state. Clients replace incremental output with terminal text, avoiding duplicate finals.

## Sessions, approvals and lifecycle

Session IDs are existing conversation IDs. There is no IPC-only session database. This foundation exposes one foreground active run, reflecting the existing controller, with multiple connected clients. A connection subscribes by starting a run or attaching a session. Disconnect does **not** cancel work. Reconnect and attach return a current snapshot, bounded output, frozen binding and pending approval, then future events. There is no new event log or historical replay service. After daemon restart the existing stores recover persisted records; active runs are not resumed and ephemeral run IDs are not durable. Historical model binding is empty when the existing persisted record does not contain it.

Approval events include a one-use approval ID, run ID, tool, risk, resources and runtime detail. Clients submit an explicit allow/deny decision. The daemon validates active state and IDs before calling the existing approval boundary. Replayed IDs, unknown sessions/runs and approval after cancellation fail. IPC clients cannot grant permanent permissions or bypass the tool gateway.

## Security and limits

Default endpoint is `~/.sentinel/run/daemon.sock`. The runtime directory is owner-only (0700), its owner and non-symlink status are checked, and the socket is restricted to its owner. Custom endpoints require an existing private owner-owned directory. macOS `getpeereid` / Linux `SO_PEERCRED` enforce same UID. No TCP listener is opened.

A `QLockFile` claims the endpoint before stores open. Existing regular files and foreign/live sockets are never blindly removed. Same-owner socket artifacts are probed and recovered only after refused/not-found connection; crash-stale locks use Qt's process identity checks. Normal shutdown removes the owned endpoint and lock.

Frames are limited to 262144 bytes; request text/string fields to 65536 characters; identifiers to 128 characters; connected clients to 32. Server pending output is limited to 1048576 bytes per client. A slow/oversized-output client's connection is closed without cancelling or changing the core run state. Rust event buffering is bounded (256 pending events, 64 TUI channel entries). Attach output retains at most 65536 UTF-16 characters with `output_truncated`; full terminal output is subject to frame limits. Large results can therefore disconnect a client truthfully rather than allocate unbounded queues.

Logs contain lifecycle and error codes, not credentials or full tool/request payloads. Same-user clients are trusted to act as the user's UI, including shutdown/cancel decisions; this is not a remote security boundary.
