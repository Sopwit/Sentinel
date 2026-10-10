# TUI V3.2 Agent reliability report

Verdict: **IMPROVED — FURTHER FIXES REQUIRED**. This is not general coding-agent acceptance or competitor parity.

## Scope and baseline

2026-10-10, macOS 27.0.1 arm64; baseline `ec19493bc9a3bdf531ae3d908217159253b396cc`, initially clean checkout. Personal-Brain was unavailable. Architecture, Agent Runtime, security, building/testing and V3/V3.1 documentation informed this scoped change. No Desktop/Quick Panel, commands, keyboard contracts, daemon IPC schema or permissions were redesigned. No commit/push/merge was performed.

Only disposable synthetic C++/CMake workspaces and private portable daemon profiles were used. The running personal daemon, model installations, server configuration and personal profiles were not modified. The active local LM Studio endpoint at `http://127.0.0.1:1234` was used with explicitly accepted `lm-studio / nvidia/nemotron-3-nano-4b`. There was no provider fallback or model download. The endpoint that refused connections during V3.1 was now reachable; the earlier cause remains undetermined.

## Phase A: real context and continuity

**IMPLEMENTED + VERIFIED / EXISTING + REUSED:** actual release TUI `@` picker selected CMakeLists.txt and README.md; `/references` displayed both. The submitted Agent task caused authorized `read-file`, a real tool result and model continuation, then authoritative `run.completed` with a scoped fact/excerpt containing `set(CMAKE_CXX_STANDARD 20)`. `context-ui.json` and raw PTY captures record the whole flow. This proves grounded single-file completion, not broad code understanding.

Client/daemon binding and readiness were inspected via `terminal.state` and `model.current`. Selected and active bindings agreed. Catalog availability was not reported as initialized inference; the TUI continued to display “Inference unverified”. Restarting this isolated daemon preserved the session and grounded history. A controlled closed endpoint in that isolated profile produced `run.failed / ConnectionFailed`; restoring the endpoint accepted the same binding. The recovery probe did not run another successful inference after restoration. A separate live run-command approval was denied: the run failed explicitly with “User denied the required action”, no command execution or file diff. The running-tool cancellation probe failed at the provider context limit before tool execution, so live cancellation remains blocked.

## Implemented reliability changes

1. `LlmAgentRuntime`: validate a complete native call batch before storing pending native result state. Unknown/malformed native calls use the existing two-attempt planning repair bound. No rejected call is executed or placed into continuation as an unanswered executed call. Native repair instructions preserve native protocol.
2. `AgentLoop`: one planning-only repair per run for repeated successful read-only observations. No repeated call is executed during repair. Repeated mutations and failed-call cycles retain stop behavior; the next read repetition also stops. Sync/async paths and the final-evidence gate share this behavior.
3. `ContextEngine`: a bounded registered-tool index preserves built-in IDs and required/optional argument names before detailed schema budgeting. Tool execution still validates against the authoritative registered schema. Arbitrarily large extension catalogs need future paging.
4. Grounding failures now expose the controlled EvidencePolicy rejection reason after the existing rejection limit; they do not expose private reasoning or invent a final result.
5. Rust TUI: an active reattachment restores the current user task and preceding conversation once, while retaining newer streamed/snapshot output. Old partial assistant history does not overwrite current output. System notices stay separate. Actual 80×24 and 120×40 PTY reconnect captures show the restored task; deterministic tests cover non-duplication and stream preservation.

## Real-model outcome and limitations

The baseline natural multi-file analysis repeated reads and hit the doom guard. The source-change request ended with an empty/malformed provider response. Verification and missing-file recovery exceeded the 300-second observer bounds and were cancelled. No baseline source changes or build/test success were evidenced. These failures are retained, not replaced with scripted successes. With the updated daemon, the source task received one scoped edit-file approval and produced the exact `a-b` → `a+b` diff while preserving other files. The next provider request failed with a context-size error; no accepted final or verification is claimed, and the mutation was not retried. Multi-file analysis still failed the evidence gate. See the final per-task [matrix](TUI_V3_2_AGENT_TEST_MATRIX.md).

Native malformed-call repairs are deterministic regression coverage; no corresponding fault was deliberately injected into live provider responses. The catalog reports native tools, but public tool events alone do not prove which repair path ran. Tool success never completes a run; only an accepted AgentLoop final does. Existing ToolRegistry/ToolExecutionGateway, IFileSystemService, ProcessExecutor, ModelBinding, approval and sandbox boundaries remain authoritative. No natural-language shell shortcut, mutating retry, unsafe rollback or new policy grant was added.

## Competitor-informed choices

[Codex agent approvals/security](https://learn.chatgpt.com/docs/agent-approvals-security), [Codex tool orchestrator](https://github.com/openai/codex/blob/main/codex-rs/core/src/tools/orchestrator.rs), and [approval handling](https://github.com/openai/codex/blob/main/codex-rs/core/src/tools/approvals.rs) reinforce separate approval, execution and cancellation states. Sentinel does not adopt automatic unsandboxed retries.

[OpenCode agents](https://opencode.ai/docs/agents/) and [session processor source](https://github.com/anomalyco/opencode/blob/dev/packages/opencode/src/session/processor.ts) show bounded execution and repeated-action detection. Sentinel uses a narrower planning-only read repair rather than loosening permissions or treating Plan as guaranteed non-mutating. These are source-informed patterns, not equivalent executable benchmarks. No same-model Codex/OpenCode comparison or reliable token telemetry was available; no speed, cost or autonomy parity is claimed.

## Verification and stopping point

Exact build/test logs, controlled failures and live outcomes are indexed in [evidence](../reviews/agent-v32-2026-10-10/README.md). Deterministic fixtures, real process tests and real-model acceptance are labeled separately. Native terminal hosts and physical Shift+Enter remain **NOT VALIDATED**; PTY captures are actual terminal byte streams, rendered previews are bounded VT replays.

Remaining work is recorded in [gaps](TUI_V3_2_REMAINING_GAPS.md). Recommend a separately authorized model/planner reliability batch with grounded multi-file explanation and levels 3–5 acceptance. No V3.3 work was started.
