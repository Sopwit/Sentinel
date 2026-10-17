# P2-M04 — Native response normalization modularization result

Incremental, behavior-preserving continuation of the P2-M03 native-protocol extraction. This document
records the extraction of the remaining native **response** part scanning (`tool_use` / `functionCall`
blocks, text extraction, tool-call identifiers, malformed/empty handling) out of `ModelService` and into
the existing transport-independent protocol modules, plus a scoped residual extraction on the
OpenAI-compatible response path.

This document does **not** claim live provider certification. No live/vendor API call, credential, or
capability was added or changed; all evidence is deterministic, in-process fixtures.

---

## 0. Isolation and baseline

| Item | Value |
|---|---|
| Repository | `Sentinel-sanitized` (single checkout) |
| Verified base revision | `3f85864136de077191dfbf8a7f4fd3f9c0d37151` (merged `main`, contains INT-07/INT-08, P2-M03, P4-04, P6-03, P7-02A) |
| Working branch | `refactor/p2-m04-provider-response-modularization` (created from the verified base; not previously existing) |
| Working-tree state at start | clean (`git status --porcelain` empty) |
| P2-M03 dependency | present in the base (`buildAnthropicNativeRequestBody` / `buildGeminiNativeRequestBody` and both native audit docs are in `HEAD`) |
| Build tree | reused the existing `build/tests` (no separate build tree, no worktree, no clone) |

Preconditions from the task were re-verified after the maintainer committed the shared-checkout
recovery work: the tree is clean, `main` contains `origin/main` as an ancestor with no unresolved
divergence, and INT-08 + P2-M03 are integrated.

No commit, push, merge, rebase, reset, stash, clean, worktree, or PR was performed. All changes are left
uncommitted on the feature branch for maintainer review.

---

## 1. Scope and exact extracted operations

The pre-refactor `ModelService::nativeEndpointReply` interleaved transport-independent response scanning
with adapter policy. P2-M04 moves only the transport-independent operations; every policy decision that
needs the tool registry, binding capabilities, continuation history, or error taxonomy stays in the
adapter.

### 1.1 Anthropic Messages — `AnthropicMessagesProtocol`

New pure operation `parseAnthropicMessagesResponse(const QJsonObject& body)` returning
`AnthropicMessagesResponse`:

* **Content presence guard** — `hasContent` is true iff `content` is a JSON array with ≥ 1 block
  (mirrors the previous `parts.isEmpty()` rejection).
* **Raw content preservation** — `contentBlocks` returns the raw `content` array verbatim (needed by the
  adapter for continuation assembly).
* **Native text extraction** — concatenation of `text` from every block whose `type == "text"`, in
  provider order.
* **Tool-call extraction** — every block whose `type == "tool_use"`, normalized to
  `{callId: "id", name: "name", input: raw "input"}`. A missing `id` surfaces as an empty `callId`
  (the adapter rejects it); a non-object `input` is forwarded unchanged (the adapter rejects it).
* **Multi-part normalization / malformed handling** — unknown block types and non-object blocks are
  ignored, exactly as the previous loop did.

### 1.2 Gemini generateContent — `GeminiGenerateContentProtocol`

New pure operation
`parseGeminiGenerateContentResponse(const QJsonObject& body, const QString& requestId)` returning
`GeminiGenerateContentResponse` with a `GeminiGenerateContentResponseStatus` classification:

* **Terminal-state ordering** — `Blocked` (empty `candidates` with a non-empty
  `promptFeedback.blockReason`) precedes `Rejected` (first-candidate `finishReason` is `SAFETY` or
  `PROHIBITED_CONTENT`), which precedes `NoContent` (no candidates and no block reason, or empty
  candidate `parts`); `Ok` otherwise. This reproduces the previous nested-branch precedence.
* **Continuation content** — `modelContent` is the first candidate's `content` object verbatim.
* **Native text extraction** — concatenation of `text` from every part carrying a `text` member.
* **Tool-call extraction** — every part carrying a `functionCall`, normalized to
  `{callId, name: "name", args: raw "args", thoughtSignature: part "thoughtSignature"}`.
