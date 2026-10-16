# P2-05 — Bounded Streaming Contract Result

Date: 2026-10-16 (Europe/Istanbul). Branch `test/p2-01-provider-contracts`, worktree HEAD `f27f893e7f32420ae28599cd182862f935847348`. Related acceptance: **AC-P2-01** and dependency **COR-11**. **This task adds a safe, deadline-aware provider streaming contract with deterministic regression evidence. It does not integrate the Agent planner onto that contract, and it does not complete COR-11 or AC-P2-01.**

## 1. Scope, isolation and inputs

Continued in the exact supplied P2-01 worktree. Verified branch, HEAD and a clean entry tree before editing. The task brief reported base HEAD `c9b99932…`; the actual worktree HEAD is `f27f893e…`, two commits ahead of that base (`868fa5c3` P2-04 fix + `f27f893e` P2-04 audit). All P2-01/P2-02/P2-03/P2-04 work is present and preserved; no uncommitted entry changes were discarded. Personal-Brain was unavailable on PATH. No reset, stash, branch switch, commit, push or merge occurred; the original checkout and other worktrees were not edited.

Read [AGENTS](../../AGENTS.md), [P2-01](P2_01_PROVIDER_CAPABILITY_CONTRACT_RESULT.md), [P2-02](P2_02_PROVIDER_ERROR_REDACTION_RESULT.md), [P2-03](P2_03_VISION_CAPABILITY_ENFORCEMENT_RESULT.md), [P2-04](P2_04_HTTP_CLASSIFICATION_RESULT.md), the [acceptance matrix](../roadmaps/ACCEPTANCE_MATRIX.md) (COR-11, AC-P2-01) and the P1-01.5 agent-reliability findings. Inspected `IChatProvider`, `OllamaChatProvider`, `SelectedEndpointChatProvider`/`HealthTrackedProvider`, `LocalInference` stream clients, `ModelRouting`/`ModelBinding` and the existing provider fixtures.

No real cloud request, credential, personal configuration, model acquisition or runtime configuration change was used. All HTTP fixtures bind ephemeral IPv4 loopback ports and use synthetic markers. The planner is not routed through the new contract and no live model is contacted.

## 2. Contract design

The change **extends the existing request/streaming interfaces** instead of introducing a parallel streaming abstraction.

- [IChatProvider.h](../../core/include/sentinel/core/interfaces/IChatProvider.h): `ChatRequestOptions` gains `bool requireBoundedStreaming = false;` (default off, so existing callers are unchanged). The interface gains:
  - `virtual bool supportsBoundedStreaming() const { return false; }` — declaring or overriding streaming support is **not** a bounded-streaming claim; a provider must opt in explicitly.
  - a new overload `sendMessageStreaming(message, onDelta, cancellationToken, const ChatRequestOptions& options)` whose **default implementation rejects explicitly** (`ChatProviderErrorCategory::CapabilityUnsupported`, `ChatProviderReply::Error::CapabilityRejected`, lifecycle `Failed`). It never silently discards the required deadline and never runs the legacy path unbounded.
- [OllamaChatProvider.h](../../core/include/sentinel/core/chat/OllamaChatProvider.h)/[.cpp](../../core/src/chat/OllamaChatProvider.cpp): `supportsBoundedStreaming()` returns `true`; the bounded overload validates and delegates to a private `streamMessage(...)` that also serves the legacy entry point.
- [LocalInference.h](../../core/include/sentinel/core/runtime/LocalInference.h): `LocalInferenceOptions` gains `bool requireBoundedStreaming = false;`.
- [ModelService.cpp](../../core/src/model/ModelService.cpp): `HealthTrackedProvider` forwards both the capability and the new overload; `SelectedEndpointChatProvider` mirrors `binding_.capabilities.streaming` in `supportsBoundedStreaming()` and implements the bounded overload via a private `streamSelected(...)`.

Semantics enforced by the contract:

1. **Deadline propagation** — the caller's finite `options.deadlineMs` is resolved and pushed into the transport request (`LocalInferenceRequest::options.timeoutMs`) and the transport client is constructed consistently. Resolution rule: `options.deadlineMs > 0 ? options.deadlineMs : providerTimeoutMs_`.
2. **A bounded request can never become an unbounded wait** — if `requireBoundedStreaming` is set and no positive deadline is available, the provider rejects *before* transport and the transport client also refuses. A provider-level timeout cannot silently satisfy a bounded request that carries no deadline.
3. **Explicit rejection over silent degradation** — providers without the capability (including the interface default and `LocalEchoProvider`) reject with a typed capability error.
4. **Capability metadata preserved** — `SelectedEndpointChatProvider::supportsBoundedStreaming()` remains derived from the frozen `ModelBinding` capability; binding identity/authorization are unchanged.

