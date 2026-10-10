# TUI V3.2 remaining gaps

Scope: reliability of the existing daemon Agent loop. This backlog does not enable new commands or permissions.

| Gap | Required work / acceptance |
|---|---|
| Runtime context capacity — PARTIAL | V3.2.1 now discovers the actual loaded 8192-token window, bounds native result excerpts, reserves native output and preflights serialized request size. A measured two-file task completes; a similar-sized request still receives provider context exhaustion. Reliable tokenizer/accounting, loaded-limit refresh and authoritative compaction remain deferred. No model/server settings changed. |
| Natural multi-file coding autonomy — NOT ACCEPTED | Repeat the recorded synthetic levels across models; distinguish observed evidence from a useful explanation. A small local model completing one file read does not establish general coding capability. |
| Evidence-derived filesystem final presentation | Current policy replaces filesystem prose with scoped facts/excerpts. This prevents unsupported claims but can obscure multi-file explanations. A future typed claim/evidence design is needed; do not relax grounding based on fluent prose. |
| Native repair live acceptance | New native-call repair paths have deterministic tests. The live run did not deliberately exercise malformed/unknown native calls, so those repair tests are not live provider acceptance. Catalog capability flags alone do not prove a protocol-repair path ran. |
| Large extension tool catalogs | The compact tool index is bounded. Built-in catalog coverage is tested; arbitrarily large plugin/MCP catalogs still need paged authoritative discovery/schema retrieval. |
| Interrupted mutating operations — DEFERRED | No new automatic retry added. V3.2.1 direct D requested the failed configure command twice with fresh approvals; repeated model-initiated commands across observations still need outcome-aware guarding. Add durable operation IDs, partial-effect evidence and explicit review before supporting resumable writes/processes. |
| Detailed safe execution inspection — PARTIAL | Existing IPC omits raw arguments/results and reconnect does not replay the full historical timeline. Add a bounded redacted projection rather than reading private runtime stores in Rust. |
| Cancellation acceptance — PARTIAL | V3.2.1 observes the real owned /bin/sleep PID, sends Ctrl+C through release TUI, receives run.cancelled and observes no descendant afterward. Pending-approval and post-tool cancellation also pass live. General child trees, native Agent token-stream abort and mid-read preemption remain unvalidated; cmake launch failed separately. |
| Cancellation/session isolation — FIXED + VERIFIED | Late cancelled text entering a newly created conversation was reproduced and fixed; deterministic regression passes in 122/122 C++ suite. Active Agent runs cannot create/switch conversations. Stale cancellation-waiting notice text remains a presentation gap. |
| Physical terminal keys | Shift+Enter on Ghostty/macOS Terminal/iTerm2/Linux/tmux/SSH remains NOT VALIDATED. PTY input and deterministic routing do not substitute for physical host observation. |
| Performance/token comparison | No equivalent Codex/OpenCode benchmark or reliable per-turn token telemetry was available. Do not claim performance or autonomy parity. |

See [reliability report](TUI_V3_2_AGENT_RELIABILITY_REPORT.md), [test matrix](TUI_V3_2_AGENT_TEST_MATRIX.md), and [execution failures](TUI_V3_2_EXECUTION_FAILURES.md). No V3.3 work was started.

V3.2.1 closure evidence: [final report](TUI_V3_2_1_FINAL_REPORT.md), [context](TUI_V3_2_1_CONTEXT_RELIABILITY.md), [multi-file acceptance](TUI_V3_2_1_MULTIFILE_ACCEPTANCE.md), [cancellation](TUI_V3_2_1_CANCELLATION_ACCEPTANCE.md). Do not infer full acceptance from the passing regression suite.
