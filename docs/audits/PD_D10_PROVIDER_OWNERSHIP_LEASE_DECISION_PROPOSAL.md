# PD-D10 — Provider Ownership, Resource Lease and Shutdown Decision Proposal

Date: 2026-10-17. Phase: Architecture decision (blocks P3, P4, P5, P11). Repository:
`https://github.com/sentinel-projects/sentinel`.

**Status: PROPOSED — not Approved. PD-D10 remains *Pending Decision*.** This document is a
read-only audit and a maintainer decision packet. It does **not** adopt an ownership policy,
does **not** implement a lease, drain or termination rule, and does **not** edit
`ACCEPTANCE_DECISIONS.md`, `ACCEPTANCE_MATRIX.md` or any production source, test, generated
contract or workflow. Every "should"/"must" below is a technical recommendation awaiting an
explicit maintainer disposition.

---

## 0. Isolation, baseline and method

| Item | Value |
| --- | --- |
| Worktree | `architecture/pd-d10-resource-ownership` |
| Worktree path | `<opencode-worktree-root>/d272e3/architecture/pd-d10-resource-ownership` |
| Base commit | `8e8d6b3f` (latest verified `main` tip in this workspace) |
| `origin/main` | `6035bf74` (`feat(resources): add native resource telemetry monitor adapters and sampling tests`) |
| Base divergence | `origin/main` is **2 commits behind** the local `main` tip: `fc4c39ca fix(daemon): set local model runtime readiness to unavailable on process failure` (the P3-04 production fix) and `8e8d6b3f docs(architecture): record P2-M01 provider modularization architecture and migration plan`. |
| Base-selection rationale | The task mandates reading **P3-01 … P3-04**. The P3-04 production fix is not yet on `origin/main`, so the worktree was based on the main tip that contains it. The two extra commits are immaterial to the decision (one readiness fix, one document). |
| Inspection only | All other worktrees (INT, CI, P4-02/03, P2-M01/M02, P3-04) were inspected **read-only**. |
| Actions not performed | No commit, push, merge, reset, stash, PR, workflow dispatch, model download, live provider call or process termination. |

