# P2-M01 — Provider modularization architecture and migration plan

Audit date: 2026-10-17. Phase: P2 — Multi-Backend & Capability Negotiation.
Criteria: [AC-P2-01](../roadmaps/ACCEPTANCE_MATRIX.md) (Unverified), AC-P2-03 (Blocked),
[PD-D10](../roadmaps/ACCEPTANCE_DECISIONS.md) (Pending Decision).
**Architecture audit and migration design only. No source, behavior, CI or workflow change.**

## Isolation and baseline

| Item | Value |
| --- | --- |
| Branch | `architecture/p2-m01-provider-modularization` |
| Worktree | `/private/var/folders/.../T/opencode/p2-m01-provider-modularization` |
| Base | `3a98bab5f040b5e46f429ba423f1d1c1a023d3e1` (`origin/main`, verified equal to local `main`) |
| Worktree state | clean; no modified or untracked files at entry |
| Root AGENTS.md | read; provider/**AgentRuntime**/**ToolExecutionGateway**/platform boundaries and the
`cmake --preset tests` validation contract applied |

`origin/main` was already equal to local `main` (`3a98bab5`); no fetch was required to establish the
base. Existing worktrees were listed and none was modified. This task performed **no** commit, push,
merge, reset, stash, PR, workflow dispatch, model download or live provider call. In particular the
`ci-05-windows-stabilization` (`3a98bab5`) and `p4-02-native-resource-telemetry` worktrees were left
untouched.

