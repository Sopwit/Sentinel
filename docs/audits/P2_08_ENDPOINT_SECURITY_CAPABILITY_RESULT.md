# P2-08 — Cloud endpoint security and capability validation

Date: 2026-10-16 (Europe/Istanbul). Criteria: [AC-P2-01 and AC-P2-03](../roadmaps/ACCEPTANCE_MATRIX.md).
**Deterministic offline evidence only. No real-cloud, model or P2 phase acceptance.**

## Baseline and preservation

Worktree: `p2-01-provider-contracts/Sentinel-sanitized` under the managed Codex worktree root.
Branch: `test/p2-01-provider-contracts`. HEAD: `f27f893e7f32420ae28599cd182862f935847348`.
Recent history: P2-04 report (`f27f893e`), HTTP classification correction (`868fa5c3`),
P2-01/02/03 reports (`c9b99932`), capability/redaction/catalog corrections (`8e1612c7`).
Personal-Brain was unavailable; the supplied worktree and Git identity were verified directly.

Entry contained ten modified tracked and eight untracked P2-05/06/07 files. Before editing,
a SHA-256 inventory covered all 5,241 existing tracked/nonignored files. Historical reports,
P2-05 bounded fixtures, P2-06 native fixtures and P2-07 vendor fixtures were retained.
The explicitly documented existing test adaptations preserve their original semantic assertions.
This agent performed no reset, stash, stage, commit, push, merge, PR, CI, model-installation,
AgentRuntime, P3 lifecycle or Rust IPC change. No other worktree was edited.

During finalization the branch advanced externally through `fe2edb00` and
`b7530494862c52178539439f1d728ec722396b20` (final observed HEAD). The first commit includes
parts of the current implementation/test changes; the second adds historical P2-05/06/07 reports.
No attribution of intent is made. File-content inventory and candidate manifest still match the
validated working tree. Review must include committed and uncommitted P2-08 portions; no attempt
was made to undo, reset or rewrite the external commits.

Root AGENTS.md, architecture/runtime/security/build/testing/provider guidance, acceptance matrix,
P2-05/06/07 reports and the existing endpoint, capability, discovery, credential and transport
implementations informed the work. Earlier reports describe their own candidates and are not rewritten.

## Endpoint security matrix

The existing `LMStudioConfig` endpoint contract now distinguishes transport validity from credential
presence. `NetworkPolicyService` remains the process-wide authority for Online/LocalOnly/Offline;
this change does not replace it with another network policy service.

| Endpoint/scenario | Actual boundary after correction | Evidence / limit |
| --- | --- | --- |
| Fixed OpenAI, DeepSeek, Groq, Mistral, Claude, Gemini | Existing authenticated HTTPS defaults retained | Vendor/native regressions; no real API requests |
| Local HTTP | Exact localhost or QHostAddress-recognized literal loopback allowed without a key | IPv4 loopback range and IPv6 loopback cases; same address interpretation as NetworkPolicyService |
| Non-loopback HTTP / FTP / file / missing or malformed host | Rejected before inference transport | Streaming/non-streaming parity; no recorded dispatch |
| URL userinfo, including local URL userinfo | Rejected; userinfo/query/fragment excluded from displayed endpoint strings | Synthetic username/password and query-marker negative assertions |
| Arbitrary lower-level HTTPS URL | Requires nonempty key; valid host and no userinfo | Existing low-level config, not general user-facing cloud-provider support |
| HTTPS localhost/private IPv4/private IPv6 under Online | Still allowed at the low-level config boundary with a key | Explicit controls record this existing policy limit; no private-network allow/deny policy adopted |
| Cloud-only discovery helper with HTTP loopback | Rejected: cloud discovery requires HTTPS | Local runtime HTTP remains separately permitted |
| Generic HTTPS path prefixes | Explicit prefix retained; query/fragment stripped for inference | `/proxy/v1`, `/other`, trailing `/v1/` and `notv1` parity cases |
| Vendor-lookalike host strings | Remain generic hosts; no substring-based rerouting to a canonical vendor | Synthetic `.invalid` hosts in both inference modes |
| POST redirects | Manual policy; every surfaced 3xx rejected even with a valid success body | 301/302/307/308 across alternate HTTPS, private HTTP and IPv6 loopback destinations |
| Authenticated GET redirects | Manual policy; surfaced 3xx rejected without following or replaying credentials | Bearer, Anthropic header and Gemini query-key discovery cases |
| Offline / LocalOnly | Existing policy rejects cloud requests before transport | Inference parity from P2-07 plus GET negatives here |
| Frozen selection/config | Captured lease retains endpoint/model; later selection/config change affects future work | Frozen local endpoint case; P2-07 frozen cloud binding/credential cases |

