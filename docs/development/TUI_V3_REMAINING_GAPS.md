# TUI V3 remaining gaps

These entries are engineering backlog, not working commands. Disabled entries explain their reason in slash discovery/help and reject dispatch without IPC mutation.

| Feature | Status | Required next work / authority |
|---|---|---|
| `/plan` | SPECIFIED ONLY | Daemon-owned no-write/no-execute policy binding, tool risk enforcement, persisted mode, rejected-mutation tests; a prompt instruction is insufficient |
| `/compact` | SPECIFIED ONLY | Authoritative context summarization, provenance, token accounting and recoverable persisted replacement semantics |
| `/undo`, `/redo` | DEFERRED | Durable snapshots, conflict detection, ownership, atomic restore and approval; never use Git discard as a substitute |
| `/skills` | SPECIFIED ONLY | Executable skill schema, allowlisted discovery, trust/content boundaries and runtime invocation; profile metadata alone is insufficient |
| `/init` | SPECIFIED ONLY | Authorized bounded workspace writes, preview, conflict handling and explicit accepted operation |
| `/grill-me` | SPECIFIED ONLY | Typed question/answer state, session persistence, reviewable plan and approval transition; see workflow specification |
| `/editor` | DEFERRED | Safe argv-based executable configuration, private temp-file lifecycle, terminal suspend/resume, cancellation and cleanup without shell evaluation |
| Raw tool arguments/results | BLOCKED by current safe projection | Redacted, bounded, typed daemon payload; no private reasoning disclosure |
| Chat file attachments | BLOCKED by contract | Validated attachment IDs, scope/type/byte limits, explicit inclusion provenance; selected paths currently require Agent tools |
| Persistent key customization/preferences | DEFERRED | Extend shared settings with one TUI schema/defaults/precedence, conflict validation and migration; no parallel config file |
| Selection editing/clipboard | DEFERRED | Unicode-safe selection semantics and terminal-compatible copy; native selection currently retained |
| Prompt soft wrapping | DEFERRED | Existing horizontal editor view retained; multiline sizing and transcript word wrapping work |
| Global history search | SPECIFIED ONLY | Bounded daemon query/pagination/search contract; `/history` covers current session only |
| Session workspace metadata | BLOCKED by list contract | Persisted per-session workspace/binding semantics; do not invent absent fields |
| Multiple executing sessions | DEFERRED | Explicit view-vs-execute identity and concurrency model; current active-run switch guard retained |
| Shell shorthand | DEFERRED | Typed authorized tool invocation through ProcessExecutor, risk/scope preview, audit/cancellation; `!` has no local executor |
| Model local/remote and capability precision | EXISTING + REUSED where reported | Richer typed row metadata before precise badges; catalog health never equals successful inference |
| Diff enhancements | EXISTING + REUSED, partial | Structured file stats and bounded per-file code navigation; current Applied evidence is not a durable snapshot |
| Crash-safe drafts | DEFERRED | Authoritative draft storage and retention/privacy rules; current bounded cache is memory-only |
| Native terminal acceptance | NOT VALIDATED | Real Ghostty, macOS Terminal, iTerm2, Linux, tmux/SSH observations; PTY evidence is not native-host acceptance |
| Live inference/Agent journeys | IMPLEMENTED + PARTIALLY VERIFIED | V3.1 exercised real Chat, read-file Agent continuation, write approval/deny/cancel, persistence and recovery. See [live acceptance](TUI_V3_1_LIVE_ACCEPTANCE.md) for per-journey limitations; no blanket parity claim |

V3.1 adds responsive gutters, streaming history retention, theme selection and Shift+Enter protocol support. Physical native-host key acceptance remains NOT VALIDATED. Agent grounding can fail after successful tool access and approval resources remain technical JSON; these are visible limits, not successful completion.

Recommended V3.2 batch: native-host keyboard/resize acceptance and reliable grounded file-reference completion, plus interrupted-stream/timeout recovery matrix, before expanding workflow semantics. Follow with a typed daemon attachment and safe tool-detail contract. Authoritative Plan policy should be a separate backend design/review batch. Do not broaden this TUI change into rollback, shell, agent orchestration or Desktop work.
