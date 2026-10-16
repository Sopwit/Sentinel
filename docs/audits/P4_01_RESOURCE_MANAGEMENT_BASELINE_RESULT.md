# P4-01 Adaptive Resource Management Baseline Result

Task: P4-01 — Adaptive Resource Management Baseline (Phase P4, `EP-P4`).
Result type: read-only audit, deterministic baseline tests and measured idle baseline.
**This task does not implement P4 and does not accept any P4 row.** No ownership policy,
no sampler service, no profile budgets and no process authority were introduced.

## 0. Isolation, environment and method

| Item | Value |
| --- | --- |
| Worktree | managed worktree `p4-01-resource-baseline/Sentinel-sanitized` under the Codex worktree root |
| Branch | `test/p4-01-resource-baseline` |
| HEAD | `70e9fa072af9b4139d86f518c6d89f9c849c29b2` (equal to `origin/main` at task start) |
| Base | `origin/main` = `70e9fa07 Merge branch 'test/p3-01-lifecycle-ownership'` |
| Fixture host | Apple MacBook Air `Mac14,2`, Apple M2, 8 logical / 8 physical cores, 16 GiB RAM, macOS 27.0.1 (Darwin 26A434, arm64) |
| Toolchain | CMake 4.4.4 + Ninja, Apple clang 21.0.0, Homebrew Qt 6.12.0, ccache |
| Measurement build | `release` preset (`CMAKE_BUILD_TYPE=Release`), target `sentinel-daemon` only |
| Test build | `tests` preset (Debug) |
| Power/thermal state | AC power, battery 100% charged, `pmset -g therm`: no thermal/performance warning recorded |
| Background load | macOS load average 2–12 during the session (opencode/ghostty/WindowServer); per-window load averages and system idle % recorded with every measurement case |
| Personal-Brain | unavailable (`brain: command not found`); repository and remote verified directly |

Rules observed: no commit, push, merge, reset or stash; no model download; no live cloud
request; no change to INT-04/P2/P3 worktrees; no production contract was added for a
capability that does not exist (missing contracts are documented as gaps in §4/§6/§8).

Validation commands are listed in §7 and §10.

## 1. Existing resource architecture

