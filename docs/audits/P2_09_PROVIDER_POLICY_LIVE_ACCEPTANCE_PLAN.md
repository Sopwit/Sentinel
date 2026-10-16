# P2-09 — Cloud security policy and live acceptance plan

Date: 2026-10-16 (Europe/Istanbul). **Proposals only: no policy adopted and no live execution authorized.**
Related criteria: [AC-P2-01 and AC-P2-03](../roadmaps/ACCEPTANCE_MATRIX.md).

## 1. Baseline, scope and source state

Repository: `sentinel-projects/sentinel`. Managed worktree: `p2-01-provider-contracts/Sentinel-sanitized`.
Branch: `test/p2-01-provider-contracts`. Actual entry HEAD:
`f87843f1edcab1e5e02defa61b8e560dcc001d92`. Entry working tree/index clean.
Recent external history: `e33db6de` commits the P2-08 implementation and
`f87843f1` commits its audit, following `b7530494` and `fe2edb00`.
The last reported `b7530494` is historical, not this task's baseline.
A private SHA-256 preservation inventory covers 5,243 existing tracked/nonignored files.
Personal-Brain was unavailable; the explicitly supplied worktree and Git identity were verified.

Read [AGENTS](../../AGENTS.md), [P2-05](P2_05_BOUNDED_STREAMING_CONTRACT_RESULT.md),
[P2-06](P2_06_NATIVE_CLOUD_PROVIDER_CONTRACT_RESULT.md),
[P2-07](P2_07_VENDOR_CLOUD_CONTRACT_RESULT.md),
[P2-08](P2_08_ENDPOINT_SECURITY_CAPABILITY_RESULT.md),
[security policy](../../SECURITY.md), [security model](../security/SECURITY_MODEL.md),
[Secrets](../security/SECRETS.md), [Permissions](../security/PERMISSIONS.md),
[Threat model](../security/THREAT_MODEL.md), acceptance matrix and
[decision register](../roadmaps/ACCEPTANCE_DECISIONS.md). Source inspection was read-only.

This report is the sole intended change. No provider implementation, capability default, fixture,
security setting, acceptance status or historical evidence changed. No live API request, credential
inspection/setup, billing action, model download, build or production test was performed.
Historical CTest results remain evidence for their own source/configuration; they do not certify this plan.
No stage, reset, stash, commit, push, merge, PR or other-worktree modification is authorized or performed.

## 2. Current endpoint and request-path boundaries

[LMStudioConfig](../../core/include/sentinel/core/runtime/LocalInference.h) accepts valid HTTPS
with a nonempty key, or exact localhost/literal loopback HTTP without requiring a key. Userinfo is
rejected; displayed endpoints omit userinfo/query/fragment. Under Online, lower-level HTTPS can
still target localhost/private/link-local destinations; no DNS address pinning or public-address
policy exists. The literal-loopback exception is independent of provider kind/scheme: a lower-level
HTTPS loopback URL with a key can pass Offline/LocalOnly too. Non-loopback private destinations are
blocked by those restrictive modes without DNS resolution. This is not a comprehensive SSRF boundary.

[ModelService](../../core/src/model/ModelService.cpp) supplies fixed HTTPS vendor endpoints;
`openai-compatible` maps to OpenAI. No arbitrary cloud origin/credential UI contract exists.
LM Studio/llama.cpp endpoint settings and low-level URL objects are not that product feature.
Credentials/configuration and ModelBinding are captured by a provider lease; later selection changes
apply to future requests. Capturing a key does not establish runtime revocation semantics.

[NetworkPolicyService](../../core/include/sentinel/core/network/NetworkPolicyService.h) is a
process-wide application gate, distinct from tool authorization and an OS firewall. Both Offline
and LocalOnly currently permit localhost/literal loopback; Online permits destinations before
caller-specific endpoint validation. The following is the inspected provider and adjacent-model path
inventory, not a claim that every plugin/MCP/updater/other application transport was audited.

