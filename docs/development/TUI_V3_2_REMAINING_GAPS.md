# TUI V3.2 remaining gaps

Scope: reliability of the existing daemon Agent loop. This backlog does not enable new commands or permissions.

| Gap | Required work / acceptance |
|---|---|
| Runtime context capacity | LM Studio catalog advertised 1,048,576 tokens for this model, but a live continuation returned context-size exceeded. Negotiate/measure the loaded instance limit and budget native schemas/history before introducing any recovery; do not silently change model/server settings. |
| Natural multi-file coding autonomy | Repeat the recorded synthetic levels across models; distinguish observed evidence from a useful explanation. A small local model completing one file read does not establish general coding capability. |
| Evidence-derived filesystem final presentation | Current policy replaces filesystem prose with scoped facts/excerpts. This prevents unsupported claims but can obscure multi-file explanations. A future typed claim/evidence design is needed; do not relax grounding based on fluent prose. |
| Native repair live acceptance | New native-call repair paths have deterministic tests. The live run did not deliberately exercise malformed/unknown native calls, so those repair tests are not live provider acceptance. Catalog capability flags alone do not prove a protocol-repair path ran. |
| Large extension tool catalogs | The compact tool index is bounded. Built-in catalog coverage is tested; arbitrarily large plugin/MCP catalogs still need paged authoritative discovery/schema retrieval. |
| Interrupted mutating operations | No automatic retry. Add durable operation IDs, partial-effect evidence and explicit review before supporting resumable writes/processes. |
| Detailed safe execution inspection | Existing IPC omits raw arguments/results and reconnect does not replay the full historical timeline. Add a bounded redacted projection rather than reading private runtime stores in Rust. |
| Physical terminal keys | Shift+Enter on Ghostty/macOS Terminal/iTerm2/Linux/tmux/SSH remains NOT VALIDATED. PTY input and deterministic routing do not substitute for physical host observation. |
| Performance/token comparison | No equivalent Codex/OpenCode benchmark or reliable per-turn token telemetry was available. Do not claim performance or autonomy parity. |

See [reliability report](TUI_V3_2_AGENT_RELIABILITY_REPORT.md), [test matrix](TUI_V3_2_AGENT_TEST_MATRIX.md), and [execution failures](TUI_V3_2_EXECUTION_FAILURES.md). No V3.3 work was started.