* **Tool-call identifiers** — a provider `id` is forwarded; a missing `id` is synthesized as
  `sentinel-gemini-<requestId>-<n>` (1-based, incremented per synthesized id in part order), exactly as
  the previous inline counter did.
* **Malformed handling** — non-object parts, parts with neither `text` nor `functionCall`, non-array
  `parts`, and non-object `content` all fall through to the same non-`Ok` classification the adapter
  already mapped.

### 1.3 OpenAI-compatible residual — `OpenAiCompatibleProtocol`

New pure operation `parseOpenAiCompatibleToolCalls(const QJsonObject& body)` returning
`QList<OpenAiCompatibleToolCall>`:

* **Envelope flattening** — `choices[0].message.tool_calls[].function.{name,arguments}` flattened to
  `{callId, name, arguments}`, in provider order; `arguments` (normally a JSON-encoded string) is
  forwarded unchanged.
* **Empty/malformed handling** — missing/non-array `choices`, missing/non-object first choice or
  `message`, or missing/non-array `tool_calls` yields an empty list; a non-object list entry is surfaced
  with empty fields.

This is the only Part C extraction; it is transport-independent, needs no registry, and preserves the
adapter's existing decode-and-validate step (see §3.3).

---

## 2. New interfaces and dependency boundaries

* Declared structs (`AnthropicResponseToolUse`, `AnthropicMessagesResponse`,
  `GeminiResponseFunctionCall`, `GeminiGenerateContentResponse`, `OpenAiCompatibleToolCall`) are
  aggregate value types in `namespace sentinel::core::provider`; they carry no policy and no Qt widget
  or transport dependency.
* New functions are free functions in the same `provider` namespace as the existing protocol parsers,
  matching `parseAnthropicMessagesText` / `parseGeminiGenerateContentText` /
  `parseOpenAiCompatibleChatText`.
* The protocol modules still depend only on `QJson*` / `QString` / `QList` / `QUrl`
  (`OpenAiCompatibleProtocol` additionally keeps its existing `OllamaRuntime` include for
  `OllamaModelSummary`). No new include edge, library target, or CMake target was added or renamed.
* `ModelService.cpp` adds one include (`OpenAiCompatibleProtocol.h`); the Anthropic/Gemini protocol
  headers were already included.

---

## 3. Before/after behavior evidence

### 3.1 Anthropic native response (pre-refactor, inline)

```cpp
parts = completion.body.value(QStringLiteral("content")).toArray();
auto history = body.value(QStringLiteral("messages")).toArray();
history.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("assistant")},
                           {QStringLiteral("content"), parts}});
claudeContinuation = /* compact(history) */;
...
if (parts.isEmpty())
    return invalid(QStringLiteral("Native provider response has no content."));
for (const auto& value : parts) {
    const auto part = value.toObject();
    const auto type = part.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("text")) { result.message += part.value(QStringLiteral("text")).toString(); continue; }
    if (type != QLatin1String("tool_use")) continue;
    /* id / name / input -> tool id resolution + validation */
}
```

### 3.2 Anthropic native response (post-refactor)

```cpp
const auto parsed = provider::parseAnthropicMessagesResponse(completion.body);
if (!parsed.hasContent)
    return invalid(QStringLiteral("Native provider response has no content."));
result.message = parsed.text;
auto history = body.value(QStringLiteral("messages")).toArray();
history.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("assistant")},
                           {QStringLiteral("content"), parsed.contentBlocks}});
claudeContinuation = /* compact(history) */;
for (const auto& call : parsed.toolUses) {
    const auto error = appendToolCall(call.callId, call.name, call.input, QString{}, claudeContinuation);
    if (!error.isEmpty()) return invalid(error);
}
```

`result.message = parsed.text` is equivalent to the previous incremental accumulation (the reply starts
with an empty message and only text blocks appended to it). The continuation embeds the raw
`contentBlocks`, so the emitted `providerContinuation` bytes are unchanged.

### 3.3 OpenAI-compatible tool calls (pre → post)

