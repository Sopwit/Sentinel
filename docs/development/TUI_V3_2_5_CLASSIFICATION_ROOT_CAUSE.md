# V3.2.5 classification root cause

Date: 2026-10-10–11 (Europe/Istanbul; trace timestamps are UTC). Read-task acceptance remains unresolved; see final measured results.

Baseline: clean `695244b73b49395a4383f45ae2376164f1d3d0f7`. Earlier external commits are preserved. Personal-Brain was unavailable; the explicitly supplied repository and its architecture, runtime, security, build and testing instructions were used. An unrelated `assets/core-warden` tree and `CORE_WARDEN_CW01_MODEL_REPORT.md` appeared during execution and are left untouched.

ObservationIntentPolicy is an independent semantic evidence-requirement classifier, not the tool planner or permission authority. It distinguishes inspection from operation and current observation from conversation, produces requirements and selected symbolic claims, and informs ContextEngine/EvidencePolicy. AgentLoop fails closed before planning on indeterminate classification. Classification does not grant filesystem/process access: descriptor validation, approval, permission, authorization, resource scope and sandbox remain in ToolExecutionGateway. Removing classification would remove an evidence obligation; this phase retains it rather than replace it with a language heuristic.

## Reproductions

The installed Nemotron GGUF Q4_K_M instance was loaded by this task at 8192 context/four slots after verifying no user-loaded models. Qwen MLX 4bit is separately installed; engine formats make timing comparisons uncontrolled. No reasoning setting was changed. The catalog explicitly reports reasoning default on for both.

Independent synthetic controls use the same local `/v1/chat/completions` user-role, non-streaming body, model and 512-output-token bound. See `docs/reviews/agent-v325-2026-10-10/independent-probes.json`. The reconstructed original classifier with synthetic context completes in 28.27 seconds: 331 prompt tokens, 437 completion tokens, including 366 reasoning tokens. A minimal JSON request takes 2.39 seconds (27 prompt/36 completion, 27 reasoning). A second original request with a 90-second observer bound completes in 16.74 seconds. This is evidence of request-dependent generation cost and variability; the model can satisfy this contract, and timeout alone is not proof of incompatibility or context exhaustion. It does not establish the hidden reasoning contents or the cause of every V3.2.4 timeout.

The original successful response incorrectly uses `operation:exists` for reading/explaining contents. The original prompt also tells the model to use the workspace root broadly, then makes exceptions for existence; this is ambiguous for file-content tasks. Independently, `conversationContext.left(1500)` discards AgentLoop's trusted workspace-root suffix when conversation is long. The latter is deterministically reproduced, not asserted as the cause of every prior live timeout.

## Changes

The prompt now explicitly distinguishes reading/explaining/comparing content (`none`) from existence questions (`exists`), while retaining hidden-entry/pattern/search semantics. It is shorter and preserves the appended workspace root after bounded conversation. No classifier deadline/output increase, provider fallback, reasoning override, download, or intent heuristic is used. A revised synthetic request completes in 23.50 seconds (253 prompt/320 completion, 272 reasoning), returning the correct `none` operation. These individual measurements establish compatibility, not a controlled performance improvement.

Diagnostics add classifier enter/exit UTC timestamps, elapsed time, a per-classification trace UUID, transport request ID, cancellation/error category and allowlisted provider metadata. AgentLoop separately logs classification/planning enter/exit, step, internal session identifier, tool-request flag and final gate outcome. IPC events carry the external run/session IDs and event sequence. Internal AgentRuntime session IDs differ from IPC chat IDs; do not join them by equality. Final-source `sentinel.agent.ipc.diagnostics` explicitly maps internal runtime session/turn IDs to external chat/run IDs, tool call IDs, event names and UTC timestamps. The first frozen batch predates that mapping; correlate its ordered isolated-run intervals and IPC sequence rather than claim a direct equality join.

No prompts, response bodies, source contents, credentials or private reasoning are emitted in these diagnostics. Probe artifacts contain only synthetic classification output and numeric reasoning counts, never reasoning text. Reported binding context and independently captured loaded context are separate facts; byte estimates are labelled estimates.

## Qwen output-budget control and final outcome

After all 15 Qwen task attempts and the two TUI controls, an independent minimal JSON transport control returns HTTP200 in **28.28 seconds**, `finish_reason=length`, **18 prompt / 511 completion / 529 total tokens**. All **511 completion tokens are reported as reasoning tokens**, and assistant content has zero characters. The bounded JSON contract fails even on the minimal request; a generic transport success from an earlier phase is not evidence that this 512-token classifier contract works. See `qwen-minimal-probe.json`. No private reasoning text is retained.

This directly demonstrates a default-reasoning/output-reservation mismatch for the observed Qwen request. It does not establish that every HTTP-less timeout has the same cause, or that more tokens, a different role, structured output, or a reasoning override would fix the workflow. None of those alternatives was silently enabled. Nemotron likewise spends most of its successful synthetic classifier output on reasoning, but can produce visible JSON within the same bound; its live reliability is still poor.

Final actual TUI task results: Nemotron classification 3/16; Qwen 0/15; correct requested user tasks 0/31. Four cancellation/reconnect controls make 35 journeys. Both exact production-classifier probes also fail at the deadline. The task-owned loaded state was restored to empty. Finite bounds and real gateway progression are verified; reliable read-only completion is NOT ACCEPTED.
