# P2-M03 — Native Anthropic Messages & Gemini generateContent protocol extraction result

Audit date: 2026-10-17. Phase: P2 — Multi-Backend & Capability Negotiation.
Criteria: [AC-P2-01](../roadmaps/ACCEPTANCE_MATRIX.md) (Unverified), AC-P2-03 (Blocked),
[PD-D10](../roadmaps/ACCEPTANCE_DECISIONS.md) (Pending Decision).
**Behavior-preserving incremental extraction. No provider behavior, settings, credentials, network
policy, CI or workflow change. No commit, push, merge, reset, stash, PR or workflow dispatch.**

## 0. Isolation and baseline

| Item | Value |
| --- | --- |
| Branch | `refactor/p2-m03-native-provider-protocols` |
| Worktree | existing Sentinel repository (no new worktree/source copy/build dir) |
| Actual base SHA | `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb` |
| Base = `origin/main` | yes (`git rev-list --left-right --count main...origin/main` → `0 0`) |
| Entry worktree state | clean, on `main`; pre-flight confirmed clean before any edit |
| Root AGENTS.md | read; provider/**AgentRuntime**/**ToolExecutionGateway**/platform boundaries and the `cmake --preset tests` validation contract applied |

### 0.1 Important pre-existing integration (state reconciliation)

The **first half of P2-M03 was already integrated into `main`** before this task started:

* `bef410b1` — *extract pure Anthropic Messages protocol serializer (P2-M03)*
* `d913e0a0` — *extract pure Gemini generateContent protocol serializer (P2-M03)*

Both are ancestors of the base SHA `cd9ec8bc` (`git merge-base --is-ancestor d913e0a0 HEAD` → true),
and `refactor/p2-m03-native-provider-protocols` was already fully contained in `main`. Those commits
created `AnthropicMessagesProtocol.{h,cpp}` / `GeminiGenerateContentProtocol.{h,cpp}`, wired the
transport client (`core/src/runtime/LocalInference.cpp`) through them, and added
`test_anthropic_messages_protocol.cpp` / `test_gemini_generate_content_protocol.cpp`.

That integration covered the **transport-independent URL, simple chat body and response-text**
operations. It did **not** touch `ModelService::nativeEndpointReply`, which still assembled the native
**tool-calling / structured-output request body and tool continuation messages inline**.

This pass completes the remaining P2-M03 scope: it extracts that native request serialization and the
tool-result **message/role conversion** from `ModelService::nativeEndpointReply` into the two protocol
modules, with exact-byte golden tests. It does not rewrite the already-integrated work.

### 0.2 Workspace note (concurrent work)

During this task the shared working tree also contained **unrelated, uncommitted** changes from other
workstreams (`ResourcePolicyTelemetryAdapter`, desktop accessibility `P6-03`, Fedora packaging
`P7-02A`, and their `core/CMakeLists.txt` / `tests/CMakeLists.txt` edits). Those files were **not
touched**. Because the extraction only extends already-registered modules and tests, **no CMake
change was needed**, so this task adds no lines to either `CMakeLists.txt`.

## 1. Scope and exact extracted operations

Extracted in this pass, all **pure and transport-independent**:

1. **Anthropic native request body assembly** — `buildAnthropicNativeRequestBody()`
   (`model`, `messages`, `max_tokens` with 2048 fallback, optional Anthropic-shaped `tools`, optional
   `output_config` json_schema envelope). The native body never carries a `stream` key (unlike the
   infer/stream body already handled by `buildAnthropicMessagesBody`).
2. **Anthropic tool-declaration shaping** — `anthropicToolDeclaration()`: flattens the OpenAI-style
   `{type,function:{name,description,parameters}}` envelope into the Anthropic
   `{name,description,input_schema}` shape.
3. **Anthropic tool-result role conversion** — `anthropicToolResultBlocks()`: provider `tool_use` ids
   and result text → `tool_result` content blocks, clipped to the native continuation limit (6800).
4. **Gemini native request body assembly** — `buildGeminiNativeRequestBody()`
   (`model`, `contents`, optional `{functionDeclarations}` tool envelope, `generationConfig` with
   `maxOutputTokens`/`responseMimeType`/`responseSchema`). `model` is retained so the transport can
   derive the URL and strip it before dispatch, exactly as before.
5. **Gemini function-result role conversion** — `geminiFunctionResponseParts()`:
   `functionResponse` parts with the `id` echoed only for provider-supplied ids (synthesized
   `sentinel-gemini-…` ids stay internal), clipped to 6800.

Operations intentionally left where they are:

* **HTTP transport, TLS/network policy, auth placement, credential redaction, deadline/cancellation,
  retries and error classification** remain in `ProviderRequestRuntime`, `NetworkPolicyService`,
  `LMStudioLocalInferenceClient::completeNativeChat` and `ModelService`.
