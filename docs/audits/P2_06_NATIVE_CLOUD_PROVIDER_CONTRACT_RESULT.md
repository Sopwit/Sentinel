# P2-06 — Native Cloud Provider Contract Result

Date: 2026-10-16 (Europe/Istanbul). Branch `test/p2-01-provider-contracts`, worktree HEAD `f27f893e7f32420ae28599cd182862f935847348`. Related acceptance: **AC-P2-01**; dependency **AC-P2-03** (real-backend parity, still blocked). **This task adds a safely constrained, injectable transport fixture for the native Claude (Anthropic Messages) and Gemini (`generateContent`) provider protocols and deterministic positive/negative contract cases. It does not contact any cloud endpoint, does not certify live compatibility, and does not satisfy AC-P2-01 or AC-P2-03.**

## 1. Scope, isolation and inputs

Continued in the exact supplied P2-01 worktree. Verified branch, HEAD and the pre-existing uncommitted P2-01…P2-05 tree before editing; nothing was reset, stashed, committed, pushed or merged, and no other worktree (including `integration-p1-01-7-bounded-planner`) was touched. All prior P2-01…P2-05 work is preserved.

Read [AGENTS](../../AGENTS.md), the P2-01…P2-05 audit results, the [acceptance matrix](../roadmaps/ACCEPTANCE_MATRIX.md) (AC-P2-01/AC-P2-03), and inspected `ModelService::nativeEndpointReply` / `SelectedEndpointChatProvider` / `providerConfig`, `LMStudioLocalInferenceClient::completeNativeChat` / `infer` / `LMStudioLocalInferenceStreamClient::startStream`, `ProviderRequestRuntime`, `NetworkPolicyService` and the existing provider fixtures.

No real cloud request, credential, personal configuration or model acquisition was used. Every fixture runs against an in-memory transport and uses synthetic credentials/markers. No AgentRuntime, AgentLoop, LlmAgentRuntime, P1 reliability test, Rust IPC, legacy CLI or GitHub workflow file was modified.

Explicitly **not** attempted: OpenAI/DeepSeek/Groq/Mistral certification, live cloud compatibility, or any claim that structural JSON acceptance equals full JSON Schema validation.

## 2. Transport injection design

The change adds one narrow seam beneath the existing request builders instead of a second registry, router or HTTP stack.

- **New interface** — [LocalInferenceTransport.h](../../core/include/sentinel/core/runtime/LocalInferenceTransport.h): `ILocalInferenceNetworkFactory` with `std::unique_ptr<QNetworkAccessManager> createManager() const`. The default implementation is the existing Qt transport.
- **Configuration seam** — [LocalInference.h](../../core/include/sentinel/core/runtime/LocalInference.h): `LMStudioConfig` gains `std::shared_ptr<ILocalInferenceNetworkFactory> networkFactory`, **empty by default**.
- **Threading** — [LocalInference.cpp](../../core/src/runtime/LocalInference.cpp): `postJsonOnce`/`postJson`/`executeJsonRequest` accept the optional factory and fall back to `std::make_unique<QNetworkAccessManager>()`; every call site that issues a provider POST (`completeOpenAiChat`, `completeNativeChat`, and the `infer` cloud branches for Anthropic/Gemini and the OpenAI-compatible path) passes `config_.networkFactory`; `LMStudioLocalInferenceStreamClient::startStream` uses the same factory.
- **Service seam** — [ModelService.h](../../core/include/sentinel/core/model/ModelService.h)/[.cpp](../../core/src/model/ModelService.cpp): `ModelService::setProviderNetworkFactoryForTesting(...)` stores the factory, and `providerConfig(...)` copies it into the constructed `LMStudioConfig` (still empty in production).
- **Test fixture** — [ScriptedProviderTransport.h](../../tests/support/ScriptedProviderTransport.h): `ScriptedNetworkAccessManager` records the observed URL/method/headers/body and returns a `ScriptedNetworkReply` synthesizing HTTP status, raw headers and `readyRead` frames; `ScriptedNetworkReply` supports `abort()` (so caller deadlines/cancellation fire) and can hold a reply open with no terminal frame. `ScriptedNetworkFactory` implements the interface. `ScopedOnlineNetworkMode` temporarily raises `NetworkPolicyService` to `Online` so the **production** host/credential gate is exercised for the real provider host names — the injected transport still issues no traffic.