`ModelService::providerConfig` still fixes the cloud vendor URLs. Its `openai-compatible` ID maps to
OpenAI; it has no arbitrary cloud URL setting. LM Studio/llama.cpp endpoint settings exist, but they
do not provide a general cloud credential configuration mechanism. Low-level `LMStudioConfig` must
not be described as a newly supported user-facing configurable-cloud feature.
Native request branches use canonical protocol URLs; arbitrary native path/port variants are not
certified configuration features. No endpoint list, router or provider registry was introduced.

### Existing versus missing defenses

Existing: explicit credentials, HTTPS for cloud transport, local-loopback exception, userinfo rejection,
manual authenticated redirects, safe public endpoint formatting, fixed vendor URLs and authoritative
network-mode checks. The injected fixture cannot bypass these checks.

Missing/unverified: DNS resolution/address pinning, DNS-rebinding resistance, complete private-network
and metadata-service restrictions, exhaustive ambiguous-host canonicalization and per-hop policy
re-evaluation for automatic redirects. Unauthenticated Online health/discovery retains Qt's existing
no-downgrade redirect behavior; it is not independently reauthorized per hop. Scripted replies verify
request redirect attributes and single dispatch, not the real Qt redirect engine or DNS behavior.
No complete SSRF protection is claimed.

Private-network HTTPS policy remains a product decision: either keep explicit trusted private endpoints,
or adopt a cloud-only public-address policy with DNS-aware enforcement and an explicit local-service
exception. This task selects neither policy. Credentialed discovery now fails closed on redirects,
matching POST behavior; legitimate APIs requiring redirects may need a future reviewed same-origin/
per-hop contract. No cloud availability claim follows from that security choice.

## Provider capability disclosure matrix

These are current **adapter-default declarations**, not measured model support. Source provenance,
model catalog metadata, user/programmatic overrides and a frozen binding remain distinct.

| Provider ID | Text / stream adapter | Native tools default | Structured default | Vision/model-specific facts |
| --- | --- | --- | --- | --- |
| `openai` | Chat / bounded text SSE | Unsupported | Unsupported | Unknown until supplied metadata/override; real model unverified |
| `deepseek` | Compatible Chat / bounded text SSE | Unsupported | Unsupported | Unknown; reasoning/model variants unverified |
| `groq` | Compatible Chat / bounded text SSE | Unsupported | Unsupported | Unknown; not blanket OpenAI feature parity |
| `mistral` | Compatible Chat / bounded text SSE | Unsupported | Unsupported | Unknown; multimodal/model variants unverified |
| `claude` | Native Messages / text stream | Supported adapter path | Unknown | Unknown model capabilities; native defaults do not prove every model supports tools |
| `gemini` | Native generateContent / text stream | Supported adapter path | Unknown | Unknown model capabilities; catalog limits are metadata, not live acceptance |
| `openai-compatible` | Fixed OpenAI route / text SSE | Unsupported | Unsupported | Unknown; arbitrary service configuration absent at product level |
| `ollama` | generate / NDJSON | Unsupported | Unsupported | Unknown until runtime metadata; no model execution here |
| `llama-cpp-server` | Compatible Chat / text SSE | Unsupported | Unsupported | Unknown until runtime/template metadata or override |
| `lm-studio` | Compatible Chat / text SSE | Unsupported | Unsupported | Unknown until runtime metadata or override |

All ten defaults declare streaming Supported; combined tools+structured output, vision and audio remain
Unknown without further metadata. No new text/cancellation/deadline tri-state fields exist in
`ModelCapabilities`: text is an adapter operation, and cancellation/finite-deadline enforcement is
behavior of `IChatProvider`/request options. The P2-05/06/07 regressions exercise those behaviors.
Zero/absent deadlines retain legacy semantics unless bounded streaming explicitly requires a finite
bound. Cancellation is client-side and does not prove upstream compute termination.

The new cases verify that constructing a synthetic Supported binding does not mutate authoritative
metadata or Unknown health. Explicit Unknown tools/schema/stream bindings reject where the selected
endpoint adapter requires Supported; explicit Unsupported vision rejects in both ordinary and bounded
streaming requests. Unknown vision retains the existing direct-attempt policy and is not promoted by
fixture success. Ollama uses its own adapter and native capability rejection; its model-specific
vision disclosure is not certified by the selected-endpoint cases.

Native/structured streaming is still unsupported. JSON object parsing is not independent full JSON
Schema validation. Configuration-derived Ready/Supported adapter declarations must not be presented
as observed endpoint health or real-model reliability; the separate observed health remains Unknown
before observations. No vendor capability defaults were promoted or silently changed.