* **Cross-protocol tool helpers** (`nativeFunctionName` SHA-256 name normalization and the OpenAI-style
  `nativeToolDefinition`) stay in `ModelService`; they are shared with the OpenAI-compatible path and
  are not native-protocol-specific.
* **Continuation-history JSON validation and tool-id mapping** stay in `ModelService`
  (`nativeEndpointReply`) — they are adapter/capability bookkeeping, not serialization.
* **Response text parsing** was already extracted in the pre-existing integration
  (`parseAnthropicMessagesText`, `parseGeminiGenerateContentText`); the response `tool_use` /
  `functionCall` scanning that maps to `ChatProviderReply::ToolCall` remains in `ModelService`.

## 2. New interfaces and dependency boundaries

Added to `namespace sentinel::core::provider`:

```cpp
// AnthropicMessagesProtocol.h
QJsonObject anthropicToolDeclaration(const QJsonObject& nativeToolDefinition);
QJsonObject buildAnthropicNativeRequestBody(const QString& model, const QJsonArray& messages,
                                            int maxTokens, const QJsonArray& tools = {},
                                            const QJsonObject& structuredSchema = {});
struct AnthropicToolResult { QString callId; QString content; };
QJsonArray anthropicToolResultBlocks(const QList<AnthropicToolResult>& results,
                                     int maxContentLength = 6800);

// GeminiGenerateContentProtocol.h
QJsonObject buildGeminiNativeRequestBody(const QString& model, const QJsonArray& contents,
                                         int maxOutputTokens,
                                         const QJsonArray& functionDeclarations = {},
                                         const QJsonObject& structuredSchema = {});
struct GeminiFunctionResult { QString callId; QString functionName; QString content; };
QJsonArray geminiFunctionResponseParts(const QList<GeminiFunctionResult>& results,
                                       int maxContentLength = 6800);
```

Dependency boundary: both modules still depend only on Qt JSON/URL types and `<optional>` — they open
no socket, read no file/setting and hold no credential. The `provider/protocols` → caller direction
from [P2-M01](P2_M01_PROVIDER_MODULARIZATION_ARCHITECTURE.md) is preserved; no `ModelService` header
is included by a protocol module.

## 3. Before/after behavior evidence

| Behavior | Before | After |
| --- | --- | --- |
| Anthropic native body | inline in `nativeEndpointReply` (model/messages/max_tokens/tools/output_config) | `buildAnthropicNativeRequestBody()` |
| Anthropic tool declarations | inline `nativeToolDefinition` unwrap | `anthropicToolDeclaration()` |
| Anthropic tool_result blocks | inline loop with `left(6800)` | `anthropicToolResultBlocks()` |
| Gemini native body | inline in `nativeEndpointReply` (contents/tools/generationConfig) | `buildGeminiNativeRequestBody()` |
| Gemini functionResponse parts | inline loop with id rule + `left(6800)` | `geminiFunctionResponseParts()` |
| Endpoint URLs, headers, auth, streaming, error categories, provider IDs, capabilities | unchanged | unchanged |

The wiring keeps every observable ordering and key set: `ModelService` still validates continuation
history and tool-result ids, still resolves tool names via `nativeFunctionName`, still removes
`model` from the Gemini body in `completeNativeChat`, and still returns the same error strings and
categories.

## 4. Request-body byte-equivalence results

Golden strings were derived from the **pre-refactor** `ModelService::nativeEndpointReply` assembly
(`QJsonObject` serializes keys in sorted order, so only the key/value set determines the bytes) and
hard-coded in the tests; they are **not** produced by the new module under test.

| Case | Test | Golden bytes | Result |
| --- | --- | --- | --- |
| Anthropic simple | `test_anthropic_messages_protocol::testNativeRequestBody` | `{"max_tokens":512,"messages":[{"content":"hello world","role":"user"}],"model":"claude-test"}` | byte-identical |
| Gemini simple | `test_gemini_generate_content_protocol::testNativeRequestBody` | `{"contents":[{"parts":[{"text":"hi"}],"role":"user"}],"generationConfig":{"maxOutputTokens":1024},"model":"gemini-test"}` | byte-identical |
| Claude native tool request (end-to-end, real provider) | `test_native_provider_contract::nativeRequestBodyByteEquivalence` | `{"max_tokens":2048,"messages":[{"content":"synthetic claude prompt","role":"user"}],"model":"claude-sonnet-4-20250514","tools":[{"description":"Read the contents of a workspace file.","input_schema":{"properties":{"path":{"description":"Workspace-relative path.","type":"string"}},"required":["path"],"type":"object"},"name":"read-file"}]}` | byte-identical |
| Gemini native tool request (end-to-end, real provider) | `test_native_provider_contract::nativeRequestBodyByteEquivalence` | `{"contents":[{"parts":[{"text":"synthetic gemini prompt"}],"role":"user"}],"tools":[{"functionDeclarations":[{"description":"Read the contents of a workspace file.","name":"read-file","parametersJsonSchema":{"properties":{"path":{"description":"Workspace-relative path.","type":"string"}},"required":["path"],"type":"object"}}]}]}` | byte-identical |

