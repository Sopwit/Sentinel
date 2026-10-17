# P2-M02 — Shared OpenAI-compatible protocol extraction result

Audit date: 2026-10-17. Phase: P2 — Multi-Backend & Capability Negotiation.
Criteria: [AC-P2-01](../roadmaps/ACCEPTANCE_MATRIX.md) (Unverified), AC-P2-03 (Blocked),
[PD-D10](../roadmaps/ACCEPTANCE_DECISIONS.md) (Pending Decision).
**Behavior-preserving incremental extraction. No provider behavior, settings, credentials, network
policy, CI or workflow change. No commit, push, merge, reset, stash, PR or workflow dispatch.**

## 0. Isolation and baseline

| Item | Value |
| --- | --- |
| Branch | `refactor/p2-m02-openai-compatible-protocol` |
| Worktree | `/private/var/folders/.../T/opencode/p2-m02-openai-compatible-protocol` |
| Base | `6035bf74cbb3f2556e29fe1bf31163a56eb5b591` (`origin/main`, verified equal to local `main`) |
| Actual source SHA | `6035bf74cbb3f2556e29fe1bf31163a56eb5b591` |
| Entry worktree state | clean; no modified or untracked files at entry |
| Root AGENTS.md | read; provider/**AgentRuntime**/**ToolExecutionGateway**/platform boundaries and the `cmake --preset tests` validation contract applied |
| P2-M01 source | read from its existing worktree `T/opencode/p2-m01-provider-modularization` → `docs/audits/P2_M01_PROVIDER_MODULARIZATION_ARCHITECTURE.md` (worktree treated read-only) |

`origin/main` was already equal to local `main` at `6035bf74`; `git fetch origin` reported no new
commits. The P2-M01 worktree and the CI-05, P4-02, P4-03 and P3-04 worktrees were not modified.
This task performed no model download, no live provider call and no mesh/vendor API contact. All
new tests use deterministic in-process fixtures.

Build/verification ran in a fresh `cmake --preset tests` tree inside this worktree
(Qt 6.12.0, AppleClang 21, ccache, Ninja). A pre-refactor build and the focused provider suites were
captured before any source change (see §5, §7).

## 1. Scope and exact extracted operations

The extraction is exactly the P2-M01 §10 first step: move **pure, transport-independent**
OpenAI-compatible request path, request body, response text and model catalog logic into a dedicated
module. No HTTP transport, TLS/network policy, credential handling, deadline/cancellation, retries or
error classification was moved or duplicated.

Extracted operations:

1. **Chat URL/path construction** — the `/v1`-aware path rule used by four call sites.
2. **Request-body construction** — the OpenAI-compatible chat body schema (model, messages, stream,
   optional temperature, max_tokens) shared by the local, cloud and streaming paths.
3. **Response-text extraction** — `choices[0].message.content` extraction with the existing
   missing-choices vs missing-text distinction.
4. **Model catalog parsing and validation** — the `/v1/models` `data` array validation and per-model
   capability mapping, preserving the P2-DEF-01 malformed-catalog rejection.

Operations intentionally left in the original implementations:

* **Native Claude Messages / Gemini generateContent** request and response handling remains in
  `LocalInference.cpp` (`completeNativeChat`, the cloud `isAnthropic`/`isGemini` branches and the
  streaming branches) and in `ModelService::nativeEndpointReply`. Explicitly out of P2-M02 scope.
* **The primary chat body built by `SelectedEndpointChatProvider`** in `ModelService.cpp`
  (tool-call continuation, `tools`/`tool_choice`, `response_format` structured output, output-budget
  clamping) is untouched; only `LMStudioLocalInferenceClient::infer`/streaming and
  `completeOpenAiChat` path selection now route through the module.
* **Discovery transport, outcome/error mapping and HTTP** stay in `fetchOpenAiCompatibleModels`.
  Only the pure JSON validation/parse was extracted; the `getJson` call, `ProviderDiscoveryOutcome`,
  error text and cancellation are unchanged.
* **Gemini/Anthropic discovery parsers** (`fetchGeminiCloudModels`, `fetchAnthropicCloudModels`
  catalog shape) remain provider-specific in `OllamaRuntime.cpp`; they are not the OpenAI-compatible
  `data`-array format.
* **`LMStudioConfig`** was not renamed, moved or changed (public compatibility type).

## 2. New interfaces and dependency boundaries

New files:

