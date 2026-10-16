# P2-07 — Vendor-specific cloud provider contract validation

Date: 2026-10-16 (Europe/Istanbul). Related criterion: AC-P2-01.
**Synthetic offline contract evidence only; no real-service compatibility or P2 completion claimed.**

## Baseline, scope and preservation

Continued in the existing managed P2 worktree, `p2-01-provider-contracts/Sentinel-sanitized`,
branch `test/p2-01-provider-contracts`, HEAD `f27f893e7f32420ae28599cd182862f935847348`.
Recent commits: P2-04 result (`f27f893e`), validated-status classification correction (`868fa5c3`),
P2-01/02/03 audit results (`c9b99932`), capability/redaction/catalog corrections (`8e1612c7`).
Personal-Brain was unavailable; the explicitly supplied worktree and Git identity were verified.

Entry state included ten modified tracked files and six nonignored untracked files from P2-05/06.
A SHA-256 inventory of 5,239 existing files was captured before editing. Existing implementations,
fixtures and reports were preserved, except the two explicitly documented additive corrections below.
No reset, stash, commit, push, merge, PR or workflow change occurred. No other worktree was edited.
P1 AgentRuntime, P3 lifecycle, Rust IPC and legacy CLI sources remain unchanged.

Read root AGENTS.md; P2-01 through P2-06 reports; acceptance matrix; architecture, Agent Runtime,
build/testing, provider capabilities/routing/extension guidance and security/Secrets documentation;
NetworkPolicyService implementation; LocalInferenceTransport and ScriptedProviderTransport fixtures.
Historical reports remain unchanged and retain their original scope/results.

## Existing adapters and per-provider matrix

All target IDs use `SelectedEndpointChatProvider` and the existing compatible builders/parsers.
There is no separate vendor SDK or new adapter. Every registered default advertises streaming Supported,
native tools and structured output Unsupported, and vision Unknown. Tests explicitly supply Supported
capabilities to exercise the conditional tool/schema paths; fixture success never promotes metadata.
Model IDs and credentials are synthetic; no vendor model inventory or service availability was queried.

| Provider ID | Observed POST host/path | Verified synthetic subset | Unsupported / unverified |
| --- | --- | --- | --- |
| `openai` | `api.openai.com/v1/chat/completions` | Bearer text, bounded text SSE, errors, frozen binding; opt-in tools/schema and tool continuation | Responses API absent from this adapter; model-specific parameter/vision/reasoning support unverified |
| `deepseek` | `api.deepseek.com/v1/chat/completions` | Same contract cases, independently parameterized by provider ID | Reasoning/thinking/reasoning_content and vendor strict/beta variations not implemented or certified here |
| `groq` | `api.groq.com/openai/v1/chat/completions` | Same cases, including `/openai` path prefix | Model-dependent tools/schema/vision unverified; no claim of total OpenAI feature parity |
| `mistral` | `api.mistral.ai/v1/chat/completions` | Same cases | Model-specific multimodal/schema behavior and non-chat APIs unverified |
| `openai-compatible` | `api.openai.com/v1/chat/completions` | Existing fixed routing, bearer/auth/error/stream/capability behavior | **No arbitrary cloud URL setting** in ModelService; generic third-party service compatibility unverified |
| Lower-level `LMStudioConfig` | `fixture.invalid` with root, `/v1`, `/proxy` and query/fragment variants | Supplied base prefix preserved; `/v1` not duplicated for the tested canonical base; query/fragment stripped; policy enforced | Four lower-level configuration cases do not establish a product-level configurable-cloud provider feature |

All rows remain **real service/model compatibility Unverified**. No inference request reached a real
cloud server. Model selection, API support, configured credentials, availability, model capability,
tool reliability and live acceptance are distinct facts.

### Vendor documentation cross-check

