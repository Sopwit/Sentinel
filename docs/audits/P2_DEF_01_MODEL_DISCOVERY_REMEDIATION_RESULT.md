# P2-DEF-01 — Model Discovery Remediation Result

Date: 2026-10-16. Existing branch `test/p2-01-provider-contracts`, HEAD `0245e5fa94584a357a6b1de01344f69b577e0e41`. **P2-DEF-01 corrected in the exercised compatible discovery contract. AC-P2-01 remains Unverified / not satisfied.**

## Isolation and inputs

Verified the supplied existing worktree, branch, HEAD and all uncommitted modifications before editing; snapshotted every pre-existing tracked and non-ignored untracked file. Personal-Brain was unavailable on PATH. No reset, stash, branch switch, commit, push, merge, cloud API, credential-store modification, model load/download/install or runtime configuration change occurred.

Reviewed [AGENTS](../../AGENTS.md), [P2-01](P2_01_PROVIDER_CAPABILITY_CONTRACT_RESULT.md), [P2-02](P2_02_PROVIDER_ERROR_REDACTION_RESULT.md), [P2-03](P2_03_VISION_CAPABILITY_ENFORCEMENT_RESULT.md), [acceptance matrix](../roadmaps/ACCEPTANCE_MATRIX.md), previously read architecture/runtime/security/build/testing guidance, provider/model documentation and the parser, transport, ModelService catalog and existing fixture sources. Historical reports remain byte-identical and keep their original evidence/status descriptions.

Only three files are authored in this task:

- core/src/runtime/OllamaRuntime.cpp
- tests/core/test_provider_contracts.cpp (extends the existing uncommitted fixtures)
- docs/audits/P2_DEF_01_MODEL_DISCOVERY_REMEDIATION_RESULT.md

## Original root cause

[fetchOpenAiCompatibleModels](../../core/src/runtime/OllamaRuntime.cpp) copied getJson's transport/JSON-object success directly to ProviderDiscoveryOutcome.completed, then called `value("data").toArray()` without checking its type. Qt converted a missing, null, object or string field to an empty array. The loop likewise converted non-object entries to empty objects and skipped missing/empty/non-string IDs. Therefore malformed responses could become completed empty catalogs, or partial valid catalogs after invalid entries were discarded.

[ModelService::acceptProviderDiscovery](../../core/src/model/ModelService.cpp) correctly trusts the supplied outcome: a completed observation replaces the stored models. A false completion therefore established Empty/Available rather than Failed/Stale and could erase an earlier valid catalog. The defect belongs in response validation, not another registry or a second catalog. Sequence rejection, native-catalog precedence and model-selection semantics already exist and remain unchanged.

The parser's established required identity is a nonempty string `id` (trimmed); optional capability fields retain their existing typed parsing and Unknown behavior. No new mandatory optional fields, fabricated IDs or provider fallback are introduced.

## Production correction

- Initialize the outcome as incomplete, retaining transport category, observed HTTP status and request ID.
- Require `data.isArray()` before iteration.
- Require every entry to be an object with a string `id` whose trimmed value is nonempty. Any invalid entry rejects the **whole** response, including preceding valid entries; no partial catalog escapes.
- Set completed=true/None only after all entries validate. A legitimate empty array is explicitly successful. Existing optional capabilities, created timestamp and descriptive metadata parsing are retained. Clear errorOut on success.
- Invalid structure yields MalformedResponse, completed=false, empty returned models and a fixed bounded public message. Raw body/IDs/errors are not projected.
- Preserve actual HTTP status in getJsonOnce's malformed-JSON/non-object result; previously that aggregate initializer dropped it to zero. This also permits an exact status receipt for HTTP 200 syntax/shape failures. Existing status classification, retries, cancellation and deadlines remain unchanged.

No ModelService, ProviderRequestRuntime, routing, default capability, AgentRuntime, Rust, CLI or workflow source changes. The known native HTTP 408/429 classification quirk is not modified.