The inline `for (const auto& value : replyMessage.value("tool_calls").toArray()) { ... }` loop became a
`for (const auto& call : provider::parseOpenAiCompatibleToolCalls(completion.body)) { ... }` loop. The
tool-name→id resolution, `QJsonDocument::fromJson(arguments)` decode, and the
`toolId.isEmpty() || callId.isEmpty() || !arguments.isObject() || parseError != NoError` rejection with
the exact string `"Invalid or unregistered native tool call."` all remain in the adapter. Only the
envelope flattening moved.

### 3.4 Gemini terminal-state equivalence

| Response shape | Before | After |
|---|---|---|
| empty `candidates` + non-empty `promptFeedback.blockReason` | `providerFailure("Gemini blocked the request.", RequestRejected, ProviderFailure, ...)` | `status == Blocked` → identical reply |
| first candidate `finishReason` `SAFETY`/`PROHIBITED_CONTENT` | `providerFailure("Gemini rejected the response.", RequestRejected, ProviderFailure, ...)` | `status == Rejected` → identical reply |
| no candidates (no block reason) or empty `parts` | `invalid("Native provider response has no content.")` | `status == NoContent` → identical reply |
| `parts` present | parts loop | `status == Ok` → identical text/tool scan |

`reply.requestId = completion.requestId` is applied on both `providerFailure` branches, as before.

---

## 4. Provider compatibility matrix (all 12 items preserved)

| # | Compatibility item | Status | Evidence |
|---|---|---|---|
| 1 | Provider IDs | Unchanged | `ModelService::providerConfig` and registration untouched (diff touches only the native response region, the OpenAI tool-call loop, and one include) |
| 2 | Model selection | Unchanged | no diff in binding/router/selection code |
| 3 | Endpoint resolution | Unchanged | no diff in URL/path helpers; `defaultTransportRemainsProductionHttps` passes |
| 4 | Request-body compatibility | Unchanged | `nativeRequestBodyByteEquivalence` (golden request bytes) passes; request builders untouched |
| 5 | Tool-calling semantics | Preserved | same resolution order, same gating (`enabled && exposedToModel && nativeFunctionName()` match), same rejection strings; characterization + contract tests |
| 6 | Structured output | Unchanged | still parsed in `ModelService` after the tool-call scan |
| 7 | Native response text | Preserved | new protocol tests + `nativeResponseNormalizationEquivalence` (mixed text+tool) |
| 8 | Error categories | Preserved | `Blocked`/`Rejected` → `RequestRejected` + `ProviderFailure`; `NoContent` → `MalformedResponse` (`invalid`); OpenAI decode failure → `malformed` |
| 9 | Streaming / bounded buffering | Unchanged | no diff in streaming paths; contract streaming tests pass |
| 10 | Cancellation / deadlines | Unchanged | no diff in cancellation/deadline handling |
| 11 | Capability negotiation | Unchanged | capability gating untouched; `gemini-3` signature policy retained in the adapter |
| 12 | Credential redaction / network restrictions | Unchanged | privacy scan clean; production HTTPS/credential assertions pass |

---

## 5. Security invariants (preserved, not loosened)

* No new network, filesystem, process, credential, or platform path was introduced; the extraction is
  pure JSON-to-struct normalization.
* Tool calls are still resolved against the registered, enabled, model-exposed tool set; an
  unregistered or disabled function name still yields `MalformedResponse`/`InvalidResponse`.
* Tool-call arguments are still required to be JSON objects before they can be appended.
* Native function identifiers are still required to be non-empty (provider id or, for Gemini, the
  deterministic synthesized id).
* The `gemini-3` continuation-signature requirement is retained in the adapter.
* Adapter-level endpoint/credential policy is untouched; the personal-path privacy scan reports 0 files
  flagged.

---

## 6. Complete test outcomes

### 6.1 Focused suites (baseline → after)

| Suite | Baseline | After P2-M04 | Δ |
|---|---|---|---|
| `test_anthropic_messages_protocol` | 14 | 19 | +5 |
| `test_gemini_generate_content_protocol` | 13 | 25 | +12 |
| `test_openai_compatible_protocol` | 56 | 57 | +1 |
| `test_native_provider_contract` | 77 | 78 | +1 |