## 3. Deadline + cancellation evidence

- `boundedStreamingDeadlineReachesTransport` constructs the Ollama provider with a **provider-level timeout of 0** and sends a bounded request with `deadlineMs = 700`; the reply reports `diagnostics["deadline_ms"] == 700`, proving the deadline came from the request and reached the transport, not from provider configuration.
- `boundedStreamingTimeoutBeforeAnyDelta` retains the HTTP connection after headers; the request deadline (150 ms) fires, yielding lifecycle `TimedOut`, category `Timeout`, no deltas and `diagnostics["deadline_ms"] == 150`.
- `boundedStreamingTimeoutAfterPartialOutput` delivers `"partial"` then holds without a terminal marker; the deadline fires and the partial response is **rejected as a whole** (`TimedOut`, not `Completed`).
- `boundedStreamingRejectsAbsentDeadline` shows that a finite provider-level timeout (500 ms) does **not** satisfy a bounded request with `deadlineMs = 0`: the request is rejected (`RequestRejected`, lifecycle `Failed`), no transport opens (`server.requests.size() == 0`) and no deltas are emitted.
- `transportRefusesBoundedRequestWithoutDeadline` drives both `OllamaLocalInferenceStreamClient` and `LMStudioLocalInferenceStreamClient` directly with `requireBoundedStreaming = true` and `timeoutMs = 0`; both return `LocalInferenceStreamStatus::Refused` with category `RequestRejected` before any transport.
- `boundedStreamingCancellationBeforeFirstDelta` (no transport opened for a pre-cancelled token) and `boundedStreamingCancellationAfterFirstDelta` (late frames on the wire are fenced after the token flips on the first delta) yield lifecycle `Cancelled`, category `Cancelled`, and no delta after cancellation.

## 4. Cancellation, terminal and malformed semantics

- **Stop deltas after cancellation/timeout/terminal failure**: cancellation and timeout cases assert exact delta counts/text; no late frame is delivered.
- **Partial-response rejection**: an incomplete stream without a terminal marker is `MalformedResponse`/`Failed` even though the partial text was streamed as a delta (`boundedStreamingMissingTerminalMarker`).
- **Terminal-marker validation and delta ordering**: `boundedStreamingCompletesWithOrderedDeltas` asserts ordered accumulation (`alpha beta gamma`) with `reply.message == deltas`; `boundedStreamingRecoversFromMalformedChunk` shows a malformed frame is never delivered as a delta and does not corrupt order, and the run still completes when a valid terminal marker follows.
- **Malformed response**: a fully unparseable body yields `MalformedResponse` with no deltas.
- **Capability-negative**: a `LegacyStreamingProvider` that only implements the legacy entry point reports `supportsBoundedStreaming() == false` and the bounded call is rejected (`CapabilityUnsupported`/`CapabilityRejected`), while its legacy streaming call still succeeds.

## 5. Redaction

`boundedStreamingRedactsUpstreamErrors` runs an upstream error envelope containing a synthetic `PRIVATE_PLANNER_MARKER` for both HTTP 200 and HTTP 500. The reply's `errorMessage` and serialized `diagnostics` never contain the marker, and `diagnostics` carries only allowlisted numeric/boolean metadata (`deadline_ms`, `bounded_streaming`) — never prompts, credentials or reasoning. This preserves the P2-02 redaction guarantee for the new path.

## 6. Backward compatibility

- **Legacy streaming is unchanged**: the original `sendMessageStreaming(message, onDelta, cancellationToken)` overload is retained and now delegates to `streamMessage(...)` with the provider-configured timeout only. `legacyStreamingAndNonStreamingUnchanged` asserts the legacy path completes normally.
- **Non-streaming is unchanged**: `sendRequest` with an options deadline keeps the existing synchronous deadline-bearing path; the fixture reply is non-streaming and carries no `stream` flag.
- **Default-safe**: `requireBoundedStreaming` defaults to `false`, and the interface's bounded overload default-rejects, so no existing caller is silently routed through a different transport.
- **ModelBinding identity/authorization/capability metadata preserved**: `SelectedEndpointChatProvider` continues to derive streaming capability from the frozen binding; a binding with `capabilities.streaming == Unsupported` is a capability-negative for bounded streaming and rejects before transport (`selectedEndpointBoundedStreamingAndCapability`).