| Request path | Current policy/redirect placement | Remaining boundary to validate |
| --- | --- | --- |
| Compatible/native JSON generation and tool/schema POST | LocalInference `postJsonOnce`: policy before manager; manual redirect; 3xx rejected | Policy revocation during active request; origin-bound credentials; real TLS/proxy behavior |
| Ollama text/stream requests | Local endpoint validation; JSON/stream policy gates; manual redirects | Local listener trust and in-flight mode change; no process-ownership inference |
| Compatible/native text streaming | LocalInference stream policy before manager; endpoint contract; manual redirect | Check actual final/native URL identity and each dispatch path under revocation; no remote-stop proof |
| Vendor and compatible synchronous catalog/health GET | OllamaRuntime `getJsonOnce`: policy plus endpoint gate; authenticated/header/query-key discovery manual redirects | Unauthenticated Online requests retain Qt no-downgrade auto-follow without per-hop reauthorization |
| Generic cloud discovery facade | ApplicationController dispatches the six existing named vendor routes | No arbitrary-compatible vendor catalog route; do not invent one from URL configurability |
| Ollama/LM Studio public library and model-detail fetchers | DaemonModelHelpers gates entry; core fetchers directly use Qt and no-downgrade auto-follow | Defense depends on caller; direct invocation, policy change and redirect destination require tests |
| Ollama pull/remove and local ModelOperationService requests | Loopback constraints; daemon helper entry policy where composed; manual operation redirects | Enumerate all direct callers; local calls remain allowed under current Offline semantics; acquisition is separate from inference |
| Hugging Face catalog/metadata | Source applies policy at request entry; same-origin automatic redirects | Same-origin DNS change and policy change; not live-tested or authorized here |
| Managed artifact download | ModelOperationService checks policy on each explicitly handled HTTPS redirect; token only attached to its scoped origin | Private/resolved address and proxy policy remain absent; acquisition excluded from the live plan |

Sources: [LocalInference](../../core/src/runtime/LocalInference.cpp),
[OllamaRuntime](../../core/src/runtime/OllamaRuntime.cpp),
[ApplicationController](../../core/src/app/ApplicationController.cpp),
[DaemonModelHelpers](../../apps/sentinel-daemon/service/DaemonModelHelpers.cpp),
[HuggingFaceModelSource](../../core/src/model/HuggingFaceModelSource.cpp),
[ModelOperationService](../../core/src/model/ModelOperationService.cpp).
A caller gate is not proof of a transport-level gate. Automatic redirects and DNS/proxy resolution
cannot be certified by scripted readyRead/HTTP status fixtures alone.

## 3. Endpoint security decision matrix — pending approval

CSEC-01…10 are **local proposal identifiers**, not approved additions to the shared PD-D register.
Every recommendation below requires a dated maintainer decision before changing production policy.
Existing PD-D02/03/04/10/12 remain pending where applicable; this report adopts none.

| Proposal | Current implemented behavior | Security risk | Recommended default, not adopted |
| --- | --- | --- | --- |
| CSEC-01 Product configuration | Fixed vendor URLs; arbitrary lower-level HTTPS accepted with key; no generic cloud UI contract | Low-level flexibility may be mistaken for an approved product/security feature | Keep arbitrary user-facing cloud configuration unavailable until an explicit profile/credential contract is accepted |
| CSEC-02 Allowed origins | Exact native host routing; fixed vendor HTTPS; lower-level HTTPS unrestricted by origin list | Credential sent to misconfigured or malicious origin; path/port ambiguity | Fixed providers use exact canonical scheme/host/effective-port plus explicit API path scope; custom origins require separate reviewed profile |
| CSEC-03 Local versus private | Literal loopback HTTP allowed; private HTTPS allowed with key while Online | Cloud configuration can reach private/admin/metadata surfaces | Cloud-only profiles deny loopback, private, link-local and special-use destinations; local runtimes stay an explicit separate loopback profile; no global behavior change by this proposal |
| CSEC-04 DNS and canonicalization | QUrl parsing and QHostAddress loopback facts; no resolved-address binding | Rebinding, mixed public/private answers, alternative IPv4/IPv6 forms, validation/connect race | Validate canonical host and all resolved answers, connect only to approved address identity while preserving TLS SNI/hostname verification; fail closed if unenforceable |
| CSEC-05 Redirects | Inference and credentialed discovery reject 3xx; some unauthenticated paths auto-follow | Cross-origin leakage or network-policy escape; same hostname may resolve differently | Retain authenticated no-follow; recommend explicit manual redirect handling with policy/origin/address checks on every hop for other provider paths |
| CSEC-06 Enterprise/private profiles | No dedicated trusted private-origin/proxy policy model | Blanket private exception defeats cloud-only boundary; enterprise compatibility pressure | Separate opt-in profile with exact origins/ports/address scope, named owner, expiry/revocation and credentials scoped to that profile; do not disable TLS |
| CSEC-07 Credential scope | Vendor key slots and frozen provider config; some Gemini paths put key in query | Implicit key reuse, URL/intermediary logs, stale leases after revoke | Bind credentials to provider + canonical origin/profile + permission scope; never copy to another origin; prefer approved header auth; redact query auth everywhere pending migration |
| CSEC-08 Proxy and trust | Existing Qt/OS proxy/TLS behavior, not independently accepted | Proxy-side DNS/egress may invalidate local address checks; TLS interception changes trust | Explicitly approve proxy mode and trust profile before live use; use normal certificate verification; block cloud-only use where destination policy cannot be enforced |
| CSEC-09 Capability/config identity | Merged capabilities with provenance; cached catalogs; frozen request bindings | Stale or inherited Supported may be presented as model proof; endpoint change may reuse inappropriate metadata | Scope metadata to provider/origin/model/version, preserve raw evidence levels, show freshness, invalidate affected future selection facts on configuration change |
| CSEC-10 Policy across paths | Central gate plus caller-dependent helpers; loopback allowed Offline; no universal in-flight revoke contract | Inconsistent entry/redirect checks; users may interpret Offline as zero network | Preserve current semantics until approved; disclose “external network disabled, local runtime allowed”; require policy at each dispatch/hop and define in-flight revoke/cancel explicitly |

