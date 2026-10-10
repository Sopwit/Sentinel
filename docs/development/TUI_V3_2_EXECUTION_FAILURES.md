# TUI V3.2 execution failures and reproductions

Evidence uses synthetic files and isolated portable daemons. Raw private model reasoning is neither needed nor exposed. See [evidence index](../reviews/agent-v32-2026-10-10/README.md).

## Fixed: rejected native call contaminates continuation

Before: `LlmAgentRuntime::nextStep` copied native calls and set `awaitingNativeResults_` before registered argument validation. A malformed call entered the repair request as an unanswered previous call even though no tool executed. Unknown native tools returned immediately instead of using bounded pre-execution repair.

Reproduce: `test_llm_agent_runtime nativeArgumentRepairDoesNotPublishUnexecutedCalls unknownNativeCallCanRepairBeforeAnyExecution invalidNativeCallsStopAfterTwoPlanningAttempts`. All three failed before the fix (`native-before.log`). These are explicitly scripted deterministic provider responses, not autonomous acceptance.

After: stage calls until the complete batch validates; commit native call/result state only on a valid decision. Unknown native calls get at most two planning attempts. Repair instructions retain native protocol, not JSON text. Invalid calls never reach permission/execution. A failed repair does not poison a later fresh request. No tool action is automatically replayed.

## Fixed: repeated read ends without one bounded planning repair

The baseline multi-file task recorded seven tools (glob followed by repeated reads) and failed with doom-loop detection after approximately 168 seconds. The guard correctly prevented false completion but offered no last planning-only repair.

After: a repeated successful read-file/glob/grep/list-directory gets one repair opportunity per run, in both sync and async paths. The rejected repeated call is not executed; the model must use existing observations, obtain genuinely missing evidence, or explain the blocker. The next repeat still stops. Mutations and failed-call cycles retain their existing behavior. Tests assert one execution despite three planning decisions, both recovery and bounded failure, and no write repair. Final evidence/approval checks remain authoritative.

## Fixed: detailed-tool budget hides late capabilities

`ContextEngine` limits detailed tool contracts. With QMap/ID ordering, later tools can disappear from the text planner context even though the runtime exposes them. A small complete built-in index now advertises registered IDs and argument names/required markers before the budgeted detailed rows. Registry validation still supplies authoritative type/default/constraint errors before execution. Large extension catalogs remain bounded and may be omitted explicitly; no unbounded prompt expansion.

## Fixed: active TUI attachment loses task text

Actual PTY attachment showed Agent/running with an empty conversation despite an active user task. `messages` replies were ignored while active. Restore the latest user task and prior messages once on active attachment; retain newer snapshot/delta output and exclude stale partial assistant history. Another active run resets the old split transcript. New-run events request canonical task history where no local draft was submitted. Regression covers task/history restoration, no stream overwrite, no duplication and system notices remaining separate.

## Improved: grounding failure diagnosis

Repeated final rejection previously returned only a generic inability message. It now includes the controlled EvidencePolicy reason. This is policy metadata, not model reasoning/raw output. A test verifies missing evidence remains failure with no final answer or tool effects.

## Observed model/provider failures retained honestly

The unmodified baseline natural source-change task made eleven inspection calls, then emitted `MalformedResponse: Chat completion returned no content or tool calls`. No source diff or successful mutation occurred. The baseline verification task exceeded the 300-second observer bound, received an explicit cancellation request, and was not marked complete. These traces do not prove that LM Studio itself is defective. The V3.1 refused endpoint was reachable again in V3.2; no personal server/configuration was changed and no cause for the earlier outage was established.

Final per-task results and remaining failures are recorded in the [matrix](TUI_V3_2_AGENT_TEST_MATRIX.md). Deterministic transport/tool tests and real-model journeys have separate labels; no passing fixture is counted as live autonomy.

## Live partial mutation and context failure

The post-fix source task obtained one exact-target edit-file approval, changed only `math.h` from `a-b` to `a+b`, and then failed with provider `CapabilityUnsupported` / HTTP 500 “Context size has been exceeded.” The real diff and unrelated-file preservation are captured. This is a partial mutation, not completed Agent work; no write retry or model/server change was attempted. The catalog advertised context 1048576, which does not attest the loaded instance capacity. Post-fix multi-file analysis failed with an explicit missing-fresh-evidence reason for `math.h` despite six completed read calls; public IPC does not expose enough redacted arguments/results to attribute that mismatch safely.
