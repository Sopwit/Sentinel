# P1-01.7 — Bounded Agent Planner Streaming Integration Result

Date: 2026-10-16 (Europe/Istanbul). Integration worktree branch `integration/p1-01-7-bounded-planner`, worktree HEAD `35cf03c31d43794660a5eefc477bb3b8aba81667` (the P1 reliability baseline), with **all integration changes uncommitted**. Related acceptance: **AC-P1-01** and dependency **COR-11**. Depends on the P2-05 bounded streaming contract.

**This task wires the existing P2-05 deadline-aware streaming provider contract into `LlmAgentRuntime`, removes the JSON after-first-delta skip, and records real end-to-end evidence through the authoritative Agent Runtime. It does not complete AC-P1-01 and does not enable native incremental tool-call assembly.**

## 1. Scope and isolation

A brand-new worktree was created from the P1 baseline and both source snapshots were imported with controlled, non-destructive patches:

- New integration worktree: `<home>/.codex/worktrees/integration-p1-01-7-bounded-planner/Sentinel-sanitized`
- New branch: `integration/p1-01-7-bounded-planner` at `35cf03c31d43794660a5eefc477bb3b8aba81667`

Neither original worktree was edited, reset, stashed, committed, pushed or merged:

| Source | Branch | HEAD | Dirty state preserved |
| --- | --- | --- | --- |
| P1 | `test/p1-01-agent-reliability` | `35cf03c31d43794660a5eefc477bb3b8aba81667` | 2 modified files (uncommitted P1-01.6 harness + report), unchanged |
| P2 | `test/p2-01-provider-contracts` | `f27f893e7f32420ae28599cd182862f935847348` | 8 modified files + 2 untracked (uncommitted P2-05), unchanged |

The main working tree (`feat/p4-desktop-live-integration`) and every other worktree were not touched. No reset, stash, commit, push or merge was performed anywhere. `git log` on the integration branch shows no commit beyond the P1 baseline `35cf03c3`.

`AGENTS.md`, the [P1-01.5/P1-01.6 report](P1_01_AGENT_RELIABILITY_HARNESS_RESULT.md) and the [P2-05 report](P2_05_BOUNDED_STREAMING_CONTRACT_RESULT.md) were read before editing.

## 2. Source integration method

Both source snapshots branch from the same base (`0245e5fa`) and their file sets are disjoint except for a single shared test-registration file, so a controlled patch integration was used instead of a merge:

1. Worktree created at the P1 HEAD `35cf03c3` (P1 reliability harness, real IPC fixture, independent compile oracle, `RealToolExecutor` fix).
2. P1 uncommitted snapshot applied verbatim (`git diff` of the P1 worktree → 2 files): the P1-01.6 `test_agent_reliability_harness.cpp` delta and its report delta.
3. P2 committed snapshot applied (`git diff 0245e5fa f27f893e` → 14 files): the P2-01…P2-04 provider-contract commits that P2-05 builds on.
4. P2 uncommitted P2-05 snapshot applied (`git diff` of the P2 worktree → 8 tracked files), plus the two untracked P2-05 files copied verbatim.
5. **Overlapping test registrations resolved explicitly in `tests/CMakeLists.txt`:** the P1 harness/IPC registrations and the P2 `test_provider_contracts`/`test_bounded_streaming_contract` registrations live in different regions; the integration file contains the union (`test_provider_contracts`, then `test_bounded_streaming_contract`, then the existing `test_credential_store`, with the P1 harness/IPC blocks retained downstream).

The imported snapshot was byte-verified against the P2 source worktree **before** any production edit: every P2-track file is byte-identical, and every imported core contract file matches the SHA-256 recorded in the P2-05 report exactly (section 9). The P1 fixture files are byte-identical to the P1 worktree. No older provider interface was substituted.

## 3. Architecture changes

One production translation unit changed: [`core/src/agent/LlmAgentRuntime.cpp`](../../core/src/agent/LlmAgentRuntime.cpp) (public header unchanged).

`LlmAgentRuntime::nextStep` now routes a plain text planner completion through the P2-05 bounded overload when — and only when — all of these hold:

- `attempt == 0`, the first (non-repair) request;
- `options.deadlineMs > 0` (the planner keeps its finite `120000` ms deadline);
- no images, no `structuredOutput`, no `nativeToolCalling`;
- the frozen `ModelBinding` advertises `streaming == Supported`;
- `provider_->supportsBoundedStreaming()` is explicitly true;
- a stream observer context is installed.

Otherwise the request uses the existing bounded non-streaming `sendRequest(request, options)` path. There is **no** code path that calls the legacy unbounded `sendMessageStreaming(message, onDelta, token)` overload, and **no** unbounded streaming fallback. Native tool calls, structured output and image requests are deliberately excluded from streaming.

