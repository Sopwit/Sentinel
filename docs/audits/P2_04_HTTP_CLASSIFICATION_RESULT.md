# P2-04 — Native HTTP Error Classification Consistency Result

Date: 2026-10-16 (Europe/Istanbul). Branch `test/p2-01-provider-contracts`, worktree HEAD `c9b99932ecd64df5c5358dd03e8d3991f0e1b6df`. Related acceptance: **AC-P2-01**. **The HTTP 408/429 native classification inconsistency is corrected for the reachable provider paths. AC-P2-01 remains Unverified / not satisfied.**

## 1. Scope, isolation and inputs

Continued in the exact supplied P2-01 worktree. Verified branch, HEAD and a clean entry tree before editing: the supplied base `0245e5fa94584a357a6b1de01344f69b577e0e41` already carries the committed P2-01/P2-02/P2-03/P2-DEF-01 work as commits `8e1612c7` and `c9b99932`, so the actual worktree HEAD is `c9b99932` and there were no uncommitted entry changes to preserve. Personal-Brain was unavailable on PATH. No reset, stash, branch switch, commit, push or merge occurred; the original checkout and other worktrees were not edited.

Read [AGENTS](../../AGENTS.md), [P2-01](P2_01_PROVIDER_CAPABILITY_CONTRACT_RESULT.md), [P2-02](P2_02_PROVIDER_ERROR_REDACTION_RESULT.md), [P2-03](P2_03_VISION_CAPABILITY_ENFORCEMENT_RESULT.md), [P2-DEF-01](P2_DEF_01_MODEL_DISCOVERY_REMEDIATION_RESULT.md) and the [acceptance matrix](../roadmaps/ACCEPTANCE_MATRIX.md). Inspected ModelService, SelectedEndpointChatProvider, LocalInference, ProviderRequestRuntime, ProviderErrorClassifier, the discovery/execution outcome mappings and the existing native-tool/structured/vision fixtures.

No real cloud request, credential, personal configuration, model acquisition or runtime configuration change was used. All HTTP fixtures bind ephemeral IPv4 loopback ports and use synthetic markers/keys.

## 2. Root cause and affected paths

The transport classifier already maps HTTP 408 to `Timeout` and HTTP 429 to `RateLimited`:

`ProviderRequestRuntime::classify` in [ProviderRequestRuntime.cpp](../../core/src/runtime/ProviderRequestRuntime.cpp) checks `status == 408` and `status == 429` before the Qt network error code. The defect was in the **projection**, not the transport.

Qt reports `QNetworkReply::UnknownContentError` for HTTP statuses it does not map to a specific error code, including 408 and 429. `LMStudioLocalInferenceClient::completeOpenAiChat`/`completeNativeChat` copy that into `OpenAiCompletionResult::malformed` (`reply.networkError == QNetworkReply::UnknownContentError`). SelectedEndpointChatProvider then consulted `completion.malformed` **before** the transport-classified `completion.providerErrorCategory`, so:

- Native tool / structured request + HTTP 408 → `MalformedResponse` instead of `Timeout`.
- Native tool / structured request + HTTP 429 → `MalformedResponse` instead of `RateLimited`.

The ordinary (non-native) compatible path (`sendMessageWithToken` → `LMStudioLocalInferenceClient::infer`) used `providerErrorCategory` directly and was already correct, which is why the mismatch only appeared in the native/structured modes.

A secondary precedence weakness existed inside `ProviderRequestRuntime::classify`: the `error == UnknownContentError` fallback was evaluated before `status >= 500` and before the generic `status >= 400` bucket, so a validated HTTP error status could be overridden by the Qt network code. No exercised fixture reached that ordering (Qt maps 5xx to `InternalServerError`), but it contradicted the required status-before-network-error precedence.

Affected paths:

