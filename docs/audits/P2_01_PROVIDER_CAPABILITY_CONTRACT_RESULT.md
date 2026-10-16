# P2-01 — Multi-backend provider capability contract result

Date: 2026-10-16 (Europe/Istanbul). Acceptance criterion: **AC-P2-01**. Candidate HEAD: `0245e5fa94584a357a6b1de01344f69b577e0e41`; branch `test/p2-01-provider-contracts`. Scope: deterministic source/fixture validation only. **AC-P2-01 is not satisfied: three reproduced production contract defects remain, and native cloud success-path fixture coverage is incomplete. No real-backend or P2 phase acceptance is granted.**

## 1. Scope and isolation

A dedicated managed worktree was created from the supplied checkout HEAD. Personal-Brain was unavailable on PATH; no brief/context was retrieved and no alternative project path was guessed. Root instructions, P0.1/P0.2, acceptance matrix, provider/model guides, architecture, Agent Runtime, security, building/testing and provider extension guidance informed this work. P0 audit findings at their older baseline are context; this report records the current candidate's inspected behavior.

Only test sources, one test registration and this report change. Original checkout and the preceding documentation worktree are preserved. AgentRuntime, AgentLoop, production providers, legacy C++ CLI, Rust IPC/TUI, dependencies and workflows are unchanged. No commit, push, merge or publication occurred. This work neither executes nor depends on changes from P0-03, P1-01, P1-05 or P1-04.4.

New HTTP fixtures bind ephemeral IPv4 loopback ports and process complete request bodies before responding. New ModelService tests inject InMemorySettingsStore and inMemoryTestCredentialStore, with LocalOnly policy; no production configuration/credential store is needed by the final fixtures. All transmitted keys/prompts/images are synthetic. No real cloud inference, personal credential use, model download/install/load/unload or native UI acceptance was performed. Existing opt-in live probes are not CTest targets and were not run.

## 2. Source architecture overview

- [IChatProvider](../../core/include/sentinel/core/interfaces/IChatProvider.h) defines lifecycle/error categories, cancellation, bounded requests, images, structured output, native tools and streaming. Its default sendRequest rejects requested capabilities it cannot implement; concurrency can remain Unknown.
- [IModelRouter](../../core/include/sentinel/core/model/IModelRouter.h) resolves explicit selection; [ModelBinding](../../core/include/sentinel/core/model/ModelRouting.h) captures identity/capability/provenance. Static routing metadata is not endpoint readiness. Existing router tests remain the route-policy evidence; this task adds no router or registry.
- [ModelService](../../core/src/model/ModelService.cpp) owns the existing provider factories, selection, capability merges and leased providers. `resolve` distinguishes unknown provider, missing model, unusable endpoint and missing authentication. `SelectedEndpointChatProvider` captures binding/config by value; `HealthTrackedProvider` sequences observations. Configured/available catalog, provider health, model capability and successful execution are distinct states. An unobserved endpoint adapter's `status()` can report Ready from configuration alone; use observed status and execution evidence rather than interpreting that as live readiness.
- [OllamaChatProvider](../../core/src/chat/OllamaChatProvider.cpp) uses Ollama inference and discovery snapshots; the current adapter rejects native tool/structured requests. [LocalInference](../../core/src/runtime/LocalInference.cpp) implements Ollama NDJSON and compatible SSE/chat completion, plus Claude/Gemini cloud request and response branches.
- [ProviderRequestRuntime](../../core/src/runtime/ProviderRequestRuntime.cpp) owns transport wait/deadline/cancellation, transport error classification and retry rules. A stream with output is not retryable; timed-out/cancelled requests are not replayed. Cancellation confirms client-side termination, not model-server compute release.
- [ProviderMessageNormalizer](../../core/src/provider/ProviderMessageNormalizer.cpp) preserves the original generic message, concatenates text parts and extracts generic tool_calls; it is not proof of every cloud wire format. [ProviderErrorClassifier](../../core/src/provider/ProviderErrorClassifier.cpp) is a separate message/status classification helper with a different taxonomy. The actual transport/adapters use ProviderRequestRuntime; helper tests do not establish provider-specific wire parity.

## 3. Provider capability and evidence matrix

“Fixture verified” means the listed synthetic API path only. All rows have **real endpoint compatibility Unverified** and **real-model execution/tool reliability Unverified**. No server availability was probed. Source defaults are adapter declarations, not guarantees for a model.

