# P3-01 Model lifecycle and ownership baseline

Audit date: 2026-10-16 (Europe/Istanbul). Fetched `origin/main` and candidate HEAD:
`619e989c419956393201a749da1cede386e83ba3`. Branch: `test/p3-01-lifecycle-ownership`.
Managed worktree: `p3-01-lifecycle-ownership/Sentinel-sanitized` under the Codex worktree root.
The initial isolated worktree was clean. Personal-Brain was unavailable (`brain: command not found`);
the supplied repository and Git remote were verified directly. No models were downloaded,
no actual user processes were terminated, and no commit, push, merge, reset or stash was performed.
No P1/P2/integration worktree was modified. This task does not depend on P2-06 or P1-01.7.

## Architecture and ownership boundaries

Read root AGENTS.md, architecture, Agent Runtime, security, building/testing documentation,
P0.1 architecture assessment, P0.2 reconciliation, documentation consistency report,
P0.03 ownership review, acceptance matrix and P3 platform roadmap requirements.
Their historical evidence does not certify this candidate.

`ModelService` owns selection, provider factories, catalog/capability and health observations.
It resolves through the existing `IModelRouter`, returns a captured `ModelBinding` and
shared provider handle, and owns no model process. Multiple resolutions can construct
independent providers. A provider `shared_ptr` lease protects C++ object lifetime;
it does not reserve server model memory or prevent process termination.
`ModelLifecycleService` is static model status/pricing metadata, not runtime lifecycle management.

`DaemonService` creates `LocalModelRuntime` and stops its timer at shutdown start;
after draining, destruction tears down the helper before its controller.
The legacy C++ CLI has separate in-process composition, not ownership of the daemon helper.
`LocalModelRuntime` uses a private `ProcessExecutor` and only its returned opaque process IDs.
It probes loopback HTTP llama.cpp endpoints, finds an existing GGUF and `llama-server`,
and launches a fixed argument list (context 8192, parallel 1, no shell planning).
The activation uses explicit unconfined permission. This audit does not approve a new sandbox policy.
`ProcessExecutor` tracks only its own starts, rejects unknown handles, escalates owned termination
and reaps its owned QProcess objects on shutdown. It is not a process-name/port killer.

`OllamaRuntime` health/discovery and `LocalInference` Ollama/LM Studio clients perform HTTP
operations; compatible llama.cpp and cloud providers use the existing endpoint adapters.
Protocol compatibility, configured credentials and successful discovery confer no process ownership.
`ModelOperationService` manages explicit acquisition/import/registration/removal and recovery records;
Ollama pull/delete are explicit API mutations, distinct from model load/unload and server shutdown.
LM Studio external-manager opening is not process adoption. Managed artifact deletion uses
`ModelStorageManager` ownership/sidecar checks; imported files and server resources remain distinct.

## Lifecycle ownership matrix

| Category | Authority actually present | Startup/load and unload | Active use and cleanup |
| --- | --- | --- | --- |
| Sentinel managed llama.cpp child | Daemon helper owns a private ProcessExecutor handle; not arbitrary listeners | Implicit selected GGUF startup after refused health connection; idle model change terminates old child; no public explicit load/unload contract | Model-change termination checks Agent loop and Chat busy flags; no counted model lease; owned exit clears handle, shutdown kills/reaps owned children |
| Shared local Ollama | User/provider owns server; Sentinel has explicit request/discovery and pull/delete API paths | No server startup/shutdown or explicit residency unload management | Request cancellation is cooperative client cancellation; explicit delete is a mutation of shared server inventory, not proof of exclusive ownership |
| Shared LM Studio | External application owns process/model | Discovery/inference and external manager link; no model residency control | Client abort/disconnection only; no process adoption or termination |
| Shared llama-server / compatible local endpoint | External listener remains external | Existing HTTP listener suppresses competing startup; HTTP 200 is currently accepted as ready | No process ID adopted from port/name; no external shutdown; selected alias is not verified by health |
| Remote endpoint | Remote provider owns compute; Sentinel owns request/client handle and credentials configuration | Catalog/auth/request state only; no local process or remote model residency authority | Client cancellation/transport abort does not establish remote compute termination |

## Capability and deterministic coverage matrix