Because the real request builders and the existing `QNetworkReply`-based parsing run unmodified, the fixtures observe the exact intended URL, headers and serialized body rather than a re-implementation of them.

## 3. Security and production-host safeguards

- **Endpoint/host validation unchanged.** `LMStudioConfig::isAllowedEndpoint()` still requires loopback HTTP or a non-empty API key for cloud; `completeNativeChat` still requires the exact `api.anthropic.com` / `generativelanguage.googleapis.com` hosts and builds the canonical `https://…` URLs; `infer` keeps its cloud branches.
- **Network policy still primary.** `postJsonOnce` runs `NetworkPolicyService::check(url)` **before** any manager is created. With the injected factory installed but policy at `LocalOnly`, `injectedTransportCannotBypassNetworkPolicy` shows a Claude request is refused (`Offline`) with **zero** transport records.
- **No redirection or interception.** No DNS override, no redirect policy change, no TLS-verification disable, no proxy/hosts edits; fixtures open no socket and bind no port.
- **No second stack.** No second provider registry, router or HTTP stack was introduced; the default path is the pre-existing Qt transport.
- **Test-only by construction.** Production never calls `setProviderNetworkFactoryForTesting`, and `LMStudioConfig::networkFactory` defaults to null (`defaultTransportRemainsProductionHttps` asserts the null factory, the loopback default, the cloud credential requirement and the HTTPS endpoint).
- **Credential hygiene.** Synthetic keys live only in the in-memory credential store; fixtures assert the key never appears in the serialized body, the error projection or the diagnostics (`claudeMessagesRequestConstruction`, `geminiGenerateContentConstruction`, `nativeSafeErrorProjection`).

## 4. Claude (Anthropic Messages) operation matrix

| Advertised operation | Positive case | Negative case(s) | Result |
| --- | --- | --- | --- |
| Messages request construction | `claudeMessagesRequestConstruction` (POST `https://api.anthropic.com/v1/messages`, `x-api-key`, `anthropic-version`, `model`, `messages`, `max_tokens=2048`) | — | Supported |
| Text response parsing | same case (`content[].type=="text"`) | `claudeInvalidToolUseRejected(missing-content/empty-content)` | Supported |
| Tool schema (`tools[].input_schema`) | `claudeToolUseSchemaAndIdentity` | — | Supported |
| Tool identity (name ↔ `toolId`) | `claudeToolUseSchemaAndIdentity`, `claudeToolNameNormalization` (hash for invalid ids, maps back) | `claudeInvalidToolUseRejected(unknown-tool)` | Supported |
| Tool arguments | `claudeToolUseSchemaAndIdentity` (`input` object) | `claudeInvalidToolUseRejected(non-object-input)` | Supported |
| Tool-call id | `claudeToolUseSchemaAndIdentity` (`id`) | `claudeInvalidToolUseRejected(empty-call-id)` | Supported |
| Continuation formatting | `claudeToolContinuationFormatsResults` (`tool_result` with `tool_use_id`/`content`) | `claudeContinuationRejections` (8 modes) | Supported |
| Continuation guards | `claudeContinuationRejections(mode7)` control | size mismatch, unregistered tool, invalid JSON history, non-array history, empty-array history, differing histories, `callId` mismatch | Supported (typed `MalformedResponse`) |
| Structured output | `claudeStructuredOutputContract(object)` (`output_config.format.json_schema`) | `…(not-json)`, `…(array)` | Supported when capability `Supported` |
| Capability gate | — | `claudeCapabilityRejectionBeforeTransport` (structured default `Unknown` rejects before transport) | Supported |
| HTTP error classes | — | `nativeHttpFailureClassification(claude-*)` (400/422/501→CapabilityUnsupported, 401/403→Auth, 404→ModelNotFound, 408→Timeout, 429→RateLimited, ≥500→ProviderUnavailable) | Supported |
| Cancellation | — | `claudeCancellationBeforeDispatch` (no transport) | Supported |
| Request deadline | — | `claudeDeadlineTimeout` (`TimedOut`/`Timeout`) | Supported |
| Streaming text + terminal | `claudeStreamingTextAndTerminal` (`content_block_delta` text, `message_stop`) | `claudeStreamingUnterminatedIsRejected` (no terminal → `MalformedResponse`) | Supported |
| Non-native cloud chat | `cloudChatRequestConstruction(claude)` | same HTTP matrix | Supported |
| Tool-use with no native request | `claudeInvalidToolUseRejected(text-only)` | `claudeInvalidToolUseRejected(tool-use-without-native)` | Supported (rejected) |