## 7. Regression evidence

[test_bounded_streaming_contract.cpp](../../tests/core/test_bounded_streaming_contract.cpp) is registered in [tests/CMakeLists.txt](../../tests/CMakeLists.txt) and covers every enumerated case:

| Case | Asserts |
| --- | --- |
| `boundedStreamingCompletesWithOrderedDeltas` | success, ordered deltas, accumulation, single transport request |
| `boundedStreamingDeadlineReachesTransport` | finite request deadline reaches transport with provider timeout 0 |
| `boundedStreamingTimeoutBeforeAnyDelta` | `TimedOut`/`Timeout`, no deltas |
| `boundedStreamingTimeoutAfterPartialOutput` | partial output streamed, response rejected as a whole |
| `boundedStreamingCancellationAfterFirstDelta` | late frames fenced after cancellation |
| `boundedStreamingCancellationBeforeFirstDelta` | cancellation before/without dispatch, no transport |
| `boundedStreamingMissingTerminalMarker` | terminal-marker validation, partial rejection |
| `boundedStreamingMalformedResponse` | malformed body → `MalformedResponse`, no deltas |
| `boundedStreamingRecoversFromMalformedChunk` | malformed frame never emitted, order preserved |
| `boundedStreamingRejectsAbsentDeadline` | zero/absent deadline never satisfies a bounded request |
| `boundedStreamingRejectsStructuredAndNative` | structured/native modes rejected before transport |
| `transportRefusesBoundedRequestWithoutDeadline` | both stream clients refuse bounded request without deadline |
| `boundedStreamingRedactsUpstreamErrors` | no private markers in errorMessage/diagnostics |
| `legacyStreamingAndNonStreamingUnchanged` | legacy streaming + non-streaming behavior preserved |
| `unsupportedProviderRejectsBoundedStreaming` | interface default and streaming-only provider reject |
| `selectedEndpointBoundedStreamingAndCapability` | binding capability mirrored, bounded success + negative |

Direct runs at the final candidate:

| Suite | Result |
| --- | --- |
| `test_bounded_streaming_contract` | **18 passed, 0 failed** (16 cases + setup/cleanup) |
| `test_provider_contracts` | 205 passed, 0 failed |
| `test_local_inference` | 51 passed, 0 failed |
| `test_chat_mode_service` | 12 passed, 0 failed |
| `test_llm_agent_runtime` | 49 passed, 0 failed |
| `test_chat_application_controller` | 5 passed, 0 failed |
| `test_application_controller` | 129 passed, 0 failed |
| `test_runtime_integration` | 6 passed, 0 failed |

The new test uses only synthetic loopback HTTP fixtures and a deterministic token flip in the delta callback; it contains no arbitrary sleeps and no wall-clock races.

## 8. Validation results

Host: macOS 27.0.1 / Darwin arm64; Homebrew LLVM 23.1.3 (`clang-format` 18.1.8 also used to match CI); CMake/Ninja `tests` preset; Qt 6.12. One host's fixture evidence, not native Linux/Windows or backend certification.