| File | Lines | Role |
| --- | --- | --- |
| `core/include/sentinel/core/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.h` | 60 | pure declarations |
| `core/src/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.cpp` | 139 | pure implementations |
| `tests/core/test_openai_compatible_protocol.cpp` | 424 | golden + characterization tests |

Public surface (`namespace sentinel::core::provider`):

```cpp
QString openAiCompatibleChatCompletionsPath(const QUrl& endpoint);

QJsonObject buildOpenAiCompatibleChatBody(const QString& model, const QJsonArray& messages,
                                          int maxTokens, bool stream,
                                          std::optional<double> temperature);

enum class OpenAiCompatibleChatTextError { None, MissingChoices, MissingText };
std::optional<QString> parseOpenAiCompatibleChatText(const QJsonObject& body,
                                                     OpenAiCompatibleChatTextError* error = nullptr);

struct OpenAiCompatibleCatalogParse {
    QList<OllamaModelSummary> models;
    bool malformed = false;
};
OpenAiCompatibleCatalogParse parseOpenAiCompatibleModelCatalog(const QJsonObject& body);
```

Dependency boundary:

* `provider/protocols/openai_compatible` depends only on Qt JSON/URL types and the existing
  `OllamaModelSummary` DTO (`runtime/OllamaRuntime.h`). It opens no socket, reads no file, consults
  no settings and holds no credential.
* `runtime/LocalInference.cpp` and `runtime/OllamaRuntime.cpp` depend on the module; the module
  depends on neither. This matches the P2-M01 allowed direction (`protocols` ← callers); no
  `ModelService`/registry/adapter layer is introduced.
* `maxTokens` and the optional `temperature` are supplied already resolved by the caller, so the
  one legitimate provider difference (local clamps `max(1, n)`; cloud/streaming fall back to 2048;
  only cloud includes `temperature`) is preserved without inventing a new policy object.

## 3. Before/after behavior evidence

| Behavior | Before | After |
| --- | --- | --- |
| `/v1/chat/completions` vs `/chat/completions` | inline ternary ×4 | `openAiCompatibleChatCompletionsPath()` ×4 |
| Local inference body | inline 4-key insert | `buildOpenAiCompatibleChatBody(model, msgs, qMax(1, maxTokens), false, nullopt)` |
| Cloud body | inline 5-key insert (with temperature) | `buildOpenAiCompatibleChatBody(..., false, temperature)` |
| Streaming body | inline 4-key insert (`stream:true`) | `buildOpenAiCompatibleChatBody(..., true, nullopt)` |
| Cloud text extraction | inline choices/message/content | `parseOpenAiCompatibleChatText()` + reason enum |
| Local text extraction | inline message/content (after finish_reason/usage diagnostics) | `parseOpenAiCompatibleChatText()` after the unchanged diagnostics block |
| Catalog parse | inline in `fetchOpenAiCompatibleModels` | `parseOpenAiCompatibleModelCatalog()`; outcome/error mapping unchanged |

Unchanged and re-verified by the existing suites: request URLs/headers, tool/structured guards,
streaming SSE/NDJSON frame acceptance, error categories, deadline/cancellation, credential redaction,
endpoint policy, provider IDs/capabilities. See §7.

## 4. Request-body byte-equivalence results

Golden bytes were captured from the base implementation at `6035bf74` (standalone harness replicating
`core/src/runtime/LocalInference.cpp`, plus a production-path capture through the real
`LMStudioLocalInferenceClient::infer` with `ScriptedProviderTransport`). Golden byte strings are
hard-coded in the test and are **not** generated by the new module.

| Case | Inputs | Golden bytes | Result |
| --- | --- | --- | --- |
| cloud-explicit | model=fixture-model, temp=0.7, maxTokens=512, stream=false | `{"max_tokens":512,"messages":[{"content":"hello world","role":"user"}],"model":"fixture-model","stream":false,"temperature":0.7}` | ×­­­ byte-identical |
| cloud-fallback-2048 | temp=0.0, maxTokens resolved 2048, stream=false | `{"max_tokens":2048,"messages":[{"content":"hello world","role":"user"}],"model":"fixture-model","stream":false,"temperature":0}` | byte-identical |
| local-explicit | maxTokens=512, stream=false, no temperature | `{"max_tokens":512,"messages":[{"content":"hello world","role":"user"}],"model":"fixture-model","stream":false}` | byte-identical |
| local-clamped-to-one | maxTokens resolved 1 (requested 0), stream=false | `{"max_tokens":1,"messages":[{"content":"hello world","role":"user"}],"model":"fixture-model","stream":false}` | byte-identical |
| stream-fallback-2048 | maxTokens resolved 2048, stream=true | `{"max_tokens":2048,"messages":[{"content":"hello world","role":"user"}],"model":"fixture-model","stream":true}` | byte-identical |
| production local (explicit 512) | `infer` via scripted transport | matches `local-explicit` capture | byte-identical |
| production local (requested 0) | `infer` via scripted transport | matches `local-clamped-to-one` capture | byte-identical |

