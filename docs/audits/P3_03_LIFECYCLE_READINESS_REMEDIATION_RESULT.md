# P3-03 — Lifecycle readiness, model identity and stale health remediation

Date: 2026-10-16 (Europe/Istanbul). Existing managed worktree
`p3-01-lifecycle-ownership/Sentinel-sanitized`; branch `test/p3-01-lifecycle-ownership`.
Verified entry HEAD: `619e989c419956393201a749da1cede386e83ba3`.
Final observed HEAD: `5d8ce5bac64d2d8772e0fe7709029f09872ee9c4` (concurrent commits; see below).
**Readiness remediation only. AC-P3-02 remains Blocked; PD-D10 is not adopted.**

## Preflight and preservation

Read AGENTS.md, [P3-01](P3_01_MODEL_LIFECYCLE_OWNERSHIP_RESULT.md),
[P3-02](P3_02_LIFECYCLE_OWNERSHIP_CONTRACT_RESULT.md), all seven existing XFAIL assertions,
the D6 skip, architecture/runtime/security/build/testing guidance and pending ownership/acceptance
contracts. Personal-Brain was attempted but unavailable (`brain: command not found`); the
user-specified worktree and Git identity were verified directly.

Entry contained uncommitted P3-01/02 source, fixtures, CMake and reports. SHA-256 entry copies
were saved privately before editing. P3-01 test, P3-01/P3-02 reports and `tests/CMakeLists.txt`
remain byte-identical. The P3-02 dependency seam and registration are preserved. No P1, P2,
SEC-01, integration or other worktree was modified. This agent executed no reset, stash, commit, push or merge. Concurrent Git changes are recorded below.

## Exact scope and corrected behavior

| Defect | Correction | Evidence and remaining limit |
| --- | --- | --- |
| D1: HTTP 200 treated as selected-model ready | Health success now establishes reachability only. A separate read-only runtime identity probe must supply the exact selected alias and explicit non-sleeping state before Ready. | Foreign alias, missing/malformed/unsupported metadata and sleeping model cannot be Ready. Matching runtime metadata has a positive counterpart. API self-report is not real-model reliability or a physical residency lease. |
| D2: implicit first GGUF when selection empty | Empty/whitespace selection returns MissingModel before probing, starting or terminating. Explicit selection must match an eligible library entry; no first-entry fallback remains. | Both empty forms preserve the selection; explicit matching selection still starts exactly once with original arguments/permission. An unavailable explicit model cannot substitute another GGUF. Clearing selection does not authorize unloading an existing child. |
| D3: stale readiness after health failure | Every health cycle projects Unknown while pending; HTTP/transport failure invalidates Ready to Unavailable. Identity failure yields Unknown/MissingModel/Unavailable as appropriate. Fresh successful evidence is required for recovery. | HTTP 500/503, owned connection refusal, shared/owned timeout and HTTP 200 with a transport error are deterministic rows. Same-selection old replies cannot restore Ready after a newer failure. |
| D5a: old endpoint callback accepted | Provider/model/endpoint observation plus a monotonically increasing probe revision and exact active-reply identity fence both health and identity callbacks. | Obsolete endpoint, ABA, duplicate and same-selection old request cases cannot publish success, dispatch follow-up probes or overwrite a newer outcome. New endpoint evidence still reaches Ready. |

These changes do not implement D4 provider-switch termination, D5b owned-child endpoint reconciliation,
D6 in-use removal protection, counted leases, explicit load/unload APIs or new shutdown escalation.

## Model identity approach and compatibility boundary