Read for this proposal: [AGENTS.md](../../AGENTS.md); [architecture](../development/ARCHITECTURE.md);
[Agent Runtime](../concepts/AGENT_RUNTIME.md); [security model](../security/SECURITY_MODEL.md);
[daemon development](../development/DAEMON.md);
[ACCEPTANCE_DECISIONS PD-D10](../roadmaps/ACCEPTANCE_DECISIONS.md#pd-d10--provider-ownership-and-hard-stop-policy);
[ACCEPTANCE_MATRIX](../roadmaps/ACCEPTANCE_MATRIX.md); and the P3-01, P3-02, P3-03, P3-04,
P4-01, P4-02, P4-03, P5-01, P5-02 and P5-02.1 reports, plus P0-03 and P2-M01. P3-04 and the
P4-03 policy foundation are not yet committed to `origin/main` and are treated as current
must-read context, not as adopted policy.

Source anchors inspected: `ModelService`, `LocalModelRuntime`, `ProcessExecutor`,
`ModelOperationService`, `ModelStorageManager`, `ModelLibraryService`, `ModelBinding`,
`IChatProvider`, `ApplicationController`, `DaemonService`, `DaemonIpcServer`,
`ResourceMonitor` (P4-02) and `ResourcePolicyEngine` (P4-03, uncommitted in the primary
checkout).

Non-claims: this proposal makes no native Linux/Windows acceptance claim, adopts no numeric
limit (all thresholds stay Pending Calibration under PD-D04), and establishes no hard-stop
guarantee. No shared server, external listener or PID was adopted, killed, unloaded or deleted.

---

## Executive summary (plain language)

Sentinel currently owns **exactly one kind of model resource**: a `llama-server` child process
that the daemon's `LocalModelRuntime` started itself, identified only by an opaque handle from
its private `ProcessExecutor`. Everything else — a user-started Ollama or LM Studio server, a
user-started `llama-server` listener, and any cloud endpoint — is **externally owned**.
Reaching an endpoint over HTTP proves nothing about who owns the server. Today Sentinel already
refuses to terminate any handle it did not start; what it lacks is a way to know **whether a
resource it does own is currently in use**, and a policy for what to do when a switch or a
delete would collide with an active task.

The recommended direction (Alt 1, §7) is:

1. **Two separate lease classes** — a **process lease** over an owned OS process, and a
   **model-artifact lease** over a model file/registration. They are different resources and
   must never be merged into a single "model lock".
2. **Leases are acquired only when a real request/run is admitted**, never merely because a
   model is selected in the UI, and are released exactly once at the run's terminal state.
3. **Refusal-first destructive operations**: an unload, a provider/endpoint switch or an
   artifact delete that would collide with an active lease is refused (or, only if the
   maintainer chooses, queued to drain) — never satisfied by killing an active dependency.
4. **Shutdown stays cooperative by default.** New-request refusal and a bounded drain apply to
   *owned* children; externally owned servers are never terminated. A *hard-stop* guarantee for
   non-cooperative in-process providers is **not** promised and would require the separate
   supervised-worker alternative (Alt 2).
5. **Leases grant no security authority.** They are admission/refusal facts only; the
   `ToolExecutionGateway` remains the only authorization boundary, and the P4 policy action
   vocabulary stays non-destructive.

This resolves **D4** (provider-switch retention), **D5b** (endpoint-switch reconciliation) and
**D6** (delete-during-use) without interrupting an active authorized task: the resource is
retained until its leases reach zero, then reconciled; a delete in use is refused. It does not
close AC-P3-02, AC-P5-04 or any acceptance row — those remain blocked until a maintainer
records the decisions in §8 and the implementation in §10 lands.

---

## 1. Current ownership map (Part A)

### 1.1 Authority inventory

| Component | What it can actually do | Ownership authority |
| --- | --- | --- |
| `ModelService` (`core/include/sentinel/core/model/ModelService.h`) | Register/construct providers, hold the single current `ModelSelection`, resolve a `ModelBinding` + `std::shared_ptr<IChatProvider>`, observe health/catalog/capability. | **Selection and construction only.** The `shared_ptr` lease protects a C++ object's lifetime; it reserves no server memory, holds no process and prevents no termination. |
| `LocalModelRuntime` (`apps/sentinel-daemon/service/LocalModelRuntime.h/.cpp`) | Owns a **private** `ProcessExecutor`; probes loopback `/health` and `/props` identity; launches a **fixed** `llama-server` command for exactly the selected GGUF after a refused health connection; terminates its **own** child on model change (when not busy) and at shutdown/destruction. | Start/terminate **its own child only**, plus readiness projection. Never adopts an external listener; no PID/port/name matching. |
| `ProcessExecutor` (`core/include/sentinel/core/runtime/ProcessExecutor.h`) | `start`/`write`/`terminate`/`kill`/`record`/`shutdown` for **opaque handles it created**. Rejects unknown handles (`terminate`/`kill`/`record` return false/empty). `terminate` is graceful, `kill` is immediate; `shutdown` reaps its own children (SIGKILL to the process group for an enforced sandbox, else `QProcess::kill`). | **Owned starts only.** Explicitly not a process-name/port killer. |
| `ModelOperationService` (`core/include/sentinel/core/model/ModelOperationService.h`) | Queued acquisition/mutation: `Pull`/`Remove` (Ollama `/api/pull`, `/api/delete`), `Import`, `RemoveRegistration`, `RemoveManagedArtifact`, `Download`/`DownloadAndRegister`, `Refresh`, `OpenExternalManager`, `RevealLocalFile`; recovery journal. | Mutates **provider inventory** and **managed artifacts**. Remove ≠ unload. **No in-use lease consultation** (`ModelOperationService.cpp` removal paths call `storageManager_.removeArtifact` / `library_.removeLocalGgufRegistration` / Ollama `DELETE` directly). |
| `ModelStorageManager` (`core/src/model/ModelStorageManager.cpp`) | `canStore` gate; `removeArtifact` deletes only a path proven owned (inside a known root **and** backed by a valid `.source.json` sidecar); `removeOrphanedSidecar`. | Deletes **owned artifacts only**. No lease awareness. |
| `AgentRuntime` + `ModelBinding` (`core/include/sentinel/core/model/ModelRouting.h`) | Each Chat request, Agent run or controlled task captures an **immutable** `ModelBinding` (providerId/modelId/contextWindow/capabilities) and a refcounted provider handle. Cancellation is cooperative (token + transport abort). | Run lifetime and request cancellation. A `ModelBinding` is a **value**, not a counted physical lease. |
| Provider interfaces (`IChatProvider`, `OllamaChatProvider`, `SelectedEndpointChatProvider`, `OllamaRuntime`, `LocalInference`) | HTTP request/discovery/streaming/cancel against an endpoint. | **No process authority.** Protocol compatibility, credentials and HTTP 200 confer no ownership. |
| `DaemonService` (`apps/sentinel-daemon/service/DaemonService.cpp`) | Composition root; creates/destroys `LocalModelRuntime`; on `shutdownStarted` stops the health timer and calls `LocalModelRuntime::beginShutdown()`; destructor order is `stop health → stopChatGeneration → agentRuntime()->shutdown() → detach controller → reset LocalModelRuntime → reset controller/settings → stopServer`. | Lifecycle ordering. Cooperative; may wait on non-cooperative code. |
| `DaemonIpcServer` (`apps/sentinel-daemon/service/DaemonIpcServer.cpp`) | Endpoint lock, same-UID peer auth, one typed request dispatcher; cooperative `requestShutdown()` → non-blocking `tryShutdownRuntime()` polled at 25 ms → `shutdownDrained` → `quit()`. | Cooperative drain only; no forced thread destruction; rejects new commands with `daemon-shutting-down`. |
| `ApplicationController` (`core/include/sentinel/core/app/ApplicationController.h`) | `tryShutdownRuntime()` (Running → Stopping → Stopped, false while borrowed work is active); `agentLoopActive()`; `chatModeService()->busy()`; `pollOllama()` discovery; `refreshModelDiscovery()`. | Execution/drain host. `busy` currently means **agent-loop or chat only**. |
| Resource monitoring / policy | P4-02 `ResourceMonitor`: opt-in sampler over one injected adapter, bounded history, freshness/staleness. P4-03 `ResourcePolicyEngine` (uncommitted): pure evaluator, sealed `ResourceOwnershipPosture::PreserveAllResidency`, action vocabulary `{None, RefuseNewAdmission, RestrictBackgroundAdmission, LimitToolConcurrency}`. | Measurement and **admission-only** recommendations. Structurally unable to unload or terminate. This is the intended integration seam for PD-D10. |

### 1.2 Authority boundary statement

- **Owned** = a process Sentinel started, addressed only by the opaque `ProcessExecutor` handle.
- **External/shared** = anything Sentinel reached over an endpoint (Ollama, LM Studio, a
  user-started `llama-server`, cloud). Sentinel owns the *request*, never the *server*.
- **Artifact** = a model file/registration. Ownership is proven by sidecar + known root, and is
  **independent** of any process or endpoint.

### 1.3 Confirmed gaps (from P3-01…P3-04, re-confirmed here)

| ID | Gap | Evidence |
| --- | --- | --- |
| G1 | No counted in-use lease across all consumers (Agent, Chat, speech/embedding, background). | P3-02 §Observable state contract; `busy` = agent-loop ∨ chat busy only. |
| G2 | No explicit typed load/unload/cancel-loading API. | P3-01/P3-02; startup loads implicitly; no unload projection. |
| G3 | **D4** — switching `selectedRuntimeProvider` away from `llama-cpp-server` returns before inspecting `processId_`; the owned child stays resident until destruction. | `LocalModelRuntime::ensureRunning` early return; XFAIL `providerSwitchRetainsOwnedChild`. |
| G4 | **D5b** — a same-model endpoint change does not reconcile the old owned child (termination is keyed on model change only). | `ensureRunning` termination guard uses `modelId_ != selection`; XFAIL `endpointSwitchDoesNotReconcileOwnedChild`. |
| G5 | **D6** — removal has no in-use fence; artifact ownership is necessary but not sufficient. | Removal paths do not consult leases; SKIP `removalDuringActualUseRemainsBlocked`. |
| G6 | `attemptedKey_` suppresses same-key retries but is not an accepted recovery/retry policy. | P3-01/P3-03. |
| G7 | No residency-unload acknowledgement for shared servers; "no owned handle" ≠ "model unloaded". | P3-02 — `Unloaded` state. |
| G8 | No finite hard-stop for a non-cooperative **in-process** provider. | `DAEMON.md` §Shutdown drain and provider lifetime. |
| G9 | No separation between a process lease and an artifact lease. | P3-03/P3-04 D6 analysis. |
| G10 | Orphan reconciliation after an unexpected daemon restart is unspecified (owned child may survive the parent). | Not implemented; see §3.6. |

---

## 2. D4 / D5b / D6 dependency analysis

| Case | Deterministic witness (P3-02 → P3-04) | Root cause | What it needs from PD-D10 | ≥1 safe resolution that does **not** interrupt active work |
| --- | --- | --- | --- | --- |
| **D4** provider switch retains owned child | `LocalModelRuntime::providerSwitchRetainsOwnedChild` — **XFAIL** | Early `return` for non-`llama-cpp-server` precedes any owned-child reconciliation. | Whether an idle owned child may be stopped on provider switch, and how "in use" is decided. | Count process leases for the owned child. On switch, **close admission for the old provider and retain** the child while leases > 0; when leases reach 0, reconcile (terminate or keep-warm per M3). No in-flight run is disturbed. |
| **D5b** endpoint switch does not reconcile owned child | `LocalModelRuntime::endpointSwitchDoesNotReconcileOwnedChild` — **XFAIL** | Termination guard keys on model only; endpoint is not part of the reconciled identity. | Whether endpoint is part of resource identity and when the old child may be stopped. | Treat `(provider, endpoint, model, endpoint-generation)` as the process-lease identity. Endpoint change opens a new generation; the old generation's child is retained while leased, then reconciled. Health/identity fencing already exists (P3-03 revision + active-reply fence). |
| **D6** delete during active use | `LocalModelRuntime::removalDuringActualUseRemainsBlocked` — **SKIP**; positive witness `modelDeletionDoesNotInvalidateActiveRunBinding` — PASS (in-memory binding survives, file is deleted un-fenced) | No residency/artifact lease and no admission interface in removal paths. | Whether removal is **refused** or **deferred** while in use, and which consumers must hold a lease. | Acquire an **artifact lease** at run admission. `removeArtifact`/`removeLocalGgufRegistration`/Ollama `DELETE` first check the artifact-lease count: refuse (`OperationRejected`/ModelInUse) by default, or enqueue to drain if M8 chooses deferral. |

Cross-cutting observations:

- **Frozen bindings already protect active runs.** A switch changes *new* selections; it does not
  rewrite a run's captured `ModelBinding` or drop its `shared_ptr<IChatProvider>`
  (`activeAgentRunRetainsModelBindingAcrossSelectionChange`). The missing guarantee is physical:
  keeping the *server process* and *artifact* alive, which is exactly what leases provide.
- **Cancellation is not release.** A cancelled request does not prove server residency or remote
  compute was released; the lease releases the *local* ownership claim only.
- **D5a/D1/D2/D3 are already remediated** (P3-03). D4/D5b remain the two XFAILs and D6 the one
  SKIP at the P3-04 baseline; none may be silently flipped to pass.

---

## 3. Resource lease alternatives and a minimal proposed contract (Part C)

### 3.1 Candidate structures

| Option | Shape | Notes |
| --- | --- | --- |
| **L0 — status quo** | No lease; rely on `busy` flags and ownership checks. | D4/D5b/D6 stay open; AC-P3-02 cannot close. Simplest; no new failure modes. |
| **L1 — single global "model lock"** | One boolean/count per selected model. | Conflates process and artifact; cannot express "artifact in use by a run whose process already exited"; rejected. |
| **L2 — two lease classes (recommended)** | `process lease` (owned OS process identity) **and** `artifact lease` (model file/registration identity), reference-counted, with owner/session identity and generation fencing. | Minimal, explicit, maps 1:1 onto existing owned handles and storage ownership. |
| **L3 — full resource manager** | Central residency manager owning admission, keep-warm, eviction, worker supervision. | Larger; overlaps P4 policy and risks a second authority. Defer; would be needed only for Alt 2. |

### 3.2 Proposed minimal lease contract (L2) — design only, not implemented

- **Resource identity**
  - *Process lease* identity: `(resourceClass = OwnedProcess, providerId, endpointGeneration,
    modelId, instanceHandle)`, where `instanceHandle` is the opaque `ProcessExecutor` id.
    `endpointGeneration` is a monotonically increasing integer bumped on endpoint/provider
    change (mirrors the P3-03 probe revision).
  - *Artifact lease* identity: `(resourceClass = ModelArtifact, providerId, modelId,
    artifactKey)` where `artifactKey` is the managed-digest/path or the registration id.
  - A read-only *residency observation* may be recorded for a shared server
    (`SharedExternal`, providerId, endpoint, modelId) but carries **no** termination authority.
- **Lease owner and session identity**: `{ownerKind, ownerId, generation}` where `ownerKind ∈
  {AgentRun, ChatRequest, InternalTask}` and `ownerId` is the runId/sessionId/requestId already
  used by the runtime and the IPC projection store. `generation` is the daemon instance
  generation (the existing `server_generation` UUID concept).
- **Acquisition**: only at **admission** of a real request/run, after `ModelService::resolve`
  succeeds. Selecting a model in the UI does **not** acquire a lease. Acquisition for an owned
  process is paired with the existing owned-handle start; for a shared server it is a
  residency observation.
- **Reference counting**: one monotonic count per resource identity; `acquire` returns a
  handle/token; `release` is idempotent and exactly once. Counts never go negative; a release of
  an unknown/expired token is a no-op and is recorded for diagnostics.
- **Release**: on the run's **single authoritative terminal state** (accepted final answer,
  `run.cancelled`, `run.failed`). Cancellation and failure release the lease; they do not imply
  server residency release.
