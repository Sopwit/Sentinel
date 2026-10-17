# P3-04 — Model Lifecycle Reliability & Ownership Gap Closure

Audit date: 2026-10-17 (Europe/Istanbul).
Worktree: `test/p3-04-lifecycle-reliability` at `/Users/emir/.codex/worktrees/p3-04-lifecycle-reliability/Sentinel-sanitized`.
Base commit / verified HEAD: `3a98bab5f040b5e46f429ba423f1d1c1a023d3e1`.
Repository: `https://github.com/sentinel-projects/sentinel`.

No commit, push, merge, reset, stash, PR creation or CI dispatch was executed.
No models were downloaded or unapproved process-ownership / hard-stop policies implemented.

---

## 1. Executive Summary & Objective

This audit investigates and addresses remaining model lifecycle reliability gaps identified in P3-01, P3-02, and P3-03, focusing specifically on:

- **D4:** Switching providers while an owned model process remains active.
- **D5b:** Changing endpoints without reconciling the existing owned process.
- **D6:** Missing protection against deleting a model currently in use.

Prior P3-03 readiness and identity remediations remain intact. No unapproved process-ownership or hard-stop policies were introduced. Where resolving gaps requires maintainer policy adoption under [PD-D10](../roadmaps/ACCEPTANCE_DECISIONS.md#pd-d10--provider-ownership-and-hard-stop-policy), deterministic failing witnesses are preserved and the exact required decisions documented.

---

## 2. Current Source Identity

| Property | Value |
|---|---|
| Worktree branch | `test/p3-04-lifecycle-reliability` |
| HEAD Commit | `3a98bab5f040b5e46f429ba423f1d1c1a023d3e1` |
| Remote Tracking | `origin/main` |
| Working Tree Status | 3 modified files + 1 new audit report |
| Host Environment | macOS 27.0.1 arm64 (Darwin 25.1.0), AppleClang 21.0.0, Qt 6.12.0, CMake 4.4.4 |

### Executable Input Checksums (SHA-256)

- `apps/sentinel-daemon/service/LocalModelRuntime.cpp`: `7e2d118c4fae4623cf460acfb6345bcc302a9d70fb0c6e32504cc3c9a1fdd577`
- `tests/core/test_model_lifecycle_ownership.cpp`: `4b15b2e8d8ed2793679167bd782c9b738d76eb6f05999918027165cccdced76e`
- `tests/desktop/test_local_model_runtime.cpp`: `8ebe127d512f65030209abf71dfc284f4d94d7eb36aa21c7b74b8b94fe229375`

---

## 3. Investigation Audit & Lifecycle Safety Analysis

### 1. Provider switching with active model processes (D4)
- **Current Flow:** In [LocalModelRuntime.cpp](../../apps/sentinel-daemon/service/LocalModelRuntime.cpp#L234), `ensureRunning()` begins with:
  ```cpp
  const auto selected = dependencies_.selection();
  if (selected.provider != "llama-cpp-server") {
      return;
  }
  ```
  If Sentinel launched an owned `llama-server` child (`!processId_.isEmpty()`), switching `selectedRuntimeProvider` to another provider (e.g., `ollama` or `lm-studio`) causes an immediate early return before inspecting `processId_`.
- **Finding:** The owned child process remains resident in the background until daemon destruction.
- **Why Blind Termination Is Prohibited:** If an active Agent run or Chat request is in flight using `llama-cpp-server`, terminating the process immediately would drop active inference connections. Furthermore, terminating a process merely because global UI selection changed violates cooperative lifetime principles without an approved lease/drain policy.
- **Witness Status:** Preserved as deterministic `XFAIL` witness in `LocalModelRuntimeTest::providerSwitchRetainsOwnedChild`.

### 2. Endpoint switching while a request is running (D5b)
- **Current Flow:** When `endpoint` changes while `model` remains unchanged and `provider == "llama-cpp-server"`:
  ```cpp
  if (!processId_.isEmpty() && modelId_ != selection && !selected.busy) {
      dependencies_.terminate(processId_); ...
  }
  ```
  Because `modelId_ == selection`, `LocalModelRuntime` does not terminate `processId_`. The probe on the new endpoint fails or connects to a different port, while the owned child remains orphaned on the old port.
- **In-Flight Stability:** In-flight Agent runs capture an immutable `ModelBinding` with their own provider instance; changing settings endpoints does not corrupt active runs. However, the physical process on the old port remains un-reconciled.
- **Witness Status:** Preserved as deterministic `XFAIL` witness in `LocalModelRuntimeTest::endpointSwitchDoesNotReconcileOwnedChild`.

### 3. Stale process ownership
- `ProcessExecutor` strictly maintains an internal table of processes spawned via its own `start()` API.
- Unknown handles, arbitrary PID adoption, port-sniffing, or external process killing are completely rejected (`terminate()`, `kill()`, and `record()` return false/default).
- Verified in `ModelLifecycleOwnershipTest::unknownProcessCannotBeTerminated` and `repeatedShutdownAndProcessOwnershipInvariants`.

### 4. Model deletion during active use (D6)
- **Current Flow:** In [ModelOperationService.cpp](../../core/src/model/ModelOperationService.cpp), three deletion paths exist:
  1. `RemoveRegistration`: removes GGUF library entry via `ModelLibraryService::removeLocalGgufRegistration`.
  2. `RemoveManagedArtifact`: deletes file and `.source.json` sidecar via `ModelStorageManager::removeArtifact`.
  3. `Remove`: dispatches HTTP `DELETE /api/delete` to Ollama.
- **Findings:**
  - `ModelOperationService` does not consult active Agent runs, Chat sessions, or `ModelService` bindings before initiating deletion.
  - While an active Agent run's in-memory `ModelBinding` and `std::shared_ptr<IChatProvider>` remain valid in memory, the underlying disk artifact is deleted beneath it without lease checks.
  - On Windows, deleting a file locked by an active process fails with a sharing violation; on Unix, it unlinks the file while open.
- **Witness Status:** Preserved `QSKIP` in `LocalModelRuntimeTest::removalDuringActualUseRemainsBlocked` and verified deterministic active-run in-memory survival alongside un-fenced file deletion in `ModelLifecycleOwnershipTest::modelDeletionDoesNotInvalidateActiveRunBinding`.

### 5. Concurrent model load requests
- `LocalModelRuntime::ensureRunning()` uses `probing_` latching and `attemptedKey_` to reject concurrent or duplicate probing while a probe is active.
- Added `duplicateEnsureRunningWhileProbingDropsExtraRequests` to verify that concurrent calls do not generate duplicate network probes.

### 6. Delayed readiness responses
- Monotonically increasing `selectionRevision_` and `probeReply_` pointer fencing guarantee that out-of-order, delayed, or ABA endpoint responses cannot overwrite current status or report false readiness.
- Validated across existing data-driven test cases (`oldRepliesCannotOverwriteCurrentObservation`, `sameSelectionOldRequestCannotResurrectReady`).

### 7. Failed model startup
- If `dependencies_.start()` produces a process whose record state is `Failed` (e.g., exec failure, bad binary, permission denied), `readiness_` is now immediately updated to `Unavailable` (not left as `Unknown`), `processId_` is cleared, and `attemptedKey_` prevents infinite restart loops.
- Verified in `synchronousFailedStartSetsUnavailable`.

### 8. Repeated shutdown requests
- Both `LocalModelRuntime::beginShutdown()` and `ProcessExecutor::shutdown()` are idempotent and safe to invoke repeatedly.
- Verified in `LocalModelRuntimeTest::repeatedShutdownIsIdempotent` and `ModelLifecycleOwnershipTest::repeatedShutdownAndProcessOwnershipInvariants`.

### 9. Agent cancellation and model binding stability
- Active Agent runs resolve their model binding via `ModelService::resolve()`, capturing an immutable `ModelBinding` and refcounted `std::shared_ptr<IChatProvider>`.
- Request cancellation sets the cooperative cancellation token and aborts transport without destroying the binding or provider instance.
- Global selection changes in `ModelService` do not mutate captured bindings. Verified in `ModelLifecycleOwnershipTest::activeAgentRunRetainsModelBindingAcrossSelectionChange`.

### 10. Shared versus managed server boundaries
- Shared runtimes (user-started Ollama, LM Studio, external llama-server) are treated strictly as external services.
- Sentinel never terminates or adopts external processes when selections change or during shutdown.

---

## 4. Ownership Invariants & Contract Matrix

| Invariant | Implementation Mechanism | Enforcement & Evidence |
|---|---|---|
| **Active Run Binding Stability** | `ModelBinding` is an immutable captured value; `IChatProvider` is held via `std::shared_ptr`. | `activeAgentRunRetainsModelBindingAcrossSelectionChange`: global selection changes do not affect captured run binding. |
| **Shared Process Protection** | `ProcessExecutor` rejects unknown handles; no PID adoption from ports/names. | `unknownProcessCannotBeTerminated` and `repeatedShutdownAndProcessOwnershipInvariants`. |
| **Stale Callback Fencing** | `selectionRevision_` + active `QPointer<QNetworkReply>` guard. | Probes on superseded endpoints or old requests cannot publish Ready or dispatch follow-up probes. |
| **Honest Failed Startup** | Immediate check of `record.state == Failed` sets `readiness_ = Unavailable` and clears handle. | `synchronousFailedStartSetsUnavailable` asserts `Unavailable` state and suppression of retry loops. |
| **Model Deletion Run Safety** | In-memory provider and binding survive artifact deletion. | `modelDeletionDoesNotInvalidateActiveRunBinding`: run binding remains valid after `removeArtifact()`. |
| **No Background Process Bypass** | LocalModelRuntime only terminates owned `processId_` handles via private `ProcessExecutor`. | Zero termination calls for non-owned entities; destructor shutdown is bounded. |

---

## 5. Exact Source and Test Changes

### Production Changes
- [`apps/sentinel-daemon/service/LocalModelRuntime.cpp`](../../apps/sentinel-daemon/service/LocalModelRuntime.cpp):
  In `ensureRunning()` process launch callback handler, if `dependencies_.record(processId_).state == core::ProcessState::Failed`, immediately set `readiness_ = core::RuntimeReadinessState::Unavailable` with `"llama.cpp process failed to start."` and clear `processId_`.

### Test Fixture Extensions
- [`tests/core/test_model_lifecycle_ownership.cpp`](../../tests/core/test_model_lifecycle_ownership.cpp):
  - Added `#include "sentinel/core/model/ModelStorageManager.h"` and filesystem/crypto helpers.
  - Added `activeAgentRunRetainsModelBindingAcrossSelectionChange()`: validates that active Agent run bindings are immutable across provider, model, and endpoint changes.
  - Added `modelDeletionDoesNotInvalidateActiveRunBinding()`: validates that active run bindings survive artifact deletion, while witnessing the absence of in-use lease fences in `ModelStorageManager`.
  - Added `repeatedShutdownAndProcessOwnershipInvariants()`: validates idempotency of repeated `ProcessExecutor::shutdown()` and rejection of unknown process handles.
- [`tests/desktop/test_local_model_runtime.cpp`](../../tests/desktop/test_local_model_runtime.cpp):
  - In `Fixture::Dependencies.start`, preserved `ProcessState::Failed` if set by test before start.
  - Added `synchronousFailedStartSetsUnavailable()`: verifies immediate `Unavailable` readiness upon failed startup and retry loop suppression.
  - Added `repeatedShutdownIsIdempotent()`: verifies calling `beginShutdown()` repeatedly is safe and idempotent.
  - Added `duplicateEnsureRunningWhileProbingDropsExtraRequests()`: verifies concurrent `ensureRunning()` calls during probing drop duplicate requests.

---

## 6. Deterministic Witness Status & XFAIL Reconciliation

| Finding | Test Location | Status | Rationale |
|---|---|---|---|
| **D4** | `LocalModelRuntimeTest::providerSwitchRetainsOwnedChild` | **XFAIL** (Preserved) | Terminating owned child on provider switch requires PD-D10 active lease counting and refusal/drain policy. Terminating unconditionally would break in-flight Agent runs. |
| **D5b** | `LocalModelRuntimeTest::endpointSwitchDoesNotReconcileOwnedChild` | **XFAIL** (Preserved) | Reconciling owned process across same-model endpoint changes requires PD-D10 drain policy to ensure old child is not stopped while serving in-flight requests. |
| **D6** | `LocalModelRuntimeTest::removalDuringActualUseRemainsBlocked` | **SKIP** (Preserved) | No general residency-lease or admission interface exists across Sentinel to refuse or defer artifact deletion during use. |
| **D6 Witness** | `ModelLifecycleOwnershipTest::modelDeletionDoesNotInvalidateActiveRunBinding` | **PASS** | Validates that while active run's in-memory binding is safe (Part D), `ModelStorageManager::removeArtifact` executes without checking active leases (D6 gap). |

---

## 7. Unresolved PD-D10 Decisions

The following decisions under [PD-D10](../roadmaps/ACCEPTANCE_DECISIONS.md#pd-d10--provider-ownership-and-hard-stop-policy) remain **Pending Decision** and require formal maintainer adoption before implementing destructive lifecycle transitions:

1. **Resource Lease Accounting:** A unified lease counter/registry across all consumers (Agent Mode runs, Chat Mode streaming, background inspection) to determine whether a physical model process or artifact is currently "In Use".
2. **Refusal vs. Drain Policy:** When an owned model is in use and a switch or deletion is requested, define whether the operation is immediately refused (`OperationRejected: ModelInUse`) or placed in a graceful drain queue until active leases reach zero.
3. **Owned Child Lifecycle on Provider Switch:** Whether an idle owned child process is terminated immediately upon provider switch, kept resident for a configurable idle timeout, or preserved until daemon shutdown.
4. **Shared Server Administration Boundary:** Maintain the policy that shared external servers (Ollama, LM Studio, external llama-server) are strictly external and Sentinel shall never attempt process termination or residency unload against them.

---

## 8. Full Validation Results

| Check | Command / Target | Outcome | Details |
|---|---|---|---|
| **CMake Configure** | `cmake --preset tests -DSENTINEL_USE_CCACHE=OFF` | **Passed** | AppleClang 21.0.0, Qt 6.12.0, build files written cleanly. |
| **Full CMake Build** | `cmake --build --preset tests -j8` | **Passed** | 1525/1525 targets built without errors. |
| **Focused Lifecycle CTest** | `ctest --preset tests -R "test_local_model_runtime\|test_model_lifecycle_ownership"` | **Passed** | 2/2 targets passed (1.09 s). |
| **test_model_lifecycle_ownership** | `./build/tests/tests/test_model_lifecycle_ownership -v2` | **Passed** | **16 passed, 0 failed, 0 skipped** (including 3 new substantive tests). |
| **test_local_model_runtime** | `./build/tests/tests/test_local_model_runtime -v2` | **Passed** | **52 passed, 0 failed, 1 skipped** (2 preserved XFAILs: D4, D5b; 1 preserved SKIP: D6; 3 new passed tests). |
| **Complete CTest Suite** | `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | **Passed** | **139/139 passed** (240.34 s real time, 100% pass rate). |
| **Rust Crates Check** | `cargo check --manifest-path cli/Cargo.toml` | **Passed** | All 4 workspace crates (`sentinel-cli`, `sentinel-ipc`, `sentinel-mascot`, `sentinel-tui`) checked successfully. |
| **Formatting** | `clang-format --dry-run --Werror <modified_files>` | **Passed** | Zero formatting violations across all 3 modified files. |
| **Static Analysis** | `clang-tidy -p build/tests/analysis_compile_commands.json -checks="clang-analyzer-*,bugprone-*,readability-braces-around-statements" --warnings-as-errors="*"` | **Passed** | Zero user-code diagnostics; 1657 non-user warnings suppressed. |
| **IPC Contract Checks** | `python3 tools/ipc/generate.py --check && python3 tools/ipc/generate_desktop.py --check` | **Passed** | IPC and desktop contracts verified in sync. |
| **Diff Hygiene** | `git diff --check` | **Passed** | Zero whitespace or conflict artifacts. |
| **Privacy Scan** | Scanned modified files and diff for personal paths / private credentials | **Passed** | Zero matches in working tree changes. One historical inherited finding in `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` noted. |

---

## 9. Next Recommended P3 Implementation Task

**Recommended Next Task: P3-05 — Adopt PD-D10 Resource Lease and Drain Contract.**

1. Obtain maintainer / role owner approval on the PD-D10 decision packet:
   - Establish `IResourceLeaseRegistry` or equivalent reference-counting interface.
   - Formalize the refusal policy for destructive operations (`removeArtifact`, `removeLocalGgufRegistration`, provider/endpoint switch) while active leases exist.
2. Implement active-use fences in `ModelOperationService` to resolve D6 with deterministic test coverage.
3. Implement idle owned-child reconciliation in `LocalModelRuntime` for provider and endpoint switches (resolving D4 and D5b).
4. Authorize native headless backend execution tests (`ENV-HEADLESS + BE-LOCAL`) to validate real process termination and startup against declared runtime profiles.