| Requested case | Current implementation | Baseline evidence / limitation |
| --- | --- | --- |
| 1 Managed vs shared ownership | Private executor handles vs external HTTP clients | New unknown-handle rejection test; source trace of daemon helper. No end-to-end daemon process mock seam |
| 2 Loading/unloading transitions | Starting status, health-ready status, exit status; model-switch termination | Source only for helper. No typed explicit load/unload state machine/API |
| 3 Duplicate load | `probing_`, existing process ID and `attemptedKey_` suppress starts | Source only; duplicate selection test verifies idempotent selection, **not duplicate load** |
| 4 Concurrent active leases | Shared provider handles and frozen bindings | New independent binding lifetime test; no model-memory/process lease counter |
| 5 Inference during unload | Busy checks only on selected model change | No unload API; removal has no inference lease guard. No substitute test presented as unload coverage |
| 6 External disconnection | HTTP classification and health observations | New data-driven observations for four local provider IDs; existing socket fixtures exercise real client disconnect/cancel |
| 7 Failed startup | ProcessExecutor Failed state; helper clears handle and reports failure | Existing `test_process_executor::failedStart`; daemon callback remains source-inspected |
| 8 Cancellation during loading | No daemon load cancellation API/token | Missing; acquisition cancellation and request cancellation are different capabilities |
| 9 Owned process exit cleanup | Executor finish/terminal tracking; helper clears ID and refreshes discovery | Existing executor output/exit and shutdown fixtures; helper integration unverified |
| 10 Unauthorized shared shutdown | Unknown executor handles refused; no external PID adoption | New terminate/kill unknown-handle test starts no process; source inspection of HTTP adapters |
| 11 Honest readiness | Unknown/unverified status before observation; sequenced health updates | New local/remote snapshot rows and late-observation rejection; helper health identity defect remains |
| 12 User-selected settings | ModelService restores/persists selection; observation does not change it | New in-memory settings fixtures preserve current and alternate provider choices |

New target `test_model_lifecycle_ownership` contains five test functions (11 data-expanded
cases, excluding Qt init/cleanup). It performs no transport requests, real process starts,
credential-store access or model operations. It uses the existing in-memory settings and credential stores.
Observation tests intentionally record current `Degraded` classification on connection loss,
not an invented disconnected enum. Independent provider handles surviving registry destruction
are object lifetime evidence only; the test does not invoke providers after borrowed settings die.

## Confirmed production defects and missing behaviors

The following defects are confirmed by source control flow, not live backend reproduction:

1. **Ready without selected identity:** `LocalModelRuntime::ensureRunning` accepts any `/health`
   HTTP 200 and sets ready before discovering/verifying the selected alias. This violates the
   identity part of AC-P3-02. Health/catalog/loaded identity remain separate facts.
2. **Implicit model choice:** an empty selection matches the first existing GGUF, allowing
   startup without an explicit selected identity. Acquisition consent is not tested or changed here.
3. **Provider-switch retention:** the early return for a non-llama provider precedes owned-child
   cleanup. Switching away leaves the child resident until later eligible reconciliation/shutdown.
4. **Endpoint-change reconciliation:** owned process tracking compares selected model only;
   endpoint changes can leave an old child running while probing another endpoint. Probe completion
   checks provider/model but not a captured endpoint revision, allowing stale endpoint results.
5. **No general in-use deletion fence:** ModelOperationService removal paths do not consult active
   inference leases; artifact ownership protection does not prove safe removal during use.
6. **Readiness can remain stale:** failed health probes of an existing child/listener return without
   clearing a previous ready status. Unavailability must be projected separately from old success.

Missing capabilities: explicit typed load/unload/cancel operations, counted process/model leases,
selected loaded identity verification, documented unload drain/refuse policy, bounded restart/backoff
with explicit retry, complete endpoint/provider switch cleanup and injectable daemon lifecycle fixtures.
`attemptedKey_` suppresses repeated same-key attempts after failure/exit until selection/library changes;
this avoids uncontrolled restart but is not an accepted retry/recovery policy. Agent/Chat busy checks
are not protection for every possible speech/embedding/other consumer. No arbitrary thresholds added.
Health is also classified differently by tracked chat replies and explicit observation reporting;
this baseline records existing behavior without changing P2 contracts.

## Validation

Host/toolchain: macOS 27.0.1 arm64, AppleClang 21.0.0, Qt 6.12.0,
CMake 4.4.4, clang-format 23.1.3. Commands use the isolated `build/tests` tree.