- **Crash recovery**: all leases belong to a daemon generation. On restart, leases of a prior
  generation are considered **dead** and are not recreated; interrupted runs are already
  reconciled to `failed`/`daemon-restarted` in the session projection store (P5-02).
- **Stale-lease handling**: a lease is stale only when its owner generation is dead or when an
  explicit, configured max-age *and* liveness check both fail. Liveness must be proven by the
  owning runtime, never assumed from a timeout alone. A live owner's lease is never reclaimed.
- **Active-request fencing**: admission checks the resource state; while a resource is
  *draining* (M2 = drain), **new** admissions are refused and existing leases are allowed to
  finish. Cancellation keeps absolute priority and always releases its own lease.
- **Model artifact deletion**: a delete acquires a short-lived *delete intent*; if the artifact
  lease count is non-zero it is refused (or deferred per M8); on success it marks the artifact
  removed and rejects future acquisition for that identity.
- **Shared vs managed**: shared servers may only receive residency observations; managed owned
  children may receive process leases. A shared server can never be terminated by a lease.
- **Concurrent consumers**: the count is the number of distinct owners; multiple runs share one
  resource safely. Reconciliation (stop owned child / allow delete) happens only at count 0.

### 3.3 How D4 / D5b / D6 resolve without interrupting active authorized tasks