| Backend / provider IDs | Existing API path and source capability defaults | Verified fixture behavior | Unsupported / remaining API coverage | Authentication / resource authority |
| --- | --- | --- | --- | --- |
| Ollama (`ollama`) | `/api/tags`, `/api/generate`, NDJSON streaming; streaming Supported, native tools/structured Unsupported; vision Unknown by default | Discovery populated/empty/wrong-shape/auth; unavailable selection; plain success, HTTP failures, malformed JSON, timeout/disconnect/cancel; ordered stream and missing terminal; cancellation after first delta | Native tools/structured explicitly rejected; no real model vision/tool certification; no lifecycle pull/load tests performed | Local endpoint; external Ollama ownership, not daemon-owned weights/process inferred from API |
| llama.cpp (`llama-cpp-server`) | OpenAI-compatible discovery/chat/SSE; loaded `/props` template metadata; tools/structured default Unsupported, streaming Supported | Actual ModelService endpoint adapter native-tool success/negative parsing with explicit capability binding; frozen config/binding; shared compatible HTTP/error/stream fixtures; existing loaded-alias/template tests | Generic protocol success is not llama-server version/template acceptance; native capability depends on observed identity/metadata or explicit override | Daemon LocalModelRuntime can own a selected process; external listener ownership is separate; not exercised here |
| LM Studio (`lm-studio`) | Compatible chat/SSE plus native catalog metadata; tools/structured default Unsupported, streaming Supported | Actual ModelService adapter tool schema serialization/parsing; structured-object response parsing; rejection gates; discovery/stale catalog; shared transport fixtures and existing loaded-context cases | Vision binding enforcement defect below; structured parsing verifies object syntax, not independent JSON Schema conformance; no native server/model acceptance | External service, synthetic fixture keys only; no load/unload performed |
| Generic local (`openai-compatible-local`) | Same compatible client; ModelService currently chooses fixed `127.0.0.1:8000`; tools/structured default Unsupported, streaming Supported | Shared lower-level client at disposable loopback endpoint: chat/SSE/error/cancel; ModelService capabilities, unavailable catalog model and stale observation | Fixed production endpoint was **not contacted or bound**. Shared protocol tests do not verify a configurable endpoint feature or production-route integration at port 8000 | External endpoint; API support does not grant process ownership |
| OpenAI / compatible / DeepSeek / Groq / Mistral (`openai`, `openai-compatible`, `deepseek`, `groq`, `mistral`, `cloud-api` when selected) | Compatible chat/SSE and model catalog paths; streaming Supported; native tools/structured default Unsupported unless capabilities supply support | All IDs: credential-missing resolution and no implicit fallback; snapshot health Unknown. Shared bearer discovery positive/negative with synthetic key; existing cloud-branch bearer chat fixture; common classifier cases | Per-vendor production URLs, model catalogs, streaming/tool/vision differences and auth semantics not independently accepted. `openai-compatible` uses OpenAI endpoint routing, not arbitrary configurable cloud URL | Required credential/network gate; provider owns remote resources. No actual cloud requests |
| Claude (`claude`, corresponding cloud-api selection) | Native Messages/text blocks, tool_use/continuations and streaming paths; native tools default Supported, structured Unknown | Missing credentials; metadata health Unknown; common classifier; existing schema-related helper evidence; local native protocol mismatch rejected without HTTP | Hard-coded native URL/host matching prevents successful local HTTP substitution through existing seam. Native discovery/generation/stream/tool success and error parsing remain unverified | Synthetic/in-memory credentials for negative cases; no real endpoint contact |
| Gemini (`gemini`, corresponding cloud-api selection) | Native generateContent/candidates/function calls, streaming; native tools default Supported, structured Unknown; full parametersJsonSchema mapping exists | Missing credentials; metadata health Unknown; common classifier; existing Gemini authoritative-schema helper test; local native protocol mismatch rejected without HTTP | Hard-coded native URL/host matching prevents successful local HTTP substitution. Native success/error/stream/continuation parity remains unverified | Synthetic/in-memory credential boundary; no real endpoint contact |

All registered defaults leave vision/audio capability Unknown until metadata or overrides supply it. Explicit booleans, unsupported values, malformed string booleans and invalid context limits are tested separately. Health/selection never supplies proof of tool-calling reliability.

## 4. Tests added or extended

Exact changed files:

- [test_local_inference.cpp](../../tests/core/test_local_inference.cpp): extended existing target with **30 data-row cases** across generation (18), stream completion/truncation (4), cancel-after-first-delta (2), and generation cancel/disconnect/timeout (6).
- [test_provider_contracts.cpp](../../tests/core/test_provider_contracts.cpp): **37 case/data-row executions**, excluding setup/cleanup, covering ModelService and helper contracts. A focused cross-contract target is justified because these cases span model selection/metadata, normalizer and classifier rather than one adapter.
- [ProviderHttpFixture.h](../../tests/support/ProviderHttpFixture.h): shared test-only loopback HTTP fixture, no fixed ports/provider registry.
- [tests/CMakeLists.txt](../../tests/CMakeLists.txt): one registration using existing `add_sentinel_core_test`.
- This report.

