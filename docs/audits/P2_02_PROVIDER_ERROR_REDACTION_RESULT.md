# P2-02 — Provider Error Redaction Result

Date: 2026-10-16. Candidate HEAD: `0245e5fa94584a357a6b1de01344f69b577e0e41`; existing branch `test/p2-01-provider-contracts`. **P2-DEF-03 is corrected within the exercised error-projection boundary. AC-P2-01 remains unaccepted.**

## Isolation and inputs

Continued in the existing P2-01 worktree without reset, stash, branch switch or commit. Personal-Brain was unavailable on PATH; the exact worktree and expected HEAD were verified through Git. Entry changes were tests/CMakeLists.txt, tests/core/test_local_inference.cpp, untracked tests/core/test_provider_contracts.cpp, tests/support/ProviderHttpFixture.h and the P2-01 report. Only the provider-contract test file is extended; all other entry files are byte-preserved.

Read [AGENTS](../../AGENTS.md), [P2-01 report](P2_01_PROVIDER_CAPABILITY_CONTRACT_RESULT.md), [acceptance matrix](../roadmaps/ACCEPTANCE_MATRIX.md), architecture/runtime/build/testing guidance, [Providers](../capabilities/PROVIDERS.md), [provider extension guidance](../development/ADDING_A_PROVIDER.md), [Secrets](../security/SECRETS.md), security/threat model and [diagnostic rules](../development/TUI_V3_2_3_PROVIDER_DIAGNOSTICS.md). No personal credentials/configuration, real cloud providers, model acquisition or runtime configuration changes were used. New HTTP tests use ephemeral IPv4 loopback fixtures and in-memory settings/credential storage.

## Confirmed original data flow

[LocalInference.cpp](../../core/src/runtime/LocalInference.cpp), `postJsonOnce`, parsed upstream JSON `error.message` or string `error`, then replaced its local HTTP summary with the first 512 characters of that **untrusted** message. Truncation limited length but did not redact bearer tokens, headers, keys or prompt fragments. `safeNetworkFailureSummary` concatenated that text. `completeOpenAiChat` / native completion and infer passed it to [ModelService](../../core/src/model/ModelService.cpp), `SelectedEndpointChatProvider::sendRequest` / `sendMessageWithToken`, through `providerFailure` into `ChatProviderReply.errorMessage`. Native endpoint adapters use the same JSON request helper. The existing Ollama adapter also projects LocalInference summaries.

The shared reply contract already supplies errors to chat/runtime clients and daemon-backed Desktop/Rust surfaces; changing client renderers would leave the provider boundary unsafe. No CLI/TUI/Desktop/daemon source changes are needed for this fix. Their end-to-end live/native acceptance was not rerun or claimed.

A second exercised path was streaming: NDJSON/SSE parsing occurred before transport failure handling. An HTTP 401/403/429/500 body shaped like valid assistant content emitted the synthetic private marker through callbacks even though the final result failed. A protocol error object with HTTP 200 and plausible assistant fields could even complete successfully. Both parsers needed to reject error payloads before accumulation/callback dispatch.

[ProviderErrorClassifier](../../core/src/provider/ProviderErrorClassifier.cpp), `userMessage`, separately reflected `error.message` in its Unknown fallback. Existing known-category strings were already allowlisted; the fallback was not.

## Minimal production correction

Only two production translation units change:

- LocalInference constructs HTTP/network failure text from the existing `ProviderRequestRuntime::classify` category and numeric HTTP status. No raw upstream body/message or QNetworkReply error string is copied into the public error. HTTP summaries remain bounded by fixed templates.
- To preserve existing ModelService context-rejection behavior, only HTTP 400/422 inspects at most 16 KiB of error JSON for the three existing context signatures. A match becomes the fixed message `Provider request rejected: context length exceeded.` No matched substring or remainder is returned. Malformed/oversized JSON that cannot provide that bounded signal remains a generic rejection. This is a compatibility hint, not trusted evidence about model capacity.
- HTTP failure summaries preserve that safe status/context text even when Qt uses UnknownContentError for a non-JSON-related HTTP error.
- Both stream parsers gate processing on HTTP 2xx and reject `error` envelopes / `type: error` before reading assistant fields. Once a stream error is seen, later frames cannot complete it. Existing malformed-response failure handling remains authoritative. No error-envelope text is emitted in deltas, accumulated text or chunk summaries.
- ProviderErrorClassifier's Unknown public fallback becomes a fixed failure message. Its category inference, retryability and existing known-category messages remain unchanged; no duplicate classifier or logging framework is added.