| Proposal | Alternative policies | Compatibility impact | Required implementation after approval | Deterministic regression cases | Approval needed |
| --- | --- | --- | --- | --- | --- |
| CSEC-01 | Retain fixed-only; add curated custom origins; allow administrator-managed profiles | Custom OpenAI-compatible services remain unavailable until explicitly supported | Extend existing settings/ModelService configuration and safe projection; no second registry/router | Missing profile, invalid origin, unset credential, selected profile freeze, no fallback | Maintainer + security owner: feature scope and allowed administrators |
| CSEC-02 | Vendor origin allowlist only; exact custom-origin allowlist; explicit per-origin user consent | Nonstandard ports, gateways and path prefixes need approved entries | Canonical origin/path comparison in existing endpoint/request construction; prevent origin confusion | Default/explicit port, IDN/ASCII normalization, userinfo, suffix lookalikes, query/fragment, path traversal/encoded separators | Maintainer: origin and path policy; no wildcard credential scope by implication |
| CSEC-03 | Deny all non-public; exact private exceptions; separate local/private/cloud modes | LAN inference, enterprise gateways and development TLS may need exceptions | Destination classes within NetworkPolicyService/request boundary; metadata and special-use coverage; no ad hoc per-adapter denylist | Loopback/private/link-local, IPv4-mapped IPv6, IPv6 scopes, metadata addresses, public control, denied address among mixed answers | Maintainer: supported network classes and exceptions |
| CSEC-04 | Controlled connection binding; approved egress gateway enforcement; do not ship arbitrary endpoints yet | Custom DNS/proxies may be unsupported; availability must fail truthfully | Resolver/connection seam within existing Qt transport and authoritative policy; explicit TTL/re-resolution/TOCTOU rules | Public→private rebinding, answer-order variation, CNAME chain, stale cache, reconnect, alternate IP notation, TLS hostname preserved | Maintainer/security owner: enforceable threat model and supported platforms |
| CSEC-05 | Reject all redirects; bounded same-origin redirects; separately approved cross-origin endpoint change | APIs requiring relocation may fail until profile updated | Reuse existing manual redirect approach; recheck origin, policy, address and credential attachment; adopt hop bound before implementation | 301/302/303/307/308, scheme downgrade, cross-origin, cycles, same-origin private rebinding, mode change | Maintainer: permitted redirects and bounded-hop rule; never implicit auth forwarding |
| CSEC-06 | No private support; administrator-issued private profiles; explicit per-session exception | Managed enterprise services need deliberate setup | Existing policy/settings/grants with profile identity, expiry and revocation; trusted local CA uses OS trust | Exception absent/expired/revoked, origin/port mismatch, bad certificate, private control, no global relaxation | Maintainer plus deployment owner: exact trust/network/credential scope |
| CSEC-07 | Current slots with exact-origin binding; separate per-profile keys; short-lived credentials | Key migration and active leases need explicit behavior | Existing CredentialStore/settings references; no plaintext; migration/readback, revoke fence and clear failure; evaluate Gemini query→header parity against approved API contract | Echoed key, query/log/export/screenshot redaction, redirect, key rotation/revoke while active, cloud-api selection change | Maintainer/privacy owner: storage, lease/revoke and query-auth transition |
| CSEC-08 | System proxy; approved explicit proxy; proxy-disabled isolated validation | Proxy-required deployments and intercepting TLS need explicit support scope | Inspect effective proxy/trust without secrets; enforce or refuse destination policy; no new HTTP stack | Proxy-auth redaction, proxy failure, CONNECT destination, remote DNS, invalid TLS, mode revocation | Maintainer/deployment owner: proxy and CA profile |
| CSEC-09 | Conservative model Unknown; explicit user override with label; validated versioned model catalog | Existing adapter defaults may need clearer labels; overrides must not erase provenance | Extend existing ModelCapabilityProvenance/projection and invalidation rules; retain one binding authority | Missing/stale model metadata, string booleans, endpoint switch, Unsupported override, fixture success cannot promote, older catalog cannot replace newer | Maintainer: evidence precedence, freshness and allowed override semantics |
| CSEC-10 | Current loopback exception; total-network-off mode; optional stop-on-revoke | Offline local inference and active requests behave differently across alternatives | Inventory all callers; use authoritative policy on dispatch/hops; approved cancel semantics; never silently retarget a run | Direct helper and daemon entry, every GET/POST/stream, mode switch before/during request, redirects, no queued dispatch after revoke | Maintainer: mode naming/semantics, in-flight behavior and audit scope |