## Valid and invalid discovery matrix

The existing discoveryContract uses fetchOpenAiCloudModels with a synthetic bearer value against an ephemeral loopback endpoint; this exercises the shared compatible parser without any real cloud request. The catalog lifecycle fixture directly calls fetchOpenAiCompatibleModels and feeds actual returned outcomes/models to ModelService.

| HTTP/body | Result | Category / catalog meaning |
| --- | --- | --- |
| 200, data containing one or multiple objects with nonempty string IDs | Exact model names returned, completed=true | None; successful observed catalog |
| 200, `data: []` | Empty list, completed=true | None; legitimate empty catalog |
| 200, missing data or data object/string/null | Empty list, completed=false | MalformedResponse |
| 200, null/array/string/number entry | Empty list, completed=false | MalformedResponse |
| 200, object with missing/numeric/null/empty/whitespace-only id | Empty list, completed=false | MalformedResponse |
| 200, valid entry followed by invalid entry | Empty list, completed=false | MalformedResponse; no partial catalog |
| 200, malformed JSON or top-level array | Empty list, completed=false; HTTP 200 retained | MalformedResponse |
| 401/403 | Non-success, no models | AuthenticationRequired |
| 404 | Non-success, no models | ModelNotFound |
| 429 | Non-success, no models | RateLimited; existing two discovery attempts retained |
| 500 | Non-success, no models | ProviderUnavailable; existing one-attempt behavior for this status retained |

Optional malformed boolean/limit metadata remains Unknown/absent under existing tests rather than invalidating an otherwise valid identity. Required structure validation does not promote metadata to Supported or claim endpoint/model readiness.

## Before/after regression evidence

Added **19 discovery data rows** to the original five-row fixture plus **one catalog-sequencing case**. Before production changes, the selected regression run had **17 failures**: malformed structures/entries, lost HTTP status on syntax/top-level-shape errors, and malformed initial catalog acceptance. The original DEF-01 witness retained its expected failure. Valid and explicitly empty responses continued to pass.

After correction, the new discovery cases and lifecycle case passed. The original DEF-01 assertion `!outcome.completed` produced **XPASS**. Only after observing that actual pass was its QEXPECT_FAIL removed. No previous safety assertion was removed or weakened; relevant request-count expectations explicitly retain the existing 429 two-attempt discovery policy.

Catalog lifecycle evidence:

1. Initial malformed observation creates Failed/MalformedResponse; resolving the selected model returns ProviderUnavailable.
2. A newer valid observation creates Available with exact fixture ID; an older malformed observation cannot overwrite it. An absent model returns ModelNotFound.
3. A newer malformed observation retains the prior model list as Stale/MalformedResponse; the existing known model remains resolvable under unchanged stale-catalog semantics. Health remains Unknown, separate from observed catalog state.
4. A newer legitimate empty observation creates Empty and the prior model resolves ModelNotFound. Selected provider/model identity is unchanged throughout; discovery does not silently select another model.
5. Registered synthetic Ollama and cloud fallback factories are invoked **zero times**. Three explicit fixture HTTP calls cover malformed, valid and empty observations; no external endpoint is contacted.

Public errorOut and ModelService safeDetail are checked for absence of PRIVATE_DISCOVERY/synthetic credentials and bounded size. No raw error body is added to logs or diagnostics. Disposable loopback fixtures use in-memory settings and test credentials only.

## Preservation of prior P2 work

P2-02 LocalInference/ProviderErrorClassifier corrections, P2-03 ModelService vision guard, all previous reports, local-inference tests, fixture support and CMake registration remain task-entry byte-identical. The complete provider target reruns public-error/streaming-envelope redaction, the **72-row vision matrix**, frozen binding/configuration tests, native-tool/structured-output tests, capability Unknown handling and no-fallback checks. Local-inference tests rerun stream completion, cancellation, disconnect and timeout boundaries. No accepted behavior is replaced by success suppression or weaker assertions.