The two end-to-end rows drive the **real** `SelectedEndpointChatProvider` → `nativeEndpointReply` →
`completeNativeChat` path through the scripted transport and compare the raw `QByteArray` POST body.
`test_native_provider_contract` additionally re-verifies tool-schema identity, tool-name
normalization, continuation formatting, structured-output envelopes, the `501` host-mismatch, and the
`gemini-3` signature rule — all unchanged (77/77).

## 5. Provider compatibility matrix

| Provider ID(s) | Path in this refactor | Change |
| --- | --- | --- |
| `claude` (native) | `nativeEndpointReply` → `buildAnthropicNativeRequestBody` + `anthropicToolDeclaration` + `anthropicToolResultBlocks` | request serialization + role conversion extracted |
| `gemini`/`cloud-api`→`gemini` (native) | `nativeEndpointReply` → `buildGeminiNativeRequestBody` + `geminiFunctionResponseParts` | request serialization + role conversion extracted |
| `openai`, `openai-compatible`, `deepseek`, `groq`, `mistral`, `lm-studio`, `llama-cpp-server`, `openai-compatible-local` | OpenAI-compatible path (already extracted in P2-M02) | unchanged |
| `ollama` | native `/api/generate` | unchanged |

Provider IDs, endpoints, credentials, capability defaults/`Unknown` semantics, `providerConfig`
persistence, discovery fan-out and reject-before-transport guards are unchanged.

## 6. Security invariants (preserved, not loosened)

* Endpoint validation/credential gating (`LMStudioConfig::isSecureTransportEndpoint` /
  `isAllowedEndpoint`, `ModelService::resolve`) is untouched; the protocol modules never build URLs
  from raw strings beyond the already-extracted canonical helpers.
* Authentication placement is unchanged: Anthropic `x-api-key` + `anthropic-version`, Gemini
  `x-goog-api-key` (chat) / `?key=` (discovery).
* No credential, prompt or upstream body is logged or reflected; the golden fixtures are synthetic
  (`claude-test`, `hello world`, `synthetic …`).
* Tool-result continuation truncation limits (`left(6800)`) and the Gemini `id` echo rule are
  preserved byte-for-byte.
* Reject-before-transport guards (unsupported capability, incomplete/mismatched tool results,
  malformed continuation history, unregistered tool, non-object arguments, `gemini-3` missing
  signature) are unchanged.
* Deadline/cancellation/bounded-streaming and error classification remain in the shared runtime.

## 7. Complete test outcomes

Environment: existing `build/tests` tree (Qt 6.12.0, AppleClang 21, Ninja, ccache), run after a full
rebuild.

| Suite | Result |
| --- | --- |
| `test_anthropic_messages_protocol` | **14 passed / 0 failed** (was 10; +4 new: tool declaration, native body, native body + tools/structured, tool-result blocks) |
| `test_gemini_generate_content_protocol` | **13 passed / 0 failed** (was 10; +3 new: native body, native body + tools/structured, function-response parts) |
| `test_native_provider_contract` | **77 passed / 0 failed** (was 76; +1 end-to-end byte-equivalence) |
| Full sequential CTest | **150/150 passed (100%)**, 0 failed, 0 skipped, 215.78 s |

Adjacent provider/credential suites were run individually and passed: `test_openai_compatible_protocol`
(56), `test_provider_contracts` (205), `test_bounded_streaming_contract` (18), `test_local_inference`
(51), `test_provider_credentials` (7).

**Formatting** — CI-pinned `clang-format==18.1.8`: `clang-format-diff.py -p1` over the changed hunks
reports no diff. (Whole-file checking flags pre-existing lines that CI ignores; only changed lines
were reformatted, so the diff has no unrelated churn.)

**Scoped clang-tidy** — LLVM 23 with an analysis compile database built via
`tools/analysis/compile_database.py` (AppleClang PCH dropped): `AnthropicMessagesProtocol.cpp` and
`GeminiGenerateContentProtocol.cpp` produce **no user-code warnings or errors**. `ModelService.cpp`
produces no errors and only the file's pre-existing `readability-braces-around-statements` style
warnings (`.clang-tidy` sets `WarningsAsErrors: ''`).

**Generated IPC checks** — `python3 tools/ipc/generate.py --check` and
`tools/ipc/generate_desktop.py --check` pass; no generated file changed.