New characterization coverage added:

* **Anthropic**: text-only normalization; `tool_use` normalization (input preserved, raw blocks
  preserved); mixed text/tool/unknown-block ordering; multiple tool uses + missing id + non-object
  input; empty/non-array/non-object malformed bodies.
* **Gemini**: text-only normalization with `modelContent`; deterministic id synthesis across mixed
  explicit/synthesized ids plus `thoughtSignature` capture; mixed text+`functionCall`; a data-driven
  terminal-state matrix (`blocked`, `empty-object`, `empty-candidates`, `safety`, `prohibited`,
  `no-parts`, `empty-parts`, `content-not-object`, `block-reason-ignored-with-candidates`).
* **OpenAI-compatible**: multi-call flattening, empty/malformed envelopes, non-object entry, non-string
  `arguments`.
* **Native contract (production path)**: `nativeResponseNormalizationEquivalence` asserts mixed
  text+tool text/tool extraction, raw-content continuation fidelity, provider-order multi-call
  acceptance, Gemini mixed text+functionCall, and synthesized Gemini ids.

### 6.2 Full validation

| Gate | Result |
|---|---|
| Full build (`cmake --build --preset tests`) | clean, 0 errors |
| Full CTest (`ctest --preset tests --output-on-failure`) | **150/150 passed** |
| C++ formatting (pinned clang-format 18.1.8 diff gate over changed hunks) | clean (0 bytes) |
| `git diff --check` | clean (no whitespace errors) |
| Scoped clang-tidy (changed core sources, LLVM 23 analysis DB) | exit 0 on all four; **0** diagnostics in the new Anthropic/Gemini code |
| Personal-path privacy scan (`tools/analysis/check_personal_paths.py`) | 0 files flagged |
| IPC contract (`python3 tools/ipc/generate.py --check`) | in sync (exit 0) |
| Rust `fmt --check` (1.94.0) | clean |
| Rust `clippy --locked --all-targets -- -D warnings` (1.94.0) | clean |
| Rust `test --locked` (1.94.0) | all suites ok |

**clang-tidy delta (honest measurement).** `ModelService.cpp` had 95 diagnostics at the base revision
(92 `readability-braces-around-statements`, 2 `performance-move-const-arg`,
1 `performance-unnecessary-value-param`) and has 97 after the change (94 braces, same others): the two
added brace diagnostics are in the new guards, matching the file's pervasive single-statement-if idiom
(the same class already dominates the file). CI runs clang-tidy without `-warnings-as-errors`, so this
is non-fatal and pre-existing in kind; the two extended native protocol modules add **no** diagnostics.

---

## 7. Exact changed files

```
 core/include/sentinel/core/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.h   (+31)
 core/include/sentinel/core/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.h (+44)
 core/include/sentinel/core/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.h      (+16)
 core/src/model/ModelService.cpp                                                                 (-84 net / 146 changed)
 core/src/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.cpp                    (+24)
 core/src/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.cpp           (+50)
 core/src/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.cpp                      (+17)
 tests/core/test_anthropic_messages_protocol.cpp                                                (+101)
 tests/core/test_gemini_generate_content_protocol.cpp                                           (+119)
 tests/core/test_native_provider_contract.cpp                                                    (+81)
 tests/core/test_openai_compatible_protocol.cpp                                                  (+49)
 11 files changed, 594 insertions(+), 84 deletions(-)
```

`ModelService.cpp` changes are confined to three regions: the new `OpenAiCompatibleProtocol.h` include,
the native response scan (~`nativeEndpointReply`), and the OpenAI-compatible tool-call loop.
`docs/audits/README.md` is intentionally left unchanged (it already does not index P2-M03/M02-era
extraction reports).

---

## 8. Remaining `ModelService` responsibilities (intentionally not moved)

1. **Request assembly** — native Anthropic/Gemini and OpenAI-compatible request bodies, including tool
   declarations, structured-output schemas, images, and max-token budgeting.
