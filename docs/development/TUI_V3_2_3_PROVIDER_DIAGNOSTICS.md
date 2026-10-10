# V3.2.3 provider diagnostics

## Isolation and observations

The initial LM Studio catalog was reachable but all loaded_instances arrays were empty. Catalog presence was not reported as loaded inference readiness. Nemotron-3-Nano-4B was explicitly restored to the earlier 8192 context / four parallel slots. The user separately approved loading the already-installed Qwen3.5-9B at the same limits. No model download, silent fallback or context increase is allowed.

A temporary loopback relay forwards original request/response bytes to the same local endpoint and records only status, model IDs, byte counts, token usage/finish reason if supplied, and tool name/argument-key/path-shape metadata for disposable fixtures. Prompts, response content, reasoning, credentials and file contents are never persisted by the relay. This is diagnostic tooling, not a production tool executor or a model-response simulator.

The pre-change real TUI A journey received HTTP200 native calls with valid schema keys and eventually stopped under doom-loop protection. It did not reproduce the historical empty response. Historical V3.2.2 empty responses still have no captured finish reason; they are not retroactively attributed to output length.

## Production instrumentation

OpenAI-compatible native requests now expose allowlisted diagnostics on ChatProviderReply and opt-in Qt logging categories `sentinel.provider.diagnostics` and `sentinel.agent.diagnostics`. Enable local info logging with `QT_LOGGING_RULES='sentinel.provider.diagnostics.info=true;sentinel.agent.diagnostics.info=true'`. Both categories default to warning level. This is local diagnostic output, not telemetry, a settings source or an IPC permission extension.

Metadata includes bound provider/model ID, actual serialized request bytes, explicitly labelled input-token estimate and method, reported binding context capacity, requested output budget, continuation count, bounded observation-size list, HTTP status, attempt count, known finish reason, numeric usage/reasoning-token counts when present, and outcome/error category. Agent metadata adds history count, planner estimate, native protocol flag and recovery count. Missing usage is omitted. Unknown finish reasons are labelled other; provider-controlled text is not logged. The binding capacity may be published metadata if no loaded instance is known; actual loaded configuration is separately captured from the local model API for each comparison.

Native output length is a distinct bounded terminal rejection. A length-finished response does not authorize even apparently complete tool calls, does not increase output/context limits and does not trigger write replay. Normal empty replies remain distinct from malformed tool/structured output. Context HTTP rejections are distinguished from schema/capability rejection with bounded known error signatures; an unrelated tool name containing context is not enough.

## Regression evidence

- output-limit-before.log: a reasoning-only length-finished fixture was misclassified as an ordinary malformed/empty response; after fix it is output_limit / RequestRejected with no accepted tools.
- schema-before.log: read-file advertised a file/directory path despite its text-file-only handler. The model-facing path description now directs directories to list-directory and avoids invented /workspace aliases. Authoritative argument names, validation and access rules are unchanged. directory-guidance-before.log separately reproduces missing workspace-root guidance; list-directory now explains '.' and workspace-relative paths, while retaining separate authorization for external paths.
- http-status-before.log plus the real relay: successful HTTP200 was lost to HTTP0. Both postJsonOnce and completeOpenAiChat now retain the observed status, including actual statuses on malformed JSON. No status is fabricated as 200.

Deterministic native fixtures separately cover measured usage, missing usage, private-field exclusion, invalid tool JSON, partial calls at output limit, context rejection, unrelated schema rejection and continuation sizes. These are regression tests, not live model acceptance. Tool-schema repairs remain the existing bounded pre-execution replan; no uncertain write is automatically retried.


## Direct endpoint controls

Fresh, explicitly loaded instances at the same 8192/four-slot settings both returned actual HTTP200 / finish_reason=stop to the same bounded 128-token synthetic request. Nemotron reported usage 21 input/25 output; Qwen reported 14 input/90 output. Those are provider-supplied counts for a transport-only request, not Agent performance or tool-capability acceptance. No response/reasoning text was persisted.

The final-source direct Nemotron TUI A control records actual HTTP200 throughout the native C++ path, six completed reads and an AgentLoop-accepted final. Its user-facing final is an authoritative file receipt rather than the requested explanation; this exposes the existing filesystem-final presentation limitation. The earlier relay/background-load observation failed. Several variables and model sampling differ, so this is not evidence that status propagation or description text caused an autonomy improvement.

A non-sensitive stack sample of the owned final-source Qwen direct daemon isolates its wait to `AgentLoop::initializeObservationIntent → ObservationIntentPolicy::classify → SelectedEndpointChatProvider::sendMessageWithToken → LMStudioLocalInferenceClient::infer`. It is before native planning/tool execution. Only function frames were retained in qwen-direct-classification-stack.log, not locals, prompts, responses or reasoning. This rules out the relay as the sole explanation of that particular direct stall and does not establish why the inference backend delays this request.

The plain local body omits max_tokens although LocalInferenceOptions has a default field, and the default client timeout is zero. The native 1024-token bound has not been reached/applied in this classification stage. A dedicated bounded classification request contract, start/outcome metadata on the plain path and an explicit indeterminate/error outcome are required. Turning off reasoning, increasing loaded context, guessing intent or silently executing tools would not be justified by these observations.