Recommended configurable-endpoint policy: fixed vendor origins remain the default; no arbitrary cloud
feature is enabled yet. If approved later, use exact HTTPS origin/path and per-profile credentials,
cloud-only destination enforcement, no authenticated redirects and explicit enterprise exceptions.
An origin is scheme + canonical host + effective port; path scope is an additional constraint, not
part of the origin. DNS resolution and the actual connection/proxy path must uphold the same decision.
HTTPS and an allowlisted hostname alone do not establish public-address or SSRF safety.

## 4. Capability provenance and presentation rules

Current [ModelCapabilities/ModelCapabilityProvenance](../../core/include/sentinel/core/model/ModelRouting.h)
records tri-state fields and one provenance per merged value, plus optional context/output limits.
ModelService merges defaults, router/provider metadata, matching catalog/runtime metadata,
programmatic metadata and user override. Unknown does not replace an earlier merged value.
This can legitimately preserve an adapter default, but cannot be shown as selected-model evidence.
There is no independent real-test evidence ledger in the capability struct today.

| Evidence level | Meaning and identity | Permitted presentation | What it cannot prove |
| --- | --- | --- | --- |
| E1 Adapter structure | Request serialization/parsing/gate exists at exact source/binary identity; synthetic fixture evidence | “Adapter supports text streaming”; “tool request format implemented” | Model accepts operation, endpoint availability, tool reliability or live success |
| E2 Provider metadata | Provider declaration/catalog with provider/origin/API identity, source and observation time | “Provider reports support”; missing/stale information explicitly labelled | Every selected model supports it, or current request will succeed |
| E3 Selected model metadata | Exact model/revision + origin/runtime identity, capability and limit source, timestamp | “Selected model advertises vision; unverified live” | Accurate inference/tool/schema behavior or cancellation of remote compute |
| E4 Validated real test | Approved exact model/API/platform/config/case/repeat packet, independent oracle and reviewer | “Validated for these operations on this model/version/profile/date” with scope link | All models, later aliases/versions, platforms or operations not tested |

Proposed presentation requires maintainer approval before UI/code changes:

- **Unknown:** “Not established” and show which evidence level is missing. No healthy/ready green state
  solely from Unknown or a synthetically Supported binding. Existing direct-attempt Unknown vision
  behavior is recorded, not silently changed; future trial behavior requires an explicit approved scope.
- **Unsupported:** “Unavailable through this adapter/binding” with source/reason. Do not claim that the
  vendor service universally lacks the feature merely because this adapter defaults to Unsupported.
- **Supported:** display the evidence level and exact provider/model identity. E1 Supported must remain
  visually/textually distinct from E3 advertised and E4 validated. Readiness/auth/health remain separate.
- Raw missing model metadata remains missing even when merged binding inherits E1 defaults. Show raw
  evidence and effective request policy separately; do not overwrite the binding of an active run.
- User/programmatic overrides are explicit declarations, not provider reports or E4 tests. Preserve
  their source and approval; a fixture constructing Supported never updates the acceptance ledger.
- Context/output limits carry source, tokenizer/estimate method and API/model version. A metadata limit
  and byte/token estimate are not a proven usable budget; account for schemas/history/images/reasoning.
- Cancellation disclosure distinguishes requested, acknowledged/client transport ended, and remote
  execution stopped. Only provider-specific receipts/observations can support the last statement.