Sentinel has **static hardware facts, storage preflight and one provider-side VRAM
reading**. It has **no continuous resource monitor, no memory-pressure signal, no power
or thermal telemetry and no resource-policy engine**. This matches the current
[architecture](../development/ARCHITECTURE.md) statement ("a continuous CPU/RAM/GPU
pressure-driven adaptive resource policy is not implemented") and the
[P0-1 audit](P0_1_CURRENT_STATE_AUDIT.md) and
[P0-2 reconciliation](P0_2_ROADMAP_RECONCILIATION.md).

### 1.1 Static hardware facts

`HardwareCapabilityService` (`core/include/sentinel/core/model/HardwareCapabilityService.h`,
`core/src/model/HardwareCapabilityService.cpp`) is the single existing telemetry seam:

* `IHardwarePlatformAdapter::collect()` is the only injection point; the default
  `NativeHardwareAdapter` is constructed when no adapter is passed.
* `HardwareFacts` = OS, CPU architecture, `logicalCpuCount`
  (`QThread::idealThreadCount()`, `-1` when undetectable), optional `physicalCpuCount`
  (macOS `sysctl hw.physicalcpu`), optional `systemRamBytes` (macOS `sysctl hw.memsize`,
  Linux `sysconf(_SC_PHYS_PAGES)*_SC_PAGESIZE`, Windows `GlobalMemoryStatusEx`), a GPU
  list and `availableBackends`.
* Linux GPU facts come from `/sys/class/drm/card*/device/vendor` and
  `mem_info_vram_total`; **Windows and macOS GPU inventory is not populated** by this
  adapter (no Metal/NVML/DXGI path exists).
* The service is **pull-only**: `facts()` returns a cached copy and only `refresh()`
  re-reads the adapter. There is no timer, no timestamp, no freshness and no staleness
  field anywhere in `HardwareFacts`.

Consumers:

* `ModelLibraryService` (`core/src/model/ModelLibrary.cpp:531-579`) derives
  `ModelFitInfo` (`ModelCompatibility::{Unknown,LikelyCompatible,ResourceConstrained,...}`)
  from artifact size, reported runtime reachability and `systemRamBytes`. Disk evidence
  uses `ModelResourceEvidence::{Reported,Estimated}`; a model larger than reported RAM is
  marked `ResourceConstrained`. This is a **static estimate at display time**, not a
  runtime policy input. `ModelLibraryService` owns a `HardwareCapabilityService` member
  constructed with the native adapter — **there is no adapter injection seam on the
  library service** (test limitation, see §7).
* `ModelStorageManager::canStore(bytes, reserveBytes = 64 MiB)`
  (`core/src/model/ModelStorageManager.cpp:100`) enforces free space, an explicit reserve
  and a stable root device before acquisition; `ModelOperationService` calls it before a
  pull (`core/src/model/ModelOperationService.cpp:672`).
* `RealToolExecutor` exposes one-shot `system_info` (OS/kernel/CPU/host/total RAM/root
  volume free) and `process_list` (`ps -eo pid,pcpu,comm` / `tasklist`) agent tools
  (`core/src/runtime/RealToolExecutor.cpp:1171,1215`). These are **model-invoked
  diagnostic tools with no sampling loop**, and `process_list` reads system-wide
  counters that are not Sentinel consumption.

### 1.2 Provider-side VRAM telemetry

`ModelOperationService::refreshOllamaTelemetry()`
(`core/src/model/ModelOperationService.cpp:1012`) issues HTTP `GET /api/version` and
`GET /api/ps` to the configured Ollama endpoint (5 s transfer timeout, 256 KiB body cap)
and stores `ModelRuntimeStatus{runtimeId, version, activeModelIds, activeModelId,
accelerationBackend, reportedActiveVramBytes}` where `reportedActiveVramBytes` is the sum
of `size_vram` across active models, or `nullopt` when the field is absent. The value is
**server-reported, server-owned residency**: it is not a Sentinel process measurement and
must not be attributed to the daemon. It is published to
`ModelLibraryService::setRuntimeStatus` and shown in library entries. LM Studio,
llama.cpp/compatible and cloud runtimes have **no equivalent telemetry path**.

### 1.3 Execution scheduling and authority (the only admission contract)

* `AgentExecutionScheduler` (`core/src/agent/AgentExecutionScheduler.cpp`) schedules tool
  invocations inside one agent step. Per-run concurrency is `qBound(1, maxParallelTools, 4)`;
  a shared `Budget` (`AgentExecutionScheduler.h:14`) caps active tools process-wide at
  `qBound(1, maxActive, 8)` and `AgentRuntime` creates it with a hard-coded `4`
  (`core/src/agent/AgentRuntime.cpp:649,1276`). Mutual exclusion comes from
  `ToolDescriptor.parallelSafe` and from overlapping `ResourceAuthorizationSnapshot`
  requests with at least one mutating access. **No hardware or pressure input exists.**
  Release of a shared grant does not wake other schedulers; there is no priority field.
* `ToolExecutionGateway` (`core/src/runtime/ToolExecutionGateway.cpp`) is the enforcement
  boundary: plan validation, argument schema validation, descriptor-snapshot equality,
  filesystem/process/network resource authorization, permission policy, approval status
  and sandbox policy are checked before a handler or executor runs. Budget availability
  cannot substitute for any of them (verified by test, §7).
* `RoutingMode` (`core/include/sentinel/core/model/ModelRouting.h:19-27`) already contains
  `Auto, Fast, Balanced, Quality, LocalOnly, CloudAllowed, BatterySaver`. These are
  **user-selected routing preferences** that gate local/cloud provider selection
  (`core/src/model/StaticModelRouter.cpp:17`, `core/src/app/StaticTaskPlanner.cpp:15-22`).
  They consume **no telemetry** and allocate **no budgets**. `Eco`/`Balanced`/`Performance`
  P4 profiles must not be silently mapped onto these names: `Balanced` already has a
  different, user-visible meaning.

### 1.4 Daemon, model runtime and background activity

| Component | Periodic work | Evidence |
| --- | --- | --- |
| `apps/sentinel-daemon/main.cpp:61` | 50 ms polled signal-safe shutdown flag | always running |
| `core/src/app/ApplicationController.cpp:993` | 5 s `pollOllama()` — also runs `checkDueAlarms()` and spawns a `QThread` health/discovery probe against configured local endpoints (loopback by default) | composed into the daemon |
| `apps/sentinel-daemon/service/DaemonService.cpp:211` | 60 s health timer → `refreshOllamaStatus()` → `refreshModelDiscovery()` | daemon only |
| `apps/sentinel-daemon/service/DaemonModelHelpers.cpp:32` | 15 min model catalog refresh; with the default `NetworkMode::Online` policy this issues outbound HTTPS catalog requests to Hugging Face/`ollama.com`/`lmstudio.ai` | daemon only |
| `apps/sentinel-daemon/service/LocalModelRuntime.cpp:87` | 3 s reconcile loop, only while the managed llama.cpp runtime is selected and automatic polling is enabled | daemon only |
| `apps/sentinel-daemon/service/DaemonIpcServer.cpp:458` | 25 ms non-blocking shutdown drain check | shutdown only |
| `apps/sentinel-daemon/service/LocalModelRuntime.cpp:321-332` | managed child launch with fixed arguments `--ctx-size 8192 --parallel 1 --jinja`, `unconfinedPermitted = true`, no context/parallel adaptation, no `--keep-alive` policy | model load |

Model **keep-warm, unload and eviction do not exist in any Sentinel component**. Sentinel
never sends `keep_alive` to Ollama; residency of shared servers is entirely the external
server's decision. `LocalModelRuntime` terminates its **own** llama-server child only on
model change/shutdown; `ProcessExecutor` terminates only opaque handles it started.

## 2. Monitoring capability matrix

| Signal | Implemented today | Where | Scope and correctness notes |
| --- | --- | --- | --- |
| CPU utilization (daemon) | No continuous sampler | — | Only one-shot `process_list` (`ps -eo pcpu`) and macOS `top` from this audit |
| CPU utilization (system) | No | — | — |
| Process RSS / PSS | No | — | No `proc_pid_rusage`, `/proc/*/smaps_rollup`, `GetProcessMemoryInfo` or `task_info` call exists anywhere in `core/`, `apps/`, `integrations/` or `plugins/` |
| System RAM total | Yes (static) | `HardwareCapabilityService` | Read once at construction; re-read only on `refresh()` |
| System available RAM / swap rate | No | — | — |
| Memory pressure | No | — | No `memory_pressure`/`kern.memorystatus`/`GlobalMemoryStatusEx` available-bytes use, no pressure callback |
| GPU inventory | Linux only | `HardwareCapabilityService.cpp:45-58` | DRM sysfs; macOS/Windows GPUs are invisible |
| GPU/VRAM usage | Ollama only, server-reported | `ModelOperationService::refreshOllamaTelemetry` | `size_vram` sum; `nullopt` when unreported; unified memory not conflated with footprint today |
| GPU utilization/temperature | No | — | — |
| Battery / AC state | No | — | Only static textual model-registry labels ("battery-friendly" size class, `core/src/model/ModelRegistry.cpp:654`) and the user routing mode `BatterySaver` |
| Thermal state | No | — | Advisory text only (`ModelRegistry.cpp:623`) |
| Disk free / capacity | Yes (point-in-time) | `ModelStorageManager::canStore`, `QStorageInfo` in `system_info` | Acquisition gate, not continuous pressure |
| Disk/IO pressure | No | — | — |
| Network usage accounting | No | — | Outbound catalog refresh is code-visible but unmeasured |
| Thread count / wakeups / energy | No | — | Not instrumented; macOS `energy` counters unused |
| Model process ownership handles | Yes | `ProcessExecutor`, `LocalModelRuntime` | Opaque PIDs, owned-only termination |
| Adaptive scheduling/policy | No | — | Fixed limits only (§1.3) |

Nothing in this matrix is stale-but-appearing-live: the only telemetry path is pull-based
and re-read only on explicit `refresh()`. That is also why **freshness/staleness cannot be
expressed at all today** (§4).

## 3. Verified benchmark measurements

All budgets in the acceptance matrix remain **Pending Calibration** (PD-D03/PD-D04), so
the numbers below are **baseline observations on one fixture host, not adopted
thresholds and not a performance pass**. No result in this section may be compared across
operating systems (Linux PSS, macOS footprint and Windows private bytes are different
units, per the measurement contract).

Method (adopted for this baseline and recorded with each case):

* Binary: `build/release/apps/sentinel-daemon/sentinel-daemon` (Release, no debug
  assertions), `--portable --socket <0700 dir>/s.sock`, freshly deleted portable profile
  (empty model library, no provider configured, no IPC client attached).
* Readiness: monotonic time from `exec` to the daemon's
  `Sentinel Daemon Service initialized successfully.` log line, polled at 10 ms.
* Sampling: `ps -o cputime=,rss=,vsz=,state=` at 1 Hz after a 120 s settle, per the
  acceptance matrix recipe; CPU seconds = delta of `cputime` across the 300 s window
  (0.01 s resolution); normalized % divides by 8 logical cores.
* Threads: `ps -M` line count; system context: `top -l 1 -n 0` idle % and
  `os.getloadavg()` before/after each window.
* Shutdown: SIGTERM → process exit, polled at 10 ms (cooperative drain path).
* Three repeats of the idle window plus one extended 1020 s window that crosses the
  15-minute catalog-refresh boundary; `lsof -nP -p <pid> -i` snapshot taken inside that
  boundary.

Results table (fill after measurement):

| Case | Startup (s) | Idle CPU (% of one core) | Idle CPU (normalized %) | RSS median / p95 / max (MiB) | Threads | Shutdown (s) | System idle % (start→end) | Load avg (start→end) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| PENDING | | | | | | | | |

Extended window (fill after measurement):

* CPU seconds across 1020 s, RSS behaviour, log activity and the `lsof` snapshot taken
  around the 15-minute catalog refresh: PENDING.

Measurement limitations (mandatory reading):

* One Apple Silicon host, one power source (AC), one OS. **No Linux/Windows equivalence
  claim is made or implied**, and no low-memory, high-memory, discrete-GPU or no-GPU
  profile was measured (ENV-HW-LOW / ENV-HW-HIGH / ENV-HW-UNKNOWN remain unavailable).
* macOS reports **physical footprint-class RSS**, not Linux PSS; shared pages are not
  separated, so the RSS figure is an upper bound on daemon-exclusive memory.
* The host was not idle: desktop applications, WindowServer, opencode and Ghostty were
  running. Per-case load averages and system idle percentages bound the contamination.
* Sampling at 1 Hz cannot resolve wakeups or per-sleep energy; no `energy` counters were
  collected (BUD-12 instrumentation gap, §8).
* No model was loaded, no provider was reachable and no inference ran, so BUD-05…BUD-08
  (cold/warm model startup, first token, throughput) are **unmeasured by design** in this
  task.

## 4. Unsupported telemetry

The following signals have **no implementation and no contract**; every P4 design must
treat them as `Unsupported`/`Unknown` and never as zero:

1. Per-process CPU and RSS/PSS for any process (daemon, desktop, model child, external
   server). There is no process-resource API usage in the codebase at all.
2. Memory pressure (available RAM, swap activity, native pressure notifications).
3. GPU on macOS (Metal unified memory) and Windows (DXGI/NVML); Linux VRAM is total-only,
   with no utilization.
4. Battery/AC state, power draw, energy counters.
5. Thermal state and throttling level.
6. Disk/IO pressure and network usage accounting.
7. Any freshness/staleness notion: `HardwareFacts` has no timestamp, so "stale reading"
   is currently indistinguishable from "fresh reading" (only §7's refresh-only test pins
   today's pull-only behaviour).
8. An unknown-encoding for `HardwareFacts.logicalCpuCount`: the struct default is `0`
   while the native adapter can produce `-1` (Qt's "undetectable"). One encoding must be
   chosen in P4-02 (see §6 and the test at §7).
9. Model keep-warm, unload, eviction and admission state machines; there is no explicit
   load/unload API (P3-01 already recorded this).
10. Any resource data for Desktop/Quick Panel, and any measurement of remote cloud
    request cost.

## 5. Resource ownership boundaries

Never attribute shared server memory to Sentinel; never terminate or unload another
application's model. The boundaries below extend the P3-01 lifecycle ownership matrix
with the resource-specific rules P4 must obey.

| # | Owner | What is measured as its resource | Authority today | P4 rule |
| --- | --- | --- | --- | --- |
| 1 | Sentinel daemon | The daemon process only: its own CPU seconds, its own RSS, its thread count, its timers | Process self-measurement (none exists yet) | The only process Sentinel may sample and (later) throttle without consent |
| 2 | Desktop and Quick Panel | Separate OS processes (`sentinel-desktop`, Rust CLI/TUI); they hold no `ApplicationController` | Presentation-only clients over typed IPC | Sample as separate processes; never fold desktop memory into the daemon budget; never make Desktop a second policy authority |
| 3 | Sentinel-managed model process | The llama-server child started by `LocalModelRuntime` through a private `ProcessExecutor`, identified by opaque handle | Owned: start, model-change termination, shutdown reaping | The only model process P4 may ever stop, and only through the existing owned-handle path; never via name/port matching |
| 4 | Externally managed Ollama / LM Studio / llama-server | Server-owned residency, e.g. Ollama `size_vram` from `/api/ps` | HTTP request/discovery/pull/delete only; no PID adoption, no shutdown | Read-only telemetry with `source = external`; a shared server's memory is never subtracted from the host budget as if it were Sentinel's, and is never released by Sentinel |
| 5 | Remote cloud requests | Remote compute; local cost is client CPU/memory of the request path only | Request/cancellation/credentials | Out of local resource scope; report request-level cost only, never "freed" local resources |

Additional boundary facts verified in this task:

* `ProcessExecutor` rejects unknown handles (existing coverage:
  `tests/core/test_model_lifecycle_ownership.cpp` `unknownProcessCannotBeTerminated`), and
  `LocalModelRuntime`'s fixture aborts the test if a terminate is attempted for anything
  other than its own child (`tests/desktop/test_local_model_runtime.cpp`).
* A shared/external listener suppresses competing managed startup and is never adopted
  (`sharedListenerAndLateShutdownNeverStartOrAdopt`).
* Agent cancellation is cooperative; a non-cooperative in-process dependency leaves the
  runtime `Stopping` with truthful ownership retention, not a fabricated kill
  ([DAEMON](../development/DAEMON.md), PD-D10 default: no hard-stop claim without
  isolation).

## 6. Recommended policy contracts (foundation only — not implemented)

These contracts are what P4-02 should add **on top of the existing seams**, without a
second scheduler and without touching provider routing, AgentRuntime structure or process
authority.

### 6.1 Telemetry value with capability, freshness and ownership

```cpp
// Recommended shape; P4-02 decision required. Do not add before calibration review.
enum class SampleState { Unknown, Unsupported, Stale, Fresh };

template <typename T>
struct ResourceSample {
    SampleState state = SampleState::Unknown;   // Unsupported = platform cannot provide
    std::optional<T> value;                     // never zero-fabricated
    qint64 capturedAtMonotonicMs = 0;
    QString sourceId;                           // e.g. "linux.smaps_rollup", "ollama.api.ps"
};
```

* `Unknown` (cannot sample now) and `Unsupported` (platform cannot ever sample) must stay
  distinct — the acceptance matrix requires missing metrics to be labelled unknown and
  platform gaps to be visible.
* `Stale` is derived from `capturedAtMonotonicMs` against a per-signal max-age decided at
  calibration; stale input degrades to the documented conservative fallback rather than
  to zero.

### 6.2 `ResourceSnapshot` (single read model, single owner)

```cpp
struct ResourceSnapshot {
    qint64 capturedAtMonotonicMs = 0;
    ResourceSample<double> hostCpuBusyFraction;      // normalized 0..1 of total capacity
    ResourceSample<qint64> hostRamTotalBytes;
    ResourceSample<qint64> hostRamAvailableBytes;
    ResourceSample<double> memoryPressureLevel;      // native indicator normalized
    QList<GpuResourceSample> gpus;                   // per device: vram total/used + state
    ResourceSample<QString> powerSource;             // "ac" | "battery" | unknown
    ResourceSample<double> batteryFraction;
    ResourceSample<QString> thermalLevel;            // native level string, not a number
    ProcessResourceSample daemon;                    // cpu seconds, rss bytes, threads
    QList<OwnedProcessResourceSample> ownedModels;   // ownership class 3 only
    QList<ExternalRuntimeResourceSample> externalRuntimes; // ownership class 4, read-only
};
```

Naming note: `ResourceSnapshot` is already used for **tool authorization**
(`core/include/sentinel/core/runtime/ToolInvocationPlan.h:61`,
`ResourceAuthorizationSnapshot`). The telemetry type must therefore be named differently
(e.g. `HostResourceSnapshot`) or placed in a distinct namespace; reusing the name would
conflate an authorization contract with a measurement contract.

### 6.3 Monitor adapters

* Extend, do not duplicate: `IHardwarePlatformAdapter` already exists for static facts.
  P4-02 should add a `ResourceMonitorAdapter` beside it (or a sampler inside
  `HardwareCapabilityService`) with per-signal capability flags, and must expose the
  injection seam at the service boundary so `ModelLibraryService`-style consumers can be
  tested with fakes (today only `HardwareCapabilityService` is injectable).
* Platform mapping (all `Unsupported` until written and calibrated):
  Linux — `/proc/self/stat`, `/proc/meminfo`, `/proc/pressure/*` (PSI),
  `/proc/<pid>/smaps_rollup` (RSS **and** PSS, never conflated);
  macOS — `task_info`/`proc_pid_rusage` for footprint, `host_statistics64`,
  `IOPSCopyPowerSourcesInfo`, thermal via `pmset`-equivalent APIs;
  Windows — `GetProcessMemoryInfo` (private bytes + working set), PDH CPU,
  `GlobalMemoryStatusEx` available bytes, `RegisterPowerSettingNotification`.

### 6.4 Profiles: Eco / Balanced / Performance

* A profile is a **named multiplier over calibrated per-device budgets** (PD-D04), not a
  universal RAM/CPU/GPU/thermal threshold. Device budgets stay configurable and are
  recorded per measured profile (PD-D03).
* Profiles adjust: tool-concurrency `Budget` limit, admission under pressure, model
  context/parallel request size for the **owned** runtime only, keep-warm window, and
  sampling rate. Profiles must **not** alter `RoutingMode`, permission, approval or
  sandbox decisions (AC-P4-03).
* `Balanced` as a P4 profile and `RoutingMode::Balanced` as a routing preference must be
  presented as distinct settings; do not silently reuse the stored routing key.

### 6.5 Memory-pressure response

* Ordered, monotonic escalation levels derived from `memoryPressureLevel` with
  hysteresis (escalate at threshold, de-escalate at a lower threshold) and a bounded
  cooldown so fluctuating pressure cannot trigger repeated load/unload oscillation
  (roadmap P4 acceptance criterion 4).
* Escalation stops at: refuse **new** admissions → reduce concurrency → propose (never
  force) context/model adjustment for owned runtimes → release **idle owned** residency
  only. It never drops an active binding, never cancels an active run, never touches
  external runtimes and never weakens a permission/sandbox/approval decision.
* `Unsupported`/`Unknown`/`Stale` pressure input maps to the documented conservative
  fallback (today's behaviour: current fixed limits), never to a fabricated "safe" value.

### 6.6 Foreground vs background scheduling

* The existing scheduler has no priority field and no cross-scheduler wakeup (verified in
  §7): a shared `Budget` release does not resume a waiting scheduler. P4-02 must add an
  explicit, bounded priority contract (foreground admission preempts **queued**
  background starts, never running work) plus a release notification, rather than a
  second scheduler.
* Cancellation keeps priority over every admission change (already true: `cancel()`
  releases grants and stops dispatch).

### 6.7 Model keep-warm policy

* Must be per ownership class: owned llama-server child (configurable idle window before
  owned termination, default derived from calibration), external Ollama/LM Studio
  (advisory only — Sentinel may set `keep_alive` on **its own** requests but must never
  command server residency), cloud (not applicable).
* No arbitrary preload defaults: a keep-warm model must be the user-selected model only.

## 7. Deterministic test results

Two new test files were added (test-only; no production code changed except
`tests/CMakeLists.txt` registration):

| Required case | Coverage in this task | Evidence |
| --- | --- | --- |
| Resource telemetry unavailable | New | `tests/core/test_resource_telemetry_contract.cpp` → see below |
| Stale readings | Contract gap (no freshness field); partial: re-read happens only on explicit `refresh()` | `refreshIsTheOnlyRereadPath` |
| Conflicting resource signals | Partial: conflicting **tool resource** requests serialize; **cross-telemetry** arbitration has no contract (gap §4.7/§6.1) | `conflictingMutationsSerializeWhileReadOnlyRunsShare` |
| Memory-pressure transitions | Contract gap (no pressure type, no level, no callback) | documented §4.2, §6.5 |
| Battery versus AC observations | Contract gap (no power telemetry; `RoutingMode::BatterySaver` is a routing preference) | documented §4.4, §1.3 |
| Foreground priority | Partial: shared-budget admission and cancellation release; **priority contract does not exist** (gap §6.6) | `sharedBudgetCapsConcurrentToolStarts`, `cancelRestoresSharedBudgetForTheNextRun` |
| No-model idle mode | Existing coverage (managed runtime stays idle without a GGUF, no substitution, no side effects) | `tests/desktop/test_local_model_runtime.cpp` (`emptyOrUnavailableSelectionNeverSubstitutesOrStops`, `emptySelectionChoosesFirstGguf`) plus the measured no-model idle baseline in §3 |
| Shared-provider ownership protection | Existing coverage | `tests/desktop/test_local_model_runtime.cpp` (`sharedListenerAndLateShutdownNeverStartOrAdopt`) |
| Active-task preservation | New (budget accounting) + existing (busy/owned-handle, cooperative cancel) | `cancelRestoresSharedBudgetForTheNextRun`; `modelSwitchRespectsBusyAndOwnedHandle`; `tests/core/test_process_executor.cpp` |
| No unauthorized model unloading | Existing coverage | `tests/core/test_model_lifecycle_ownership.cpp` (`unknownProcessCannotBeTerminated`); `LocalModelRuntime` fixture fails the test on any foreign terminate |

New test file 1 — `tests/core/test_resource_telemetry_contract.cpp` (registered as
`test_resource_telemetry_contract`, links `sentinel_core`):

* `unavailableTelemetryRemainsUnknown` — an adapter that reports nothing yields no RAM, no
  GPU and no physical-CPU value; zero is never fabricated; `refresh()` re-reads and still
  reports unknown.
* `gpuVramIsNeverSubstitutedForSystemRam` — a reported GPU VRAM allocation never becomes
  system RAM (AC-P4-01 non-conflation rule, Apple unified memory included).
* `refreshIsTheOnlyRereadPath` — repeated `facts()` reads never re-sample (no background
  sampler, hence no implicit staleness), and a changed adapter value appears only after
  `refresh()`.
* `nativeAdapterNeverReportsZeroCapacity` — on this host the native adapter reports
  positive RAM/VRAM when it reports anything, and never a `logicalCpuCount` of zero.

New test file 2 — `tests/core/test_execution_budget_policy.cpp` (registered as
`test_execution_budget_policy`, links `sentinel_core`):

* `sharedBudgetCapsConcurrentToolStarts` — a shared `Budget(1)` admits exactly one active
  tool even when the scheduler's own limit is 4; each release admits exactly one more; the
  grant count returns to zero at completion.
* `cancelRestoresSharedBudgetForTheNextRun` — cancelling a run releases every
  started-but-unfinished grant (no leak), forwards one cooperative cancel to in-flight
  work, does not report a fabricated completion, and the next run acquires the same budget
  immediately.
* `conflictingMutationsSerializeWhileReadOnlyRunsShare` — overlapping mutable
  authorization resources never run concurrently; overlapping read-only work shares the
  budget.
* `gatewayBlocksUnauthorizedResourcesBeforeExecution` — the execution boundary (registry
  path) refuses a missing or unauthorized `ResourceAuthorizationSnapshot` before any
  handler/executor runs, so no admission or budget change can substitute for
  authorization (AC-P4-03 precondition).

Notable, deliberately unpinned behaviours discovered while writing these tests (documented
here instead of asserted, so P4-02 can change them):

1. `AgentExecutionScheduler` performs **no authorization gate of its own**: dispatch of
   the first call in a batch only depends on dependency/conflict state; enforcement lives
   in `ToolExecutionGateway`. A resource policy must therefore be attached at/before the
   gateway, never as a bypass around it.
2. Releasing a shared `Budget` grant does not wake another waiting scheduler; there is no
   priority or fairness contract.

Validation of this task (commands and results):

* `cmake --preset tests && cmake --build --preset tests` — passed on the fixture host.
* `ctest --preset tests --output-on-failure` — PENDING (138 tests: 136 existing + 2 new).
* `clang-format --dry-run -Werror` on both new files — passed after formatting with the
  repository `.clang-format` (local clang-format 23.1.3; CI pins 18.1.8, so the Linux CI
  formatting diff remains authoritative).
* `python3 tools/analysis/check_personal_paths.py` — PENDING (privacy check).
* `python3 tools/ipc/generate.py --check` — PENDING (generated-contract check).
* `qmllint` — unchanged surface (no QML edited); not rerun for this scoped change.
* `clang-tidy` — PENDING limited note: macOS scoped analysis requires the
  `compile_database.py` helper; no production source changed in this task, so only the two
  new test files are candidates.
* No live model, no cloud request, no model download and no process outside this task's
  own children were touched during measurement.

## 8. P4 acceptance gaps

| Acceptance row | Requirement | Current state | Gap blocking acceptance |
| --- | --- | --- | --- |
| AC-P4-01 Telemetry truth | Sample CPU/RAM/GPU with capability/freshness and unknown fallback | Only static RAM facts, Linux-only GPU facts and Ollama `size_vram`; no capability flags, no freshness, no fallback logic | Platform sampler adapters (§6.3), freshness contract (§6.1), per-signal capability; BUD-12 monitoring-overhead instrumentation (wakeups/energy) |
| AC-P4-02 Adaptive policy | Pressure alters admissions/concurrency/context without changing authority/binding | Fixed `Budget(4)` and fixed llama context 8192/parallel 1; no pressure input, no hysteresis, no cooldown, no override surface | Pressure model + hysteresis (§6.5), profile budgets (§6.4), scheduler release notification and priority (§6.6), keep-warm contract (§6.7) |
| AC-P4-03 Adaptive security invariance | Adaptive tuning never weakens authorization/sandbox/protocol bounds | Boundary exists and is enforced (verified by test §7); no adaptive layer exists to violate it | Must be re-proven by replaying the bound/denial fixtures at every pressure tier once a policy exists |
| AC-P0-03 / PD-D03 | Concrete hardware profiles and environment versions | Single Apple Silicon fixture host; Linux/Windows and low/high/unknown-GPU profiles unconfirmed | Maintainer decision + native profile access |
| PD-D04 | Calibrated numeric budgets per profile | All budgets Pending Calibration; this task produced baseline samples only | Sample collection on declared hardware, then adoption of thresholds |
| PD-D10 | Provider ownership / hard-stop policy | Pending Decision; this task implemented no ownership change | Decision before any keep-warm/unload policy |
| P4 automated test requirement | Deterministic fake samplers across profiles, missing metrics, pressure/recovery/hysteresis, override limits | Fake-sampler seam exists only for static hardware facts; no pressure/hysteresis/override types exist | Contracts of §6.1/§6.4/§6.5 must exist before those tests can be written |
| P4 native/manual verification | Native low/high-memory and GPU profiles, sampling overhead, peaks | Not possible on this host and not attempted | Native hardware access (PD-D03) |

Explicitly out of scope for P4-01 and unchanged: INT-04/P2/P3 worktrees, provider
routing, AgentRuntime structure, permission/approval/sandbox policy, IPC protocol
(`tools/ipc/generate.py --check` passes unchanged), and any process termination
authority.

## 9. Recommended P4-02 implementation

Ordered so that each step is independently reviewable and reversible:

1. **Telemetry contract first.** Add the `SampleState`/`ResourceSample` value types and a
   `HostResourceSnapshot` read model in `sentinel_core` (new headers beside
   `HardwareCapabilityService.h`), plus `IResourceMonitorAdapter` with per-signal
   capability flags. Provide a fake adapter and a deterministic clock; keep
   `HardwareCapabilityService` as the static-facts owner and let the snapshot reference
   it rather than duplicating collection. Decide the single unknown encoding for
   `logicalCpuCount` and the `ResourceSnapshot` naming collision (§6.2).
2. **Native adapters, one platform at a time.** Start with macOS (fixture host):
   per-process CPU/RSS via `proc_pid_rusage`/`task_info`, system available RAM via
   `host_statistics64`, power via `IOPSCopyPowerSourcesInfo`, thermal as a native level
   string. Linux next (`/proc`, PSI, `smaps_rollup` with RSS **and** PSS). Windows after a
   PD-D03 environment decision. Each adapter ships with its capability matrix row filled
   and `Unsupported` where it cannot sample.
3. **Publishing, not policy.** Expose the snapshot through the existing daemon status/
   diagnostics surfaces (no new IPC command, no protocol change) and record sampling
   overhead (BUD-12: CPU-seconds, RSS delta, sample drops) to prove monitoring cost before
   any behaviour depends on it.
4. **Deterministic tests per adapter and per state** using the fake adapter: unavailable,
   unsupported, stale (now expressible), conflicting signals, monotonic timestamps;
   plus a regression test that the gateway/sandbox/permission fixtures still pass when a
   snapshot is present (AC-P4-03 precondition).
5. **Pressure model and hysteresis as pure functions** (no side effects): inputs are
   snapshots, outputs are a level plus a bounded cooldown. Unit-test escalation,
   de-escalation, oscillation damping and unknown/stale fallback before wiring anything.
6. **Bounded admission wiring** through the existing `AgentExecutionScheduler::Budget`
   (adjust the shared limit, add a release notification and an explicit foreground
   priority), never a second scheduler; cancellation keeps absolute priority.
7. **Calibration before thresholds** (PD-D03/PD-D04): measure low/high-memory and
   GPU-absent/present profiles on declared hardware, then adopt per-device budgets;
   `Eco`/`Balanced`/`Performance` remain named multipliers over those budgets and stay
   user-overridable and visible.
8. **Keep-warm and unload only after PD-D10**, only for the owned llama-server child, via
   the existing owned-handle termination path; external runtimes stay read-only.

## 10. Deliverable verification log

| Check | Command | Result |
| --- | --- | --- |
| Configure/build (tests preset) | `cmake --preset tests && cmake --build --preset tests` | PENDING |
| Full CTest | `ctest --preset tests --output-on-failure` | PENDING |
| Focused new tests | `ctest --test-dir build/tests -R 'test_resource_telemetry_contract|test_execution_budget_policy' --output-on-failure` | PENDING |
| Formatting | `clang-format --dry-run -Werror` on new files | passed |
| Generated contracts | `python3 tools/ipc/generate.py --check` | PENDING |
| Privacy | `python3 tools/analysis/check_personal_paths.py` | PENDING |
| Static analysis | `clang-tidy` (macOS scoped helper) on changed sources | PENDING |
| Git hygiene | `git status --short`; no commit/push/merge/reset/stash | 3 changed paths, uncommitted |