Native tool negatives include unknown tool, missing call identity, array/malformed arguments and empty choices. Positive tests compare serialized tool parameters to the authoritative ToolDescriptor schema; runtime argument authorization/validation is not duplicated here. Frozen binding is verified during request handling while selection, caller binding and endpoint change; the captured provider lease continues with its original model/config. Existing ChatModeService late-success/replacement tests provide stale generation callback evidence, without editing the runtime.

## 5. Error, cancellation and diagnostic results

Ollama and compatible mock generation distinguish 401/403 authentication, 404 model absence, 429 rate limit, 500 unavailable provider, 501 unsupported capability, 422 rejected request, invalid JSON, timeout, disconnect and cancellation. Nonzero bounded generation requests make one attempt. Both stream parsers preserve ordered deltas, require a terminal marker and cancel after first output; client cancellation is not a receipt for upstream GPU/process release. Shared retry helper refuses cancelled/timed-out requests and retry after stream output.

Missing credentials for every cloud registration reject resolution rather than choosing Ollama; the fallback factory is never called. Stale discovery sequences cannot overwrite newer catalog state. Explicit unsupported tool/structured/stream capabilities reject before transport. Diagnostic JSON excludes synthetic keys/upstream secret text in the exercised paths, but user-facing errors fail the broader secret-safety requirement as recorded below. No blanket log/export/redaction certification follows from these checks.

## 6. Genuine defects retained for separate fixes

Four explicit `QEXPECT_FAIL(..., Continue)` assertions witness **three unresolved production defects**. They are visible in Qt XML as `xfail`; CTest success is not defect closure. If behavior is fixed later the assertions become unexpected passes until intentionally updated. No provider implementation was changed to make these tests green.

| ID | Reproduced contract defect | Source and test boundary | Impact / next fix |
| --- | --- | --- | --- |
| P2-DEF-01 | HTTP 200 `{unexpected: object}` without a data array is accepted as a completed empty compatible model catalog | `fetchOpenAiCompatibleModels` in [OllamaRuntime.cpp](../../core/src/runtime/OllamaRuntime.cpp); `malformedDiscoveryShapeIsNotHealthy` | Malformed and legitimately empty catalogs are conflated. Ollama's own tags path correctly rejects missing models array. Validate compatible response shape separately |
| P2-DEF-02 | Direct SelectedEndpointChatProvider request with visionInput Unsupported sends images and can succeed | `SelectedEndpointChatProvider::sendRequest` in ModelService; `unsupportedVisionIsRejectedBeforeTransport` (two xfail assertions) | Missing direct-provider capability enforcement. This witness does **not** establish an end-to-end Desktop/Agent bypass: callers may have their own preflight. Fix the provider contract in a separate scoped task |
| P2-DEF-03 | Synthetic upstream HTTP 401 error message is reflected into ChatProviderReply.errorMessage without redaction | `postJsonOnce` / `safeNetworkFailureSummary` in LocalInference, ModelService error projection; `echoedSecretsDoNotEnterDiagnostics` | Allowlisted diagnostic object is clean, but untrusted upstream error text can carry echoed sensitive material into user-facing errors. No real credential was used/leaked. Redact/bound public error projection separately |

Known limits, not newly proven defects: native cloud mock-injection seams are absent; provider configuration Ready is not inference Ready; structural tool/JSON parsing is not semantic schema validation or real-model capability; generic local ModelService endpoint is fixed; supported cloud operation parity remains incomplete.

## 7. Validation results

Final validation results are recorded below after the final isolated fixture build. No real endpoints or native model tests are included.

Host: macOS 27.0.1, Darwin arm64; AppleClang 21.0.0; CMake 4.4.4; Qt 6.12.0; clang-format/clang-tidy 23.1.3. This is one host's fixture evidence, not native Linux/Windows/backend certification.