ProviderRequestRuntime, ModelService, cancellation/deadlines/backoff, request retry limits and permissions are unchanged. Accepted assistant output from earlier valid stream frames cannot be recalled; this fix blocks the error envelope and subsequent content, rather than claiming to redact arbitrary successful model prose.

## Negative regression evidence

Extended the existing [provider-contract target](../../tests/core/test_provider_contracts.cpp) with 21 case/data-row executions: ten public-error rows (each exercising ordinary and native requests), ten streaming rows (NDJSON/SSE), and an Unknown classifier fallback test. Uses only PRIVATE_BEARER, PRIVATE_KEY, PRIVATE_PAYLOAD and PRIVATE_ERROR synthetic markers. HTTP bodies include echoed Authorization/Bearer/API-key/prompt fragments; malformed JSON and a 100,000-character error value are included. The log capture enables the actual Qt provider diagnostics category, verifies structured metadata was emitted, checks absence of private markers and restores the prior logging handler/filter.

| Evidence stage | Observed result |
| --- | --- |
| Before production correction | New regression run: 20 failing rows/assertions; original DEF-03 witness retained one expected failure. Failures include reflected 401/403/404/500/501 messages, oversized public text, ten streaming cases and Unknown fallback. Three failures additionally exposed pre-existing native 408/429/context category projection; not all 20 are secret-reflection assertions. |
| First corrected witness run | Original `echoedSecretsDoNotEnterDiagnostics` assertion produced **XPASS**: its private marker was absent. Only after this evidence was its DEF-03 QEXPECT_FAIL removed. Context hint handling was refined to retain the established context-exhausted outcome. |
| Final focused run | All new rows and the original DEF-03 assertion pass; private markers absent from errorMessage, diagnostic JSON, captured Qt logs, streaming callbacks and accumulated error payload. Failures stay non-success and HTTP status is retained. |
| Other expected failures | DEF-01 keeps one xfail; DEF-02 keeps two xfails. Their test assertions and production sources are unchanged. |

Raw local XML/build/static-analysis logs are disposable local evidence, not public artifact payloads; they may contain host paths. No real credential was leaked in reproduction.

## Classification and diagnostic separation

The existing transport classifier still maps 401/403 to AuthenticationRequired, 404 to ModelNotFound, 408/deadline to Timeout, 429 to RateLimited, 500 to ProviderUnavailable, 501 to CapabilityUnsupported, 400/422 to RequestRejected, malformed successful JSON to MalformedResponse and cancellation to Cancelled. Existing provider-helper authentication/quota/content-filter/context and retry tests pass. New failures do not become successes or permission grants.

An independently observed **pre-existing limitation** remains: native-tool ModelService projection treats Qt UnknownContentError for HTTP 408/429 as MalformedResponse, despite the transport category being Timeout/RateLimited. This correction preserves that existing classification behavior; public text still identifies the safe transport category/status. Regression expectations explicitly document the quirk rather than changing ModelService outside the redaction scope. A separate classification follow-up should make HTTP category precedence consistent. Context-exhausted native outcome is preserved by the fixed signature message.

Public errors contain only bounded locally authored templates/categories/status or established local policy/validation summaries. Diagnostic objects retain request ID where already supplied, provider/model binding identity, HTTP status, attempt counts, elapsed/deadline and request-size/token estimates, cancellation/outcome and known category. Raw error bodies, echoed request fragments, credentials and headers are excluded. Retry-After remains an internal transport/backoff input; retry summaries contain locally authored category/delay metadata. Private transport structures retain numerical network errors for existing control flow. This fix introduces no raw-body diagnostic archive or new private logging sink. The helper classifier's input/message field remains internal untrusted data; only its allowlisted userMessage is safe for public projection.

## Files changed by P2-02

1. core/src/runtime/LocalInference.cpp
2. core/src/provider/ProviderErrorClassifier.cpp
3. tests/core/test_provider_contracts.cpp (extends pre-existing uncommitted P2-01 fixture)
4. docs/audits/P2_02_PROVIDER_ERROR_REDACTION_RESULT.md (this report)