Primary public documentation was read on the audit date as protocol context, with no API calls:
[OpenAI Chat reference](https://developers.openai.com/api/reference/resources/chat),
[DeepSeek API entry point](https://api-docs.deepseek.com/),
[Groq compatibility](https://console.groq.com/docs/openai),
[Mistral Chat reference](https://docs.mistral.ai/api/endpoint/chat).
Groq documents its `/openai/v1` base and lists compatibility exceptions. DeepSeek's root-base
examples differ from Sentinel's explicit `/v1` suffix; this fixture verifies Sentinel construction,
not whether a current service accepts an alias. OpenAI and Mistral document chat tools/response formats
and streaming, but their existence does not make every model or parameter supported.
No default model, acquisition policy or advertised capabilities were changed based on these pages.

## Implementation and exact P2-07 files

| File | P2-07 change |
| --- | --- |
| `tests/core/test_vendor_cloud_contract.cpp` | New offline data-driven vendor contract target |
| `tests/CMakeLists.txt` | Append one target registration; existing P2-05/06 registrations retained |
| `core/src/runtime/LocalInference.cpp` | Add a pre-dispatch NetworkPolicyService guard to compatible/native streaming |
| `tests/core/test_native_provider_contract.cpp` | Add existing ScopedOnlineNetworkMode RAII to three synthetic streaming cases |
| `docs/audits/P2_07_VENDOR_CLOUD_CONTRACT_RESULT.md` | This report |

The production change is limited to an 11-line guard before manager creation in
`LMStudioLocalInferenceStreamClient::startStream`. It returns Refused/Failed, category Offline,
using a fixed policy summary, when the existing policy refuses `config_.endpoint`.
It mirrors the existing Ollama/JSON transport policy boundary and does not create an override,
change endpoint routing, alter parsing or loosen credential/TLS/redirect rules.

The three native fixture additions are `claudeStreamingTextAndTerminal`,
`claudeStreamingUnterminatedIsRejected`, and `geminiStreamingFinishReasonTerminal`.
Their scripted response bodies and protocol assertions remain byte-identical; only explicit,
scoped test policy setup is added. No P2-06 report was rewritten.

## Positive and negative fixture evidence

The new target executes **338 substantive data-row cases**, excluding Qt init/cleanup:
**330 vendor rows (66 each)**, **4 configurable-base controls**, and **4 native policy negatives**.
There are **34 positive/mixed control rows** and **304 negative/rejection/failure rows**;
the four configurable-base controls each include a blocked request and then an allowed synthetic request.
A row can contain multiple assertions or requests; it is not a CTest target count.

| Function | Cases | Independent oracle |
| --- | --- | --- |
| constructionAndFrozenBinding | 5 | Exact HTTPS host/path, POST, bearer/content-type, model/messages/temperature/token budget; selection/caller binding and credential mutations cannot change captured request |
| toolAndStructuredWire | 10 | Exact authoritative schema, parsed tool ID/call ID/arguments, tool-result continuation; named strict json_schema request and parsed object |
| unsupportedCapabilities | 40 | Default tools/schema rejection; explicit Unsupported vision/stream; combined capability gate; bounded tool/schema rejection; absent finite deadline; zero dispatch |
| missingCredentialsNoFallback | 5 | AuthenticationRequired resolution, zero transport, zero alternate Ollama factory calls, selection unchanged |
| httpErrors | 80 | Five IDs × eight statuses × plain/native paths; typed category/status, one attempt, no success text, secret-free bounded public errors/diagnostics |
| nativeMalformedPayload | 30 | Unknown tool, missing ID, array/broken arguments, structured non-JSON/array reject with no accepted calls/result |
| malformedBodies | 40 | Five IDs × four malformed/empty bodies × plain/native paths; no successful result |
| interruption | 30 | Five IDs × plain/native × pre-cancel/dispatched-cancel/deadline; terminal category and zero/one request, no replay |
| streaming | 75 | Five IDs × 15 modes: DONE/finish-reason controls, malformed/missing terminal/error event, cancellation/timeout after partial output, eight HTTP statuses; failure never returned as completed text, no late deltas |
| networkPolicy | 14 | Seven IDs × Offline/LocalOnly; both plain and bounded stream rejected, zero recorded requests |
| cloudAlias | 5 | Existing `cloud-api` facade uses selected vendor route; compatible control uses OpenAI selection |
| configurableEndpoint | 4 | Existing lower-level prefix construction and policy/query/fragment handling |

HTTP classification: 401/403 AuthenticationRequired; 404 ModelNotFound; 408 Timeout;
429 RateLimited; 500 ProviderUnavailable; 501 CapabilityUnsupported. Plain/stream 400 is
RequestRejected; the existing native-capability projection maps 400 to CapabilityUnsupported.
This asymmetry is documented, not silently changed. Nonzero deadlines keep each HTTP failure to one
attempt. Streaming accepts `[DONE]` or the existing nonempty `finish_reason` marker; no claim of
streamed tool-call/structured-output support follows from text SSE tests.

## Security boundary and reproduced defect

**P2-07-DEF-01: compatible streaming skipped NetworkPolicyService.** Before correction, all ten
vendor Offline/LocalOnly cases observed **one dispatched scripted request, expected zero**.
JSON request paths already blocked before transport. Compatible streaming constructed a manager and
posted directly without a policy check. This is a production control-flow defect; no real network
request was made while reproducing it. The guard fixes it for the shared compatible/native stream path.

After the guard, the three inherited native stream cases initially failed because their fixture setup
left policy LocalOnly and had relied on the missing check. Explicit ScopedOnlineNetworkMode restores
only their intended synthetic protocol exercise; it does not disable the production gate.
Four additional native policy rows verify Claude/Gemini rejection with no scripted dispatch.

All positive cloud construction tests install the existing P2-06 in-memory factory and explicitly
scope Online policy; every returned manager is ScriptedNetworkAccessManager, never Qt's default network
manager. No socket, DNS, hosts/proxy manipulation, TLS disabling or real credential access is used.
Fixtures use in-memory settings/credential stores. Synthetic keys, upstream private markers and prompts
are absent from exercised public error/diagnostic projections. Existing P2-02 regressions remain required.
Client cancellation/timeout does not prove remote compute termination.

## Validation results

Host: macOS 27.0.1 arm64, AppleClang 21.0.0, Qt 6.12.0, CMake 4.4.4,
clang-format/clang-tidy 23.1.3. No native Fedora/Windows or real backend acceptance is inferred.

| Check | Result |
| --- | --- |
| Full `CCACHE_DISABLE=1 cmake --build --preset tests -j6` | Passed, including production guard and native fixture corrections |
| Focused provider regression | **13/13 CTest targets passed**, 10.59 s |
| New vendor Qt XML | **340 pass records**, 338 substantive cases; no fail/xfail/xpass/skip |
| Native Claude/Gemini Qt XML | **76 pass records**, 74 substantive cases; no failures/skips |
| P2-05 bounded-streaming Qt XML | **18 pass records**, 16 substantive cases; no failures/skips |
| Existing provider-contract Qt XML | **205 pass records**, 203 substantive cases; no failures/expected failures/skips |
| Existing local-inference Qt XML | **51 pass records**, 49 substantive cases; no failures/skips |
| Formatting | New vendor and three-line-adjusted native test sources pass clang-format dry run; new production guard independently matches clang-format; inherited source not reformatted |
| Scoped clang-tidy | New vendor TU: zero user-code findings, 550 non-user warnings suppressed. Production inference and adjusted native TU: analyzer/use-after-move/dangling-handle scope passed with warnings-as-errors |
| Generated contracts | Both IPC generators with `--check` passed; no generated changes |
| Privacy | Repository scanner and changed-file pattern checks passed; zero flagged files |
| Git / report structure | Diff whitespace and Markdown structure/link checks passed; no index changes |
| Preservation | Entry digest inventory passes exact reconstruction; only documented guard/RAII/registration edits plus new test/report |

Final full `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure`: **131/131 targets passed**, 122.66 s. No failures or skips reported.

Retained development outcomes: the first vendor run had 19 failed rows: five incorrect plain-400
expectations, ten real streaming-policy violations (initially surfaced by category mismatch), and four
lower-level policy controls with incorrect fixture ambient policy setup. Corrected fixture setup and
the plain/native 400 oracle first; an independent pre-fix run then confirmed ten streaming dispatch
violations (actual one request, expected zero). After the guard, initial focused regression was 12/13
and the first full regression was **130/131**, 157.13 s: only the native target failed, with the three
stream fixture policy setup omissions described above. Those fixtures were corrected using existing
RAII policy setup. No failure was erased, timeout increased or production behavior weakened to pass.
Initial clang-tidy style findings in the new test were fixed before the final clean run.
Raw XML/build output stays in private temporary files (`/tmp/sentinel-p2-07-*`) because compiler paths
can identify the local machine. Only aggregate outcomes and nonidentifying digests belong in this report.

## Candidate source identity

These digests include the preserved P2-05/06 candidate content plus the scoped P2-07 edits.

| File | SHA-256 |
| --- | --- |
| `core/src/runtime/LocalInference.cpp` | `3ff51818cff2e14b2e1812975c421a0295ebe725111da9046cb5595af2efd316` |
| `tests/core/test_native_provider_contract.cpp` | `aeb488dacfc9bebb579b6843d18bcb4f7af6ab679a39dff210d232c3a8d0cefb` |
| `tests/CMakeLists.txt` | `38591bdd3e4f9d14cddac05f98e2a8d36407b3a517c94e9437fe29f154e553e9` |
| `tests/core/test_vendor_cloud_contract.cpp` | `c7b7a3b9467fd90f23bd26bfc7b93043f6f1b65c7776b745207b51c91f31696d` |

## Deficiencies, acceptance and next task

- Native tools/schema remain opt-in at the binding level; a synthetic Supported binding does not
  establish that any vendor model advertises or implements those capabilities.
- Bounded streaming explicitly rejects native tools/structured output; streamed tool deltas,
  reasoning-specific fields and multimodal/audio operations are not certified.
- Structured results are parsed as JSON objects; full independent JSON Schema conformance is absent.
- `openai-compatible` uses fixed OpenAI routing. Lower-level configurable transport is not an arbitrary
  cloud URL product feature. Canonical tested base paths do not certify every trailing-slash/path variant.
- Existing LMStudioConfig cloud gating checks nonempty credentials rather than independently enforcing
  HTTPS-only URLs. Registered target vendors use fixed HTTPS; arbitrary lower-level configurations need
  separate security design/acceptance. This task introduces no new endpoint bypass.
- Historical P2-01/P2-03 reports describe DEF-01 as open, but the current source now validates
  compatible discovery shape and the retained witness passes normally. The current provider-contract
  binary has 203 substantive passing cases and no xfail. Historical reports are not current-state
  assertions. Vendor discovery GETs remain outside the POST-only scripted fixture's new coverage.
- Real model identities, credentials/scopes, vendor current API/model versions, rate limits, tokenizer,
  context/output budgets, schema/tool reliability, streaming errors and cancellation receipts still need
  an authorized disposable real-provider evidence packet. No account/cost or live request authorization
  is inferred from this task.

**AC-P2-01 remains Unverified / not satisfied**, with substantially expanded deterministic coverage.
No matrix status, P2 completion or AC-P2-03 live acceptance is granted. Recommended next task:
**P2-08 — configurable-cloud endpoint security and vendor capability disclosure validation**, reusing
current services and a bounded GET-capable fixture for vendor catalogs; then separately authorize selected real-provider/model
acceptance under AC-P2-03. No additional transport override or provider router is recommended.