The bounded call passes `requireBoundedStreaming = true` plus the unmodified `ChatRequestOptions` (carrying `deadlineMs = 120000` and `maxOutputTokens = 2048`), so the deadline reaches the transport as `LocalInferenceRequest::options.timeoutMs`. `sendRequest` was not duplicated and `ProviderRequestRuntime` was not modified.

## 4. Bounded planner streaming implementation

The `send` lambda in `nextStep`:

- builds `streamingOptions` from the planner `options` and sets `requireBoundedStreaming = true`;
- calls `provider_->sendMessageStreaming(request, callback, cancellationToken, streamingOptions)`;
- the callback (a) **ignores any delta whose cancellation token is already set**, so late deltas are never accumulated, and (b) accumulates the private planner text into an internal buffer under a fixed `kMaxPlannerStreamedChars = 262144` safety ceiling;
- the accumulated planner text is **never** forwarded to `streamObserver_`, so raw private planner protocol can never be published as `ModelOutputDelta` (the code comment records this boundary);
- if the internal ceiling is exceeded, the run fails closed with a `GiveUp` decision ("Planner streamed output exceeded the internal safety limit");
- the assembled planner text is still taken from the provider reply (`reply.message`), exactly as the non-streaming path, and every existing parser, tool-authorization, grounding and final-answer gate is unchanged.

`buildPlannerPrompt`, `decisionFromLlmOutput`, `decisionFromObject`, the structured/native handling, the transient-retry policy, the frozen `ModelBinding`, the session/turn identity and the shared cancellation token are all preserved. The planner's finite `deadlineMs = 120000` and `maxOutputTokens = 2048` are unchanged.

## 5. COR-11 actual end-to-end results

The fixture provider `CancellationProbeProvider` now implements the real bounded contract (`supportsBoundedStreaming() == true` and the options-carrying `sendMessageStreaming` overload). It delivers a first real delta, then blocks on a deterministic semaphore barrier, then delivers a late delta and a late result after the barrier is released. `exerciseGenerationCancellation` was updated so the text-only rows exercise the streaming path and the native rows assert the non-streaming path.

Focused COR-11 run (`cor11_cancelDuringGeneration`, `cor11_cancelledRequestBindingAndNextTask`, `cor11_cancelAfterFirstStreamedDelta`, `cor11_boundedTransportAndPlannerPrivacy`): **13 passed, 0 failed, 1 skipped** (the single remaining skip is the native-assembly row).

| Required oracle | Actual observed evidence |
| --- | --- |
| Streaming request actually starts through `LlmAgentRuntime` | `json-stream` reports `stream=1`, `request=0`, runtime `ModelOutputDelta = 0` |
| At least one real provider delta arrives before cancellation | The provider barrier is released only after the first delta; the test observes `emittedDeltas >= 1` before `cancel()`. The loopback row `cor11_boundedTransportAndPlannerPrivacy(cancel-after-first)` proves a real OllamaChatProvider delta over HTTP bytes arrives while the transport is still active |
| Cancellation occurs while the provider stream is active | After `cancel()` the runtime phase is `Cancelling`, `AgentCancelled = 0`, provider `cancellationAcks = 0`; the provider call is still blocked on its barrier |
| Late deltas are ignored | The provider emits a second (late) delta after the barrier is released; the runtime publishes `ModelOutputDelta = 0` and accepts no planner text |
| Late tool calls cannot execute | `cor11_cancelledRequestBindingAndNextTask(late-native-tool-call)` and `cor11_cancelDuringGeneration` fence late native/final results; the replay row drives a late `write-file` fragment through the real gates with `write-file = 0` |
| Exactly one `AgentCancelled` terminal | `TerminalAudit` reports `singleTerminal() && cancelled == 1` in every row |
| No successful final after cancellation | `finalAnswer` empty, `AgentCompleted = 0` |
| No unauthorized workspace mutation | `WorkspaceVerifier::untouched()`, `unauthorizedCalls = 0`, `executor.calls = 0` |
| Original `ModelBinding` remains frozen | Every frozen event carries `providerId = ollama`, `modelId = original-model`, one turn id, even after `ModelService` selects `next-model` |
| Next task starts with a fresh token and correct model | Fresh distinct non-cancelled token; `next-model`; one authorized read; exactly one completed terminal; the cancelled session history is byte-frozen |
| Finite transport deadline enforced | Streaming rows record `originalDeadline == 120000` and `originalRequireBounded == true`; the transport rows `boundedStreamingDeadlineReachesTransport` / `deadline-after-first` enforce it |
| Incomplete planner JSON rejected | `cor11_boundedTransportAndPlannerPrivacy(incomplete-planner-json)` replays the actual transport result through the real gates: 2 repair requests, `Failed`, empty final |
| Private planner markers never published | `cor11_boundedTransportAndPlannerPrivacy` asserts the private marker is absent from `errorMessage`, diagnostics, every public event payload, log output, with `ModelOutputDelta = 0` |