- [ProviderRequestRuntime.cpp](../../core/src/runtime/ProviderRequestRuntime.cpp) — `classify` precedence.
- [ModelService.cpp](../../core/src/model/ModelService.cpp) — `nativeEndpointReply` and the ordinary/native `completeOpenAiChat` projection.
- [test_provider_contracts.cpp](../../tests/core/test_provider_contracts.cpp) — fixtures/assertions.

## 3. Error-classification precedence

The corrected precedence is: **cancellation → explicit context/capability rejection → validated HTTP status / transport category → Qt network error fallback.**

`ProviderRequestRuntime::classify` now interprets the validated HTTP status (408, 429, 401/403, 404, 400/422, 501, ≥500) before consulting `QNetworkReply::UnknownContentError`, and only classifies `MalformedResponse` when the status is a success/absent status with an unparseable body.

SelectedEndpointChatProvider now orders the projection as: `cancelled` → native context/capability rejection (`contextRejected`/`rejected`) → `completion.providerErrorCategory` → `completion.malformed` → `ProviderRequestRuntime::classify`. The Qt `UnknownContentError` flag is a last-resort fallback and can no longer override a classified transport failure.

Preserved deliberately:

- Malformed **successful** HTTP responses (HTTP 200 with an unparseable body) remain `MalformedResponse`, because the transport category for a status < 400 with `UnknownContentError` is still `MalformedResponse`.
- 401/403 → `AuthenticationRequired`, 404 → `ModelNotFound`, 500 → `ProviderUnavailable`, 501 → `CapabilityUnsupported`, 400/422 → `RequestRejected` (native capability rejection retained).
- The native diagnostic `outcome` is now derived from the final category so a 408/429 no longer reports `response_format_failure`.
- Retry, deadline, cancellation and the bounded single-attempt behavior for bounded requests are untouched.

## 4. HTTP classification matrix

Categories are the typed `ChatProviderErrorCategory`. "Native" = native-tool and/or structured-output request mode through SelectedEndpointChatProvider.

| HTTP / fault | Body | Ordinary compatible | Native tool / structured | Lifecycle |
| --- | --- | --- | --- | --- |
| 408 | JSON or malformed | `Timeout` | `Timeout` (was `MalformedResponse`) | TimedOut |
| 429 | JSON or malformed | `RateLimited` | `RateLimited` (was `MalformedResponse`) | RateLimited |
| 401 | JSON or malformed | `AuthenticationRequired` | `AuthenticationRequired` | Failed |
| 403 | JSON or malformed | `AuthenticationRequired` | `AuthenticationRequired` | Failed |
| 404 | JSON or malformed | `ModelNotFound` | `ModelNotFound` | Failed |
| 500 | malformed body | `ProviderUnavailable` | `ProviderUnavailable` | Failed |
| 501 | JSON or malformed | `CapabilityUnsupported` | `CapabilityUnsupported` | Failed |
| 200 | malformed body | `MalformedResponse` | `MalformedResponse` | Failed |
| disconnect (no status) | — | `ConnectionFailed` | `ConnectionFailed` | Failed |
| deadline expiry | — | `Timeout` | `Timeout` | TimedOut |
| cancellation (token) | — | `Cancelled` | `Cancelled` | Cancelled |

`classify` precedence clarification also makes an unmapped 4xx (for example 405/409) a `RequestRejected` and a 5xx `ProviderUnavailable` instead of `MalformedResponse`; no exercised fixture previously depended on the old ordering.

## 5. Minimal correction

Only two production translation units change:

- `ProviderRequestRuntime::classify` reorders the validated-status buckets ahead of the Qt `UnknownContentError` fallback and simplifies the terminal return. No new category, router, registry or classifier is introduced; existing interfaces are reused.
- `SelectedEndpointChatProvider::nativeEndpointReply` and its ordinary/native sibling move `completion.providerErrorCategory` ahead of `completion.malformed`, keep the explicit native 400/422/501 rejection, and derive the diagnostic `outcome` from the final category.

No `LocalInference.cpp`, `OllamaRuntime.cpp`, `ProviderErrorClassifier.cpp`, AgentRuntime, AgentLoop, legacy CLI, Rust IPC, workflow, generated-contract or advertised-capability changes. `ProviderErrorClassifier` (the separate message/status helper) is untouched. No second error-classification system.