| Provider | Current E1 operation/defaults | E2/E3 gaps and model limitations | E4 status here |
| --- | --- | --- | --- |
| OpenAI | Compatible text/SSE; tools/schema default Unsupported, conditional format implemented | Selected model tool/schema/vision/context/reasoning/parameter compatibility must be identified; Chat support does not imply Responses API support | No live validation |
| DeepSeek | Compatible text/SSE; tools/schema default Unsupported | Thinking/reasoning fields, strict/beta variants, model aliases and token budgets unaccepted | No live validation |
| Groq | `/openai/v1` compatible text/SSE; tools/schema default Unsupported | Not blanket OpenAI parity; exact model/parameter/tool/schema/vision support needed | No live validation |
| Mistral | Compatible text/SSE; tools/schema default Unsupported | Selected multimodal/schema model and non-chat API differences unaccepted | No live validation |
| Claude | Native Messages/text stream; tools E1 Supported; structured Unknown with conditional format | Native tool availability is adapter-level; exact model/schema/vision/context and continuation support need metadata and tests | No live validation |
| Gemini | Native generateContent/text stream; tools E1 Supported; structured Unknown with conditional format | Model names/limits, function continuation and version-specific signature requirements need exact identity; query/header auth paths differ | No live validation |
| Generic compatible | Fixed OpenAI product route; compatible lower-level text/SSE/tools/schema builders | No arbitrary product-origin/catalog contract; any third-party service needs its own approved metadata/profile | No generic service accepted |
| Ollama | generate/NDJSON; native tools/schema default Unsupported | Exact installed/runtime model metadata, template/context/vision and loaded identity required; server ownership separate | No live validation in this task |
| llama.cpp | Compatible Chat/SSE; tools/schema default Unsupported | Loaded alias/template/GGUF/runtime revision/context needed; metadata or override is not tool reliability | No live validation in this task |
| LM Studio | Compatible Chat/SSE and native catalog metadata; tools/schema default Unsupported | Exact loaded model/runtime/capability/context required; external process ownership separate | No live validation in this task |

All ten defaults expose streaming support structurally; model vision/combined capabilities remain
Unknown without additional facts. Bounded native-tool/structured streaming is explicitly unsupported.
The current JSON-object parser does **not** independently validate arbitrary JSON Schema compliance.
Choose an approved schema dialect/subset and independent oracle before claiming schema acceptance;
no new dependency/validator is selected by this plan. A bounded known schema can use an independent
field/type/enum oracle for that test alone, without claiming general schema validation in the product.

## 5. Bounded AC-P2-03 protocol — no execution authorized

### Approval packet and budgets

The approval packet must name an actual maintainer, operator and reviewer and link the relevant
PD-D02/03/04/10/12 dispositions. An empty field blocks execution; time passing is not approval.
No credential setup, account creation, billing limit change or purchase is authorized by this report.

| Required field | Proposed pilot value or required input | Gate |
| --- | --- | --- |
| Providers/models | Initially at most two already authorized providers: one compatible and one native, chosen by maintainer; exact model IDs/revisions mandatory | No provider or model is selected/approved here; no default/first-model substitution |
| Origin/network | Exact fixed HTTPS origin/path, effective port, region/project, network mode and proxy/trust profile | Custom/private origins deferred until CSEC policy adopted; policy approval separate from live-call approval |
| Credentials | Disposable least-privilege test project/key, read/inference scopes only where supported; no personal production key | Owner provisions/revokes separately; OS CredentialStore or approved ephemeral secret reference; never pasted into evidence/CLI arguments |
| Cost | Proposed hard ceiling USD 5 total, at most USD 2 per selected provider; unallocated remainder is not automatic spending authority | Maintainer must approve currency/amount/current price assumptions; currently authorized expenditure is **zero** |
| Requests | Proposed maximum 20 generation POST attempts + 4 catalog/auth GET attempts per provider; two-provider global cap 48 HTTP attempts | Count underlying retries, not just logical calls; serial concurrency 1; no automatic budget extension |
| Tokens | Proposed maximum 1,024 input tokens and 256 requested output tokens per generation, 60,000 input+output tokens globally | Validate count/upper bound including tools, history, hidden reasoning and images; stop if cost/usage cannot be bounded |
| Time | Proposed 15-second whole-generation/stream deadline; finite discovery deadline adopted before run; cancel injection after first nonempty delta | These are pilot abort bounds, not calibrated performance acceptance or production defaults; unavailable enforcement blocks run |
| Retry/failure | No manual retry-until-green or automatic provider/model fallback; every retry counts as an attempt/cost | Predeclare allowed transport retries and expose attempt counting; stop on privacy/policy/cost violation |
| Data | Synthetic short prompts, harness-only tool schema/results, generated geometric test image if separately approved | No repository code, files, memory, chats, audio, screenshots, personal identifiers or remote image URLs |
| Retention | Proposed private access-controlled evidence for at most seven days; sanitized summary only after review | PD-D12/owner must approve duration/location/access/deletion, provider retention/training/region terms and any extensions |