Pre-existing tests/CMakeLists.txt, test_local_inference.cpp, ProviderHttpFixture.h and P2_01 report remain unchanged since task entry. No AgentRuntime, AgentLoop, P1-01 tests, Rust, legacy CLI, workflow, dependency or generated-contract changes were made. Original checkout remains clean; no other worktree was edited.

## Validation

Host: macOS 27.0.1 / Darwin arm64, installed CMake 4.4.4 and LLVM clang-format/clang-tidy 23.1.3; existing Qt/AppleClang build configuration. No tooling was installed.

| Check | Result |
| --- | --- |
| `cmake --preset tests -DSENTINEL_USE_CCACHE=OFF` | Passed |
| `CCACHE_DISABLE=1 cmake --build --preset tests -j6` | Full build passed |
| Direct provider-contract Qt XML | **60 pass records** including setup/cleanup, **58 cases**; **3 expected-failure assertions** for DEF-01/02; no unexpected failures/skips |
| Direct local-inference Qt XML | **51 pass records** including setup/cleanup, **49 cases**; no failures/skips |
| `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | **128/128 targets passed**, 156.13 seconds; no unrelated unexpected failure |
| Changed-file `clang-format --dry-run --Werror` | Passed for both production files and provider-contract tests |
| Scoped clang-tidy | Passed for all three translation units with clang-analyzer-*, bugprone-use-after-move, bugprone-dangling-handle and warnings-as-errors; existing one NOLINT suppression reported; no new diagnostic |
| Analysis compile database | Existing helper removed production compiler PCH from an analysis-only database in temporary storage; production PCH/build configuration unchanged |
| Both IPC generators `--check` | Passed; no generated contract changes |
| `tools/analysis/check_personal_paths.py` | Zero flagged files |
| `git diff --check` and report links/format | Passed |
| Entry manifest preservation | All pre-existing files except the two authorized production units and provider-contract test file retained their task-entry SHA-256; protected P2-01 fixture/report bytes unchanged |

Scoped static analysis is not a whole-repository security certification. Runtime tests are deterministic fixtures, not real backend/model, native Linux/Windows or client UI acceptance. No unrelated failing test source was changed to obtain these results.

## Remaining defects and acceptance

**P2-DEF-01 remains open:** compatible discovery accepts an object lacking data as an empty healthy catalog; one xfail remains. **P2-DEF-02 remains open:** direct selected-endpoint vision-unsupported binding can reach transport; two xfails remain. Neither implementation is modified. P2-01 report is immutable historical evidence and still describes its original three defects; this result supersedes DEF-03 only for the corrected candidate diff.

**AC-P2-01 is not satisfied / remains Unverified.** Error redaction has passing synthetic negative evidence for the common HTTP helper, both stream parsers and client-visible reply/diagnostic projections. Other advertised operation coverage, native Claude/Gemini fixture seams, per-vendor compatibility and the two remaining contract defects are not cleared. No P2 phase, real provider reliability or release acceptance is granted.

Recommended next step: review this scoped security diff, then separately fix DEF-01 and DEF-02 with their existing red witnesses and fresh full validation. Track native HTTP 408/429 category precedence as an independent follow-up. Complete native/vendor operation coverage before reassessing AC-P2-01. No cloud access, personal configuration or model download is authorized by this recommendation.

Final evidence SHA-256 (exact corrected source/test bytes):

| File | SHA-256 |
| --- | --- |
| core/src/provider/ProviderErrorClassifier.cpp | `d0f76c72773fd7066bfa95d574e2588b6e5c86b5d09f5737bb0dc9ccf23a7161` |
| core/src/runtime/LocalInference.cpp | `576ccea25a7ffd61328f4efcd4d436d53f51c34e0198a0dcaf10f2ea52be3827` |
| tests/core/test_provider_contracts.cpp | `ea25116797ffb44aafa54b4ddc9517caf953dbf7a613911ccb61140dbc0a5f12` |

Preservation check compared 5229 pre-existing files; only the three authorized edits differ. Report links/table structure/whitespace passed. Original HEAD and branch retained; no commit, push or merge.
