# V3.2.3 Agent acceptance

Status: **NOT ACCEPTED** for complete coding autonomy. The initial ten relay-observed A–E real TUI observations finished without an accepted final answer; final-source direct controls are separated below. No unit-test result establishes autonomous acceptance.

## Method and authority

The actual release Rust TUI starts each task in Agent mode through typed IPC. A private, mode-0700 portable daemon profile and a new disposable C++20/CMake workspace are used for every model, with a separate project and session per journey. No user project is edited. The model chooses tools; the observer does not prescribe a successful tool sequence or replace model responses.

Only calc.cpp/test.cpp writes may be approved through the actual TUI y/n UI. Each of the three named verification commands has at most one exact resource-hash approval; other/repeated commands are denied. No authority, executable scope, backend tool contract, provider fallback or uncertain-write retry is introduced. An accepted AgentLoop final answer is required for completion. Changed files without accepted build/test evidence remain partial.

A–E use identical goals, fixtures, 8192 context, four parallel slots and native output budget 1024. Nemotron GGUF Q4_K_M and Qwen MLX 4bit are different model/engine implementations; these are workflow observations, not a controlled speed or competitor-parity benchmark. Cold loading and background developer builds also affect timings.

The comparison uses the same frozen, instrumented daemon binary for both models. The HTTP-status propagation and directory-description corrections were discovered afterward and receive deterministic regression validation separately. The frozen binary incorrectly reports HTTP0 for successful native calls; the independent byte-preserving relay records actual HTTP200. Zero there is not evidence of failed transport. Cancellation outcomes labelled transport_failure by that earlier instrumentation must be interpreted together with their Cancelled category and actual terminal event.

## Journeys and evidence

A reads/explains calc.h, calc.cpp and test.cpp without mutation. B requests the implementation and test edits plus read-back review. C adds real configure/build/CTest/final evidence. D supplies a genuinely failing host-baseline test before asking the Agent to repair both files and verify. E requests six reads and exact synthetic markers under the same context window. An observer bound of 240 seconds cancels through Ctrl+C and records the terminal event; observer-timeout is not completion.

D's host configure/build pass and CTest exit 8 establish a failing fixture only. They never count as Agent build/test verification. F (cancel an active authorized build and inspect descendants) is BLOCKED on macOS because the real gateway cannot safely launch that build under existing confinement. Cancelling an inference request is not F.

Raw evidence lives in [phase evidence](../reviews/agent-v323-2026-10-10/README.md): before/after SHA-256 manifests and synthetic file contents, actual diffs, approval decisions, public tool/run events, authoritative history/change queries, loaded-model snapshots and allowlisted diagnostic metadata. Raw ANSI captures come from a 120×40 PTY. PNG/TXT files are bounded VT replays, not physical terminal screenshots.

Physical Ghostty, macOS Terminal, iTerm2, Linux terminals, tmux/SSH are NOT VALIDATED in this phase. There is no available Linux runner; no Linux success is inferred from source inspection.

## Completed model matrix

| Journey | Nemotron-3-Nano-4B | Qwen3.5-9B |
|---|---|---|
| A — multi-file explanation | FAIL: five completed read-file results, then doom-loop protection; no accepted explanation | NOT ACCEPTED: 240-second observer bound, run.cancelled, zero tool results |
| B — implementation + test edits | FAIL: five completed reads, doom loop; neither file changed | NOT ACCEPTED: observer bound, run.cancelled, zero tool results/edits |
| C — edit/build/test/final | FAIL: four failed discovery/read results and two completed results, then local estimated-context refusal; no request sent for the rejected continuation | NOT ACCEPTED: observer bound, run.cancelled, zero tool results/edits |
| D — recover a real failing test | NOT ACCEPTED: four failed discovery/read results, observer bound and run.cancelled; no edits or Agent verification | NOT ACCEPTED: observer bound, run.cancelled, no tool results/edits |
| E — context pressure | PARTIAL safety only: six completed reads and a native content reply, but no accepted final; observer cancellation | NOT ACCEPTED: observer cancellation, no tool results |
| F — cancel an authorized running build | BLOCKED by macOS execution confinement; not simulated | BLOCKED by the same confinement; not simulated |

No write/process approval was requested in either matrix. The observer did not manufacture approvals or edits to make a journey progress. All original fixture files, including unrelated.txt, have identical before/after hashes in all ten cases. No run.completed was observed. N's D and Q's D host-baseline CTest each exited 8 after successful host configure/build; neither is Agent evidence.

Nemotron produced 26 public tool results, of which 18 completed and eight failed. Its C refusal records estimated input 7208 + reserved output 1024 > loaded window 8192, transport_attempted=false, and no actual usage for that unsent request. This is a conservative local estimate, not measured provider exhaustion. Repeated reads with valid JSON keys remain an unresolved model/runtime continuation/grounding usability problem; they are not proof of a broken JSON parser.

Qwen produced no tool result or completed response in the response-only relay log. Its zero native diagnostics do not establish tool-format failure: the initial plain intent-classification path does not carry the new native metadata, and the relay cannot guarantee upstream cancellation. All five actual terminal events confirm run.cancelled after the observer bound. This is not a controlled model-capability ranking. Direct transport controls are recorded separately below; they do not replace missing Agent acceptance.

A-read, C-complete Nemotron and A-read Qwen final VT replays were visually inspected. They show the real failure/cancel state and preserved task/header/composer. The Qwen replay also exposes a stale cancellation-waiting notice after cancellation. Long notices can clip; native-host rendering and physical keys remain NOT VALIDATED.


## Final-source direct controls

After all developer builds and the initial model matrix finished, fresh sequential instances used the same final release daemon without a relay. Binary SHA-256 matches are recorded for both profiles. Both transport-only probes had already returned HTTP200/stop with a 128-token request limit; those probes are not Agent runs.

| Control | Actual result | Acceptance |
|---|---|---|
| Nemotron direct A | Six completed read-file results; all three target files observed; run.completed after 161 seconds; native metadata reports actual HTTP200; no changes/approvals | **PARTIAL**: authoritative accepted final contains file receipts and escaped excerpts, not the requested relationship/bug explanation |
| Qwen direct A | 240-second bound, actual run.cancelled after 241 seconds; zero tool results/native responses; no changes/approvals | **NOT ACCEPTED**: stalled before tool planning; direct B–E intentionally not attempted after this zero-progress gate |

The owned Qwen daemon stack pinpoints the wait in initial ObservationIntentPolicy classification through the plain LM Studio infer path. Thus this direct failure precedes tool validation, native continuation, edits, build and final grounding. Basic provider transport works, while completion of this classification request is unresolved. No finish reason or token count is invented for it, and no inference is made about Qwen's multi-file coding competence from these observations.

The plain local request has no serialized max_tokens and defaults to no client timeout. A dedicated bounded classification contract and metadata are needed; the native 1024-token limit is not active at this stage. No intent heuristic, silent model switch or unrestricted tool execution was added to bypass the wait.

Final direct before/after hashes and unrelated markers are unchanged. Both task-owned models were unloaded, returning the initially empty loaded-model state. No owned test daemon or diagnostic relay should remain; cleanup.json records the final inspection. Before/after actual PTY replays were inspected: the compact visual foundation and composer remain, but the file-receipt final is difficult to read and the cancelled Qwen notice remains stale. Physical hosts remain NOT VALIDATED.