Budget numbers are proposed experiment bounds, not approved product policy or current vendor prices.
Before calls, the owner must supply a dated price/usage model, reasoning/image/request charges and a
conservative worst-case estimate for the exact allocation. Provider billing can lag; a local ledger
alone is not proof of a monetary cap. Use a separately approved enforceable project limit where
available; if cost cannot be bounded under the approved ceiling, that cell stays Blocked.
No billing action is performed here. Reserve the full worst-case charge before dispatch; a cancelled
or timed-out request does not release that reservation until independently reconciled. Explicitly
allocate requests/repeats within all ceilings before run.

A future bounded runner must use existing ModelService/LocalInference paths and account for every
underlying transport attempt before dispatch. Existing per-request deadlines do not by themselves
implement a global cost/request ledger. If this accounting/admission cannot be demonstrated offline,
live execution remains blocked. No new provider router, scheduler or HTTP stack is proposed.

### Case definitions and independent oracles

| Case | Future approved procedure | Required result/evidence |
| --- | --- | --- |
| LIVE-01 Authentication | Missing key rejected locally; one explicitly approved invalid synthetic credential request; valid disposable credential through approved route | Zero dispatch for missing key; truthful auth classification for invalid key; no real key/body/log leakage; 403 scope issues distinct from success |
| LIVE-02 Text | Prompt: “Return exactly SENTINEL_FIXTURE_17.” Three planned repetitions on the frozen identity | Exact output and truthful Completed; recorded usage/version; all attempts including mismatches retained |
| LIVE-03 Streaming | Request synthetic short text; capture ordered visible deltas, actual vendor terminal marker and final result, three repetitions | No incomplete stream marked Completed; final/delta consistency; model/origin unchanged; text-only streaming scope |
| LIVE-04 Tools | Only if selected model evidence and approval permit it: harness-only `fixture_echo`, argument nonce `fixture-17` and ordinal integer 17; three repetitions including safe continuation | Exact tool ID/call ID/arguments/schema mapping and result correlation; harness returns constant `fixture-result`; no filesystem/process/network tool side effect |
| LIVE-05 Structured | Only approved model/dialect/subset: object with required `answer` integer enum [17], no additional properties; three repetitions | Independent exact type/key/enum oracle; wire schema compared to original; object parsing alone insufficient |
| LIVE-06 Vision | Conditional, separately priced/approved: deterministic small black/white quadrant PNG generated locally with known answer; three repetitions | Exact independently known pattern answer; recorded byte/hash/dimensions and model vision evidence; no hosted image fetch or private image |
| LIVE-07 Unsupported/invalid | Prefer local capability-negative request with zero dispatch; at most one approved harmless invalid schema/parameter request where useful | Truthful typed rejection, no silent text-only substitution or provider/model fallback; invalid live test consumes budget |
| LIVE-08 Rate/failure | Do not induce throttling/outages by floods. Use retained deterministic 429/5xx fixtures; classify naturally encountered live 429/5xx | Live failure receipt if encountered; otherwise live row Unverified/deferred, not fabricated Passed; no retry amplification |
| LIVE-09 Cancel/deadline | Pre-cancel zero dispatch; one cancel after first visible delta; one finite-deadline injection with approved limit | Client terminal timestamp, no late deltas or false final success; provider request receipt/usage where available; remote termination explicitly Unconfirmed unless proven |
| LIVE-10 Privacy | Review safe request metadata, public errors, diagnostics and evidence derivatives for every case | No keys/auth URLs/private data in logs/export; no synthetic key echo reflected; credential references only; stop on leakage |

The proposed allocation fits at most 20 generation attempts/provider: text 3 + stream 3 +
tools 3 requests plus 3 continuation POSTs + structured 3 + vision 3 + cancel/deadline 2 = 20.
Pre-cancel, missing-key and local capability rejections use zero HTTP attempts. An optional invalid
live request or any generation retry requires an explicitly reduced/preallocated alternative scope
or amended approval; there is no spare generation attempt in the complete optional-capability set.
The four GET attempts/provider must also include any discovery/auth transport retries. Unsupported
optional features do not automatically free spending authority for other experiments. Never silently
omit required repeats or claim omitted cases passed. If a failure exhausts the allocation, stop and
retain the incomplete matrix rather than exceed it.