The JSON after-first-delta skip (`json-stream`) was removed because its assertions genuinely pass. The `native-tool-assembly` row keeps an explicit, asserted skip (section 7).

## 6. Timeout, terminal and privacy oracles

- **Timeout:** the transport-level `deadline-after-first` row (`TimedOut`/`Timeout`, token never set) and `boundedStreamingDeadlineReachesTransport` (request deadline, not provider config) are retained.
- **Terminal:** the runtime publishes exactly one `AgentCompleted`/`AgentCancelled`/`AgentFailed`; late approvals and duplicate cancels are rejected with the frozen history unchanged.
- **Privacy:** no planner delta reaches `streamObserver_`; the private-marker assertions cover `errorMessage`, diagnostics, public events, logs and `ModelOutputDelta`.

## 7. Remaining native tool-call assembly limitation

`IChatProvider` exposes only a text callback for streaming and returns completed `toolCalls` at the end of a call; there is no typed incremental tool-argument assembly event. `LlmAgentRuntime` therefore keeps `nativeToolCalling` strictly on the bounded non-streaming path. The `cor11_cancelAfterFirstStreamedDelta(native-tool-assembly)` row asserts this routing (`request=1`, `stream=0`, `emittedDeltas=0`, `originalNative=true`, one cancelled terminal, successful next task) and then `QSKIP`s with an explicit message. **No incremental assembly contract was invented and no partially parsed native call can be dispatched.**

## 8. Validation results

Host: macOS 27.0.1 / Darwin arm64; Homebrew ccache; CMake/Ninja `tests` preset; Qt 6.12.0. One host's evidence, not native Linux/Windows or live-backend certification.

| Check | Actual result |
| --- | --- |
| Imported-snapshot consistency | Full build and both imported suites ran before any production edit |
| `cmake --preset tests` + `cmake --build --preset tests` | Configure valid; complete build passed (1217 targets); the final revision rebuilt cleanly (120 targets relinked) |
| Focused COR-11 (4 functions) | **13 passed, 0 failed, 1 skipped** |
| Complete Agent Reliability harness | **97 passed, 0 failed, 1 skipped** (~16.5 s); previously 96 passed / 0 failed / 2 skipped |
| `test_bounded_streaming_contract` (P2-05) | **18 passed, 0 failed, 0 skipped** |
| `test_llm_agent_runtime` | **49 passed, 0 failed, 0 skipped** |
| `test_agent_runtime` | 21 passed, 0 failed, **4 skipped** (pre-existing macOS strict no-fork fixture skips, unrelated) |
| Real COR-17/18 IPC fixture (`test_agent_reliability_ipc`) | **5 passed, 0 failed, 0 skipped** (`QT_QPA_PLATFORM=offscreen`) |
| Full CTest regression (final revision) | **131/131 targets passed**, 188.24 s |
| Formatting (CI-pinned clang-format 18.1.8, `clang-format-diff.py` on the working diff) | No violations |
| Scoped clang-tidy (`-p` PCH-stripped `build/tests`, repo `.clang-tidy`) on `LlmAgentRuntime.cpp` | 0 compiler errors; **no diagnostics on added/changed lines**; 31 pre-existing warnings elsewhere in the file |
| Generated IPC / Desktop contracts (`generate.py --check`, `generate_desktop.py --check`) | Both exit 0, no generated change |
| `git diff --check` | Clean |
| `tools/analysis/check_personal_paths.py` | 1 file flagged: inherited, unmodified `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` |

The 1 remaining harness skip is the intentionally blocked native-assembly row. The 4 `test_agent_runtime` skips are the pre-existing macOS no-fork shell fixtures. The 1 privacy-scan flag is inherited and unmodified.

## 9. Integrated source baseline and exact imported changes

Integration worktree: `<home>/.codex/worktrees/integration-p1-01-7-bounded-planner/Sentinel-sanitized`
Branch: `integration/p1-01-7-bounded-planner` — HEAD `35cf03c3` (uncommitted working tree).

P1 source snapshot: branch `test/p1-01-agent-reliability`, HEAD `35cf03c3…`, uncommitted P1-01.6 delta (`P1_01_AGENT_RELIABILITY_HARNESS_RESULT.md`, `tests/core/test_agent_reliability_harness.cpp`).
P2 source snapshot: branch `test/p2-01-provider-contracts`, HEAD `f27f893e…`, committed P2-01…P2-04 plus uncommitted P2-05 delta.