The existing Sentinel discovery implementation in
[OllamaRuntime.cpp](../../core/src/runtime/OllamaRuntime.cpp) already reads llama.cpp `/props`
`model_alias` metadata. Upstream [llama.cpp server source](https://github.com/ggml-org/llama.cpp/blob/master/tools/server/server-context.cpp)
reports that alias and `is_sleeping`; its health response does not attest selected identity.
The [server API documentation](https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md)
describes sleeping-state metadata, read-only properties and router autoload behavior.
Sources inspected on the audit date; mutable upstream master is context, not a pinned supported runtime version.

The helper now performs:

1. Existing allowed loopback HTTP `/health` GET, with the existing 1500 ms transfer timeout.
2. On HTTP 200 **and** no transport error, `/props?autoload=false` GET at the same captured endpoint,
   also using the unchanged 1500 ms transfer timeout and checked against network policy.
3. Accept only successful JSON-object metadata with nonempty string `model_alias` exactly equal to
   the captured selected model and boolean `is_sleeping=false`.

There is no model-selection query, inference request, model acquisition, API mutation or autoload request.
Redirect following is explicitly disabled for both probes, preventing endpoint identity from silently moving
elsewhere. [Qt documents](https://doc.qt.io/qt-6/qnetworkrequest.html#setTransferTimeout) this as a no-data transfer timeout, not an absolute wall-clock lifecycle deadline. No hard timing or upstream compute-release bound is claimed. Catalog membership, a filename, process startup, cached provider capabilities and HTTP 200
alone are never substituted for runtime identity. Raw alias/body/server diagnostics are not copied into status.

An older/compatible server missing either identity or explicit sleeping evidence remains **Unknown /
unverified**, even if chat works. A router without unambiguous runtime properties also remains unverified;
no query is added to select/load a model merely to make readiness pass. Matching metadata is a
point-in-time observation from the selected trusted endpoint, not cryptographic attestation, residency
reservation, successful inference or broad backend compatibility. No native endpoint/model execution
was attempted here. Additional backend schemas require separately reviewed evidence, not permissive fallback.

## Readiness projection and fencing

[LocalModelRuntime](../../apps/sentinel-daemon/service/LocalModelRuntime.h) reuses existing
`core::RuntimeReadinessState`; no new enum, IPC schema, QML contract, router or runtime was introduced.
The existing status string remains the production presentation property; the typed C++ accessor supports
independent fixture assertions and does not rewrite ModelService's separate provider health/catalog state.

| Observation | Helper state |
| --- | --- |
| Health or identity request pending; selection/provider/endpoint invalidated | Unknown |
| Health reachable, identity unavailable/malformed/unsupported | Unknown, explicit identity-unverified status |
| Explicit selection empty, eligible file absent, or runtime alias foreign | MissingModel |
| Health HTTP/network failure, or selected runtime explicitly sleeping | Unavailable |
| Endpoint rejected by helper gate | InvalidEndpoint |
| Identity request rejected by network policy | Unauthorized |
| Exact current alias plus non-sleeping metadata after successful current health | Ready |

Each new health cycle advances the revision; the health/identity pair shares its revision. Selection
invalidation and shutdown also advance it. The controller's existing selection/library signals retain their
wiring, and `ollamaStatusChanged` is observed because endpoint setters emit that signal. Tuple changes
are recorded synchronously, including A -> B -> A before callbacks. Same-tuple library/health signals
do not create a new identity or implicitly reset process retry policy.

Callbacks re-observe provider/model/endpoint and must match both revision and active QNetworkReply.
Old callbacks cannot clear a newer probe, publish status, start a process or launch identity follow-up.
A QPointer guards the active reply. Cancellation fences and clears the pointer before abort, because
abort may synchronously emit finished. Destruction disconnects this receiver before abort and invokes
no borrowed presentation callback; QObject-context connections also reject post-destruction delivery.
Explicit `beginShutdown()` invalidates readiness and rejects late completion; it does not change owned
process shutdown policy or claim upstream compute release.

Identity confirmation is used only to avoid repeated discovery-refresh loops. It is reset on invalidation
or failed final evidence; every periodic cycle still performs fresh health and identity checks. A recovery
refreshes discovery, while repeated healthy observations do not continually trigger model-library refresh.
Frozen selected values and provider binding are not rewritten. No settings or credentials are modified.

## Regression witnesses and XFAIL reconciliation

| Original P3-02 assertion | Before | After |
| --- | --- | --- |
| D1 foreign alias | 1 XFAIL | Normal negative assertion plus matching positive counterpart |
| D2 empty selection | 1 XFAIL | Normal empty/whitespace negatives and explicit-selection positive |
| D3 shared HTTP failure / owned refusal | 2 XFAIL | Normal assertions, expanded to six health-failure/recovery rows |
| D5a old endpoint result | 1 XFAIL | Normal assertions plus valid-new-endpoint, ABA and stale-request cases |
| D4 provider-switch owned cleanup | 1 XFAIL | **Unchanged XFAIL**; pending PD-D10 policy |
| D5b same-model endpoint owned cleanup | 1 XFAIL | **Unchanged XFAIL**; pending reconciliation policy |
| D6 deletion while actually in use | 1 SKIP | **Unchanged SKIP**; no residency-lease/admission interface exists |

Seven XFAIL assertions become **two**, converting only the five repaired assertions. The expanded helper
contains **49 QtTest passes including init/cleanup, zero unexpected failures and one skip**, with two
XFAIL assertions inside passing witness rows. This is 47 executed substantive rows plus one skipped
row, **30 more executed rows than P3-02**. No new CTest target was registered.

Additional cases cover malformed/array/health-only/catalog-only props, missing/wrongly typed alias/sleep
metadata, unsupported props, metadata timeout/redirect, explicit sleeping, old health/identity versus newer
failure or success, old request in unchanged selection, endpoint ABA during both phases, synchronous
abort completion, duplicate events, repeated healthy refresh, shutdown/destruction, and empty/unavailable
selection without substitution or owned-child stop. Original busy/model-switch, process exit, shared-listener,
endpoint-policy and superseded-selection tests remain.

Every fixture uses synthetic model bytes and existing P3-02 injected IO; no actual network request or model
process is started by the helper fixture. Exact process start/termination counts are asserted, only the
synthetic owned handle is accepted, and artifact bytes/changed-file allowlist are independently checked.
P3-01 additionally verifies actual ModelService settings/binding behavior and unknown process-handle refusal.
Simulation is not native process/backend validation.

## Validation and retained observations

Host: macOS 27.0.1 arm64, AppleClang 21.0.0, Qt 6.12.0; existing isolated
`build/tests` tree. No dependencies were installed. Validation uses synthetic fixtures;
no real models, cloud credentials or external user processes were used.

| Validation | Actual outcome |
| --- | --- |
| Configure | `cmake --preset tests -DSENTINEL_USE_CCACHE=OFF`: passed. |
| Complete build | `CCACHE_DISABLE=1 cmake --build --preset tests -j6`: complete default-target build passed; final destructor-safe source rebuilt successfully. |
| Focused lifecycle/process/provider regression | **8/8 CTest targets passed**, 6.92 s: P3-01 ownership, actual helper, ProcessExecutor, local inference, Ollama runtime, runtime provider, provider contracts and model registry. |
| Final actual-helper fixture | **49 passed, 0 failed, 1 skipped**, two XFAIL assertions; 47 substantive executed rows, one substantive skipped row, plus init/cleanup. |
| Final full sequential CTest | **132/132 targets passed**, 144.44 s, offscreen; final source hashes below. |
| Full parallel CTest `-j6` | **132/132 targets passed**, 52.56 s, offscreen; final destructor-safe source. |
| Formatting | `clang-format --dry-run --Werror` passed on both helper files and fixture. |
| Scoped static analysis | PCH-free temporary compile database; clang-tidy `clang-analyzer-*`, `bugprone-*`, `readability-braces-around-statements`, helper header filter and warnings-as-errors: **passed**, no user-code diagnostics; 977 non-user warnings suppressed. Not whole-repository certification. |
| Generated contracts | Both `python3 tools/ipc/generate.py --check` and `generate_desktop.py --check` passed; generated sources unchanged. |
| Whitespace / report / scoped privacy | `git diff --check`, relative links, Markdown fences and changed-file personal-path/obvious private-key/token pattern checks passed. Limited pattern scanning is not complete secret-security certification. |
| Full repository personal-path scan | **Failed**, one inherited `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` finding. Preserved unchanged; sensitive content not reproduced here. |
| P3-01/02 preservation | Both historical reports, P3-01 test and complete CMake file match entry SHA-256. D4/D5b witness bodies and D6 skip match entry text exactly. |

An earlier development helper run recorded **19 passed, 6 failed, 1 skipped**. The six failures
were recovery discovery-refresh counts (actual 1 vs expected 2) in the initial confirmation-latch
implementation. Failure/invalidation now resets that notification latch while every request still
requires fresh evidence; the six recovery rows pass. No assertion was weakened or timeout increased.
The first strict static analysis also failed on a destructor-to-presentation callback exception path;
destruction now disconnects/aborts the owned probe without invoking a borrowed presentation callback.
Both initial results remain in private logs. An earlier sequential development run passed 132/132
in 159.37 s; the final identified source run above is the validation candidate.

The inherited P3-01 parallel `nativeDiagnostics(content-usage)` failure remains an unresolved
intermittent observation. P3-02 recorded 20 passing bounded repeats; this task's full sequential
and parallel runs passed the existing LLM target. Neither these reruns nor helper readiness fixes
establish a root-cause correction in that unrelated fixture. No provider implementation or unrelated
fixture timeout was changed. Native Linux/Windows, real endpoint/schema compatibility, physical
residency and live-model task reliability remain unverified.

Raw local evidence is retained privately under `/tmp/sentinel-p303-*`: entry copies/hashes, configure,
builds, initial and expanded/final helper output, focused CTest, full sequential/parallel CTest and
initial/final scoped static analysis. Raw machine-home output is not embedded in public documentation.

### Concurrent Git change and exact modified files

During validation, HEAD advanced through two commits created outside this agent's commands:

- `3863cdd51600c020721b0867692b111bbd0f7903` — `feat(daemon): adopt model lifecycle ownership contract in LocalModelRuntime`;
  includes the accumulated P3-01/02 work **and the in-progress P3-03 header, fixture and readiness implementation**, before the final destructor correction.
- `5d8ce5bac64d2d8772e0fe7709029f09872ee9c4` — `docs(audit): add P3-01/P3-02 lifecycle ownership results`;
  contains the unchanged historical reports.

This was detected by final HEAD/status/reflog inspection. No commit command was issued here;
those commits were preserved without reset or amendment. Their subjects do not establish task completion
or PD-D10 adoption: the acceptance/decision registers remain unchanged and Pending Decision.
Source SHA-256 values remained equal to the final built inputs despite Git metadata changing.

Relative to task entry, the exact four P3-03 files written are:

- [LocalModelRuntime.h](../../apps/sentinel-daemon/service/LocalModelRuntime.h).
- [LocalModelRuntime.cpp](../../apps/sentinel-daemon/service/LocalModelRuntime.cpp).
- [test_local_model_runtime.cpp](../../tests/desktop/test_local_model_runtime.cpp).
- This new report.

At the final observed HEAD, only `LocalModelRuntime.cpp` remains modified (the destructor correction)
and this report is untracked; the other two P3-03 source files were included in the concurrent commit.
The full working source, including that uncommitted correction, is the tested candidate; HEAD alone
is not the validated binary identity. Do not publish or import these mixed-stage commits as a clean,
completed P3-02/P3-03 change without maintainer review and explicit authorization.

Final executable-input SHA-256 values:
- `apps/sentinel-daemon/service/LocalModelRuntime.h`: `34b8bac726fef3ffb327133a19bced301964190e072ffe765cd57b16b9cd991a`.
- `apps/sentinel-daemon/service/LocalModelRuntime.cpp`: `9490a57d4c242ad1fddf7fbad64f88b6c14852f5a0efad19f4ea37f3da17ff60`.
- `tests/desktop/test_local_model_runtime.cpp`: `bf1b716a84bde498608b5ca40db77af86d607d9fc541c0b0c53edb3a9a4e808a`.

## Remaining ownership decisions and next implementation task

- **D4:** Switching provider still retains an existing owned child; no new termination behavior is approved.
- **D5b:** Endpoint switch still does not reconcile the old owned child. Fencing its health observations is
  separate from deciding whether/when that resource may be stopped.
- **D6:** ModelOperationService still lacks a general active-use deletion fence; retained C++ provider
  handles do not prove actual model/process/artifact leases.
- Explicit load/unload/loading cancellation, all-consumer leases, bounded recovery/retry and shared-server
  administration remain missing or pending design. Clearing selection conservatively does not unload.
- PD-D10 remains Pending Decision; AC-P5-04 still depends on approved ownership, shutdown budgets and
  other required decisions. No external listeners/PIDs were adopted, killed, unloaded or deleted.

Recommended next task: obtain the named Models/IPC/security owner and maintainer's PD-D10 decision
on resource identity, active leases and refusal/drain policy, then implement narrowly scoped ownership
reconciliation and lease/removal witnesses. In parallel, separately authorize disposable native llama.cpp
readiness compatibility validation for declared versions; do not download models or use personal profiles
as an implicit next step.

**AC-P3-02 remains Blocked. P3 is not complete.** These fixes close deterministic helper false-readiness,
implicit-selection and stale-health defects; they do not close ownership/unload/lease/recovery, native backend
or independent acceptance evidence requirements. This agent did not commit, push or merge.