Focused provider tests were run against the pre-built `build/tests` tree (configured from this same
commit) and all passed — see [Validation](#validation).

## 1. Actual provider inventory

### 1.1 Real provider count

The nine requested target identities map onto **11 registered provider IDs plus one routing facade**.
There are exactly **two chat-implementation boundaries**: one dedicated provider (`ollama`) and one
shared multi-provider adapter class (`SelectedEndpointChatProvider`). No provider except Ollama has an
independent implementation module.

| # | Target identity | Registered provider ID(s) | Implementation boundary |
| --- | --- | --- | --- |
| 1 | Ollama | `ollama` | Dedicated `OllamaChatProvider` |
| 2 | llama.cpp server | `llama-cpp-server` | Shared `SelectedEndpointChatProvider` |
| 3 | LM Studio | `lm-studio` | Shared `SelectedEndpointChatProvider` |
| 4 | OpenAI | `openai`, `openai-compatible` | Shared `SelectedEndpointChatProvider` |
| 5 | DeepSeek | `deepseek` | Shared `SelectedEndpointChatProvider` |
| 6 | Groq | `groq` | Shared `SelectedEndpointChatProvider` |
| 7 | Mistral | `mistral` | Shared `SelectedEndpointChatProvider` |
| 8 | Anthropic Claude | `claude` | Shared `SelectedEndpointChatProvider` (native /v1/messages branch) |
| 9 | Google Gemini | `gemini` | Shared `SelectedEndpointChatProvider` (native generateContent branch) |
| — | Generic local compatible | `openai-compatible-local` | Shared `SelectedEndpointChatProvider` (fixed `127.0.0.1:8000`) |
| — | Cloud routing facade | `cloud-api` | Shared `SelectedEndpointChatProvider` (aliases one vendor by `selectedCloudProvider`) |

`cloud-api` is not a tenth backend: `ModelService::providerConfig` and the discovery orchestrator
resolve it to whichever vendor `AppSettings::selectedCloudProvider()` names.

### 1.2 Execution topology (verified from source)

* Single static library `sentinel_core` (`core/CMakeLists.txt`). **Every** provider file is compiled
  into that one target; there are no per-provider libraries, object libraries or subdirectories that
  scope provider code. Provider code is spread across `core/src/chat`, `core/src/model`,
  `core/src/runtime`, `core/src/security`, `core/src/provider` and `core/src/app`.
* `IChatProvider` (`core/include/sentinel/core/interfaces/IChatProvider.h`) is the only chat contract.
* `ModelService` (`core/include/sentinel/core/model/ModelService.h`, `core/src/model/ModelService.cpp`)
  owns registration, lazy construction, current selection, capability merge, health and catalog state.
* `SelectedEndpointChatProvider` is a **file-local class inside `ModelService.cpp`** (line 540) that
  serves all ten non-Ollama endpoint IDs. It reaches provider protocols through
  `LMStudioLocalInferenceClient` in `LocalInference.cpp` and does hostname branching
  (`api.anthropic.com`, `generativelanguage.googleapis.com`) inside `sendRequest`/streaming.
* Ollama reaches `OllamaLocalInferenceClient` / `OllamaLocalInferenceStreamClient` (both in
  `LocalInference.cpp`) plus `OllamaHttpRuntimeClient` (in `OllamaRuntime.cpp`).

### 1.3 Per-provider matrix

All rows below are **verified from source**. "Unverified" means the syntactically-present behavior has
no real-backend acceptance evidence.

| Provider ID | Protocol / endpoints | Native vs OpenAI-compat | Auth | Streaming | Tool calling | Structured output | Vision | Discovery | Model lifecycle ownership |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `ollama` | native: `/api/generate` (`stream:false`, `stream:true` NDJSON), `/api/version`, `/api/tags`, `/api/pull`, `/api/delete` | Native | None (loopback only) | Yes (NDJSON, bounded-capable) | Rejected as unsupported | Rejected as unsupported | Base64 image passthrough; capability Unknown → gated | Native `/api/tags` | External/shared; Sentinel does not own the process |
| `llama-cpp-server` | OpenAI-compat `/v1/chat/completions` + `/v1/models`, `/props` | OpenAI-compatible | None (loopback) | Yes (SSE) | Unsupported by default | Unsupported by default | Unknown | `/v1/models` (+ optional `/props` caps) | Daemon may launch an owned `llama-server` child (P3; PD-D10 pending) |
| `lm-studio` | OpenAI-compat `/v1/chat/completions` + `/v1/models` | OpenAI-compatible | None (loopback) | Yes (SSE) | Unsupported | Unsupported | Unknown | `/v1/models` | External/shared; no ownership |
| `openai-compatible-local` | OpenAI-compat, fixed `http://127.0.0.1:8000` | OpenAI-compatible | None (loopback) | Yes (SSE) | Unsupported | Unsupported | Unknown | `/v1/models` | External/shared; no ownership |
| `openai` | `https://api.openai.com/v1/chat/completions`; discovery `/v1/models` | OpenAI-compatible | Bearer (OpenAI key) | Yes (SSE) | Unsupported by default | Unsupported by default | Unknown | `/v1/models` | None (cloud) |
| `openai-compatible` | Resolves to the OpenAI endpoint (no arbitrary cloud URL) | OpenAI-compatible | Bearer (OpenAI key) | Yes (SSE) | Unsupported | Unsupported | Unknown | `/v1/models` | None (cloud) |
| `deepseek` | `https://api.deepseek.com/v1/chat/completions`; discovery `/v1/models` | OpenAI-compatible | Bearer | Yes (SSE) | Unsupported | Unsupported | Unknown | `/v1/models` | None (cloud) |
| `groq` | `https://api.groq.com/openai/v1/chat/completions`; discovery `/openai/v1/models` | OpenAI-compatible | Bearer | Yes (SSE) | Unsupported | Unsupported | Unknown | `/openai/v1/models` | None (cloud) |
| `mistral` | `https://api.mistral.ai/v1/chat/completions`; discovery `/v1/models` | OpenAI-compatible | Bearer | Yes (SSE) | Unsupported | Unsupported | Unknown | `/v1/models` | None (cloud) |
| `claude` | native Anthropic Messages `https://api.anthropic.com/v1/messages`; discovery `/v1/models` | Native | `x-api-key` + `anthropic-version: 2023-06-01` | Yes (SSE, `stream:true`) | Supported (native tool_use/tool_result + continuation) | `output_config.format=json_schema`; declared Unknown | Unknown | `/v1/models` | None (cloud) |
| `gemini` | native `https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent` and `:streamGenerateContent?alt=sse`; discovery `/v1beta/models?key=` | Native | `x-goog-api-key` (chat) / `?key=` (discovery) | Yes (SSE) | Supported (`parametersJsonSchema` + functionResponse continuation) | Declared Unknown | Unknown | `/v1beta/models` | None (cloud) |
| `cloud-api` | facade over the selected vendor above | mixed | Inherits vendor | Inherits vendor | Inherits vendor | Inherits vendor | Unknown | Inherits vendor | None |

Verified caveats (from existing P2 evidence, not assumptions):

* Tools/structured output are rejected **before transport** for the default OpenAI-compatible path
  when the binding capability is not `Supported`; Claude/Gemini declare `nativeToolCalling = Supported`.
* `openai-compatible` is an **alias** of OpenAI; it has no arbitrary cloud-URL product setting.
* Capability defaults are `streaming = Supported`, `structuredOutput`/`nativeToolCalling = Unsupported`
  for every endpoint provider, with Claude/Gemini using `nativeToolCalling = Supported` and
  `structuredOutput = Unknown` (`ModelService` constructor, lines 1173–1189).
* Vision is `Unknown` by default and therefore fail-closed only when a binding is explicitly
  `Unsupported` (P2-03 guard); `Unknown` still permits transport for selected endpoints.

### 1.4 Implementation files (verified)

| Responsibility | File(s) |
| --- | --- |
| Chat contract, capability/error enums, reply/failure metadata | `core/include/sentinel/core/interfaces/IChatProvider.h` |
| Capability/route/provider/model types, provenance | `core/include/sentinel/core/model/ModelRouting.h` |
| Router contract | `core/include/sentinel/core/model/IModelRouter.h`, `core/src/model/StaticModelRouter.cpp` |
| Provider registry/factory/selection/health/capability/config/discovery | `core/include/sentinel/core/model/ModelService.h`, `core/src/model/ModelService.cpp` (incl. `SelectedEndpointChatProvider`, `nativeEndpointReply`, `geminiFunctionDeclaration`) |
| Static provider/model catalog | `core/include/sentinel/core/model/{IProviderCatalog,ProviderCatalog,StaticProviderCatalog}.h`, `core/src/model/StaticProviderCatalog.cpp` |
| Shared request runtime: deadline/cancellation/retry/classification | `core/include/sentinel/core/runtime/ProviderRequestRuntime.h`, `core/src/runtime/ProviderRequestRuntime.cpp` |
| Transport seam for deterministic fixtures | `core/include/sentinel/core/runtime/LocalInferenceTransport.h` |
| Ollama native inference + streaming + LM Studio/OpenAI-compatible + native Claude/Gemini chat clients | `core/src/runtime/LocalInference.cpp` (2141 lines; `LMStudioConfig` declared in `LocalInference.h`) |
| Ollama runtime client, health, `/api/tags` discovery, cloud/vendor discovery fetchers, puller/library fetchers | `core/include/sentinel/core/runtime/OllamaRuntime.h`, `core/src/runtime/OllamaRuntime.cpp` (1869 lines) |
| Dedicated Ollama chat provider | `core/include/sentinel/core/chat/OllamaChatProvider.h`, `core/src/chat/OllamaChatProvider.cpp` |
| Runtime provider descriptors/registry | `core/include/sentinel/core/runtime/RuntimeProvider.h`, `core/src/runtime/RuntimeProvider.cpp` |
| Runtime capability registry (feature negotiation) | `core/include/sentinel/core/runtime/RuntimeCapabilities.h`, `core/src/runtime/RuntimeCapabilities.cpp` |
| Credential policy/readiness metadata | `core/include/sentinel/core/security/ProviderCredentials.h`, `core/src/security/ProviderCredentials.cpp` |
| Network policy | `core/include/sentinel/core/network/NetworkPolicyService.h` |
| Discovery orchestration (threaded) | `core/src/app/ApplicationController.cpp` (`pollOllama`, lines ~10160–10310) |
| Legacy/simple classifier + normalizer | `core/src/provider/ProviderErrorClassifier.cpp`, `core/src/provider/ProviderMessageNormalizer.cpp` |
| Dev/test provider | `core/src/chat/LocalEchoProvider.cpp` |

### 1.5 Dead / orphaned provider code (verified)

* `ProviderErrorClassifier` (`ProviderErrorCategory`, HTTP→category, user message): **no production
  caller**. Referenced only by its own translation unit and `core/CMakeLists.txt`.
* `ProviderMessageNormalizer`: **no production caller**. Same situation.
* `IRuntimeCapabilityRegistry` / `StaticRuntimeCapabilityRegistry` / `OllamaRuntimeCapabilityRegistry`
  (`RuntimeCapabilities`) is a **runtime-feature** negotiation registry, not a per-model capability
  registry. Per-model capability metadata lives in `ModelCapabilities` + `ModelCapabilityProvenance`.
* `ProviderCredentials::defaultProviderCredentialRegistry` currently seeds only `ollama`,
  `openai-compatible`, `claude`, `gemini`; it is metadata/readiness only and does not gate execution.
  Execution gating is done in `ModelService::resolve`/`providerConfig` and `LMStudioConfig`.

The authoritative error classification is `ProviderRequestRuntime::classify` →
`ChatProviderErrorCategory`; `ProviderErrorClassifier`'s parallel `ProviderErrorCategory` must not be
reintroduced as a second classification authority.

### 1.6 Discovery (verified)

`ApplicationController::pollOllama` runs discovery on a worker thread and selects exactly one branch by
provider ID, then feeds `ModelService::acceptProviderDiscovery`:

| Provider ID | Discovery call | Endpoint |
| --- | --- | --- |
| `ollama` | `OllamaHttpRuntimeClient::discoverModels` | `/api/tags` |
| `lm-studio` | `fetchOpenAiCompatibleModels` | `{lmStudioEndpoint}/v1/models` |
| `llama-cpp-server` | `fetchLlamaCppModels` | `{llamaCppEndpoint}/v1/models` (+ `/props`) |
| `openai-compatible-local` | `fetchOpenAiCompatibleModels` | `http://127.0.0.1:8000/v1/models` |
| `cloud-api`/`gemini` | `fetchGeminiCloudModels` | `/v1beta/models?key=` |
| `cloud-api`/`claude` | `fetchAnthropicCloudModels` | `/v1/models` (`x-api-key`) |
| `deepseek` | `fetchOpenAiCloudModels` | `https://api.deepseek.com/v1/models` |
| `groq` | `fetchOpenAiCloudModels` | `https://api.groq.com/openai/v1/models` |
| `mistral` | `fetchOpenAiCloudModels` | `https://api.mistral.ai/v1/models` |
| `openai`/`openai-compatible` | `fetchOpenAiCloudModels` | `https://api.openai.com/v1/models` |

Discovery endpoint formats and catalog validation live in `OllamaRuntime.cpp`
(`fetchOpenAiCompatibleModels` enforces `data` array + per-entry `id`, remediation P2-DEF-01).
Discovery is **provider-specific parsing inside one runtime file**, not a protocol module.

### 1.7 Existing deterministic tests and missing real-provider evidence

| Test | Covers |
| --- | --- |
| `tests/core/test_provider_contracts.cpp` (1040) | Multi-backend mock capability/error matrix |
| `tests/core/test_native_provider_contract.cpp` (1207) | Native Claude/Gemini fixtures via `ScriptedProviderTransport` |
| `tests/core/test_bounded_streaming_contract.cpp` | Deadline/bounded streaming contract |
| `tests/core/test_provider_credentials.cpp` (90), `test_credential_store.cpp` | Credential metadata/readiness |
| `tests/core/test_ollama_runtime.cpp` (533) | Ollama health/discovery/props |
| `tests/core/test_local_inference.cpp` (693) | Local inference + LM Studio boundary |
| `tests/core/test_static_provider_catalog.cpp`, `test_runtime_provider.cpp` | Catalog/runtime descriptors |
| `tests/core/test_endpoint_security_capability.cpp` | Endpoint security + capability disclosure matrix |
| `tests/core/test_local_echo_provider.cpp` | Test provider |
| `tests/core/test_provider_certification_root.cpp` | Explicit-root refusal contract |
| `tests/core/test_model_lifecycle_ownership.cpp` | Lifecycle/ownership (P3) |
| `tests/helpers/provider_certification_probe.cpp` (428) | `EXCLUDE_FROM_ALL` probe |
| Fixtures | `tests/support/ScriptedProviderTransport.h`, `tests/support/ProviderHttpFixture.h`, `tests/support/LocalInferenceWorkerFixture.h` |

**Missing real-provider evidence (from P2 evidence, unchanged):** no live call to any cloud vendor,
no real TLS/proxy/DNS/rate-limit exercise, no real-model tool/structured/vision completion, no
vendor-specific advertised-operation parity, and no injectable same-origin transport for native cloud
success paths beyond the scripted host-gated fixtures. AC-P2-03 remains Blocked; authorized expenditure
is zero.

## 2. Verified current code structure

```
core/
  include/sentinel/core/
    interfaces/IChatProvider.h            # contract
    model/{IModelRouter,ModelRouting,ModelService,StaticProviderCatalog,StaticModelRouter}.h
    runtime/{LocalInference,LocalInferenceTransport,OllamaRuntime,ProviderRequestRuntime,RuntimeProvider,RuntimeCapabilities}.h
    chat/{OllamaChatProvider,LocalEchoProvider}.h
    security/ProviderCredentials.h
    network/NetworkPolicyService.h
  src/
    chat/OllamaChatProvider.cpp
    model/ModelService.cpp        # registration + SelectedEndpointChatProvider + nativeEndpointReply
    model/StaticProviderCatalog.cpp
    runtime/LocalInference.cpp    # Ollama + LM Studio/OpenAI-compat + native clients + streaming
    runtime/OllamaRuntime.cpp     # health, discovery, vendor fetchers, puller
    runtime/ProviderRequestRuntime.cpp
    provider/{ProviderErrorClassifier,ProviderMessageNormalizer}.cpp   # orphaned
    security/ProviderCredentials.cpp
    app/ApplicationController.cpp # discovery orchestration
```

Structural facts that constrain the target design:

1. Everything is one `sentinel_core` static target; provider code is **not** grouped by provider.
2. Provider protocol details are interleaved in two large service files (`LocalInference.cpp`,
   `OllamaRuntime.cpp`) and one private adapter class (`ModelService.cpp`).
3. Endpoint/credential identity is a **hand-written `if/else` chain** in
   `ModelService::providerConfig`, duplicated in the discovery orchestrator and in QML.
4. `LMStudioConfig` is misnamed: it is the shared config for every non-Ollama provider and carries the
   only endpoint-security policy (`isSecureTransportEndpoint`, `isAllowedEndpoint`, `isCloud`,
   `providerDisplayName`). Renaming/relocating it is a public-header compatibility risk.
5. Discovery parsing, endpoint security, credential selection and protocol serialization are **not
   separated**; they are colocated by accident of file history.

## 3. Proposed final module structure

Use existing repository conventions (`core/include/sentinel/core/<domain>/` + `core/src/<domain>/`,
one `sentinel_core` CMake target). The illustrative layout in the task is adopted with real names:

```
core/include/sentinel/core/provider/
  common/                                  # provider-neutral, security-authoritative
    ProviderIdentity.h                     # providerId, display name, protocol kind, endpoint default, auth kind
    ProviderDescriptor.h                   # identity + capability defaults + discovery kind (data, not code)
    ProviderEndpointPolicy.h               # re-exports/uses LMStudioConfig policy; no duplicate policy
    ProviderErrorMapping.h                 # maps transport result -> ChatProviderErrorCategory (uses ProviderRequestRuntime)
  contract/
    IProviderProtocol.h                    # protocol strategy: build request, parse response, parse stream frame, parse catalog
  protocols/
    openai_compatible/OpenAiCompatibleProtocol.{h,cpp}   # pure serialization/parsing
    anthropic_messages/AnthropicMessagesProtocol.{h,cpp}
    gemini_generate_content/GeminiGenerateContentProtocol.{h,cpp}
    ollama_native/OllamaNativeProtocol.{h,cpp}
  adapters/
    ollama/OllamaChatAdapter.{h,cpp}       # moved from chat/OllamaChatProvider (re-export kept)
    llamacpp/LlamaCppAdapter.{h,cpp}
    lmstudio/LmStudioAdapter.{h,cpp}
    openai/OpenAiAdapter.{h,cpp}
    deepseek/DeepSeekAdapter.{h,cpp}
    groq/GroqAdapter.{h,cpp}
    mistral/MistralAdapter.{h,cpp}
    claude/ClaudeAdapter.{h,cpp}
    gemini/GeminiAdapter.{h,cpp}
  registry/ProviderRegistry.{h,cpp}        # descriptor table + factory lookup (replaces if/else)
core/src/provider/…                        # same tree
```

Design rules:

* **Adapters own only** identity, default endpoint, auth requirement, discovery rule, provider-specific
  request/response deltas, capability declaration and provider-specific error mapping.
* **Protocol modules own** reusable serialization and parsing, including OpenAI-compatible chat,
  Anthropic Messages, Gemini generateContent and Ollama native `/api/generate`.
* **Shared infrastructure retains** HTTP transport, TLS/network policy, credential redaction,
  deadline/cancellation enforcement, bounded streaming, diagnostics and security-policy enforcement.
  No provider module may open its own HTTP stack or re-implement policy.
* `LMStudioConfig` stays a public compatibility type; a later phase introduces
  `ProviderEndpointPolicy` as the shared owner and keeps `LMStudioConfig` as a thin structural alias
  so existing headers/tests do not break.

## 4. Shared versus provider-specific responsibility matrix

| Concern | Shared owner (module) | Provider-specific owner (adapter) |
| --- | --- | --- |
| HTTP engine / manager creation | `LocalInferenceTransport` + `ProviderRequestRuntime` | — |
| Network policy (Online/LocalOnly/Offline), TLS/loopback, redirects | `NetworkPolicyService` + `ProviderEndpointPolicy` | — (adapters may only *require* a stricter policy, never loosen) |
| Credential storage/redaction | `CredentialStore` / `AppSettings` / `ProviderCredentials` | which credential key each identity reads |
| Deadline, cancellation, retry, backoff | `ProviderRequestRuntime` | mode selection (discovery vs generation) |
| Bounded streaming enforcement | `ProviderRequestRuntime` + `IChatProvider::supportsBoundedStreaming` | protocol frame parsing |
| Error classification | `ProviderRequestRuntime::classify` | status/body→`ChatProviderErrorCategory` refinement |
| Diagnostics shape | `SelectedEndpointChatProvider` finish helper / redaction helpers | provider-specific diagnostic fields |
| Request serialization | protocol module | deltas (system field, tool schema key, image encoding) |
| Response parsing | protocol module | deltas (text path, tool-call shape, finish reasons) |
| Streaming frame parsing | protocol module | deltas (SSE vs NDJSON, `[DONE]` vs `finishReason`) |
| Model discovery parsing | protocol module | identity + endpoint + auth of the discovery call |
| Capability declaration | `ModelService` merge + `ProviderDescriptor` defaults | provider/model advertised capability wire keys |
| Registration/identity | `ProviderRegistry` | descriptor row |
| Model lifecycle (load/unload/pull/remove) | daemon `LocalModelRuntime` (owned) / external (shared) | which identities are ownable vs external |

## 5. Dependency graph

```
IChatProvider ───────────────► ModelRouting (capability/provenance types, ToolDescriptor)
      ▲
      │ implements
provider/adapters/*  ──uses──► provider/protocols/*  ──uses──► provider/common/*
      │                                   │                        │
      │                                   └──► ModelRouting          └──► ProviderRequestRuntime
      │                                                                     └──► NetworkPolicyService
      └──constructed by──► provider/registry ──used by──► ModelService ──used by──► ApplicationController
```

Allowed direction: `common` ← `protocols` ← `adapters` ← `registry` ← `ModelService`. Forbidden:
adapters must not include `ModelService`; protocols must not include adapters; `ModelService` must not
contain protocol logic or hostname branching after migration. `ModelService` keeps only: selection,
factory lookup via `ProviderRegistry`, capability merge, health/catalog state and discovery fan-out.

## 6. Migration phases

Each phase is independently buildable and testable, preserves all existing provider IDs, settings,
stored model associations, credential handling, network restrictions, streaming/cancellation, tool
schemas, error classification, tests and CMake targets, and introduces no live provider calls.

| Phase | Scope | Risk | Exit criteria |
| --- | --- | --- | --- |
| 0 | Freeze baseline: add provider-descriptor/identity headers; no behavior change | Very low | Build + all existing provider tests pass unchanged |
| 1 | **P2-M02 (first extraction):** shared OpenAI-compatible request builder + path resolver + response-text parser + catalog parser moved to `protocols/openai_compatible`; callers forward | Low | Byte-identical golden requests; P2-DEF-01 catalog negatives preserved |
| 2 | Extract native `anthropic_messages` and `gemini_generate_content` builders/parsers from `ModelService::nativeEndpointReply` + `LMStudioLocalInferenceClient::completeNativeChat` + stream branches | Medium | Native fixture matrix (`test_native_provider_contract`) unchanged |
| 3 | Extract discovery parsing (`fetch*Models`) into protocol modules; keep public `OllamaRuntime.h` forwarders | Medium | Discovery tests + hosted IPC/stream parity unchanged |
| 4 | Extract Ollama native protocol + `OllamaChatProvider` into `adapters/ollama`, keep `chat/OllamaChatProvider.h` as re-export | Medium | `test_ollama_runtime`, `test_local_inference`, bounded-streaming tests unchanged |
| 5 | Introduce `ProviderRegistry` descriptor table; replace `providerConfig`/registration/discovery `if/else`; `ModelService` consumes descriptors | Medium-high | All provider IDs/config/discovery paths identical; endpoint-security matrix unchanged |
| 6 | Provider-specific error-mapping strategies on top of `ProviderRequestRuntime::classify` | Medium | HTTP classification matrix identical (P2-04) |
| 7 | Retire orphaned `ProviderErrorClassifier` / `ProviderMessageNormalizer` | Low (product decision) | Confirmed no external/plugin consumer; CMake target list updated |

Compatibility invariants for every phase (assert explicitly): provider IDs unchanged; `providerConfig`
endpoints/credentials unchanged; `selectedRuntimeProvider`/`selectedCloudProvider`/
`selectedModelForProvider` persistence unchanged; capability defaults and `Unknown` semantics
unchanged; reject-before-transport guards unchanged; error categories unchanged; request bytes
unchanged; stream frame acceptance unchanged; Agent tool schemas (`nativeToolDefinition`,
`geminiFunctionDeclaration`) unchanged; existing CMake target names unchanged.

## 7. Compatibility test matrix

| Invariant | Existing test | New/extended test for migration |
| --- | --- | --- |
| Provider IDs and capability defaults | `test_static_provider_catalog`, `test_provider_contracts` | Registry-descriptor parity snapshot |
| Endpoint/credential mapping per ID | `test_provider_contracts`, `test_endpoint_security_capability` | Table-driven `providerConfig` equivalence (old vs new) |
| OpenAI-compatible request bytes | `test_provider_contracts` | Golden serialized-body comparison via `ScriptedProviderTransport` |
| OpenAI-compatible catalog validation (P2-DEF-01) | `test_ollama_runtime` | Protocol-level catalog parser negatives |
| Native Claude/Gemini wire format | `test_native_provider_contract` | Protocol module golden fixture parity |
| Streaming/SSE/NDJSON parse | `test_local_inference`, `test_bounded_streaming_contract` | Frame-parser parity vectors |
| Cancellation/deadline/retry | `test_bounded_streaming_contract`, `test_provider_contracts` | Unchanged runtime calls; timing fixtures |
| Credential readiness metadata | `test_provider_credentials`, `test_credential_store` | Registry descriptor → credential scope parity |
| Error classification/redaction | `test_provider_contracts`, `test_native_provider_contract` | Classification table parity |
| Explicit-root refusal | `test_provider_certification_root` | Unchanged |
| Lifecycle/ownership | `test_model_lifecycle_ownership` | Unchanged |

## 8. Security boundaries

Shared, fail-closed checks that provider modules must not bypass:

* **Unsafe endpoints** — `LMStudioConfig::isSecureTransportEndpoint` (scheme/host/userinfo) and
  `isAllowedEndpoint` (credential required for cloud). A protocol module must receive an already
  validated endpoint, never construct URLs from raw user strings.
* **Missing credentials** — `ModelService::resolve` rejects cloud IDs with no key; discovery helpers
  reject empty keys with `AuthenticationRequired`. Adapters must not default to any key or fallback.
* **Unsupported capabilities** — reject before transport (tools/structured/vision guards).
* **Unauthorized redirects** — manual redirect policy for credentialed requests; surfaced 3xx rejected.
* **Unbounded requests** — explicit deadline + `requireBoundedStreaming`; Ollama only opts in.
* **Streaming timeouts** — bounded-streaming refusal when no positive deadline is supplied.
* **Sensitive error bodies** — redaction helpers (`safeNetworkFailureSummary`,
  `safeOllamaNetworkFailureSummary`); raw upstream bodies must never be reflected.
* **Ambiguous provider/model identity** — normalization + `isKnownProvider` + no silent fallback.

Explicitly **not** approved and must remain unresolved after this design: complete SSRF protection,
DNS resolution/address pinning, rebinding resistance, private-network/metadata-service deny policy,
configurable cloud URL support, and authenticated per-hop redirects. Private-network HTTPS policy
remains a product decision (CSEC-01…10 proposals are not adopted).

## 9. Product decision dependencies

| Decision | Relevance to modularization |
| --- | --- |
| PD-D10 (provider ownership / hard-stop) | Determines whether `llama-cpp-server` (and any future local identity) is "shared external" or "Sentinel-owned"; blocks a unified lifecycle interface across adapters |
| PD-D02/D03/D04 (models, hardware, budgets) | Determines which capability defaults can be calibrated/budgeted rather than declared |
| AC-P2-03 (real backend parity) | Blocks any claim that a modular boundary is certified against real providers |
| CSEC-01…10 (cloud/private-network/DNS policy) | Blocks a provider-neutral endpoint-policy module from adopting a single Online/private rule |
| Configurable-cloud-URL product scope | Determines whether `openai-compatible` becomes a real configurable provider or stays an OpenAI alias |
| Capability provenance/override policy | Determines whether `ProviderDescriptor` may declare `Supported` beyond current defaults |

## 10. Exact first implementation task — P2-M02

**Goal:** create the shared `protocols/openai_compatible` module by extracting only pure,
transport-independent OpenAI-compatible request/response/catalog logic. No behavior change.

**Exact current source files to read/modify:**

* `core/src/runtime/LocalInference.cpp` — `LMStudioLocalInferenceClient::completeOpenAiChat` (path
  resolution, lines ~1200–1203), `infer` local body build (lines ~1557–1569) and cloud body build
  (lines ~1488–1507), response text extraction (lines ~1533–1537, ~1633–1634).
* `core/src/runtime/OllamaRuntime.cpp` — `fetchOpenAiCompatibleModels` catalog validation/shape
  (lines ~1643–1710).
* `core/CMakeLists.txt` — header list (near line 259) and source list (near line 496).

**Proposed new files:**

* `core/include/sentinel/core/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.h`
* `core/src/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.cpp`
* `tests/core/test_openai_compatible_protocol.cpp`

**Suggested API (pure functions, no `QNetworkAccessManager`):**

```cpp
namespace sentinel::core::provider {
QString openAiCompatibleChatCompletionsPath(const QUrl& endpoint);   // /v1 vs /chat/completions rule
QJsonObject buildOpenAiCompatibleChatBody(const QString& model, const QJsonArray& messages,
                                          int maxTokens, bool stream, bool includeTemperature,
                                          double temperature);
std::optional<QString> parseOpenAiCompatibleChatText(const QJsonObject& body, QString* malformedReason);
struct OpenAiCatalogParse { QList<OllamaModelSummary> models; bool malformed = false; };
OpenAiCatalogParse parseOpenAiCompatibleModelCatalog(const QJsonObject& body);
} // namespace sentinel::core::provider
```

**CMake registration changes:** add the new header to the `sentinel_core` header list and the new source
to the source list in `core/CMakeLists.txt`; add
`add_sentinel_core_test(test_openai_compatible_protocol core/test_openai_compatible_protocol.cpp)` in
`tests/CMakeLists.txt`. No new library target, no target rename.

**Test files:** new `test_openai_compatible_protocol.cpp` (golden bodies, path resolution, text
extraction, catalog negatives including P2-DEF-01, empty-but-valid catalog) plus unchanged existing
provider tests as the regression gate.

**Compatibility invariants:** identical serialized request bytes for the existing local/cloud paths;
identical catalog accept/reject decisions; identical text extraction incl. empty→malformed behavior;
identical `/v1` vs `/chat/completions` path selection; no change to endpoint policy, credentials,
streaming, cancellation, error categories or provider IDs.

**Expected integration risk:** low. The extracted functions are pure and already covered by fixtures.
Residual risk is accidental byte differences from parameter omissions (temperature, `stream`, max
token defaults) — mitigated by golden byte comparison.

**Rollback strategy:** revert the two CMake additions and delete the three new files; the forwarders in
`LocalInference.cpp`/`OllamaRuntime.cpp` are single-function bodies whose prior inline text is preserved
in the diff, so revert restores exact previous behavior with no data/settings migration.

**Out of scope for P2-M02:** native protocol extraction, discovery extraction, registry descriptor
table, error-mapping strategies, orphaned-code retirement, and any live provider call.

## Validation

* **Provider tests (pre-built `build/tests`, same commit):** `test_static_provider_catalog`,
  `test_runtime_provider`, `test_provider_contracts`, `test_native_provider_contract`,
  `test_bounded_streaming_contract`, `test_provider_credentials`, `test_ollama_runtime`,
  `test_local_inference`, `test_provider_certification_root` → **9/9 passed** (12.3 s).
* **Source inspections:** `IChatProvider`, `IModelRouter`, `ModelService`, `LocalInference`,
  `OllamaRuntime`, `OllamaChatProvider`, `ProviderRequestRuntime`, `NetworkPolicyService`,
  `ProviderCredentials`, `RuntimeProvider(Capabilities)`, `StaticProviderCatalog`, discovery
  orchestration, CMake target/source lists, tests and fixtures.
* **Documentation paths** verified relative to `docs/audits/` (links resolve in-tree).
* **Git hygiene:** worktree clean at entry; this task added exactly one documentation file and performed
  no commit/push/merge/reset/stash/PR/workflow dispatch. No personal paths, credentials or captured
  provider payloads were added. Privacy scan failures documented by earlier workstreams were not
  touched.

## Deliverable index

1. Actual provider inventory — §1. 2. Verified current code structure — §2.
3. Proposed final module structure — §3. 4. Shared vs provider-specific matrix — §4.
5. Dependency graph — §5. 6. Migration phases — §6. 7. Compatibility test matrix — §7.
8. Security boundaries — §8. 9. Product decision dependencies — §9. 10. Exact first task (P2-M02) — §10.

## Scope statement

**AC-P2-01 remains Unverified / not satisfied. AC-P2-03 remains Blocked. PD-D10 remains Pending
Decision. This document is architecture and migration design only and grants no acceptance.**