Every provider uses LIVE-01/02/03/07/08/09/10. LIVE-04/05/06 are conditional on explicit evidence and
approval, not on vendor name or a synthetic Supported binding. Native-tool and structured streaming
remain excluded through the current adapter. An approved exploratory Unknown-model trial is labelled
exploratory and cannot replace required E3/E4 acceptance without review.

### Proposed provider/model acceptance matrix

No model IDs below are guessed; every “not supplied” cell is an execution blocker.

| Provider/profile | Exact approved model ID/revision | Text/SSE/auth/cancel/privacy | Tools/schema/vision | Additional limitation | Present status |
| --- | --- | --- | --- | --- | --- |
| OpenAI | Not supplied | LIVE common cases | Conditional LIVE-04/05/06 | Chat versus Responses API and reasoning/token parameters must be explicit | Not authorized / Blocked |
| DeepSeek | Not supplied | LIVE common cases | Conditional | Thinking/reasoning/strict variants and model alias behavior need separate expectations | Not authorized / Blocked |
| Groq | Not supplied | LIVE common cases | Conditional | Exact model and compatibility subset; no assumption of OpenAI parity | Not authorized / Blocked |
| Mistral | Not supplied | LIVE common cases | Conditional | Exact multimodal/schema model and API version | Not authorized / Blocked |
| Claude | Not supplied | LIVE common native cases | Conditional | Tool continuation and schema mode; adapter default is not model evidence | Not authorized / Blocked |
| Gemini | Not supplied | LIVE common native cases | Conditional | Model identity, continuation signatures and auth placement captured | Not authorized / Blocked |
| Generic compatible | Not supplied; product origin/profile absent | Future only after policy/configuration decision | Conditional | Existing fixed OpenAI alias is not an independently accepted third-party service | Blocked on product/security contract |
| Ollama | Existing disposable installed model/revision not supplied | Separate approved local common cases | Only adapter/model-supported subset | No download/start/unload/process termination authority; no cloud credentials | Blocked on PD-D02/environment |
| llama.cpp | Existing GGUF digest/alias/runtime revision not supplied | Separate approved local common cases | Conditional template/model subset | Loaded identity and ownership acceptance separate; no lifecycle changes | Blocked on PD-D02/environment |
| LM Studio | Existing loaded model/runtime revision not supplied | Separate approved local common cases | Conditional | External application ownership; no load/unload authority implied | Blocked on PD-D02/environment |

This is a proposed matrix, not a selected run set. A first two-provider cloud pilot cannot close
AC-P2-03's whole BE-ALL/native scope. Skipped capability, unavailable provider or platform remains
open unless the maintainer explicitly defines a narrower release scope; exclusion does not turn it
into a passed criterion. P1 Agent reliability/COR cases remain separate prerequisites and are not
claimed by direct provider-only tests.

### Environment, failure and evidence procedure

1. **Approve and freeze:** name operator/reviewer; adopt required decisions and exact case/repeat
   allocation, model/API/origin/credential references, budget, egress and retention. Capture candidate
   HEAD, dirty diff, source/binary/configuration digests and prior evidence without overwriting it.
2. **Offline preflight:** rerun relevant deterministic policy/redaction/capability/deadline cases for
   that candidate under separate execution authorization. Confirm secure-store availability, attempt
   accounting, finite deadlines, payload bounds and no fallback. This task runs none of those tests.
3. **Environment:** declare exact Fedora KDE/Linux, macOS and Windows cells under PD-D03, Qt/compiler/
   TLS/proxy/secret-store versions and network profile. Use a disposable Sentinel profile, no personal
   sessions, and one frozen ModelBinding per case. Unknown platform availability stays Blocked.
4. **Model/API capture:** record requested and returned model IDs, versioned alias/revision where
   available, API version/header, endpoint/region, context/output limits and tokenizer. Unavailable
   immutable revision is explicitly Unknown; acceptance attaches to the observed alias/date and cannot
   be transferred to later service changes. Never assert a returned model ID was cryptographically verified.
5. **Run serially:** execute only approved cases, checking remaining requests/tokens/cost before each
   underlying attempt. Network or permission denial stops dispatch; selection changes do not rebind an
   active request. No automatic provider, model, region, protocol or credential fallback.
6. **Handle failure:** retain first and every subsequent outcome with request/case identity; distinguish
   auth/configuration/rate-limit/provider/malformed/timeout/cancel. Stop on egress, identity, privacy,
   invalid TLS, budget, unauthorized side effect or unexpected redirect. No automatic retry-until-green,
   timeout inflation or credential substitution. Resumption needs a bounded explicit disposition.