- **D4**: on provider switch, bump `endpointGeneration`, close new admission for the old
  generation, keep the owned child while its process lease > 0; at 0, terminate (or keep-warm
  per M3). No active run loses its provider.
- **D5b**: endpoint is part of the identity; the old child is retained while leased and
  reconciled at 0 leases. Stale health cannot publish (already fenced).
- **D6**: the artifact lease count gates deletion; an in-use artifact is never deleted under a
  live run.

### 3.4 Process leases vs artifact leases (distinction)

| Aspect | Process lease | Artifact lease |
| --- | --- | --- |
| Guards | An **owned OS process** (start/stop/keep-warm). | A **file/registration** (delete/import/overwrite). |
| Applies to shared servers? | Never (owned only). | N/A (artifacts are local Sentinel state). |
| Released by | Owned-child exit/termination or run terminal. | Run terminal. |
| Failure if ignored | Kills a server under a live run (loss of work). | Deletes a model under a live run (Unix unlink-while-open; Windows sharing violation). |
| Needed for | D4, D5b, keep-warm. | D6. |

Conflating these is the central design error to avoid: a run can hold an artifact lease with no
live owned process (shared server, or a crashed child), and an owned process can outlive its
running requests (idle keep-warm).

---

## 4. Shutdown authority alternatives (Part D)