The production rows (test `localInferenceBodyByteEquivalence`) drive the real
`LMStudioLocalInferenceClient::infer`, capture the emitted POST body from the transport seam and
compare raw `QByteArray` equality against the pre-refactor golden values, and assert the request path
is `/v1/chat/completions`. Message payloads (including tool-call/tool-result and structured JSON
content) are asserted to pass through byte-identically.

## 5. Provider compatibility matrix

All nine requested provider identities and the routing facade keep identical IDs, endpoints,
credentials and capability behaviour. The extracted module is used only by the OpenAI-compatible path.

| Provider ID(s) | Path in this refactor | Body builder | Text parser | Catalog parser | Change |
| --- | --- | --- | --- | --- | --- |
| `lm-studio` | module | module (local variant) | module | module (`/v1/models`) | extracted |
| `llama-cpp-server` | module | module (local variant) | module | module (+ `/props` augmentation unchanged) | extracted |
| `openai-compatible-local` | module | module (local variant) | module | module | extracted |
| `openai`, `openai-compatible` | module | module (cloud variant) | module | module | extracted |
| `deepseek` | module | module (cloud variant) | module | module | extracted |
| `groq` | module (`.../openai/v1`) | module (cloud variant) | module | module | extracted |
| `mistral` | module | module (cloud variant) | module | module | extracted |
| `oud-api` → `claude` | native (unchanged) | `ModelService`/`LocalInference` native | native | `fetchAnthropicCloudModels` (uses the extracted `data`-array parser) | discovery parser extracted only |
| `cloud-api` → `gemini` | native (unchanged) | native | native | `fetchGeminiCloudModels` (unchanged) | none |
| `ollama` | native `/api/generate` | unchanged | unchanged | `/api/tags` unchanged | none |

Provider-specific compatibility exceptions preserved: `completeOpenAiChat` still rejects
`api.anthropic.com` / `generativelanguage.googleapis.com` hosts with the same `501`
(`"Endpoint does not use OpenAI chat completions."`) before any transport; the Groq `/openai/v1`
base path still resolves to `/openai/v1/chat/completions` via `endpointUrl` + the extracted path
rule.

## 6. Catalog parser regression results (P2-DEF-01)

The extracted `parseOpenAiCompatibleModelCatalog` reproduces the inline behaviour exactly:

* `data` missing / not an array → **malformed** (empty result). Catalog `{}`, `{"data":{}}`,
  `{"data":"x"}`, `{"data":null}` and `{"unexpected":[]}` are rejected.
* Every entry must be an object with a non-empty trimmed string `id`; otherwise the **entire**
  catalog is rejected and no model is returned (partial results discarded). Rejected: non-object and
  `null` entries, missing/empty/whitespace/numeric/bool/null `id`, and a valid-then-invalid sequence.
* A valid **empty** catalog (`{"data":[]}`) is accepted (0 models, not malformed) — the "catalog
  Empty" state used by `ModelService` is unchanged.
* Duplicate ids and provider-specific extra fields are accepted and preserved in order; unknown
  fields are ignored without rejection.
* `created` (number and string), `context_window`, `max_context_length`, `max_output_tokens`,
  `tool_calling`, `structured_output`, `vision`, `audio_input`, `audio_output`, `streaming`,
  `publisher`, `family`, `architecture`/`arch` and the nested `capabilities` object
  (`trained_for_tool_use`, `vision`) map to the same `OllamaModelSummary` values as before.

`fetchOpenAiCompatibleModels` keeps its transport call, `ProviderDiscoveryOutcome`
(`{completed, category, httpStatus, requestId}`), `MalformedResponse` category and the exact
`"Model list returned an invalid model catalog."` error text. `fetchLlamaCppModels`,
`fetchAnthropicCloudModels`, `fetchOpenAiCloudModels` and their tests are unchanged.

## 7. Complete test outcomes

Environment: worktree `build/tests`, Qt 6.12.0, AppleClang 21, ccache/Ninja.