## 6. Before/after regression evidence

Deterministic reproduction used the extended provider-contract fixture with the new tests present and only the two production files reverted to HEAD:

| Stage | `test_provider_contracts` result |
| --- | --- |
| Entry (HEAD production + HEAD tests) | 153 passed, 0 failed |
| HEAD production + new classification tests | **191 passed, 14 failed** |
| Corrected production + new classification tests | **205 passed, 0 failed** |

The 14 reproduced failures were exactly:

- `publicErrorsAreAllowlisted(408)` and `publicErrorsAreAllowlisted(429)` — native mode expected the preserved old `MalformedResponse`; corrected fixture now asserts `Timeout`/`RateLimited`.
- `httpFailureClassification(mode1|mode2|mode3 × native-408, native-429, 408-malformed-body, 429-malformed-body)` — 12 rows.

The ordinary `mode0` 408/429 rows passed before and after, confirming the defect was confined to the native/structured projection.

## 7. Retry and cancellation preservation

- `classifierAndRetryContracts` still passes: 503 retryable for generation, not after stream output, not cancelled, not timed out.
- `discoveryContract` still makes exactly two discovery attempts for 429 and one otherwise; no retry was added.
- Every row of the new `httpFailureClassification` asserts `reply.attempts == 1` and exactly one fixture request for bounded requests, matching the existing single-attempt behavior.
- Disconnect, deadline expiry and cancellation rows assert `ConnectionFailed`, `Timeout` and `Cancelled` with non-Completed lifecycles; `test_local_inference` cancellation/disconnect/timeout and stream-cancellation cases still pass unchanged.

No automatic retry or backoff change was introduced.

## 8. P2-01 through P2-DEF-01 regression preservation

Rerun at the final candidate:

| Preservation target | Result |
| --- | --- |
| P2-DEF-01 malformed discovery (`malformedDiscoveryShapeIsNotHealthy`, `malformedDiscoveryPreservesCatalogSequencing`, discovery contract) | Passed |
| P2-DEF-02 vision enforcement (`unsupportedVisionIsRejectedBeforeTransport`, `visionGuardUsesFrozenBinding`) | Passed |
| P2-DEF-03 redaction (`echoedSecretsDoNotEnterDiagnostics`, `publicErrorsAreAllowlisted`, streaming error envelopes) | Passed |
| 72-row vision capability matrix | 72/72 passed |
| Frozen ModelBinding (`bindingAndEndpointRemainFrozenDuringRequest`) | Passed |
| Streaming error envelopes (`streamingErrorsNeverEmitPayload`) | Passed |
| Provider cancellation and deadlines (`test_local_inference` mock cancellation/disconnect/timeout, bounded plain request) | Passed |

No previous assertion was weakened or deleted. The only changed assertion is the documented P2-02 native 408/429 quirk, which now expects the corrected typed category instead of `MalformedResponse`; that expectation change is required by this task, not a suppression. Public errors remain bounded (<512 bytes) and free of the synthetic `PRIVATE_*` markers in errorMessage, diagnostics and captured Qt logs.

## 9. Validation results

Host: macOS 27.0.1 / Darwin arm64; Apple clang 21.0.0; CMake 4.4.4; Qt 6.12.0; clang-format/clang-tidy 23.1.3. One host's fixture evidence, not native Linux/Windows or backend certification.