| Strategy | Current status | Safety | Bound? | Applies to |
| --- | --- | --- | --- | --- |
| **Cooperative drain** | Implemented (P5-01/P5-02) | Highest; truthful `Stopping`; no false exit. | **No** for non-cooperative in-process code. | Daemon runtime (`tryShutdownRuntime`). |
| **New-request refusal** | Partially implemented (`daemon-shutting-down`, `Stopping` rejects new sessions). | Safe. | N/A. | IPC + runtime. |
| **Bounded graceful shutdown** | Not adopted (needs PD-D04 deadline). | Safe if it never destroys active dependencies. | Yes, for **owned** children; not for in-process non-cooperative code. | Owned child processes only. |
| **Explicit user-authorized termination** | Not implemented. | Safe when authored and recorded; still owned-only. | Yes. | Owned child processes. |
| **Forced termination of owned children** | Exists at the `ProcessExecutor` level (`terminate`→grace→`kill`; `shutdown` SIGKILLs the process group of an enforced sandbox). | Acceptable **only** for Sentinel-started children, after leases drain or with explicit authorization. | Yes. | Owned children only. |
| **Never terminating externally owned processes** | Already enforced (unknown handles rejected; no adoption). | Highest. | N/A. | Ollama, LM Studio, external `llama-server`, cloud. |

Recommended posture (proposed, not applied): cooperative drain by default; refuse new requests at
shutdown; the only permissible escalation is graceful-then-forced termination of an **owned**
child after its leases drain (or after an explicit user authorization), bounded by a
PD-D04-calibrated deadline. Externally owned servers are never terminated. Do **not** select a
destructive default automatically.

**Conditions that require explicit maintainer approval before implementation:**

1. Terminating an owned child while any process lease is non-zero.
2. Adopting a shutdown/drain deadline value (couples to PD-D04).
3. Any SIGKILL/process-group escalation policy and its trigger.
4. Any form of shared-server administration/adoption (not recommended).
5. Whether an idle owned child is terminated on provider/endpoint switch, or kept warm (M3), and
   any keep-warm window.