**New OpenAI-compatible protocol tests** — `test_openai_compatible_protocol`
(CTest `#54`, binary `56 passed / 0 failed / 0 skipped`):

* `chatPathGeneration` (8 rows), `requestBodyByteEquivalence` (5 rows),
  `standardTextResponseParsing` (5 rows), `emptyAndMalformedResponses` (11 rows),
  `toolPayloadPassThrough`, `structuredJsonOutputPassThrough`, `validCatalogParsing`,
  `malformedCatalogRejected` (17 rows), `missingModelIdentifierRejected`,
  `duplicateAndInvalidEntries`, `providerSpecificCatalogFields`,
  `localInferenceBodyByteEquivalence` (2 rows) → **56/56 passed**, 2.24 s wall (CTest).

**Required focused suites** (pre-refactor baseline and post-refactor, all pass):

| Suite | Baseline | Post-refactor |
| --- | --- | --- |
| test_provider_contracts | Passed | Passed |
| test_native_provider_contract | Passed | Passed |
| test_bounded_streaming_contract | Passed | Passed |
| test_local_inference | Passed | Passed |
| test_provider_credentials | Passed | Passed |
| test_ollama_runtime | Passed | Passed |
| test_endpoint_security_capability | Passed | Passed |
| test_vendor_cloud_contract | Passed | Passed |
| test_credential_store | Passed | Passed |
| test_runtime_provider | Passed | Passed |
| test_static_provider_catalog | Passed | Passed |
| test_model_lifecycle_ownership | Passed | Passed |

**Full CMake build** (`cmake --preset tests` + `cmake --build --preset tests`): successful, no
compiler warnings on the changed/added files.

**Full sequential CTest** (`QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure`):
**143/143 passed (100%)**, 0 failed, 0 skipped. Run twice (231 s and 234 s); the second run was against
the final binaries after all formatting/`const` edits. No main-baseline failures were observed and no
regression was introduced.

**Rust checks** (`cli/`, toolchain pinned to `1.94.0` in CI; run with `+1.94.0`):
`cargo fmt --all --check` → clean; `cargo clippy --locked --all-targets -- -D warnings` → clean
(exit 0); `cargo test --locked` → all crates green (15 + 27 + 22 + 78 = 142 tests). The change is
C++-only, so Rust results are unchanged from baseline.

**Formatting** — CI-pinned `clang-format==18.1.8` (pip wheel):
`clang-format-diff.py -p1` on the working-tree diff reported **no diff** for changed hunks, and the
three new files are whole-file clean. (AppleClang/PCH note: clang-format 18.1.8 is used explicitly;
the repo default `clang-format` is 23.1.3 and was not used for the gate.)

