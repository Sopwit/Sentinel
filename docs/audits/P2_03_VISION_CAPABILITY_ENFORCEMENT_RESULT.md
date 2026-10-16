# P2-03 — Vision Capability Enforcement Result

Date: 2026-10-16. Branch: `test/p2-01-provider-contracts`; HEAD: `0245e5fa94584a357a6b1de01344f69b577e0e41`. **P2-DEF-02 corrected in the direct selected-endpoint request contract; AC-P2-01 remains Unverified / not satisfied.**

## Isolation and review

Continued in the exact existing P2-01 worktree without resetting, stashing, switching branches or committing. Verified HEAD, branch and all entry changes. Personal-Brain was absent on PATH; the supplied worktree and Git identity resolved the project. Entry SHA-256 snapshot includes all pre-existing tracked and non-ignored untracked files, including P2-01 fixtures and P2-02 corrections/reports.

Read [AGENTS](../../AGENTS.md), [P2-01](P2_01_PROVIDER_CAPABILITY_CONTRACT_RESULT.md), [P2-02](P2_02_PROVIDER_ERROR_REDACTION_RESULT.md), [acceptance matrix](../roadmaps/ACCEPTANCE_MATRIX.md), architecture/runtime/security/build/testing guidance, provider capabilities, [ModelBinding metadata](../../core/include/sentinel/core/model/ModelRouting.h), [IChatProvider](../../core/include/sentinel/core/interfaces/IChatProvider.h), [image serialization](../../core/include/sentinel/core/chat/ChatImageContent.h), [attachment loader](../../core/src/chat/ChatAttachmentLoader.cpp), ModelService and existing provider tests.

Only ModelService.cpp, test_provider_contracts.cpp and this report change in P2-03. P2-01/P2-02 historical reports and other fixture sources/CMake registration remain byte-identical. No cloud service, credential store, real model, download, runtime configuration, AgentRuntime, Rust, legacy CLI or workflow changes. Original checkout and other worktrees were not edited. No commit, push or merge.

## Original defect and request path

[SelectedEndpointChatProvider](../../core/src/model/ModelService.cpp) captures its ModelBinding/configuration by value. Its `sendRequest` checked native-tool and structured-output capabilities, but did not check visionInput. Ordinary requests delegated options.images to `sendMessageWithToken`, which created a LocalInferenceRequest and dispatched LMStudioLocalInferenceClient. Native requests serialized images through chatImageContent and then compatible/native completion. Thus an explicit Unsupported binding still transmitted image bytes.

Attachment loader validation (format/dimensions/size) does not enforce model capability. The direct provider API must enforce its own explicit binding even when a higher-level caller has preflight checks. The original witness demonstrated this direct contract defect; it did not prove an end-to-end Desktop/Agent bypass.

## Minimal correction

Added one five-line guard at the start of `SelectedEndpointChatProvider::sendRequest`: when images are nonempty and the captured binding's visionInput is exactly Unsupported, return the existing providerFailure with a fixed safe message, category CapabilityUnsupported, error CapabilityRejected and Failed lifecycle.

The guard runs before ordinary/native/structured branching, endpoint transport, request-body construction and image serialization. It rejects the entire request, including multiple images; it neither drops attachments nor issues a text-only fallback. Existing routing, capability registry, ModelBinding snapshot, provider defaults, cancellation, streaming and P2-02 redaction code remain unchanged.

Unknown remains a distinct metadata value. The existing direct endpoint contract permits attempting an image request when capability is Unknown and lets the backend determine the outcome; this correction preserves that behavior without promoting metadata or certifying vision support. General product guidance that images require a vision-capable model is not evidence that every Unknown binding is supported. A future policy change for Unknown would need a separately reviewed contract; it is not implied by this fix.

## Capability behavior and transport matrix

New deterministic tests exercise LM Studio and llama.cpp endpoint adapters, three vision states, 0/1/3 synthetic image inputs and four request modes: ordinary, native tools, structured output, combined native+structured. Other requested capabilities are explicitly Supported in the fixture to isolate vision behavior. Total: **72 matrix rows**, plus a frozen-binding case. Tests verify typed outcomes, exact image-part/base64 preservation when allowed, safe errors/diagnostics/logs and metadata remaining Unknown in the service catalog.

| Bound vision state | Images | Existing behavior before | Corrected behavior | HTTP calls per invocation before → after |
| --- | --- | --- | --- | --- |
| Unsupported | None | Valid text request succeeds | Unchanged, in all four modes | 1 → 1 |
| Unsupported | One or three | Images transmitted and fixture succeeds | Whole request rejected with typed capability failure | 1 → 0 |
| Supported | One or three | All images serialized and fixture succeeds | Unchanged; no dropped images | 1 → 1 |
| Unknown | None, one or three | Direct attempt permitted, metadata Unknown | Unchanged; fixture success does not promote metadata | 1 → 1 |
| Frozen Unsupported lease after caller/selection changes | One | Reaches transport | Rejected by captured Unsupported binding | 1 → 0 |
| Frozen Supported lease after caller/selection changes | One | Uses captured model/binding | Still uses captured model/binding | 1 → 1 |