**Privacy/credential scan** — `python3 tools/analysis/check_personal_paths.py` flags only one
**unrelated** untracked file from another workstream
(`docs/audits/P7_01_ROASD_DISTRIBUTION_INTEGRATION_READINESS.md`); none of the P2-M03 files are
flagged. A targeted scan of the P2-M03 diff for personal paths, `sk-`/`AIza` keys, PEM headers and
`password=`/`secret=` found none.

**Git hygiene** — `git diff --check` clean (no whitespace errors). No commit, push, merge, reset,
stash, PR or workflow dispatch.

## 8. Exact changed files

| File | Change |
| --- | --- |
| `core/include/sentinel/core/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.h` | +28 (native body, tool declaration, tool-result declarations) |
| `core/src/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.cpp` | +40 (implementations) |
| `core/include/sentinel/core/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.h` | +27 (native body, function-result declarations) |
| `core/src/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.cpp` | +43 (implementations) |
| `core/src/model/ModelService.cpp` | 79 changed (−55 / +24 net): native request assembly delegated to the modules |
| `tests/core/test_anthropic_messages_protocol.cpp` | +92 (4 new characterization tests) |
| `tests/core/test_gemini_generate_content_protocol.cpp` | +87 (3 new characterization tests) |
| `tests/core/test_native_provider_contract.cpp` | +51 (end-to-end byte-equivalence test) |
| `docs/audits/P2_M03_NATIVE_PROVIDER_PROTOCOL_EXTRACTION_RESULT.md` | **new** (this document) |

No change to `core/CMakeLists.txt`, `tests/CMakeLists.txt`, `LMStudioConfig`, `ProviderRequestRuntime`,
`NetworkPolicyService`, `ProviderCredentials`, `StaticProviderCatalog`, presets or workflows. No new
library target, no target rename.

## 9. Remaining provider modularization work

1. **Native response part scanning** — `tool_use` / `functionCall` extraction (text + tool-call
   mapping) is still inline in `ModelService::nativeEndpointReply`; only text extraction lives in the
   protocol modules.
2. **Primary OpenAI-compatible chat body** in `SelectedEndpointChatProvider` still assembles
   tool/structured fields inline (P2-M02 residual).
3. **Discovery** — transport + outcome mapping remain coupled in `OllamaRuntime.cpp`; Gemini/Anthropic
   discovery parsing is still provider-specific inline.
4. **Registration/identity** — provider IDs, endpoints, credentials and discovery fan-out remain a
   hand-written `if/else` chain in `ModelService::providerConfig` and `ApplicationController`.
5. **Error-mapping strategies** and retirement of the orphaned `ProviderErrorClassifier` /
   `ProviderMessageNormalizer` are untouched.
6. `LMStudioConfig` remains a mis-named shared config; the `ProviderEndpointPolicy` alias step is not
   started.

`AC-P2-01` remains Unverified; `AC-P2-03` remains Blocked; `PD-D10` remains Pending Decision. No
pending product/security decision was adopted.

## 10. Proposed P2-M04

**Goal:** extract the native **response part scanning** into the two protocol modules, completing the
"message/role conversion + response parsing" separation for the native protocols.

* Add pure parsers that yield text plus normalized `{callId, name, arguments, signature}` tool-call
  parts for Anthropic `content` and Gemini `candidates[0].content.parts`, preserving the exact
  empty/`MissingContent` semantics already used by `parseAnthropicMessagesText` /
  `parseGeminiGenerateContentText`.
* Keep in `ModelService`: tool-name→id resolution, continuation-history assembly, `gemini-3`
  signature policy and error-category mapping.
* **Golden gates:** add fixture vectors for mixed text/tool responses (mirroring
  `test_native_provider_contract`'s existing data rows) captured before the extraction.
* **Explicitly still out of scope:** discovery extraction, the `ProviderRegistry` descriptor table,
  error-mapping strategies, provider-owned lifecycle and any live provider call.

## Validation summary

* New protocol characterization tests: 14 (Anthropic) + 13 (Gemini); native contract 77.
* Full sequential CTest: 150/150. Formatting (18.1.8 diff gate): clean. Scoped clang-tidy: clean for
  the new protocol code. IPC/privacy/git-diff checks: clean.
* **This document does not claim live provider certification.** No live/vendor API call, credential or
  capability was added; all evidence is deterministic in-process fixtures.

## Scope statement

This is an incremental, behavior-preserving continuation of the P2-M03 native-protocol extraction
(the first half of which was already integrated into `main` as `bef410b1`/`d913e0a0`). It does not
complete provider modularization, does not adopt any pending product/security decision and grants no
acceptance. `AC-P2-01` remains Unverified, `AC-P2-03` remains Blocked, `PD-D10` remains Pending
Decision. Changes are left uncommitted on `refactor/p2-m03-native-provider-protocols` for maintainer
inspection.