## Catalog and discovery coverage

Existing GET routes were used, not invented: OpenAI `/v1/models`, DeepSeek `/v1/models`, Groq
`/openai/v1/models`, Mistral `/v1/models`, Anthropic `/v1/models`, Gemini `/v1beta/models`.
The optional P2-06 network factory is now accepted at these existing helper boundaries and threaded
through their existing GET implementation. Production callers omit it and retain Qt transport.
Network/endpoint validation runs before manager creation. No application settings transport override
or second HTTP stack was added.

`ScriptedProviderTransport` now records GET versus POST, the real operation and redirect policy.
The same scripted reply supplies incremental frames, cancellation and held-open deadline responses.
Six providers × fifteen modes gives **90 catalog rows**: valid, empty, malformed JSON, missing array,
missing/numeric ID, 401/403, timeout, pre/dispatched cancellation, redirect, Offline, LocalOnly and
missing key. Auth failures echo a synthetic key/private marker; public errors omit both.

Nine registry sequencing controls preserve valid model IDs and context metadata after newer failure,
reject older discovery outcomes and clear metadata only on a newer successful empty catalog.
Malformed Gemini payloads now fail atomically rather than masquerading as a successful empty catalog.
No production generic-compatible discovery route was invented: the controller has no corresponding
arbitrary-vendor catalog dispatch. Lower-level compatible GET support is not product integration.

## Reproduced issues and production corrections

| Finding | Before evidence | Narrow correction |
| --- | --- | --- |
| SEC-EP-01 insecure/misleading endpoint acceptance | Initial endpoint matrix: 14 failed rows covering insecure schemes/private HTTP, userinfo and query display leakage | Validate scheme/host/userinfo in existing LMStudioConfig; recognize literal loopback consistently; redact displayed endpoint |
| SEC-EP-02 endpoint identity/path mismatch | Seven identity rows failed: duplicate `/v1`, `notv1` stream mismatch, vendor substring matches | Normalize trailing slash for suffix selection; require `/v1` suffix; exact canonical native-host matching |
| SEC-EP-03 redirect success-body acceptance | Twelve non-streaming redirect rows accepted a valid success body under 3xx | Reject POST 3xx with RequestRejected and preserve explicit category through retry projection |
| SEC-EP-04 authenticated GET redirect boundary | Source inspection showed Qt auto-follow while Online, without per-hop policy re-evaluation | Manual credentialed discovery redirects; surfaced 3xx rejected; default unauthenticated behavior remains explicit limitation |
| CAP-01 malformed Gemini catalog | Three GET rows reported completed for absent models/missing ID/numeric ID | Require array and nonempty string names; return MalformedResponse atomically |
| CAP-02 streaming Unsupported vision | Nine selected-endpoint rows dispatched one image-bearing request, expected zero | Apply existing non-streaming vision preflight to bounded streaming |

The 14 initial endpoint failures were captured before endpoint hardening. After that correction,
the expanded matrix captured 22 failures (seven identity, twelve POST redirects and three Gemini
catalog failures), which motivated the corresponding fixes. Vision-stream regressions then captured
nine actual dispatch violations. A development-only escaped string compilation error and an initial
new vision fixture dispatch typo were corrected before collecting their valid witnesses.
No real credential, user data or cloud request was involved.

## Exact P2-08 changed files

| File | Change |
| --- | --- |
| `core/include/sentinel/core/runtime/LocalInference.h` | Existing endpoint validity/credential distinction, loopback parity and safe display |
| `core/src/runtime/LocalInference.cpp` | POST redirect rejection/category preservation, canonical host matching and path parity |
| `core/include/sentinel/core/runtime/OllamaRuntime.h` | Optional existing factory parameter on four discovery helpers |
| `core/src/runtime/OllamaRuntime.cpp` | Existing GET factory plumbing, endpoint/redirect checks and Gemini catalog shape validation |
| `core/src/model/ModelService.cpp` | Bounded-streaming Unsupported vision guard |
| `tests/support/ScriptedProviderTransport.h` | GET/POST operation and redirect-policy observation |
| `tests/core/test_endpoint_security_capability.cpp` | New security, metadata and discovery regression target |
| `tests/core/test_provider_contracts.cpp` | Existing cloud discovery rows use scripted HTTPS instead of HTTP stand-in |
| `tests/core/test_local_inference.cpp` | Existing cloud bearer case uses scripted HTTPS instead of trailing-dot HTTP stand-in; stale comment corrected |
| `tests/CMakeLists.txt` | Append one target registration |
| `docs/audits/P2_08_ENDPOINT_SECURITY_CAPABILITY_RESULT.md` | This report |

