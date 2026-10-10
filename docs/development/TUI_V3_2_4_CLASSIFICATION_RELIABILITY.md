# V3.2.4 classification reliability

Status: **IMPLEMENTED + VERIFIED for finite client lifetime; successful read-task classification remains unreliable.** Date: 2026-10-10.

## Reproduction and root cause

The V3.2.3 owned Qwen stack isolates the wait to `ObservationIntentPolicy::classify → SelectedEndpointChatProvider::sendMessageWithToken → LMStudioLocalInferenceClient::infer`, before native planning. Source inspection confirms that classification only passed a cancellation token, the selected provider defaulted to timeout zero, and the local OpenAI-compatible body omitted `max_tokens`. A successful bounded transport probe did not establish classifier success. Personal-Brain CLI was unavailable; the user-supplied checkout and architecture/runtime/security/build/testing instructions were used.

## Contract and authority

- `ChatRequestOptions` carries a per-request deadline and output budget. Classification explicitly requests **30000 ms and 512 output tokens**, on the existing active binding.
- Selected endpoint and Ollama providers pass these bounds to local inference. The base provider rejects bounded capabilities it cannot implement rather than silently discarding them. Test doubles explicitly implement their immediate bounded contract.
- Local OpenAI-compatible plain bodies now serialize `max_tokens`. Length-finished output is rejected even if its visible text looks like complete JSON.
- A bounded JSON request has one transport attempt. It does not restart a full deadline through automatic retry/backoff. Unbounded existing requests retain their previous retry behavior. This intentionally changes transient retry behavior for bounded requests.
- Existing `ProviderRequestRuntime` aborts the client reply on deadline/cancellation. Deadline callbacks are subject to Qt timer/event scheduling, not hard real-time guarantees.
- Both synchronous and asynchronous Agent entry points terminate indeterminate classification before planning or tool execution. Timeout becomes a recoverable failed run; user cancellation retains the cancelled terminal category. Existing once-only cancellation protection remains.
- No intent heuristic, provider/model fallback, tool grant, shell path, model download, settings change or sandbox relaxation was introduced.

## Allowlisted observability

Opt-in categories are `sentinel.provider.diagnostics`, `sentinel.agent.diagnostics` and `sentinel.classification.diagnostics`. Plain local records include bounded provider/model identifiers, binding-reported context, output reservation, deadline, serialized request bytes, labelled UTF-8 byte estimate, elapsed time, actual HTTP status, safe error category, known finish reason, numeric usage when supplied, and classified/indeterminate outcome. Missing response usage is omitted. Unknown finish reasons are labelled unknown/other rather than inferred.

Request estimates are heuristics, not tokenizer measurements or evidence of provider exhaustion. Loaded context is independently captured from the local provider API. The classifier copies only allowlisted bounded scalar fields. No prompt body, response body, private reasoning, credential or file content is logged by these diagnostics. Existing provider request IDs are retained on replies. There is no confirmed upstream cancellation signal: `upstream_cancellation=unconfirmed` must never be interpreted as the provider stopping generation.

## Deterministic verification

Focused HTTP fixtures exercise a server that accepts but never responds, a length-finished response containing apparently valid classifier JSON, real HTTP200 retention and provider-supplied usage. The 100-ms stall exits finitely with Timeout and one attempt. Existing cancellation fixture exits finitely with Cancelled. Classifier options and schema parsing are tested separately. Both AgentLoop entry modes prove zero planner calls/tools after indeterminate classification; cancellation during initial classification remains terminal once.

These fixtures do not count as successful live-model reasoning or the complete real-TUI deterministic-stall journey.

## Live results

Both already-installed models were loaded sequentially in task-owned instances at API-reported **8192 context / four slots**, with private portable daemon profiles and disposable workspaces. The same frozen release daemon hash was used. No models were initially loaded; task instances were unloaded afterward and the empty loaded state restored.

Qwen's five real TUI journeys all terminate with failed/indeterminate classification. Recorded classifier elapsed times: **29738–30411 ms**, no HTTP response, no usage/finish reason, zero tool results. This satisfies the finite safe-failure alternative, not successful classification.

Nemotron's normal C classification returns **HTTP200 / stop in 8506 ms**, usage **392 prompt / 110 completion / 502 total**, 1734 serialized bytes, labelled estimate 578 tokens, output reservation 512. The complete Agent journey later answers `4` and emits run.completed at 58.11 seconds. Its four read-task classifications time out in **29819–29956 ms** and execute no tools.

The observed variability includes Qt scheduling and local inference conditions; no speed/model-capability ranking or upstream cancellation claim is made. A 30-second bound is effective for safety but fails these read requests. Further bounded classification reliability work is necessary before grounded live acceptance.

See [acceptance matrix](TUI_V3_2_4_ACCEPTANCE_MATRIX.md) and [raw metadata](../reviews/agent-v324-2026-10-10/summary.json).