2. **Tool-name→id resolution** — `nativeFunctionName()` matching plus `enabled && exposedToModel`
   gating; unregistered names are rejected.
3. **Continuation-history assembly** — appending raw response content to the request messages/contents
   and compact-serializing it.
4. **`gemini-3` signature policy** — first function call must carry a `thoughtSignature`.
5. **Error-category mapping** — `invalid`/`malformed`/`providerFailure` lambdas and HTTP/attempts/retry
   fields.
6. **Structured-output decoding** — JSON parse/validation of the final message.
7. **Diagnostics** — `finish_reason`, usage/token counts, response sizes.
8. **Streaming, cancellation, deadlines, capability negotiation, credential/endpoint policy.**

---

## 9. Remaining OpenAI-compatible logic inventory (for the ProviderRegistry migration)

Still inline in `SelectedEndpointChatProvider::sendRequest` / `ModelService`, transport-independent but
not yet extracted:

* **Primary OpenAI-compatible chat body** (P2-M02 residual) — message/role conversion with image
  content, tool-call continuation (`tool_calls` + `tool` results), `response_format`, `max_tokens`
  budget clamps, and budget diagnostics.
* **`choices[0]` response normalization** — content extraction (still read inline here rather than via
  `parseOpenAiCompatibleChatText`), `finish_reason` mapping/rejection, usage-token extraction, and
  response diagnostics.
* **Tool-call resolution/validation** — intentionally registry-bound; stays in the adapter.

Still coupled elsewhere (unchanged by P2-M04):

* **Discovery** — transport + outcome mapping coupled in `OllamaRuntime.cpp`; Gemini/Anthropic discovery
  parsing remains provider-specific inline.
* **Registration/identity** — provider IDs, endpoints, credentials, and discovery fan-out remain a
  hand-written `if/else` chain in `ModelService::providerConfig` and `ApplicationController`.
* **Error-mapping strategies** — retirement of the orphaned `ProviderErrorClassifier` /
  `ProviderMessageNormalizer`.
* **`LMStudioConfig` misnomer** — the `ProviderEndpointPolicy` alias step is not started.

---

## 10. Recommended P2-M05 scope

Complete the "message/role conversion + response parsing" separation for the OpenAI-compatible path, on
the same behavior-preserving pattern:

* Extract the **primary OpenAI-compatible chat body** assembly (tool/structured/image fields) into
  `OpenAiCompatibleProtocol`, leaving max-token clamps and temperature presence as caller-supplied
  values.
* Extract **`choices[0]` response normalization** (content + `finish_reason` + usage) as a pure
  operation, keeping the registry-bound tool-name resolution and the error-category mapping in the
  adapter.
* Add golden fixture vectors mirroring the existing `test_native_provider_contract` data rows, captured
  before extraction.

Explicitly still out of scope: the `ProviderRegistry` descriptor table, discovery extraction,
error-mapping strategies, provider-owned lifecycle, and any live provider call. `AC-P2-01` remains
Unverified, `AC-P2-03` remains Blocked, and `PD-D10` remains Pending Decision.

---

## Validation summary

* New protocol characterization tests: 19 (Anthropic) + 25 (Gemini) + 57 (OpenAI-compatible); native
  contract 78. Full sequential CTest: **150/150**.
* Formatting (pinned clang-format 18.1.8 diff gate): clean. Scoped clang-tidy: exit 0 on all changed
  core sources; new native protocol code adds 0 diagnostics. `git diff --check`, personal-path scan,
  IPC contract regen, and the Rust fmt/clippy/test gates: all clean.
* No live/vendor API call, credential, or capability was added; all evidence is deterministic
  in-process fixtures.

## Scope statement

This is an incremental, behavior-preserving continuation of the P2-M03 native-protocol extraction. It
does not complete provider modularization, does not adopt any pending product/security decision, and
grants no acceptance. `AC-P2-01` remains Unverified, `AC-P2-03` remains Blocked, `PD-D10` remains
Pending Decision. Changes are left uncommitted on `refactor/p2-m04-provider-response-modularization`
(based on `3f858641`) for maintainer inspection.
