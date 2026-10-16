# P3-02 — Ownership contract and lifecycle testability

Date: 2026-10-16 (Europe/Istanbul). Branch `test/p3-01-lifecycle-ownership`, HEAD
`619e989c419956393201a749da1cede386e83ba3`, existing managed P3 worktree.
**Contract proposal and deterministic witnesses; PD-D10 remains Pending Decision.**
This document specifies current authority and proposed invariants; it does not adopt new product authority.

## Baseline and preservation

Preflight verified branch, HEAD, status and AGENTS.md. Personal-Brain was attempted but unavailable
(`brain: command not found`); the user-specified worktree was verified directly.
Entry changes were `tests/CMakeLists.txt`, `tests/core/test_model_lifecycle_ownership.cpp` and
[the P3-01 report](P3_01_MODEL_LIFECYCLE_OWNERSHIP_RESULT.md). The existing test and report remain
byte-identical; the existing CMake registration is retained, with a separate P3-02 target appended.
No other worktree, provider implementation, acceptance/decision register, workflow or Agent Runtime
was modified. No reset, stash, commit, push, merge, cloud call, model download or user-process termination occurred.

Reviewed sources: [P0-03 ownership review](P0_03_OWNERSHIP_COMPATIBILITY_REVIEW.md),
[acceptance matrix](../roadmaps/ACCEPTANCE_MATRIX.md), [PD-D10](../roadmaps/ACCEPTANCE_DECISIONS.md#pd-d10--provider-ownership-and-hard-stop-policy),
[architecture](../development/ARCHITECTURE.md), [Agent Runtime](../concepts/AGENT_RUNTIME.md),
[daemon shutdown](../development/DAEMON.md#shutdown-drain-and-provider-lifetime),
[security](../security/SECURITY_MODEL.md), [building](../development/BUILDING.md) and
[testing](../development/TESTING.md).
Implementation trace includes LocalModelRuntime, ModelService, ModelOperationService, ModelBinding,
ModelStorageManager, ProcessExecutor and DaemonService, plus their existing tests.

## Ownership and operation contract

Current code is authoritative for existing behavior. The proposed lease/drain/reconciliation requirements below
need implementation and maintainer approval; naming them here does not grant that authority.

| Resource | Current owner and permitted operations | Forbidden inference / remaining requirement |
| --- | --- | --- |
| Sentinel-managed llama.cpp child | LocalModelRuntime may start the fixed eligible local command through its private ProcessExecutor. Only the returned opaque handle identifies its child. Idle model change requests termination; destructor shuts down its own executor. | No PID/name/port adoption. Current automatic model/endpoint reconciliation is incomplete. Starting a child loads its fixed GGUF; there is no public typed load/unload API. |
| Shared Ollama | External server owns process/residency. Sentinel owns HTTP requests and supports explicit pull/delete operations through existing model operation paths. | Discovery and API mutation permission confer no process ownership. No server kill or residency-unload authority; explicit deletion still needs an in-use safety policy. |
| Shared LM Studio | External application owns process/model. Sentinel owns requests, discovery and an external manager link. | Opening the manager does not adopt it. No process stop or model residency unload authority. |
| Shared llama-server listener | Listener owner retains process/model. Existing HTTP listener prevents competing startup by the helper. | HTTP health or a successful connection cannot create an owned handle, verify selected alias or authorize shutdown. |
| Remote provider | Remote service owns compute/residency; Sentinel owns client request handles, selected binding and credential configuration. | Client abort/cancellation is not evidence of upstream compute release; no remote model/process stop authority. |

| Operation | Existing authority | Proposed invariant / missing implementation |
| --- | --- | --- |
| Start owned process | Fixed executable lookup and argument list after allowed loopback HTTP connection refusal, existing GGUF and provider eligibility. `unconfinedPermitted=true` is preserved from baseline. | Require explicit selected model identity; record owned resource identity and endpoint generation. No new command or permission is approved here. |
| Stop owned process | ProcessExecutor accepts only its own start handles; selection change checks Agent/Chat busy. Existing executor may escalate termination of its own children. | Count all users, refuse/drain while borrowed; never terminate external/shared resources or destroy active dependencies to claim completion. |
| Load/unload model | Owned child startup loads its model; explicit typed unload/cancel-loading API absent. Pull/import/download are acquisition, not load. | Define idempotent load/unload and refusal/drain result before implementing it. No shared-server unload is implied. |
| Cancel request | Existing request token, abort and terminal fencing; frozen binding remains attached to active request. | Report client acknowledgement separately from server residency/compute release. Cancellation cannot transfer process authority. |
| Remove artifact | ModelOperationService delegates local registration removal or managed artifact removal; ModelStorageManager checks owned sidecars. Ollama delete is an explicit server inventory mutation. | Artifact ownership is necessary but insufficient: active leases must explicitly refuse/defer removal. Registration removal is not deleting an imported external file. |
| Switch provider/model/endpoint | ModelService updates selection; ModelBinding/provider handles already captured by a run persist. Helper partially reconciles model changes. | New selections affect new requests; fence old health by endpoint/model revision, drain/refuse old owned child consistently, never stop external listeners. |

A `shared_ptr<IChatProvider>` protects the C++ provider object, not the server model, GGUF file or
process residency. ModelBinding is a captured provider/model/capability value, not a counted physical-resource
lease. `ModelLifecycleService` static status/pricing metadata does not implement residency management.
DaemonService stops helper polling at shutdown start, retains dependencies during drain and destroys
LocalModelRuntime before its controller. Non-cooperative work may truthfully remain Stopping;
AC-P5-04 does not permit pretending it exited.

## Observable state contract

These are contract vocabulary, not a new runtime enum or IPC state machine. Preserve existing typed request,
process and operation states; add any future unified projection only through the existing single-source contracts.

| Contract state | Existing observations | Invariant and missing implementation |
| --- | --- | --- |
| Unavailable | Provider health/error classifications, missing runtime/model status strings. | Current inability to serve must invalidate old readiness; helper does not reliably do this. Distinguish unknown from unavailable. |
| Unloaded | No owned process handle; catalog/registered files may exist. | Absence of a helper child does not prove a shared server model is unloaded. No explicit residency-unloaded acknowledgement exists. |
| Starting / Loading | Helper starting string; ProcessState Starting/Running; acquisition has separate Queued/Running. | Process Running does not mean model Ready. Loaded alias and endpoint identity must be verified; download Running is not model Loading. |
| Ready | Helper currently projects HTTP 200 ready; provider health/catalog observations are separate. | Required identity includes selected model/endpoint revision and fresh successful observation. Current helper lacks that verification. |
| InUse | Agent loop and Chat busy flags; active bindings/provider handles. | No general counted model/process/artifact lease exists. Busy flags do not cover all consumers or prove residency. |
| Stopping / Unloading | ProcessState Stopping, runtime shutdown drain; helper has no explicit unloading state. | Reject new admission, retain borrowed dependencies, wait for owned-resource acknowledgement; never label a shared listener stopped. |
| Failed | ProcessState Failed, error strings, provider/request/operation errors. | Clear obsolete readiness and retain failure provenance; retry must be bounded/explicit. `attemptedKey_` suppression is not a recovery policy. |
| Cancelled | Request/operation/process cancellation states exist; helper reports generic stopped/error. | Exactly one authoritative request terminal; cancellation during loading is not implemented. Request Cancelled does not imply unloaded or remote compute released. |

## Injection architecture and scope

[LocalModelRuntime](../../apps/sentinel-daemon/service/LocalModelRuntime.h) now accepts a narrow
in-process `Dependencies` adapter bundle in addition to its unchanged controller constructor.
The controller adapter reads provider/endpoint/model/busy observations, library entries and publishes
status/discovery. IO adapters cover health reply creation, executable lookup and the existing process
start/record/terminate operations. Tests explicitly drive `ensureRunning()` with automatic polling disabled.
Production uses the existing 3000 ms timer, queued reconciliation, 1500 ms health timeout,
QNetworkAccessManager and private ProcessExecutor. The fixed command, existing unconfined permission,
endpoint/network gates and reconciliation conditions remain unchanged.

The helper's actual algorithm is exercised; there is no copied runtime, second process manager,
provider router or model lifecycle state machine. Tests inject QNetworkReply objects with manually emitted
completion and synthetic process callbacks, so no sleep, socket, real model binary or external service
is needed. A synthetic GGUF exists only to satisfy the original filesystem eligibility check; its bytes
and exact changed-file allowlist are independently checked on fixture teardown.

There is deliberately **no shutdown/kill injection or arbitrary PID entry**: destruction still invokes
its private ProcessExecutor shutdown. The overload is trusted C++ composition only, not settings/QML/IPC
input. Callback adapters must remain on the owning Qt thread, outlive the helper and not throw; health
replies must finish asynchronously or be explicitly completed after connection, as QNetworkAccessManager
normally guarantees. The fixture keeps adapters/replies alive until runtime destruction. Production
controller signal wiring is retained; manually mutated fixture observations do not certify that wiring
or native process behavior. Loaded identity can be supplied in synthetic health content but is currently
ignored by the production helper; no fake identity verifier was added.

## Deterministic findings and boundaries

| ID | Witness in `test_local_model_runtime` | Result / classification |
| --- | --- | --- |
| D1 | `health200DoesNotVerifyAlias` returns 200 with a foreign alias. | Reproduced helper readiness defect; expected failure of identity-aware contract. |
| D2 | `emptySelectionChoosesFirstGguf` provides one synthetic eligible file and no selected model. | Reproduced implicit selection; one exact synthetic start with first alias. Contract assertion XFAIL; no actual process started. |
| D3 | `failedProbeRetainsReady` shared HTTP 503 and owned-child connection-refused rows after ready. | Reproduced stale readiness in both branches; two XFAIL assertions. |
| D4 | `providerSwitchRetainsOwnedChild` changes to Ollama after synthetic owned start. | Reproduced retention: zero termination calls because provider guard returns early. Cleanup policy remains pending; observation is confirmed. |
| D5a | `oldEndpointCanPublishReady` completes old pending health after endpoint switch. | Reproduced missing endpoint-generation fence; old result publishes ready. |
| D5b | `endpointSwitchDoesNotReconcileOwnedChild` changes endpoint with the same model. | Reproduced old owned-child retention; new health probes different port but no reconciliation. |
| D6 | `removalDuringActualUseRemainsBlocked` | Explicit SKIP. Source inspection shows no lease consultation in removal paths. Actual in-use deletion is not reproduced: no general residency lease/admission interface exists. A retained provider pointer is not an adequate substitute. |

Positive regressions cover busy vs idle model switch, exact owned handle termination, Exited/Failed/Cancelled
callback observation and same-key restart suppression, duplicate-probe suppression, shared listener protection,
late health after shutdown, disallowed endpoints, superseded provider/model results and unchanged artifacts.
Expected failures are executable witnesses of unresolved contracts, not fixes or acceptance passes; unexpected
passes fail the suite so future remediation must deliberately update the witness.

P3-01 source findings D1–D5 are now reproduced through the actual helper with synthetic IO. Native
server identity, real startup/exit timing, residency release and D6 remain source-only/unverified.
No new production defect was repaired in this task.

## PD-D10 maintainer decision packet — unapproved

| Alternative | Ownership / termination | Lease, unload and recovery consequences |
| --- | --- | --- |
| A: cooperative owned-child management (recommended default) | Explicit owned start handle only; never adopt external listeners. Existing process authority stays limited to owned children. | Count active resource users, refuse new destructive operations or drain according to explicit policy; retain dependencies while stopping. Explicit retry after failure; no finite hard-stop promise. |
| B: supervised isolated provider worker (future optional design) | Separate reviewed worker boundary may permit bounded termination of that owned worker only. Shared backend remains external. | Requires protocol, lease transfer, crash recovery, evidence and security review before implementation; worker termination does not prove remote compute release. |
| C: external-server administration / adoption | Would add shared shutdown/residency authority beyond current contracts. | Not recommended by default. Requires separate explicit administrative consent, ownership verification and multi-client policy; name, port or API compatibility is never sufficient. |

Recommended default: A, with refusal as the initial destructive-operation behavior when any active lease
exists; draining must explicitly close admission and retain dependencies until zero users. Provider-object
leases and model/process/artifact leases must be named separately. Reject shared-server termination;
never force destruction of an active in-process worker to satisfy a timeout. On owned child failure,
mark readiness invalid, record identity/error, and require explicit bounded retry rather than restart loops.
This recommendation defines no arbitrary timeout/resource threshold and implements none of these missing policies.

Required approval: named Models/IPC/security owner plus maintainer adopt exact ownership identity,
termination/escalation rules, all-consumer leases, refusal/drain semantics, switch reconciliation,
retry/recovery and shared-server protection. PD-D05 legacy disposition and PD-D04 calibrated shutdown
budgets remain separate dependencies for AC-P5-04. Neither register nor acceptance threshold was edited.

## Validation and intermittent-test investigation

Host: macOS 27.0.1 arm64, AppleClang 21.0.0, Qt 6.12.0. Existing isolated
`build/tests` preset; no dependency installation or model execution.

| Check | Actual result |
| --- | --- |
| `cmake --preset tests -DSENTINEL_USE_CCACHE=OFF` | Passed. |
| `CCACHE_DISABLE=1 cmake --build --preset tests -j6` | Complete default-target build passed; final formatted source rebuilt successfully. |
| Focused lifecycle/process/provider CTest | **8/8 targets passed**, 7.68 s on final fixture. Includes P3-01, new helper, ProcessExecutor, local inference, Ollama runtime, runtime provider, provider contracts and model registry. |
| New helper QtTest | **19 passed, 0 failed, 1 skipped**, including init/cleanup; 17 executed substantive rows plus one skipped row. **Seven XFAIL assertions** within passing witness rows preserve unresolved contracts. |
| Full parallel CTest `-j6` | **132/132 targets passed**, 34.16 s. This preceded the final test-only braces cleanup; production seam was final. |
| Final full sequential CTest, offscreen | **132/132 targets passed**, 137.30 s, after final source build. |
| Inherited `nativeDiagnostics:content-usage` probe | **20/20 separate subprocess attempts passed**, maximum four concurrent, 20 s timeout per attempt, no timeout. |
| Formatting / scoped analysis | `clang-format --dry-run --Werror` passed. PCH-free compile database and clang-tidy `clang-analyzer-*`, `bugprone-*`, `readability-braces-around-statements` with warnings-as-errors yielded no user-code diagnostics on helper/test; 977 non-user warnings suppressed. Initial added braces warnings were corrected; no global analysis certification. |
| Generated contracts | Both `python3 tools/ipc/generate.py --check` and `generate_desktop.py --check` passed. Full CTest also passed generated-contract targets. |
| Preservation / whitespace / documentation | P3-01 test/report SHA-256 unchanged; original CMake bytes retained as exact prefix. `git diff --check`, report relative links, balanced fences and trailing whitespace checks passed. |
| Changed-file privacy / credential patterns | All five P3-02 files passed personal-path and obvious private-key/token pattern checks. This limited scan is not a full credential-security certification. |
| Repository personal-path scan | **Failed: one inherited file**, `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md`. Its content was not exposed or rewritten; inherited P3-01 evidence was preserved. |

Initial P3-02 focused run was **7/8 targets passed**: new helper had 17 Qt passes,
one XPASS failure and one skip. The stale-health fixture used connection refusal without
an owned process; the actual algorithm legitimately attempted a start and changed status.
The fixture was corrected to independently cover shared HTTP 503 and an existing owned
child's connection refusal. This was a fixture setup correction; the stale-health production
branch was not changed. Historical P3-01 failures and skips remain in the original report.
An earlier sequential run during development passed 132/132 in 142.09 s; the final identified
source run above is the candidate evidence.

The P3-01 parallel `nativeDiagnostics(content-usage)` failure was investigated separately:
the complete parallel regression and the twenty bounded synthetic probes did not reproduce it.
Its root cause remains **unresolved**, not fixed or reclassified as environmental. The old
fixture replies in a readyRead handler without complete request-frame buffering; that is a
source-inspected hypothesis requiring failing transport traces, not an established explanation.
No unrelated provider/test timeout logic was modified and no retries replaced a failed attempt.
Native Linux/Windows, actual backend startup/model identity/residency and upstream compute
release remain unverified. CTest target success does not erase the new required skip/XFAILs.

Raw local logs are retained under `/tmp/sentinel-p302-*`, outside public source. These include
focused initial/final outcomes, complete sequential/parallel output, static-analysis logs and
all twenty diagnostic attempts with `results.json`. Public report contains no raw machine-home paths.
One final validation tool request encountered an automatic approval-review timeout; its permitted
single retry succeeded. It did not block any requested validation.

### Exact P3-02 files and source identity

- [LocalModelRuntime.h](../../apps/sentinel-daemon/service/LocalModelRuntime.h): adapter contract.
- [LocalModelRuntime.cpp](../../apps/sentinel-daemon/service/LocalModelRuntime.cpp): default adapters and existing reconciliation.
- [tests/CMakeLists.txt](../../tests/CMakeLists.txt): append helper target, retain P3-01 registration.
- [test_local_model_runtime.cpp](../../tests/desktop/test_local_model_runtime.cpp): new deterministic witnesses.
- This new report.

Inherited uncommitted P3-01 files remain in Git status but were not edited:
`tests/core/test_model_lifecycle_ownership.cpp` and `docs/audits/P3_01_MODEL_LIFECYCLE_OWNERSHIP_RESULT.md`.
No P2-06/P1-01.7 or other-worktree files were written.

SHA-256 of final executable inputs (report has no self-referential digest):
- `apps/sentinel-daemon/service/LocalModelRuntime.h`: `1c87fb67ebe8140e99d7a1cec75acb35c02ee143a4f8d73724766cb2fb016662`.
- `apps/sentinel-daemon/service/LocalModelRuntime.cpp`: `5f689d20189f7bf985a67085c89245c343dc8ace7fea1eb7bc6beec7068e81ba`.
- `tests/CMakeLists.txt`: `319ac0acad8821f96af7a944b876d31c0bbcfe82bd65695163e4d3986348ded0`.
- `tests/desktop/test_local_model_runtime.cpp`: `959e98f99abd710d3927fec404b354d114720bb761be23674c82ba0c8618de00`.

## Implementation dependencies and next action

1. Review/adopt PD-D10 ownership, leases and drain/refusal policy with designated owners.
2. Separately scoped identity/readiness fix: explicit selected alias, captured endpoint revision and honest
   failed-health invalidation, using these defect witnesses; do not silently change them in this contract task.
3. Add explicit idempotent load/unload/loading-cancel projection within current services and generated contracts.
4. Add all-consumer resource leases and remove/unload fences; then reproduce D6 through actual admission/use.
5. Reconcile provider/endpoint changes and owned process exit/retry safely; verify default controller wiring and
   real ProcessExecutor integration with disposable synthetic processes before authorized native backend tests.

**AC-P3-02 remains Blocked:** PD-D10, selected identity, unload/lease/recovery implementation and native
ENV-HEADLESS + BE-LOCAL evidence are incomplete. **AC-P5-04 remains Blocked:** no new hard-stop authority,
calibrated cooperative shutdown or native ownership acceptance is established. Fixture success and XFAIL
witnesses do not close either criterion. P2-06 and P1-01.7 are independent and untouched.