6. Whether a non-cooperative **in-process** provider may ever be hard-stopped (requires Alt 2).

---

## 5. Shared-provider safety model (Part E, part 1)

| Scenario | Permitted Sentinel action | Forbidden action | Enforcement today |
| --- | --- | --- | --- |
| User-started **Ollama** | Request, discovery, explicit `/api/pull`, explicit `/api/delete` (inventory mutation, gated by the artifact lease once adopted). | Kill/stop the server; force residency unload; adopt its PID/port. | `ProcessExecutor` unknown-handle rejection; no PID adoption; `LocalModelRuntime` never terminates external listeners. |
| User-started **LM Studio** | Request, discovery, open external manager. | Stop the app; unload its model. | No process handle exists for it. |
| User-started **`llama-server` / OpenAI-compatible local** | Request; existing listener suppresses a competing managed start. | Adopt/terminate the listener; treat HTTP 200 as ownership. | `sharedListenerAndLateShutdownNeverStartOrAdopt`. |
| **External cloud endpoint** | Request, cancel local client, hold credentials config. | Claim remote compute release; kill anything. | No local process. |
| **Sentinel-owned `llama-server` child** | Start (fixed command), readiness, keep-warm, terminate/kill **its own** handle. | Terminate any handle it did not start. | Opaque-handle table; `unknownProcessCannotBeTerminated`. |

Invariant to ratify explicitly (M1): **control over an API endpoint never grants ownership of the
server process**; Sentinel administers only processes it started, addressed by an opaque handle.

---

## 6. Decision matrix (Part F, part 1)

| Criterion | Alt 1 Cooperative owned-child + leases (recommended) | Alt 2 Supervised isolated worker | Alt 3 External administration/adoption | Alt 4 Status quo (no lease) |
| --- | --- | --- | --- | --- |
| Shared-provider safety | Strong (external never touched) | Strong | **Weak** (would add shared shutdown authority) | Strong (but unpoliced) |
| Active-task preservation | Strong (refuse/drain, no kill) | Strong | Risk of disrupting other tenants | No guarantee |
| Resolves D4/D5b/D6 | Yes | Yes | Yes (unsafely broad) | No |
| Complexity | Medium (one lease module, wiring in 3–4 seams) | High (worker process, protocol, lease transfer, crash recovery) | High and risky | Very low |
| Cross-platform | Lease is platform-neutral; owned-child stop already has Unix/Windows paths | Isolation is the hard platform-dependent part (job objects/containers/sandbox) | High platform variance | N/A |
| Hard-stop guarantee | **No** (truthful `Stopping` retained) | **Yes** for the owned worker only | Not applicable | No |
| Fit with P4 policy | Direct (`PreserveAllResidency` → ownership-driven admission) | Good but needs a new worker boundary | Conflicts with `PreserveAllResidency` | No |
| Fits AC-P3-02 / AC-P5-04 | Partial path to closure | Would satisfy the non-cooperative branch | Wrong boundary | Blocks both |

---

## 7. Recommended policy (Part F, part 2, and Deliverable §7)

**Recommended: Alt 1 — cooperative owned-child management with two lease classes and
refusal-first destructive operations.** Status: **Proposed, not Approved.**

Reasoning:

1. It matches the code that already exists (owned opaque handles, sidecar-proven artifacts) and
   adds the *smallest* missing concept — a count — instead of a new authority.
2. It preserves the two existing hard invariants (never touch external servers; truthful
   `Stopping`) while making D4/D5b/D6 decidable.
3. It keeps the P4 policy non-destructive (`PreserveAllResidency`) and keeps the
   `ToolExecutionGateway` as the only authorization boundary.
4. Alt 2 is the only path to an actual hard-stop guarantee; it is materially larger and should be
   an explicitly funded follow-on, not a default. Alt 3 is rejected: it dilutes the ownership
   boundary and risks other tenants' work. Alt 4 is rejected because it leaves AC-P3-02/P5-04
   permanently blocked.

Recommended default parameters (all awaiting ratification):

- Destructive-operation default = **refuse** while leased (M2); drain is an opt-in mode.
- Provider/endpoint switch = retain the owned child while leased; at zero leases, **terminate**
  (M3), unless keep-warm is chosen.
- Shutdown = cooperative drain; owned-child escalation only after drain and only under an
  explicit policy (M4).
- Lease kill-switch: the whole behavior ships **disabled** (presence-only observations) until the
  decisions in §8 are recorded, so no destructive default is ever active by accident.

---

## 8. Exact maintainer approval questions (Deliverable §8)

These cannot be inferred from source code. Each requires a named accountable human (PD-D11) and a
dated record. Until answered, PD-D10 stays *Pending Decision*.