| Check | Final result |
| --- | --- |
| `cmake --preset tests -DSENTINEL_USE_CCACHE=OFF` | Configure passed using installed dependencies |
| `CCACHE_DISABLE=1 cmake --build --preset tests -j6` | Full build passed; no production/dependency edits |
| `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | **128/128 targets passed**, 119.03 seconds, final fixture setup |
| Direct Qt XML: test_provider_contracts | **39 pass records**, including setup/cleanup; **37 cases** plus **4 expected-failure assertions** across three known-defect cases; no unexpected failures/skips |
| Direct Qt XML: test_local_inference | **51 pass records**, including setup/cleanup; **49 cases**, of which **30 newly added**; no failures/skips |
| New case coverage | **67 new case/data-row executions** (37 + 30); does not include lifecycle setup/cleanup or count each loop iteration separately |
| `clang-format --dry-run --Werror` on all three changed/new C++ files | Passed |
| Scoped clang-tidy on both test translation units | Passed, no diagnostics: `clang-analyzer-*`, `bugprone-use-after-move`, `bugprone-dangling-handle`, warnings-as-errors; not a whole-repository static certification |
| Both IPC generators with `--check` | Passed, no generated contract changes |
| `git diff --check`, report relative-link/fence checks | Passed; report links resolve |
| `python3 tools/analysis/check_personal_paths.py` | Zero flagged files |
| Isolation/change allowlist | Exactly five allowed files; original checkout clean and previous documentation worktree preserved; no production or audit/matrix changes |

Relevant unchanged provider/model targets passed within the full run, including runtime_provider, provider_credentials, static_provider_catalog, model_registry, static_model_router, ollama_runtime, chat_mode_service, chat_attachments and llm_agent_runtime. Existing late-success/cancel and native diagnostics/deadline tests are fixture evidence only. Two earlier full-suite runs also passed 128/128 (158.99 and 117.95 seconds); final totals above supersede their earlier test-source/setup snapshots.

Reproduction uses the commands above and direct `build/tests/tests/test_provider_contracts` / `test_local_inference` with `-o result.xml,xml`. Raw local logs/XML remain disposable local evidence because generated outputs contain machine-specific paths; public-safe outcome counts and identities are recorded here.

Evidence identity (SHA-256):

| Artifact | Digest |
| --- | --- |
| Final four-file test-source manifest | `219a25e506824925beb2749c992e7914064cfb1329d75b264d3340c56470830e` |
| test_provider_contracts executable | `38960da87422e7326bfa623ea3d39c5fdc049df3befd81777d022bb4a5048617` |
| test_local_inference executable | `fb21aab4aac8372e01e82d8ec65e43032c3f0faa05d18e67bd79b80cd8b4770d` |

The source manifest hashes sorted repository-relative filenames plus NUL, file bytes plus NUL for tests/CMakeLists.txt, both test translation units and ProviderHttpFixture.h. Model/runtime configuration, weights and native acceptance evidence are not applicable to these synthetic fixtures; an EV-01 live packet has not been fabricated.

Development-only test compilation issues (Qt initializer and moc discovery) were corrected within test files before successful execution; they were not production failures. Prior P0.1 CTest/Rust results remain historical and are not counted here. Rust source/contracts were not changed; no Cargo rerun was necessary for this scoped C++ fixture task.

## 8. Remaining live-backend and API validation

Before acceptance, finish per-operation positive/negative fixture coverage for native Claude/Gemini and each advertised vendor-specific variant using an approved existing/injectable transport seam in a separate task; do not redirect production DNS or use live cloud as a mock substitute. Close the three defects with focused negative regression evidence. Broaden unknown/unsupported vision/audio/model-specific capability and structured semantic-validation cases as applicable.

Then independently select authorized backend/version/model/revision/tokenizer/context/quantization/platform profiles and collect real endpoint discovery, loaded identity, streaming, tool schema, unsupported capability, auth and cancellation evidence. Verify CPU/resource release separately from client cancellation. Run the adopted live corpus only with approved disposable configurations and independent artifact oracles. No model acquisition or paid provider call is authorized by this report.

## 9. AC-P2-01 assessment

[AC-P2-01](../roadmaps/ACCEPTANCE_MATRIX.md) requires positive/negative cases for every supported advertised operation, Unknown remaining Unknown and secrets absent from projection. **Not satisfied**: mock coverage verifies a substantial subset, but P2-DEF-01/02/03 violate contract expectations and native cloud wire cases are incomplete. Four expected-failure assertions remain despite green test-target status. The broader criterion stays open; the acceptance matrix and AC-P0-02 prerequisite/adoption status were not modified. AC-P2-02/03/04 and P2 phase completion are not granted from these mocks.

## 10. Recommended next task

A separate **P2 provider-contract defect remediation** task for P2-DEF-01/02/03, starting with upstream error redaction, followed by safe native-cloud fixture coverage planning. Keep it independent of P0-03, P1-01, P1-05 and P1-04.4. Real-backend parity remains a later explicitly configured/authorized validation step, not a consequence of green CTest.