| Check | Result |
| --- | --- |
| `cmake --preset tests -DSENTINEL_USE_CCACHE=OFF` | Configure valid (existing install) |
| `CCACHE_DISABLE=1 cmake --build --preset tests -j6` | Full build passed |
| `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | **128/128 targets passed**, 159.97 s |
| Direct `test_provider_contracts` | **205 passed, 0 failed** (203 case/data-row executions plus setup/cleanup) |
| Direct `test_local_inference` | **51 passed, 0 failed** (49 cases plus setup/cleanup) |
| New case/data-row coverage | 52 rows (4 request modes × 10 HTTP rows + 4 modes × 3 fault rows) |
| `git clang-format --diff HEAD` on changed lines | Empty (no newly introduced violations) |
| Scoped clang-tidy on all three changed TUs | Passed, no diagnostics: `clang-analyzer-*`, `bugprone-use-after-move`, `bugprone-dangling-handle`, warnings-as-errors, analysis-only database strips production PCH |
| `python3 tools/ipc/generate.py --check` | Passed, no generated change |
| `python3 tools/ipc/generate_desktop.py --check` | Passed, no generated change |
| `python3 tools/analysis/check_personal_paths.py` | 0 files flagged |
| `git diff --check` | Passed |

Formatting note: the repository has substantial pre-existing whole-file formatting debt relative to `.clang-format` (for example `AllowShortIfStatementsOnASingleLine: Never` versus the pervasive single-line `if (…) return …;` style). The changed lines were normalized with `git clang-format` on the diff only; unrelated formatting debt in the touched files was left untouched. This distinguishes existing debt from newly introduced violations.

Changed files and final bytes:

| File | SHA-256 |
| --- | --- |
| core/src/runtime/ProviderRequestRuntime.cpp | `02de4168a7d630160683d6dcc70473ddf25ecbf4468880cfde40d44f67761248` |
| core/src/model/ModelService.cpp | `b71c26d93974f0213b052475d42d55a0d37b1a9c4907cec9ff7f5b81dfcebb15` |
| tests/core/test_provider_contracts.cpp | `4e5ecb20c157ccf74dedbed786369369ae8d8ad44ef23fb24b55bf1f07e492f7` |

## 10. Remaining native Claude/Gemini and vendor fixture gaps

- Native Claude/Gemini cloud success, error, stream, tool and continuation wire formats still have no injectable transport seam: `completeNativeChat` hard-codes `api.anthropic.com`/`generativelanguage.googleapis.com` and rejects loopback substitution with a pre-transport 501 (`nativeCloudProtocolCannotBeRedirectedToLocalMock`). Their 408/429/401/403/404/500/501 classification therefore cannot be fixture-verified through the existing seam.
- Vendor-specific advertised-operation coverage for OpenAI/DeepSeek/Groq/Mistral (production URLs, catalog shape, streaming/tool/vision differences, auth semantics) remains unverified; `openai-compatible` still routes to the OpenAI endpoint, not an arbitrary configurable cloud URL.
- The precedence clarification for unmapped 4xx (405/409/etc.) and 5xx-with-`UnknownContentError` has no dedicated fixture, because Qt does not surface those combinations in the current harness.
- The native cloud endpoint 400/422 projection now consistently yields the native capability rejection instead of `MalformedResponse`; that path is host-gated and remains unexercised.
- No real-model or real-backend behaviour is claimed; a successful synthetic category is not proof of a loaded, capable, healthy or reliable model.

## 11. AC-P2-01 assessment

[AC-P2-01](../roadmaps/ACCEPTANCE_MATRIX.md) requires positive/negative cases for every supported advertised operation, Unknown remaining Unknown and secrets absent from projection. This task corrects one established inconsistency and adds deterministic typed regression evidence for the reachable native/compatible HTTP paths. **AC-P2-01 remains Unverified / not satisfied**: native Claude/Gemini fixture seams, vendor-specific advertised-operation coverage and real-backend acceptance are still outstanding, and no acceptance threshold, matrix cell or phase status was changed. This result does not by itself grant AC-P2-01 or any P2 phase completion.

## 12. Recommended next task

Add an approved injectable transport seam for native Claude/Gemini (without redirecting production DNS or contacting live cloud) and cover their success/error/stream/tool/continuation wire cases, then extend vendor-specific advertised-operation coverage for OpenAI/DeepSeek/Groq/Mistral before reassessing AC-P2-01. No cloud access, credential use or model acquisition is authorized by this recommendation.