- `cmake --preset tests -DSENTINEL_USE_CCACHE=OFF`: passed.
- `CCACHE_DISABLE=1 cmake --build --preset tests -j6`: complete default-target build passed;
  corrected-source incremental full build also passed. No production source changes.
- Focused CTest regex covering lifecycle ownership, ProcessExecutor, local inference,
  Ollama runtime, runtime provider, provider contracts and model registry: **7/7 passed**, 7.19 s.
- First focused run: existing targets 6/6 passed, new target failed three compatible-provider rows.
  Fixture incorrectly assumed catalog acceptance also updated health; corrected to report both
  observations as the interfaces require. This was a test setup defect, not a production fix.
- First full parallel CTest (`-j6`): **129/131 passed**, 45.17 s. It used the pre-correction fixture
  and also failed existing `test_llm_agent_runtime::nativeDiagnostics(content-usage)` at line 352:
  actual `reply.success=false`, expected true. This result is retained; no unrelated code changed.
- `clang-format --dry-run --Werror` on new C++ and `git diff --check`: passed.
- Scoped `clang-tidy` using `tools/analysis/compile_database.py` without production PCH:
  passed on final new test source, no user-code findings (538 non-user warnings suppressed).
  First attempt ran before Qt moc generation and failed on missing generated include;
  generated build followed by rerun resolved it. Two braces warnings were fixed in the test.
- `python3 tools/ipc/generate.py --check`, `generate_desktop.py --check` and branding check: passed.
- Repository privacy check: **failed**, one inherited file
  `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md`; confirmed present in HEAD.
  Changed-file scan using the same pattern: **3/3 passed**. Existing audit left untouched.

Final full sequential CTest (`QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure`):
**131/131 passed**, 138.22 s. The existing nativeDiagnostics case passed sequentially;
the parallel-run failure remains an unresolved intermittent regression observation, not a confirmed
production defect or a proven timing diagnosis. No test timeouts or unrelated source were changed.
New target: **13 QtTest passes including init/cleanup**, 11 substantive data-expanded cases.

Source identity (SHA-256):
- New test: `cb3e016845b43b04a22fcb3717fcff3ab436f94383d8cbc01f1fe6f06a670792`.
- Test registration: `0424c9cff334ed2b190efdc2bce686925c6bc46ee668dccc09800f1cb4b58aed`.

Exact changed files: this report, `tests/core/test_model_lifecycle_ownership.cpp`,
and `tests/CMakeLists.txt`. Build/test logs are private temporary evidence under
`/tmp/sentinel-p3-01-*`; no raw machine-specific command output is embedded in public documentation. Full fixture success cannot establish native
Fedora/KDE, Windows, actual model residency, server identity, remote compute termination or release acceptance.
No live model/native acceptance was attempted.

## Decisions, acceptance and recommended sequence

PD-D10 process ownership/hard-stop remains pending. PD-D02 acquisition/consent, PD-D03 backend/model
matrix and PD-D04 calibrated budgets also remain open where applicable. This report adopts none of them.
P0 ownership review recommendations are not approved product policy.

| Acceptance | Status after this baseline |
| --- | --- |
| AC-P3-01 acquisition integrity/consent | Remains Blocked; not a lifecycle-load test result |
| AC-P3-02 runtime ownership/lifecycle | Remains Blocked; partial deterministic evidence, identity/unload/lease gaps and PD-D10 outstanding |
| AC-P3-03 format/storage | Remains Unverified for product acceptance; existing storage fixtures are regression evidence only |
| AC-P2-02 provider readiness | Existing contract regression only; no P2 closure or contract edits |
| AC-P5-04 shutdown authority | Remains Blocked; no native ownership/hard-stop acceptance |

Recommended next task: **P3-02 — define the bounded ownership and lifecycle contract under PD-D10,
with an injectable LocalModelRuntime test seam**. Obtain the required decision record before implementing
new authority. Then proceed in this order: selected alias/endpoint revision readiness; explicit idempotent
load/unload/cancel states; counted leases and unload/remove fences; owned exit/retry/switch recovery;
acquisition/storage fault coverage; authorized disposable backend/native acceptance. Reuse current
ModelService, helper, ProcessExecutor and AgentRuntime binding boundaries; introduce no new router,
process manager, AgentRuntime redesign or Rust IPC change.

**P3 readiness: baseline investigation only; not P3 completion.** Production gaps and pending decisions
prevent full lifecycle acceptance. The added tests establish available interface behavior, not absent capabilities.
