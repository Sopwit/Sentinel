# SENTINEL — CROSS-PROVIDER RESIDUAL CLOSURE REPORT

Certification date: 2026-10-03 (Europe/Istanbul).

## 1. llama.cpp Capability

- Provider capability: implements OpenAI-compatible native requests; not proof of every model's capabilities.
- Model capability: the selected existing NVIDIA-Nemotron-3-Nano-4B-Q4_K_M.gguf, alias `sentinel-nemotron`, supports native tools with its current Jinja template.
- Runtime: llama-server 0.5.0, build 11146, commit 7fe450e19; `http://127.0.0.1:8081`. Flags: `--alias sentinel-nemotron --ctx-size 8192 --jinja --reasoning off --host 127.0.0.1 --port 8081`. Existing GGUF under `/Volumes/NeuralSilicon/LLM/Model/lmstudio-community/NVIDIA-Nemotron-3-Nano-4B-GGUF/`; no download.
- `/v1/models` identifies the model but omits native-tool flags. `/props` identifies the same alias and reports `supports_tools=true`, `supports_tool_calls=true`, object arguments and parallel tools supported. Its Nemotron XML tool template serializes assistant calls and tool results. Effective context 8192; training context 1048576; approximately 3.97B parameters, Q4_K Medium.
- Chat: verified baseline PASS. Streaming: verified baseline PASS and binding Supported. Native tools: runtime and binding Supported. Reasoning: disabled by server flag; template preserves reasoning, effort control false. Embeddings: not advertised/validated. Binding has no separate reasoning/embedding capability fields; absence is not proof of unsupported behavior.
- Blocker: **B + C**: metadata exists on `/props`, but Sentinel's discovery only consumed `/v1/models`, leaving provider-default native Unsupported. Not a model-name/GGUF incompatibility.
- Fix: optional `/props` enrichment only for a single discovered model with exact matching `model_alias`. Both explicit native-tool flags required. Wrong identity, missing metadata, multi-model routers and failed optional metadata requests do not enable tools. Discovery flows through ModelService's runtime-reported provenance into the selected ModelBinding and Agent eligibility. No global override.

## 2. Direct llama.cpp Tool Probe

- Model: `sentinel-nemotron`.
- Initial request: Turkish workspace listing, native `list_directory(path)` function.
- Tool call ID: `DofZYJxjODJUvIepf3YajZ4aekF2rbLw`; arguments `{"path":"."}`; finish reason `tool_calls`.
- Continuation: actual directory result supplied under matching tool-call ID.
- Final: stop, workspace-listing acknowledgement. Direct protocol loop PASS; this acknowledgement alone was not counted as grounded Agent Completed.

## 3. Sentinel llama.cpp Agent

- Real production ModelService binding → LlmAgentRuntime → AgentRuntime/AgentLoop → ToolExecutionGateway → RealToolExecutor.
- Exact requested listing goal; planning, native tool request, Awaiting Approval, approve once, successful directory observation, native continuation, evidence-backed accepted final, **Completed**, one tool step.
- Final correctly references observed project directories/files and states hidden entries were excluded.
- Evidence obtained with opt-in `provider_certification_probe`, not a synthetic provider and not a GUI Completed claim. Only required read-only tools were exposed; no persistent grant was created. NullAgentRuntime supplies tool catalog metadata only; real planning and execution use LlmAgentRuntime and RealToolExecutor.
- Earlier real GUI run verified corrected native binding but failed before the planner-prompt fix. Post-fix desktop end-to-end Completed remains unverified.

## 4. Filesystem

- List: PASS / Completed, actual workspace root, 22 non-hidden entries.
- Read: PASS / Completed; actual CMakeLists.txt line 15 `project(Sentinel`, final project name Sentinel.
- Negative: PASS / Completed; real glob returned no matching `definitely-does-not-exist-12345.txt` under the workspace; final correctly says absent.
- All cases used real model requests, native tools, per-run approval and real observations. Grounding gates were not weakened. Local model answered some cases in English despite Turkish instructions; evidence correctness still passed.

## 5. Process / Sandbox

- pwd: NOT COMPLETED; real model failed to determine a grounded next action, zero executed steps.
- git: BLOCKED; native run-command reached approval and production sandbox, then `Sandbox denied launch (DetachedProcessControlUnavailable)`. Repeated attempts did not execute unsandboxed; run ended Failed after ten steps.
- Process probe tool contract narrowed `command` to the exact requested command, preserving production permission/sandbox policy.
- Actual separate production ProcessSandbox child evidence: `/bin/cat` of workspace CMakeLists.txt under macOS sandbox-exec Enforced exited 0, 3457 stdout bytes.
- Actual unauthorized existing `/Users/emir/.zshrc` read under the same production sandbox exited 1, stdout zero, stderr `Operation not permitted`. File contents never printed.
- This establishes real allowed/denied child access, not detached-child containment. No sandbox bypass or new fix to the prior dependency-access defect.