## Validation

Host: existing macOS 27.0.1 / Darwin arm64, installed CMake 4.4.4 and LLVM clang-format/clang-tidy 23.1.3; existing Qt/AppleClang build setup. No tooling installed. Local fixture results do not establish Linux/Windows/native cloud compatibility.

| Check | Result |
| --- | --- |
| `cmake --preset tests -DSENTINEL_USE_CCACHE=OFF` | Passed |
| `CCACHE_DISABLE=1 cmake --build --preset tests -j6` | Full build passed |
| Focused provider-contract Qt XML | Final totals appended below; no remaining expected-failure annotation for DEF-01 |
| Local-inference Qt XML | Final totals appended below; includes cancellation/timeouts |
| Scoped clang-tidy | Passed on OllamaRuntime.cpp and provider-contract tests: clang-analyzer-*, bugprone-use-after-move, bugprone-dangling-handle, warnings-as-errors; analysis-only database strips production PCH |
| clang-format | Full provider-contract test file passed. Touched discovery function matches clang-format output exactly. Whole OllamaRuntime.cpp dry-run reports **55 pre-existing violations outside the touched function**, down from 64 at entry; unrelated formatting preserved |
| Both IPC generators `--check` | Passed; generated files unchanged |
| Privacy/path scan | Zero flagged files |
| Whitespace | git diff --check passed |

Complete CTest, focused totals and final preservation evidence are appended below. Whole-file formatting debt is a pre-existing validation limitation, not a runtime test failure or reason to reformat unrelated paths.

## Remaining issues and acceptance

The three recorded P2 defects now each have scoped correction evidence: DEF-01 in this report, DEF-02 in P2-03 and DEF-03 in P2-02. Historical reports are not rewritten. **Native-tool HTTP 408/429 category precedence remains outstanding**: ModelService can project MalformedResponse despite correct Timeout/RateLimited transport categories; its existing tests and behavior are preserved.

**AC-P2-01 remains Unverified / not satisfied.** Native Claude/Gemini successful/negative protocol fixture seams, vendor-specific advertised-operation coverage, and applicable capability/availability/error dimensions still require review and evidence. Optional metadata parsing or a successful mock catalog is not proof a real model is loaded, capable, healthy or reliable. No real-backend/model, phase or release acceptance is granted; no shared matrix or acceptance threshold changes.

Recommended next task: separately address native 408/429 classification precedence, then add approved transport seams and native/vendor operation coverage for Claude/Gemini and advertised compatible providers before reassessing AC-P2-01. No cloud access or model acquisition is authorized by this recommendation.

## Final validation receipt

Complete `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure`: **128/128 targets passed**, 153.55 seconds. No unexpected or unrelated runtime test failures observed.

Final direct provider-contract XML: **153 pass records**, including setup/cleanup; **151 case/data-row executions**, zero failures, expected failures or skips. Final local-inference XML: **51 pass records**, including setup/cleanup; **49 cases**, zero failures/skips. All P2-02 security rows, P2-03 vision rows and frozen-binding tests passed. The original DEF-01 witness is now an ordinary passing assertion.

Preservation compared 5231 pre-existing files; only the two authorized source/test paths differ. P2-02 and P2-03 production/report bytes and other fixtures are unchanged. An exact-method comparison additionally verified 27 prior safety test methods byte-identical; only discovery data/assertions, the proven DEF-01 annotation and a new lifecycle test were changed. Original checkout remains clean. Report links/table structure/whitespace and privacy checks passed. Whole-file formatting debt remains explicitly recorded above.

| Corrected source/test | SHA-256 |
| --- | --- |
| core/src/runtime/OllamaRuntime.cpp | `b289ef1498c97c38d488c759529d069fcff585157a2e1963014e8556ae285051` |
| tests/core/test_provider_contracts.cpp | `66f9a52500a3c489b7eda9425241b1c36f88d2b8a68a73d847b8af739ccd1335` |