The matrix covers all four request modes on both adapters; the frozen case additionally changes the caller's binding and selected model after constructing separate Unsupported and Supported leases. No model-selection change can replace the captured capability. Existing frozen endpoint/native tool/structured tests remain in the complete focused suite. Streaming has no image-bearing sendMessageStreaming argument and is untouched; its existing completion/cancellation/error regressions are rerun.

## Before/after evidence

Before production edit, new tests produced **17 failures**: 16 Unsupported image matrix rows and the frozen rejection case. Each failed on actual HTTP invocation count **1**, expected **0**. The other 56 matrix rows passed, establishing existing text/Supported/Unknown behavior. The original DEF-02 witness still recorded its two expected failures.

With the guard, all 72 matrix rows and the frozen case passed. Both original DEF-02 assertions produced **XPASS** (typed unsupported category and zero transport calls). Only after those actual passes were the two DEF-02 QEXPECT_FAIL annotations removed. DEF-01's expected failure is retained unchanged.

Synthetic PRIVATE_IMAGE payloads are used only with disposable loopback ports and in-memory settings/credentials. Public errors, diagnostic JSON and captured Qt logs exclude the synthetic image marker. The fixed rejection message contains no image contents, model-supplied error text or prompt fragments. Permitted requests verify exact serialized image payloads rather than accepting silent attachment removal.

## P2-02 security preservation

P2-02 LocalInference and ProviderErrorClassifier production bytes are unchanged. Its HTTP 401/403/404/408/429/500/501, malformed/oversized/context, streaming-error, Unknown classifier and original DEF-03 regression assertions still pass. Real Qt diagnostic capture still verifies private-marker absence. All **21 P2-02 added case/data rows**, plus the original DEF-03 witness, pass within the focused provider suite. No error-category/retry/cancellation or logging safeguards were weakened.

## Validation

Host: macOS 27.0.1 / Darwin arm64, existing installed Qt/AppleClang setup, CMake 4.4.4 and LLVM clang-format/clang-tidy 23.1.3. No tooling installation or cross-platform native acceptance is claimed.

| Check | Result |
| --- | --- |
| `cmake --preset tests -DSENTINEL_USE_CCACHE=OFF` | Passed |
| `CCACHE_DISABLE=1 cmake --build --preset tests -j6` | Full build passed |
| Final direct provider-contract Qt XML | **133 pass records**, including setup/cleanup; **131 cases**; **one expected failure** (DEF-01); no unexpected failures/skips |
| Direct local-inference Qt XML | **51 pass records**, including setup/cleanup; **49 cases**; no failures/skips |
| Changed-file clang-format | Passed for ModelService.cpp and test_provider_contracts.cpp |
| Scoped clang-tidy | Passed on both changed translation units with clang-analyzer-*, bugprone-use-after-move, bugprone-dangling-handle and warnings-as-errors; analysis-only database strips production PCH; no diagnostics |
| Both IPC generators `--check` | Passed; no generated contract changes |
| Privacy/path and whitespace checks | Zero files flagged; git diff --check passed |

Complete CTest result and final preservation receipt are recorded below. Source/test fixtures are not real backend/model compatibility evidence or Linux/Windows certification.

## Remaining findings and acceptance

**P2-DEF-01 remains open:** compatible discovery accepts an object lacking data as a completed empty catalog. One expected-failure assertion remains explicitly labelled DEF-01. **Native 408/429 classification remains open:** existing ModelService native projection can return MalformedResponse despite Timeout/RateLimited transport categories; P2-02 documents and tests this unchanged behavior. This task does not edit either path.

**AC-P2-01 remains Unverified / not satisfied.** This result closes the targeted DEF-02 direct-provider fixture boundary and preserves DEF-03 security evidence. Native Claude/Gemini fixture seams, per-vendor advertised-operation coverage and DEF-01 still prevent broader acceptance; successful synthetic image serialization does not prove a model can interpret an image. No acceptance thresholds or shared registers are changed. Historical P2-01/P2-02 reports remain intact and describe their original candidates.

Recommended next task: fix DEF-01 using its retained malformed-discovery witness, then separately correct native HTTP category precedence and complete native/vendor capability coverage before reassessing AC-P2-01. No real-provider contact or model acquisition follows from that recommendation.

## Final validation receipt

Complete `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure`: **128/128 targets passed**, 157.88 seconds. No unexpected or unrelated test failure observed.

Preservation compared 5230 pre-existing files: only the authorized ModelService and provider-contract test bytes differ. All other P2-01/P2-02 files retain task-entry SHA-256. A separate exact-text check verified all previous provider tests unchanged except removal of the two proven DEF-02 annotations. Original checkout remains clean. Report links/tables/whitespace and final privacy scan passed.

| Changed source/test | SHA-256 |
| --- | --- |
| core/src/model/ModelService.cpp | `5ccdf7e7c228a2489cece5e60dc8d6a75e581fbb97504cd867b2fd32799f164f` |
| tests/core/test_provider_contracts.cpp | `e46ceafe542c8cb0dba75bc2e09fa8cab303d142f3bc96ae953ee04e694b55ba` |