## 5. Gemini (`generateContent`) operation matrix

| Advertised operation | Positive case | Negative case(s) | Result |
| --- | --- | --- | --- |
| `generateContent` construction | `geminiGenerateContentConstruction` (POST `/v1beta/models/<model>:generateContent`, `x-goog-api-key` header, `model` in URL only, `contents[].parts[].text`) | — | Supported |
| Function declaration schema | `geminiFunctionDeclarationSchema` (`tools[0].functionDeclarations[].parametersJsonSchema` equals public `geminiFunctionDeclaration`) | — | Supported |
| Function-call response | `geminiFunctionCallIdentity` (`functionCall.name`/`args`) | `geminiMalformedResponseRejected(unknown-function/missing-name/args-not-object)` | Supported |
| Synthesized call id | `geminiFunctionCallIdentity` (`sentinel-gemini-<requestId>-<n>`) | — | Supported |
| gemini-3 signature | `geminiSignatureRequirement` (signed success control) | `geminiSignatureRequirement` (missing `thoughtSignature` → `MalformedResponse`) | Supported |
| Continuation formatting | `geminiFunctionContinuationFormatsResults` (`functionResponse.{name,response.output}`, explicit `id` forwarded, synthesized `id` omitted) | `claude`-parity guards apply through the shared `MalformedResponse` rejections | Supported |
| Structured output | `geminiStructuredOutputContract` (`generationConfig.responseMimeType`+`responseSchema`) | `geminiCapabilityRejectionBeforeTransport` (default `Unknown` rejects) | Supported when capability `Supported` |
| Blocked / safety | `geminiMalformedResponseRejected(valid-text)` control | `geminiBlockedOrSafetyRejected` (`promptFeedback.blockReason`, `finishReason` `SAFETY`/`PROHIBITED_CONTENT` → `RequestRejected`) | Supported |
| Malformed responses | `geminiMalformedResponseRejected(valid-text)` | empty, no-parts, non-object args, unknown function, missing name → `MalformedResponse` | Supported |
| HTTP error classes | — | `nativeHttpFailureClassification(gemini-*)` (same matrix) | Supported |
| Cancellation | — | `geminiCancellationBeforeDispatch` (no transport) | Supported |
| Request deadline | — | `geminiDeadlineTimeout` (`TimedOut`/`Timeout`) | Supported |
| Streaming text + terminal | `geminiStreamingFinishReasonTerminal` (`streamGenerateContent?alt=sse`, non-empty `finishReason` terminal) | `claude`-parity streaming negative | Supported |
| Non-native cloud chat | `cloudChatRequestConstruction(gemini)` (`?key=` query auth, `generationConfig.maxOutputTokens`) | same HTTP matrix | Supported |

## 6. Request/response fixture evidence

The fixtures assert on what the production builders actually transmitted and accepted:

- **Observed requests** are recorded as `{url, method, lowercased headers, body}` by the scripted manager, so headers (`x-api-key`, `anthropic-version`, `x-goog-api-key`, `content-type`) and bodies (`tools[].input_schema`, `functionDeclarations[].parametersJsonSchema`, `tool_result.tool_use_id`, `functionResponse.response.output`, `output_config.format.json_schema`, `generationConfig.responseSchema`) are compared field-for-field.
- **Responses** are synthetic frames delivered through `readyRead`; streaming parsers see real incremental frames, and an unterminated or absent payload drives the caller deadline/`abort()`.
- Secret markers (`PRIVATE_*`, the synthetic keys) are asserted absent from `errorMessage` and serialized `diagnostics`.
- The key placement difference is explicitly verified: the **native** `generateContent` path authenticates with the `x-goog-api-key` header, while the **non-native** cloud chat/infer branch uses the `?key=` query parameter (pre-existing behavior, unchanged).

## 7. Test counts

[test_native_provider_contract.cpp](../../tests/core/test_native_provider_contract.cpp) is registered in [tests/CMakeLists.txt](../../tests/CMakeLists.txt).

| Metric | Count |
| --- | --- |
| QTest cases (excluding `initTestCase`/`cleanupTestCase`) | **74** |
| Positive/behavior-control cases | **20** |
| Negative/rejection/failure cases | **54** |
| Total invocations reported by the binary | **76 passed, 0 failed** |

Data-driven coverage: Claude invalid tool-use (9 rows), Claude continuation rejections (8 rows), Claude structured output (3 rows), Gemini blocked/safety (3 rows), Gemini malformed (6 rows), cloud chat construction (2 rows), HTTP classification (2 providers × 10 statuses = 20 rows), safe-error projection (2 rows).

## 8. Preserved P2-01…P2-05 regressions

No prior work was altered or discarded. The pre-existing uncommitted P2-05 bounded-streaming changes (`IChatProvider.h`, `OllamaChatProvider.h/.cpp`, `LocalInference.h`, `ModelService.{h,cpp}`, `LocalInference.cpp`, `tests/support/DeterministicChatFixture.h`, the untracked `test_bounded_streaming_contract.cpp` and `P2_05_…md`) remain in the tree. Notably:

- `test_provider_contracts` (P2-01/P2-02/P2-03/P2-04 plus `nativeCloudProtocolCannotBeRedirectedToLocalMock`) — **205 passed, 0 failed**.
- `test_bounded_streaming_contract` (P2-05) — **18 passed, 0 failed**.
- `test_local_inference` — **51 passed, 0 failed**.
- Full `ctest` — **130/130 passed** (169.93 s), including Agent/runtime, IPC and desktop suites.

## 9. Validation results

Host: macOS / Darwin arm64; Homebrew LLVM 23.1.3; CMake/Ninja `tests` preset; Qt 6.12. One host's synthetic fixture evidence, not native Linux/Windows or backend certification.

| Check | Result |
| --- | --- |
| `cmake --build --preset tests` | Full build passed |
| `ctest --preset tests --output-on-failure` | **130/130 targets passed**, 169.93 s |
| `test_native_provider_contract` | 76 passed, 0 failed |
| `test_provider_contracts` | 205 passed, 0 failed |
| `test_bounded_streaming_contract` | 18 passed, 0 failed |
| `clang-format` **18.1.8** (CI-pinned) changed-line `clang-format-diff.py` gate + `--dry-run --Werror` on the three new files | 0 violations / 0 changes |
| Scoped clang-tidy on the three changed core TUs (PCH stripped via `tools/analysis/compile_database.py`) | 3/3 exit 0, **0 errors**; only pre-existing `readability-braces-around-statements` and `performance-*` warning debt |
| `python3 tools/ipc/generate.py --check` | Passed, no generated change |
| `python3 tools/ipc/generate_desktop.py --check` | Passed, no generated change |
| `python3 tools/analysis/check_personal_paths.py` | 0 files flagged |
| `git diff --check` | Passed (no whitespace errors) |

A local clang-format 23.1.3 was used first to normalize changed lines, then the CI-pinned 18.1.8 was run and reports the same clean result; the new/changed files are stable under both.

Changed files and SHA-256 at this candidate:

| File | SHA-256 |
| --- | --- |
| core/include/sentinel/core/runtime/LocalInferenceTransport.h (new) | `0b52dfc818fe4646217de33564d81be9abb9403cebfbd84be866cfa31eebeef6` |
| core/include/sentinel/core/runtime/LocalInference.h | `4a0fb643608834449f9adb74b619de5060328965a3bfe8e17469b76ca80f2264` |
| core/include/sentinel/core/model/ModelService.h | `c3b21a7b903dfea78e862e2f74a7fc7e92826292fac6a520c9df8cc5f3d391a0` |
| core/src/runtime/LocalInference.cpp | `86ac7ab1fddcdc524a096d5df1eec0b5b2e7e3858cf1d9a1f2756b560942b98a` |
| core/src/model/ModelService.cpp | `10840df5f46e0cef7b32139d78bceb6d2a21d16e9ec9377d3ad5d65374e3dea1` |
| core/CMakeLists.txt | `074945750e71607484e4c550323eca02e2d3bba73f4f69e18d0c236dad238855` |
| tests/CMakeLists.txt | `3927f837c7d491bd8bb1b16318cbe59f0642cfcc25765f43dac4bd7b54d39a18` |
| tests/support/ScriptedProviderTransport.h (new) | `33772cc7e744e0679f16b5d7a17bba1662f53ec2c3ca8f293e348148ec73caab` |
| tests/core/test_native_provider_contract.cpp (new) | `fa007ab3d56daf6586c7956a3281c768504d910214ebc7e356790571aa2b5972` |

## 10. Unsupported operations and remaining vendor gaps

Not implemented or not verified (no live claim):

- **Bounded/streaming native tool calls.** The streaming parsers extract text only; function/tool calls in a streamed native response are not assembled. The bounded-streaming overload rejects native/structured modes (P2-05 behavior preserved).
- **Full JSON Schema validation.** Native tool schemas are passed through and compared for exactness, but acceptance of a response is **structural JSON** only — this is not cryptographic or full JSON Schema validation.
- **Signature scope.** The gemini-3 `thoughtSignature` requirement is enforced for the **first** function call only (`result.toolCalls.isEmpty()` guard).
- **Finish reasons.** Gemini `finishReason` values other than `SAFETY`/`PROHIBITED_CONTENT` (e.g. `MAX_TOKENS`, `RECITATION`) are not specially surfaced.
- **Anthropic-specific content.** `thinking`/`redacted_thinking`, citations, prompt caching and image parts inside `tool_result` are not modeled.
- **Non-native Gemini credential placement.** The `infer` cloud chat branch sends the API key as a `?key=` query parameter (pre-existing); this task records but does not change that.
- **Other cloud providers.** OpenAI/DeepSeek/Groq/Mistral native paths were not fixture-certified here, per scope.
- **No live cloud, TLS, proxy, DNS or rate-limit behavior was exercised**, and no real provider response shape is claimed.

## 11. AC-P2-01 assessment

[AC-P2-01](../roadmaps/ACCEPTANCE_MATRIX.md) requires per-advertised-operation positive/negative API cases, Unknown capability remaining Unknown, and secrets absent from projection. This task adds deterministic positive/negative transport-level cases for the native Claude and Gemini advertisements, keeps the structured-output capability `Unknown` (not silently promoted) and shows secrets never enter the projection. **AC-P2-01 remains Unverified / not satisfied**: real-backend parity, live acceptance, complete per-provider matrices and the remaining vendor gaps above are outstanding, and **AC-P2-03 remains Blocked**. No acceptance threshold, matrix cell or phase status was changed.

## 12. Recommended next task

Extend the same injected-transport fixtures to the remaining OpenAI-compatible cloud request builders (`openai`/`deepseek`/`groq`/`mistral` `completeOpenAiChat` and their error projection) as **synthetic construction/classification cases only**, explicitly without certification. In parallel, with separate explicit authorization and credentials, plan an AC-P2-03 live-acceptance task for the Claude/Gemini subsets that reuses these exact fixture expectations as the oracle. Do not claim live compatibility, full JSON Schema validation or AC-P2-01/AC-P2-03 from this synthetic evidence.