**Scoped clang-tidy** — `clang-tidy` (LLVM 23, repo `.clang-tidy`, analysis DB built with
`tools/analysis/compile_database.py` to drop the AppleClang PCH) on the new
`OpenAiCompatibleProtocol.cpp`: exit 0, only 10 `readability-braces-around-statements` warnings that
mirror the pre-existing style of the extracted code (`OllamaRuntime.cpp` already emits 54 of the same
warning; warnings are not errors and CI's clang-tidy step only fails on errors). The one
`performance-no-automatic-move` warning initially introduced was removed by dropping a spurious
`const`.

**Generated IPC contract checks** — `python3 tools/ipc/generate.py --check` and
`tools/ipc/generate_desktop.py --check` pass (`test_ipc_generated_contract`,
`test_desktop_generated_contract` also pass in the CTest run). No generated file changed.

**Privacy and credential scans** — `python3 tools/analysis/check_personal_paths.py` → 0 files
flagged; targeted scan of the new files/diff for personal paths, `sk-`/`AIza`-style keys, PEM
headers and `password=`/`secret=` found none. No upstream body text, credential, prompt or model
payload was added; the golden fixtures are synthetic (`fixture-model`, `hello world`).

**Git hygiene** — `git diff --check` clean (no whitespace errors). This task added exactly three new
files and edited four existing files; no commit, push, merge, reset, stash, PR or workflow dispatch.

## 8. Exact changed files

| File | Change |
| --- | --- |
| `core/include/sentinel/core/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.h` | **new** (60 lines) |
| `core/src/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.cpp` | **new** (139 lines) |
| `tests/core/test_openai_compatible_protocol.cpp` | **new** (424 lines) |
| `core/src/runtime/LocalInference.cpp` | 33 insertions / 63 deletions (4 path sites, 3 body sites, 2 text sites, 1 include) |
| `core/src/runtime/OllamaRuntime.cpp` | 4 insertions / 70 deletions (catalog parse delegation, 1 include) |
| `core/CMakeLists.txt` | +2 (header + source registration in `sentinel_core`) |
| `tests/CMakeLists.txt` | +1 (`test_openai_compatible_protocol`) |

No new library target, no target rename, no change to `LMStudioConfig`, `ModelService`,
`ProviderRequestRuntime`, `NetworkPolicyService`, `ProviderCredentials`, `StaticProviderCatalog`,
build presets or workflows.

## 9. Remaining architectural dependencies

The full P2-M01 target architecture is **not** complete. Remaining provider logic still lives outside
protocol modules:

1. **Native protocols** — Anthropic Messages and Gemini generateContent request/response builders and
   streaming frame parsing remain in `ModelService::nativeEndpointReply` and
   `LocalInference.cpp` (`completeNativeChat`, Anthropic/Gemini `infer` and stream branches).
2. **Primary OpenAI-compatible chat body** — `SelectedEndpointChatProvider` in `ModelService.cpp`
   still constructs the model/messages/tool-call/`tools`/`tool_choice`/`response_format`/budget body
   inline; the extracted builder is not yet reused there because it carries no tool/structured schema
   concerns in this minimal step.
3. **Discovery** — transport + outcome mapping remain coupled in `OllamaRuntime.cpp`; Gemini and
   Anthropic discovery parsing is still provider-specific inline.
4. **Registration/identity** — provider IDs, endpoints, credentials and discovery fan-out are still a
   hand-written `if/else` chain in `ModelService::providerConfig` and `ApplicationController`.
5. **Error-mapping strategies** and retirement of the orphaned `ProviderErrorClassifier` /
   `ProviderMessageNormalizer` are untouched.
6. `LMStudioConfig` remains a mis-named shared config; the later `ProviderEndpointPolicy` alias step
   has not been started.

No unresolved CSEC-01–10 or PD-D10 policy decision was adopted. `AC-P2-01` remains Unverified;
`AC-P2-03` remains Blocked; `PD-D10` remains Pending Decision.

## 10. Recommended P2-M03 — native protocol extraction

Next incremental step, mirroring this task's shape and P2-M01 phase 2:

* **Goal:** create `provider/protocols/anthropic_messages/AnthropicMessagesProtocol.{h,cpp}` and
  `provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.{h,cpp}` for pure
  request-body/response-text/stream-frame logic, extracted from
  `ModelService::nativeEndpointReply` tool/structured assembly and
  `LMStudioLocalInferenceClient::completeNativeChat` plus the native `infer`/stream branches in
  `LocalInference.cpp`.
* **Explicitly keep** host routing (`api.anthropic.com`, `generativelanguage.googleapis.com`),
  headers (`x-api-key`/`anthropic-version`, `x-goog-api-key`/`?key=`), auth-required checks, native
  tool continuation, `output_config.format=json_schema`, `parametersJsonSchema`, `functionResponse`,
  image encoding, SSE parsing, deadline/cancellation and error classification where they are today.
* **Golden gates:** freeze the `test_native_provider_contract` fixtures and add byte-level golden
  bodies (Anthropic `messages`/`max_tokens`/`tools`; Gemini `contents`/`generationConfig`) captured
  from `6035bf74` before extraction, exactly as done here.
* **Compatibility invariants:** identical native request bytes for equivalent inputs, identical text
  extraction and tool-call/structured-output shapes, identical `501` host-mismatch and missing-model
  behaviour, unchanged `NativeProtocol` enum, unchanged tests and CMake targets.
* **Still out of scope for P2-M03:** discovery extraction, the `ProviderRegistry` descriptor table,
  error-mapping strategies, provider-owned lifecycle and any live provider call.

## Validation summary

* 4 focused provider suites + 8 adjacent provider/credential suites: pass before and after.
* New `test_openai_compatible_protocol`: 56/56.
* Full build: pass. Full sequential CTest: 143/143. Rust (fmt/clippy/test): pass.
* clang-format 18.1.8 gate: clean. Scoped clang-tidy: 0 new warning classes. IPC/privacy/git checks:
  clean.

## Scope statement

This is the first incremental, behavior-preserving extraction step only. It does not complete provider
modularization, does not adopt any pending product/security decision and grants no acceptance.
`AC-P2-01` remains Unverified, `AC-P2-03` remains Blocked, `PD-D10` remains Pending Decision.
