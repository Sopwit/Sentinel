# Phase 11 Checkpoint

Phase 11.7-11.9 completes the local AI usability checkpoint and runtime QA pass for the current
Desktop Alpha local Ollama path.

## Completed Local AI Scope

- Loopback-only Ollama health and installed-model metadata discovery.
- Explicit selected local model setting with discovered-model validation when metadata exists.
- Explicit local chat inference opt-in; disabled remains the default.
- Explicit local streaming opt-in; disabled remains the default.
- Guarded non-streaming local inference through `ILocalInferenceClient`.
- Guarded streaming local inference through `ILocalInferenceStreamClient`.
- Safe summaries for unavailable Ollama, missing model, invalid selected model, disabled local
  routing, disabled streaming, endpoint blocking, permission blocking, safety blocking, client
  unavailability, request failure, timeout, invalid response, cancellation, and malformed chunks.
- QML-safe runtime exposure through strings, string lists, booleans, counts, and chat-list models.
- Settings persistence for selected local model, local chat inference enablement, and streaming
  enablement.

## Current User Flow

- If Ollama is unavailable or no installed-model metadata is available, Settings shows unavailable
  runtime/model status and local chat inference refuses with a missing-model summary unless a
  selected/effective model can be resolved.
- If no model is selected and discovered models exist, the first discovered model is a fallback
  candidate. If no model can be resolved, local inference refuses before a client call.
- If the selected model is absent from discovered metadata, local inference and streaming are
  blocked before the local client is called.
- If local chat inference is disabled, chat stays on the existing local safe provider path and no
  Ollama prompt is sent.
- If local chat inference is enabled and streaming is disabled, chat uses guarded non-streaming
  local inference after model, endpoint, permission, and safety checks pass.
- If local chat inference and streaming are both enabled, chat uses guarded streaming only when
  the stream client is available and the same model, endpoint, permission, and safety checks pass.
- Permission or safety denial produces one safe assistant error message and no runtime client call.
- Successful fake non-streaming inference appends one assistant message.
- Successful fake streaming accumulates chunks into one final assistant message, clears the live
  preview after completion, and persists the final message once.

## Known Limitations

- Ollama must already be installed and running outside Sentinel.
- Sentinel cannot pull, delete, install, or manage models.
- No cloud providers, provider credentials, or API keys exist.
- No filesystem/system actions, subprocess launch, tools/plugins, autonomous loops, or external
  process ownership exist.
- Streaming is synchronous in the current controller path and has no user-facing cancellation
  control.
- Chat history remains one local transcript, not multi-conversation storage.
- Model recommendations and requirement summaries are static readiness metadata only.

## Safety Guardrails

- Ollama endpoint normalization and execution allow only local loopback HTTP.
- Local chat inference and streaming are opt-in settings and default to disabled.
- Runtime permission and safety metadata are evaluated before local inference or streaming.
- Known invalid selected models are refused before any local inference client call.
- Streaming live text is transient and cleared on completion, refusal, cancellation, or error.
- User and assistant chat messages are appended through the normal chat history boundary; local
  success and failure paths append exactly one assistant message.
- QML receives no raw clients, requests, responses, traces, model-management service objects,
  runtime policies, stores, paths, credentials, or endpoint internals.

## Phase 12 Readiness Criteria

- Keep the local AI path behind the same model, endpoint, permission, and safety gates.
- Keep selected model, local chat inference, and streaming persistence stable across settings
  reloads.
- Preserve duplicate-message protection for non-streaming, streaming, refusal, and error paths.
- Preserve QML-safe exposure and avoid raw core/runtime internals.
- Do not add model pull/delete/install, cloud providers/API keys, tools/plugins, subprocess
  launch, filesystem/system actions, or autonomous behavior unless Phase 12 explicitly scopes it.
- Re-run the full test preset and clang-format dry-run before starting any broader Phase 12 work.