| ID | Question | Options | Engineering recommendation |
| --- | --- | --- | --- |
| **M1** | Ratify the ownership identity model (owned opaque handle vs external/shared) and the rule "endpoint control ≠ process ownership"? | Ratify / amend | Ratify |
| **M2** | When a destructive op collides with an active lease, refuse or drain? | Refuse-first / Drain-first / Refuse for delete + drain for switch | Refuse-first |
| **M3** | Disposition of an **idle owned child** on provider/endpoint switch. | Terminate immediately / Keep-warm window / Keep until shutdown | Terminate at zero leases (keep-warm is a separate, calibrated feature) |
| **M4** | Owned-child shutdown escalation and any deadline. | Graceful-only / Graceful→forced with a PD-D04 deadline | Graceful→forced for owned children only, after drain, with a calibrated deadline |
| **M5** | Pursue Alt 2 (supervised isolated worker) to enable a real hard-stop guarantee? | Pursue now / Defer / Reject | Defer (fund separately); do not promise hard-stop before it exists |
| **M6** | Which consumers must acquire leases? | Agent+Chat only / Add ControlledTask / Add speech+embedding+future | All consumers that resolve a binding for a real request |
| **M7** | Orphan reconciliation after an unexpected daemon restart: may a new daemon generation reap a previously-owned child it can identify? | Yes via persisted handle/pidfile / No (leave orphan) | Yes, narrowly — owned-only, persisted identity, never by port/name |
| **M8** | Delete-during-use behavior when M2 = refuse. | Refuse (`ModelInUse`) / Offer a queued deferred delete | Refuse; a deferred delete is a separate feature |
| **M9** | Whether "no hard-stop claim" is acceptable for the first accepted scope (AC-P5-04 non-cooperative branch stays blocked). | Accept truthful `Stopping` / Block release on Alt 2 | Accept truthful `Stopping`; keep AC-P5-04's non-cooperative branch blocked |
| **M10** | Named accountable role(s) and reviewer independence. | Assign / defer | Models/IPC/security owner + maintainer, recorded with date |

---

## 9. Migration and testing plan (Deliverable §9)

Ordered so each step is independently reviewable and reversible. **Nothing here is authorized
until §8 is answered.**

1. **Contract only (no behavior).** Add a pure lease contract (identity, acquire/release, counts,
   generation) at the `sentinel_core` boundary. No consumer wired; behavior is byte-identical.
2. **Admission wiring.** Acquire on run/request admission; release on the single authoritative
   terminal state. Keep the lease disabled by default until M2/M6 are ratified.
3. **Artifact fence (D6).** Gate `ModelStorageManager::removeArtifact`,
   `ModelLibraryService::removeLocalGgufRegistration` and Ollama `DELETE` on the artifact count.
   Flip the SKIP `removalDuringActualUseRemainsBlocked` into a normal assertion and keep a
   positive refusal witness.
4. **Owned-child reconciliation (D4/D5b).** Use the process count plus endpoint generation to
   retain-then-reconcile. Flip both XFAILs to normal assertions, preserving the old behavior as a
   deliberate negative witness.
5. **Shutdown integration.** Extend `beginShutdown`/drain to release/refuse leases; add owned-child
   graceful→forced escalation **only** if M4 authorizes it.
6. **Status projection.** Publish lease counts/reasons through existing status surfaces; add no
   IPC command or protocol change without explicit approval.

Required tests (deterministic fixtures; no real model, no external server):

- Lease contract unit tests: acquire/release idempotency, ref count, unknown-token no-op,
  generation fencing, crash/stale reclamation, no-negative-count.
- `LocalModelRuntime` injected-IO fixtures: D4 and D5b become normal negative→positive cases;
  D6 SKIP becomes a refusal case; existing owned-handle/foreign-terminate guards preserved.
- Ownership regression: `unknownProcessCannotBeTerminated`,
  `sharedListenerAndLateShutdownNeverStartOrAdopt`, `modelSwitchRespectsBusyAndOwnedHandle`,
  `activeAgentRunRetainsModelBindingAcrossSelectionChange`.
- Daemon IPC: refusal during drain, no cancellation of unrelated runs, restart releases prior
  generations (extends P5-02 restart reconciliation).
- Policy invariance: `resourcePolicyDecisionPreservesSecurity()` stays pinned across every
  profile × tier; replay gateway/approval/sandbox denial fixtures at each tier (AC-P4-03).
- Cross-platform: the lease logic is platform-neutral; the owned-child stop path is exercised on
  Unix now and must compile/pass on the shared Windows build. Native Linux/Windows runs remain
  gated by PD-D03.