Files imported unchanged from P2 (SHA-256, match the P2-05 report):

| File | SHA-256 |
| --- | --- |
| core/include/sentinel/core/chat/OllamaChatProvider.h | `a2c4ba696335f7147ce12e002e977fd140c716d6d5d79c6247a53aa52df78c1b` |
| core/include/sentinel/core/interfaces/IChatProvider.h | `c0000502003e7f48c8dbe0454473984f9ed71ed1a362598a14c2785e97e42397` |
| core/include/sentinel/core/runtime/LocalInference.h | `c9ebf1e38dfce902468192745008af5bac5da9900db09aa6c8b537250efa1257` |
| core/src/chat/OllamaChatProvider.cpp | `b90756515aa03d081d70e8c2fc39142410f7f5709725df00eadab4b76578d886` |
| core/src/model/ModelService.cpp | `e72c9d76aa4657b0804b6c1e26295b7b865930f648872420b7454e862730e808` |
| core/src/runtime/LocalInference.cpp | `97b4da21169716e954cae5a9c310cd5db118cc13c845687f787379ccc0e0b27f` |
| tests/support/DeterministicChatFixture.h | `9d79e71bcb18027921bc8f2093f1fdd0c60b88255fcfead4dd3b1c71814c59a9` |
| tests/core/test_bounded_streaming_contract.cpp (new) | `794f2c99e20e5aee2a66d6739ff9358eec22a73d5c05a6b7f0cdd2ff7b6eb831` |

Also imported from the P2 track (committed P2-01…P2-04, byte-identical to the P2 worktree): `core/src/provider/ProviderErrorClassifier.cpp`, `core/src/runtime/OllamaRuntime.cpp`, `core/src/runtime/ProviderRequestRuntime.cpp`, `tests/core/test_local_inference.cpp`, `tests/core/test_provider_contracts.cpp`, `tests/support/ProviderHttpFixture.h`, and the P2 audit documents.

Files changed by this task (production + test):

| File | SHA-256 |
| --- | --- |
| core/src/agent/LlmAgentRuntime.cpp | `0cf84d2885bf9cea69ca0aedda54033a8d9b3ba6fe9cabcd3d646491ad2c318b` |
| tests/core/test_agent_reliability_harness.cpp | `54c2abdfcb49a7591061283eb9e72ef3723bd2825d6b04eb35a99d730165bae2` |
| tests/CMakeLists.txt | union of the P1 and P2-05 registrations (overlap resolved explicitly) |

## 10. Preservation

- Branch/HEAD of both source worktrees unchanged; their dirty state matches entry.
- Nothing staged, committed, pushed, merged, reset or stashed in any worktree.
- `LlmAgentRuntime.h` and all other core headers unchanged; no new provider interface was added (P2-05 already extended `IChatProvider`).
- The P1 COR-07 revocation rows, independent compile oracles, real IPC fixture, COR-01/06/10/16 rows and `IndependentCompileOracle.h` are preserved; only the COR-11 fixture provider, the COR-11 cancellation exercise and its report were edited.

## 11. Production defects

None newly established. The P1-01.5 contract blocker (the planner could not carry a finite deadline into streaming) is resolved by importing P2-05 and gating on `supportsBoundedStreaming()`. No cancellation, deadline, terminal, authorization or privacy gate was weakened, and no fallback, silent elevation or misleading success was introduced.

## 12. AC-P1-01 assessment and remaining gaps

- **COR-11** now has real after-first-delta cancellation evidence through the authoritative Agent Runtime for the JSON planner path, with the after-first-delta skip removed. The native incremental tool-call assembly portion remains explicitly blocked and skipped.
- Other remaining P1-01 gaps are unchanged from P1-01.6: COR-06 capability rows exercise injected registry/handler/provider metadata rather than live Git/browser/Playwright/network/Whisper or real-model integrations; COR-01 broader truncation/absence variants, COR-10 planning-stage/same-runtime variants and COR-16 mutated-resource-without-reread remain as previously assessed; real-IPC/native-platform/live-model limits remain.

**AC-P1-01 remains incomplete / not accepted.** This integration closes the COR-11 planner-streaming contract gap but does not by itself satisfy AC-P1-01.

## 13. Recommended next action

Run the complete Agent Reliability harness, real IPC fixtures, full CTest and static analysis on native Linux (Fedora KDE Platinum target) to confirm the integration beyond this macOS host. Separately decide the native incremental tool-assembly event contract (typed, fenced) before removing the `native-tool-assembly` skip. Continue the remaining COR-01/06/10/16 corpus work. No cloud access, credential use, model acquisition or native-assembly interface is authorized by this result.