7. **Cancellation receipt:** record monotonic cancel request, acknowledgement/client abort, last visible
   delta and terminal times; request/response IDs and later usage may indicate continued billing. Do not
   query undocumented remote-stop APIs. Without a provider-supported termination receipt and independent
   validation, report “client cancelled; remote computation/charge termination unconfirmed.”
8. **Evidence:** follow EV-01: candidate/config/source/binary identities, case/repeat/expected terminal,
   model/API/OS details, timing/usage/attempts, safe request IDs, schema/image hashes, independent oracle,
   all failures and unauthorized-side-effect diff. Do not serialize Authorization headers, query keys,
   secret store contents or raw private profiles. Keep raw material restricted with credentials removed;
   publish only a reviewed minimal derivative. If exposure occurs, stop and follow private security intake;
   owner-approved rotation/removal is separate, never performed automatically by this plan.
9. **Review and teardown:** reviewer signs each exact cell; owner separately revokes disposable keys and
   applies adopted local/provider retention/deletion policy. Revocation is not proof that provider logs,
   remote computation or billing stopped. Record Unknowns and remaining cells; no publication authority.

## 6. Concrete follow-up order

| Order / proposed task | Deliverable and dependency | Completion boundary |
| --- | --- | --- |
| 1 — P2-10 Endpoint policy adoption and enforcement | Maintainer adopts CSEC origin/private/DNS/redirect/proxy/credential decisions; implement only approved rules in NetworkPolicyService, ModelService and existing Qt request paths; enumerate helper callers | Deterministic direct/caller/redirect/rebinding/IPv4/IPv6/credential-negative oracles and cross-platform transport feasibility; no live acceptance claim |
| 2 — P2-11 Capability evidence disclosure | Adopt E1–E4/override/freshness rules; extend existing provenance/projection and exact-model catalog behavior; define independent schema oracle/subset | Unknown/Unsupported/support-source, stale model/origin, context/vision and fixture-no-promotion regressions; no new registry or AgentRuntime change |
| 3 — P2-12 Bounded live harness and vendor compatibility preparation | Reuse existing provider probes/fixtures; approved global admission/attempt/cost ledger, per-vendor expectation sheets and credential-safe evidence packet; depends on policy/disclosure decisions | Offline dry run proves budget/deadline/egress/fallback fencing; exact provider/model approvals still required |
| 4 — P2-13 Approved real-provider acceptance and independent platform validation | Explicitly authorize bounded selected cells; separate operator/reviewer, Fedora-first plus approved macOS/Windows profiles; retain every result | Exact E4/EV-01 cells accepted only after review; unavailable/failed/omitted cells remain open and whole P2/P1/P3 criteria do not close automatically |

Tasks/roles above are proposals, not created issues, assigned humans or approved implementation work.
Reuse ModelService, LocalInference, NetworkPolicyService, ModelBinding and
[existing scripted fixtures](../../tests/support/ScriptedProviderTransport.h). No new router or HTTP
stack is recommended. P3 ownership, P1 Agent integration and release publication remain independently scoped.

## 7. Acceptance and documentation validation

**AC-P2-01: Unverified / not satisfied. AC-P2-03: Blocked.** AC-P2-04 credential/network acceptance
and PD-D02/03/04/10/12 decisions remain open where applicable. E1 fixtures, provider catalog declarations
and this planning document do not establish E4 or authorize any remote call. No shared acceptance
matrix or decision-register status changed.

Documentation checks only: repository-relative links and acceptance/report references, Markdown
structure, changed-file privacy, preservation inventory and `git diff --check`.

| Check | P2-09 result |
| --- | --- |
| Repository links | 23 relative file links resolve; no external API/document requests required |
| Acceptance/report references | P2-05…08 reports and every cited AC/PD identifier exist in the current repository |
| Markdown | Nine planning tables plus this receipt table have consistent columns; balanced fences and no trailing whitespace |
| Changed-file privacy | Passed existing personal-path pattern; one new report, no sensitive original values |
| Git whitespace | `git diff --check` passed; explicit new-file diff check produced no whitespace diagnostics (no-index exit 1 denotes the new-file difference) |
| Preservation | All 5,243 entry files byte-identical; report is sole addition; index untouched |
| Final identity | Branch remains `test/p2-01-provider-contracts`, HEAD `f87843f1edcab1e5e02defa61b8e560dcc001d92` |

No CMake/CTest or live/cloud test ran in P2-09; historical results are not reused as validation of this
new report. This agent performed no stage, commit, push, merge or remote write.