| Check | Result |
| --- | --- |
| `cmake --preset tests` | Configure valid (existing install) |
| `cmake --build --preset tests` | Full build passed (113/113 targets) |
| `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | **129/129 targets passed**, 163.10 s |
| `clang-format` 18.1.8 (`clang-format-diff.py` on the diff + `--dry-run -Werror` on the new file) | No newly introduced violations |
| Scoped clang-tidy on the three changed core TUs (PCH stripped from the analysis database) | No diagnostics on added/changed lines; only pre-existing `readability-braces-around-statements`/`performance-*` debt elsewhere in the touched files |
| `python3 tools/ipc/generate.py --check` | Passed, no generated change |
| `python3 tools/ipc/generate_desktop.py --check` | Passed, no generated change |
| `python3 tools/analysis/check_personal_paths.py` | 0 files flagged |
| `git diff --check` | Passed (no whitespace errors) |

Formatting note: two added lines in `core/src/model/ModelService.cpp` and one signature in `OllamaChatProvider.h` were normalized with clang-format 18.1.8 (matching CI's pinned version); the changed lines were also given explicit braces where clang-tidy flagged new `readability-braces-around-statements` debt. Pre-existing whole-file formatting debt in the touched files was left untouched, distinguishing existing debt from newly introduced violations.

Changed files and SHA-256:

| File | SHA-256 |
| --- | --- |
| core/include/sentinel/core/chat/OllamaChatProvider.h | `a2c4ba696335f7147ce12e002e977fd140c716d6d5d79c6247a53aa52df78c1b` |
| core/include/sentinel/core/interfaces/IChatProvider.h | `c0000502003e7f48c8dbe0454473984f9ed71ed1a362598a14c2785e97e42397` |
| core/include/sentinel/core/runtime/LocalInference.h | `c9ebf1e38dfce902468192745008af5bac5da9900db09aa6c8b537250efa1257` |
| core/src/chat/OllamaChatProvider.cpp | `b90756515aa03d081d70e8c2fc39142410f7f5709725df00eadab4b76578d886` |
| core/src/model/ModelService.cpp | `e72c9d76aa4657b0804b6c1e26295b7b865930f648872420b7454e862730e808` |
| core/src/runtime/LocalInference.cpp | `97b4da21169716e954cae5a9c310cd5db118cc13c845687f787379ccc0e0b27f` |
| tests/CMakeLists.txt | `5155ab321c81033ef8c08010d958b122accc8fbb57e778034cbb163ddbed0621` |
| tests/support/DeterministicChatFixture.h | `9d79e71bcb18027921bc8f2093f1fdd0c60b88255fcfead4dd3b1c71814c59a9` |
| tests/core/test_bounded_streaming_contract.cpp (new) | `794f2c99e20e5aee2a66d6739ff9358eec22a73d5c05a6b7f0cdd2ff7b6eb831` |

## 9. Remaining COR-11 / Agent integration requirements

This task establishes the provider-layer contract only. The following remain **out of scope and unverified**:

- **Planner streaming is not enabled.** `LlmAgentRuntime` still sets `options.deadlineMs = 120000` and `options.maxOutputTokens = 2048` and is intentionally **not** routed through the bounded overload; no AgentRuntime/AgentLoop/LlmAgentRuntime file was touched.
- **No private planner-delta publication.** There is no subscription that exposes planner deltas; only the bounded provider replies/diagnostics (allowlisted) exist. A future integration must keep planner output private and publish only the accepted final answer.
- **Incremental native tool-call assembly** still lacks a typed incremental callback contract; the bounded overload deliberately rejects `nativeToolCalling`/`structuredOutput` rather than inventing a speculative assembly interface.
- **COR-11 execution evidence** — "cancel during model generation after first delta: `run.cancelled` once, late provider result injected and ignored, active binding unchanged, no unrequested tool calls" — requires AgentRuntime-level wiring and run-record assertions that this provider-layer task does not provide.

## 10. AC-P2-01 assessment

[AC-P2-01](../roadmaps/ACCEPTANCE_MATRIX.md) requires per-advertised-operation positive/negative cases, Unknown capability remaining Unknown, and secrets absent from projection. This task adds a bounded-streaming capability boundary with deterministic, provider-layer regression evidence for the reachable Ollama/SelectedEndpoint streaming paths, including capability-negative cases and redaction. **AC-P2-01 remains Unverified / not satisfied**: real-backend parity, complete per-backend advertised-operation matrices and live acceptance are still outstanding. **COR-11 remains Unverified**: the provider-layer tests do **not** demonstrate AgentRuntime cancellation-after-first-delta behavior. No acceptance threshold, matrix cell or phase status was changed.

## 11. Recommended next task

Wire the Agent planner's streaming path through this bounded contract in a dedicated, separately-scoped task: set `requireBoundedStreaming` with the planner's finite deadline, keep planner deltas private, and add AgentRuntime run-record fixtures proving `run.cancelled` fires exactly once after the first delta with late provider results ignored and the active binding unchanged (the COR-11 oracle). Do not publish planner deltas, do not add a native incremental tool-assembly interface, and do not claim COR-11 from provider-layer evidence alone. No cloud access, credential use or model acquisition is authorized by this recommendation.