The two adapted historical test functions retain their data rows and response/auth/classification/
capability oracles. No P2-05/06/07 report or native/vendor/bounded test source was modified.
Formatting is scoped to changed functions/lines and new fixture code, not a repository-wide rewrite.

## Validation and counts

New target: **246 substantive cases**, **248 Qt pass records** including init/cleanup;
zero fail/xfail/xpass/skip. There are 72 positive/mixed-control rows and 174 negative rows;
control rows can include failure assertions and multiple requests.

| New function | Substantive cases |
| --- | --- |
| endpointValidation | 40 |
| localWithoutCredentials | 5 |
| pathAndHostIdentity | 12 |
| redirectRejected | 24 |
| capabilityDisclosure | 10 |
| unknownCapabilityRejected | 45 |
| cloudDiscoveryEndpoint | 10 |
| discovery | 90 |
| staleCatalogPreserved | 9 |
| frozenEndpointAndFixedVendors | 1 |

Host: macOS 27.0.1 arm64, AppleClang 21.0.0, Qt 6.12.0, CMake 4.4.4,
clang-format/clang-tidy 23.1.3. No native Fedora/Windows or real-service acceptance inferred.

- Full `CCACHE_DISABLE=1 cmake --build --preset tests -j6`: passed.
- Focused provider/security regression: **14/14 CTest targets passed**, 14.21 s.
- P2-07 vendor, P2-06 native and P2-05 bounded streaming targets passed within that focused run;
  their substantive case counts remain **338**, **74** and **16**, respectively.
- Initial focused run: **12/14 targets**, 25 failed rows/assertions. Twenty-four old cloud discovery
  rows and one bearer generation row used HTTP stand-ins prohibited by the new cloud contract.
  They were migrated to existing scripted HTTPS, retaining or strengthening original oracles;
  no security guard was relaxed, skipped or excluded to pass.
- New test/fixture clang-format dry run and scoped production/fixture formatting: passed.
- Scoped clang-tidy: new test has zero user-code findings (549 non-user warnings suppressed);
  three production translation units and two adapted test translation units passed the
  clang-analyzer/use-after-move/dangling-handle scope with warnings-as-errors. This is not
  whole-repository static analysis certification.
- Both IPC generator `--check` commands: passed, no generated changes.
- Repository personal-path scanner: passed, zero files flagged. Changed-file privacy: **11/11 passed**.
- Markdown structure/local links and `git diff --check`: passed; index unchanged.
- Preservation: all 5,241 entry files except the nine documented source/test edits remain
  byte-identical; the endpoint test and this report are the only additions relative to entry.
  External commits changed index/HEAD ownership during finalization; the final index is clean,
  and source-content digests remain the verification authority.
- Final full `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure`:
  **132/132 CTest targets passed**, 163.69 s. No failures/skips reported.

Raw local XML/logs remain private temporary evidence under `/tmp/sentinel-p2-08-*`; public-safe
counts and source hashes identify the candidate without exposing machine paths or private content.

## Candidate identity

The ten-file source manifest hashes sorted repository-relative filenames plus NUL, file bytes plus
NUL, covering the nine edited source/test files and the new test (excluding this receipt).

- Source manifest SHA-256: `2033486fd8a50bc67fd68fbb2225435ae0f4985613a7329a8ca4a8e6ef439e0e`.
- Endpoint test binary SHA-256: `ed5ed7a0d21d9f295e08922aaadd54cb440fc1a301414fce113a882bf80a8e34`.

## Acceptance, remaining decisions and next task

**AC-P2-01 remains Unverified / not satisfied. AC-P2-03 remains Blocked.** Current fixtures establish
specific construction, capability gates, safe projection and negative security behavior, not all
supported operations on every provider/model or real-service reliability. No acceptance matrix cell,
threshold or phase status changed.

Outstanding decisions/evidence: product-level configurable-cloud support and credential scope,
DNS-aware public/private endpoint policy, trusted proxy/private deployment exceptions, authenticated
same-origin/per-hop redirects, model-specific capability provenance and permitted overrides.
Real-provider acceptance must separately choose authorized accounts/models/API versions/platforms,
collect exact safe configuration/evidence, and verify text/stream/tool/schema/vision/error/cancel
behavior and cost controls. No such calls or model acquisition are authorized by this report.

Recommended next task: **P2-09 — adopt configurable-cloud endpoint and capability-disclosure policy,
then prepare a bounded AC-P2-03 live-acceptance plan**. Keep the existing services, transport seam and
frozen binding authority. Do not infer live authorization, full SSRF protection or P2 completion from
this synthetic baseline. This agent performed no commit, push or merge; externally observed commits are recorded above.