## 6. Permissions

- Ask: real Awaiting Approval → approve once → tool execution, both local and cloud.
- Allow: approved-once authorization admitted the requested filesystem operation; independent persistent Allow-policy runtime case not manufactured.
- Deny: earlier real outside-assigned-workspace filesystem calls were denied; actual external sandbox access denied. Independent explicit Deny-policy configuration not manufactured.
- Deterministic permission/gateway suites pass; policy-state runtime matrix remains partial.

## 7. Real Agent Completed

- llama.cpp / sentinel-nemotron: PASS for list, read and negative filesystem goals.
- Gemini / gemini-3.8-flash: PASS for requested workspace listing; real cloud model, one approved list-directory call, actual 33 entries including hidden, correlated continuation, Turkish grounded final, accepted Completed, one step.
- No fake/synthetic provider counted. Primary real Completed gate achieved twice across providers.

## 8. Cloud Readiness

| Provider | Configured | Credential |
| --- | --- | --- |
| OpenAI | not configured | absent |
| Claude | not configured | absent |
| Gemini | configured | present |
| DeepSeek | not configured | absent |
| Groq | not configured | absent |
| Mistral | not configured | absent |
| OpenAI-compatible cloud | not configured | absent |

Only credential presence was reported. Existing Gemini credential successfully discovered models and executed requests; no credential obtained/created. Gemini 2.5 Flash returned provider error that it was unavailable for new users; existing catalog's 3.8 Flash was explicitly selected, never silent fallback.

- Missing credential: existing automated negative-path coverage passes; ModelService rejects before construction/request, no empty fake key or local fallback. This is not Cloud Chat PASS.
- Real cloud Agent: PASS / Completed after Gemini wire-schema fix.
- Cloud streaming: PASS, three real deltas for 1–20 response.
- Cloud Chat first direct request: MAVI returned. Same-conversation controller gate NOT VALIDATED: initial helper waited on the unrelated localInferenceBusy property and was corrected; final continuity attempt failed discovery with ConnectionFailed. No false PASS assigned.
- Cancellation: NOT VALIDATED; attempted call returned RateLimited before cancellation could be established.
- Recovery: NOT VALIDATED; subsequent call failed parsing provider response. No silent provider/model switch observed.
- Cloud desktop UI selection/readiness end-to-end remains unverified.

## 9. Defects

1. llama.cpp `/props` model/template capability lost during discovery: FIXED at discovery owner, six deterministic metadata scenarios.
2. Native Agent prompt still demanded legacy JSON action format: FIXED with native-protocol-specific planning instructions; structured/legacy branch unchanged, native request/continuation assertions added.
3. Gemini native function declaration used limited `parameters` with full JSON Schema `additionalProperties`: FIXED to `parametersJsonSchema`, preserving authoritative tool schema; deterministic exact-schema preservation test. Real Gemini Completed verifies the fix. API contract: https://ai.google.dev/api/generate-content.

Remaining certification gaps: process Agent macOS detached-process enforcement, cloud cancellation/recovery/conversation/UI, independent runtime Allow/Deny policy states. No claims that these are fixed. Existing unrelated dirty-tree changes preserved.

## 10. Regression

- Full configured build: PASS.
- Final full CTest: **103/103 PASS**, 64.53 seconds, after all three production fixes.
- ApplicationController in final full run: **127 passed / 0 failed / 0 skipped**.
- `git diff --check`: PASS.
- An earlier full run had an unrelated conversation timestamp-order test failure; focused rerun and subsequent full runs passed. No test/production workaround made for that transient failure.
- Opt-in real-provider helper additionally built; not registered in CTest. Subsequent helper-only changes do not change production regression result.

## Certification

- LM Studio: existing Chat/stream/cancel PASS retained; Agent remains blocked by tested model behavior, not rerun to Completed.
- llama.cpp: capability FIXED; real filesystem Agent PASS / Completed; process matrix incomplete.
- Cloud: Gemini native Agent FIXED + PASS, streaming PASS; overall cloud matrix PARTIAL.
- Real Agent Completed: PASS (llama.cpp and Gemini).

**FINAL VERDICT: PARTIAL — primary closure targets achieved; full process/cloud/UI matrix not closed.**

Cleanup: temporary llama-server stopped. Desktop UI cleanup could not be confirmed because Computer Use timed out; the previously selected test endpoint 8081/provider selection may remain saved. No model or credential was downloaded, removed, or created.