Migration requirements: no data migration (leases are in-memory, generation-scoped); no settings
migration; no generated-contract change unless a status field is added under a separate approval.
Rollback: remove the wiring and leave the disabled contract; the two XFAILs and the SKIP return
to their exact current text (preserved from P3-02/P3-03/P3-04).

---

## 10. Implementation task breakdown after approval (Deliverable §10)

Proposed follow-on tasks (each needs separate authorization; none is opened here):

| Task | Scope | Depends on | Exit evidence |
| --- | --- | --- | --- |
| PD-D10-R1 | Lease contract types + unit tests (disabled). | M1, M2, M6 | Contract build + hermetic lease tests |
| PD-D10-R2 | Admission wiring in Agent/Chat (+ M6 consumers). | R1 | Acquire/release fixtures; no behavior change when disabled |
| PD-D10-R3 | Artifact fence in `ModelOperationService`/`ModelLibrary`/Ollama delete. | R2, M8 | D6 SKIP → refusal test; ownership tests unchanged |
| PD-D10-R4 | Owned-child reconciliation on provider/endpoint switch. | R2, M3 | D4/D5b XFAIL → pass; negatives preserved |
| PD-D10-R5 | Shutdown drain + optional owned-child escalation. | R4, M4, PD-D04 | Drain/refusal fixtures; truthful `Stopping` retained |
| PD-D10-R6 | Status projection (no protocol change). | R2 | Read-only status; `generate.py --check` unchanged |
| PD-D10-R7 | Native headless owned-child terminator validation (ENV-HEADLESS + BE-LOCAL). | R4, PD-D03 | Disposable synthetic child lifecycle evidence |
| PD-D10-R8 (optional) | Supervised isolated worker (Alt 2) for a real hard-stop. | M5 | Worker protocol, lease transfer, crash recovery, security review |

Explicitly out of scope until decided: shared-server administration, any forced kill of external
processes, any second scheduler/authority, and any claim of a bounded hard-stop for in-process
non-cooperative providers.

---

## 11. Appendix — references and source anchors

- Decisions: [PD-D10](../roadmaps/ACCEPTANCE_DECISIONS.md#pd-d10--provider-ownership-and-hard-stop-policy),
  [PD-D04, PD-D05, PD-D11](../roadmaps/ACCEPTANCE_DECISIONS.md), [matrix](../roadmaps/ACCEPTANCE_MATRIX.md)
  (AC-P1-03, AC-P3-02, AC-P5-04, AC-P11-01 are the direct PD-D10 edges).
- Lifecycle: [P3-01](P3_01_MODEL_LIFECYCLE_OWNERSHIP_RESULT.md),
  [P3-02](P3_02_LIFECYCLE_OWNERSHIP_CONTRACT_RESULT.md),
  [P3-03](P3_03_LIFECYCLE_READINESS_REMEDIATION_RESULT.md),
  P3-04 (`P3_04_MODEL_LIFECYCLE_RELIABILITY_RESULT.md`, branch `test/p3-04-lifecycle-reliability`).
- Resources: [P4-01](P4_01_RESOURCE_MANAGEMENT_BASELINE_RESULT.md),
  [P4-02](P4_02_NATIVE_RESOURCE_TELEMETRY_RESULT.md), and the P4-03 design foundation
  (`docs/audits/P4_03_ADAPTIVE_RESOURCE_POLICY_DESIGN.md`, an uncommitted P4-03 work product in
  the primary checkout and therefore not linked from this worktree).
- Daemon/IPC: [P5-01](P5_01_DAEMON_IPC_BASELINE_RESULT.md),
  [P5-02](P5_02_DAEMON_IPC_RELIABILITY_RESULT.md), [P5-02.1](P5_02_1_IPC_SECURITY_PERSISTENCE_REVIEW.md).
- Ownership review: [P0-03](P0_03_OWNERSHIP_COMPATIBILITY_REVIEW.md); provider architecture:
  [P2-M01](P2_M01_PROVIDER_MODULARIZATION_ARCHITECTURE.md); routing: [ADR-0003](../decisions/ADR-0003-provider-model-routing.md).
- Source: `apps/sentinel-daemon/service/{LocalModelRuntime,DaemonService,DaemonIpcServer}.cpp`,
  `core/src/runtime/ProcessExecutor.cpp`, `core/src/model/{ModelService,ModelOperationService,ModelStorageManager,ModelLibrary}.cpp`,
  `core/include/sentinel/core/{model,policy,resource,runtime}/*.h`, `docs/development/DAEMON.md`.

**This document is a proposal. PD-D10 remains Pending Decision; no ownership, lease, drain or
termination rule is adopted or implemented, and no acceptance criterion is changed.**
