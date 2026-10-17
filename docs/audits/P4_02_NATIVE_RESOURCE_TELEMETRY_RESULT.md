# P4-02 Native Resource Telemetry Foundation Result

Audit date: 2026-10-17.
Phase: P4 — Resource Management.
Priority: High. Dependency: P4-01 Resource Management Baseline.
Result type: foundation implementation (contracts + native adapters + sampling
lifecycle + deterministic tests + measured overhead). **This task does not
implement adaptive resource policy and does not accept any P4 policy row.** No
Eco/Balanced/Performance thresholds, no admission change and no process
authority were introduced.

## 1. Exact source identity

| Item | Value |
| --- | --- |
| Repository | `https://github.com/sentinel-projects/sentinel` |
| Verified `origin/main` HEAD at start | `d9fbcd463620ab4e9aa543034dcd436580c332a0` (`Merge branch 'test/p5-02-daemon-ipc-reliability'`) |
| Branch | `feature/p4-02-native-resource-telemetry` |
| Worktree | `<worktree-root>/p4-02-native-resource-telemetry/Sentinel-sanitized` |
| Base | `origin/main` = `d9fbcd46` |
| Fixture host | Apple MacBook Air `Mac14,2`, Apple M2, 8 logical / 8 physical cores, 16 GiB RAM, macOS 27.0.1 (Darwin 26A434, arm64) |
| Toolchain | CMake 4.4.4 + Ninja, Apple clang 21.0.0, Homebrew Qt 6.12.0, ccache; Homebrew clang-format/clang-tidy 23.1.3 (CI pins 18.1.8) |
| Test build | `tests` preset (Debug) |
| Measurement build | `release` preset (`CMAKE_BUILD_TYPE=Release`), target `sentinel-daemon`, SHA-256 `f1ba193101f94a443be15b4e8cc023b0ccab52829f3632e07341ee0b71775764` |

Isolation rules observed: a new independent worktree branched from the latest
verified `origin/main`; the P4-01 worktree (branch `test/p4-01-resource-baseline`)
was read only and not modified; the CI-04
(`integration/ci-04-main-stabilization`) and WIN-01 (`audit/win-01-platform-failures`)
worktrees were inspected but not touched or duplicated. No commit, push, merge,
reset, stash, PR or hosted workflow dispatch was performed.

### 1.1 Changed files

New production sources and contracts:

* `core/include/sentinel/core/resource/ResourceTelemetry.h`
* `core/include/sentinel/core/resource/IResourceMonitorAdapter.h`
* `core/include/sentinel/core/resource/ResourceMonitor.h`
* `core/src/resource/ResourceTelemetry.cpp`
* `core/src/resource/ResourceMonitor.cpp`
* `core/src/resource/NativeResourceMonitorAdapter.cpp`

Modified:

* `core/CMakeLists.txt` (register new sources; link IOKit on Apple, psapi on Windows)
* `apps/sentinel-daemon/service/DaemonService.h` (opt-in monitor member)
* `apps/sentinel-daemon/service/DaemonService.cpp` (opt-in start/stop, default off)
* `tests/CMakeLists.txt` (register new tests)

New tests and audit:

* `tests/core/test_native_resource_telemetry.cpp`
* `tests/core/test_resource_monitor_sampling.cpp`
* `tests/core/resource_telemetry_test_support.h`
* `tests/benchmarks/bench_resource_telemetry.cpp`
* `docs/audits/P4_02_NATIVE_RESOURCE_TELEMETRY_RESULT.md` (this file)

No existing production behavior was changed; the daemon integration is inert
unless `SENTINEL_RESOURCE_TELEMETRY=1`.

## 2. Existing architecture review (Part A)

Findings reused, not duplicated:

* `HardwareCapabilityService` / `IHardwarePlatformAdapter`
  (`core/include/sentinel/core/model/HardwareCapabilityService.h`) remains the
  **static hardware facts** owner (OS, CPU, total RAM, GPU inventory). It is
  pull-only and has no timestamp. P4-02 does not modify it and does not depend on
  it at run time; live sampling is a separate contract.
* `AgentExecutionScheduler` / `Budget`
  (`core/include/sentinel/core/agent/AgentExecutionScheduler.h`) is the only
  execution admission contract. P4-02 does not touch it and adds no second
  scheduler.
* `ModelStorageManager::canStore` remains the acquisition storage gate. No disk
  telemetry was added.
* Daemon lifecycle (`apps/sentinel-daemon/service/DaemonService.cpp`) owns a 60 s
  health timer, model helpers and IPC. P4-02 adds an **opt-in** monitor member
  that is off unless `SENTINEL_RESOURCE_TELEMETRY` is set.
* The existing telemetry contract test
  `tests/core/test_resource_telemetry_contract.cpp` (P4-01) is unchanged; its
  documented gaps (no freshness, no capability flags) are now addressable by the
  new contracts.
* Platform abstraction convention (`IPlatformService`, `IPathProvider`, and the
  `#if defined(Q_OS_*)` style in `HardwareCapabilityService.cpp`) is followed: all
  native code is isolated in one platform-conditional source file.
* Event/status reporting (`AgentEvent`, daemon `daemon.status`) is unchanged; no
  new IPC command and no protocol change were introduced.
* Provider/model ownership is unchanged: no model server is started, polled or
  stopped by any telemetry code.

## 3. Telemetry contracts (Part B)

New headers under `core/include/sentinel/core/resource/`:

* `ResourceTelemetry.h`
  * `ResourceMetricAvailability`: `Unknown`, `Measured`, `Stale`, `Unavailable`,
    `Unsupported` — all five are distinct and none is encoded as zero.
  * `ResourceMeasurementOrigin` (`SentinelProcess`, `HostKernel`,
    `PlatformService`, `ExternalProvider`, `Synthetic`), `ResourceReliability`
    (`Authoritative`, `Approximate`, `Unreliable`), `ResourceScope`.
  * `ResourceSampleTimestamp` (monotonic ms + best-effort wall clock).
  * `ResourceMetric<T>`: optional value + availability + timestamp + origin +
    reliability + `sourceId` + `detail`.
  * Snapshot structures: `ProcessResourceMetrics`, `HostMemoryMetrics`,
    `PowerResourceMetrics`, `ThermalResourceMetrics`, `GpuResourceMetrics` and
    the read model `HostResourceSnapshot`.
  * `ResourceSignal` / `ResourceCapabilityReport` for static per-adapter support.
  * `IMonotonicClock` / `SteadyMonotonicClock` (injectable clock).
  * Pure helpers `deriveCpuFractions` and `applySnapshotFreshness`.
* `IResourceMonitorAdapter.h`: the adapter seam and
  `makeNativeResourceMonitorAdapter()`.
* `ResourceMonitor.h`: the sampling lifecycle.

Naming decision: the read model is `HostResourceSnapshot`, **not**
`ResourceSnapshot`, because `ResourceAuthorizationSnapshot`
(`runtime/ToolInvocationPlan.h`) already exists for tool authorization. A
measurement contract must never be conflated with a grant contract.

Metrics captured (values, units and scope fixed by type):

1. Process CPU — `process.cpuSecondsTotal` (cumulative seconds), plus derived
   `cpuFractionOfCore` and `cpuFractionOfHost` (0..1 of total host capacity).
2. Process resident memory — `process.residentBytes` in **bytes**.
3. Process virtual memory and thread count.
4. System available memory — `hostMemory.availableBytes` (reclaimable, bytes).
5. System memory pressure — `hostMemory.pressureLevel` normalized 0..1, marked
   `Approximate` where it is a mapping/proxy.
6. Battery presence, power source and charge — `power.*`.
7. Thermal state — `thermal.level` (native level string) and
   `thermal.temperatureCelsius` where a Celsius sensor is independently readable.
8. GPU memory / utilization — `gpus[]`, only when independently measurable.
9. Freshness — per-metric `ResourceSampleTimestamp` + `applySnapshotFreshness`.
10. Origin and reliability — per-metric `origin` / `reliability` / `sourceId`.

Explicit truth rules:

* Never substitute VRAM for system RAM (separate structures; test-pinned).
* Never represent unavailable metrics as zero (demo: absent battery → no value,
  not `0.0`).
* Host-wide and Sentinel-process metrics are separate nested structures.
* A "measured" metric with no timestamp degrades to `Unknown` rather than
  passing as current.

## 4. Platform adapters (Part C)

One platform-conditional file, `core/src/resource/NativeResourceMonitorAdapter.cpp`,
selected by `Q_OS_*`. All APIs are public, unprivileged and need no kext, private
API or privileged helper: no administrator rights are required.

| Signal | macOS (`macos.native`) | Linux (`linux.proc`) | Windows (`windows.native`) |
| --- | --- | --- | --- |
| Process CPU | `mach_task_basic_info` (user+system) — Measured | `/proc/self/stat` utime+stime — Measured | `GetProcessTimes` — Measured |
| Process resident | `mach_task_basic_info.resident_size` — Measured | `/proc/self/status` VmRSS — Measured | `GetProcessMemoryInfo` working set — Measured |
| Process virtual | `virtual_size` — Measured | VmSize — Measured | `GetProcessMemoryInfo` private bytes (Approximate) |
| Thread count | `task_threads` — Measured | `/proc/self/status` Threads — Measured | `Toolhelp32` thread enumeration — Measured |
| Host memory total | `hw.memsize` — Measured | `/proc/meminfo` MemTotal — Measured | `GlobalMemoryStatusEx` — Measured |
| Host memory available | `host_statistics64` free+inactive+speculative+purgeable (Approximate) | `MemAvailable` (Approximate) | `ullAvailPhys` — Measured |
| Memory pressure | `kern.memorystatus_vm_pressure_level` 1/2/4 → 0/.5/1 (Approximate) | PSI `/proc/pressure/memory` avg10 (Approximate) | `dwMemoryLoad` (Approximate) |
| Battery / power | IOKit `IOPSCopyPowerSourcesInfo` — Measured (absent → Unavailable) | `/sys/class/power_supply` — Measured (absent → Unavailable) | `GetSystemPowerStatus` — Measured (absent → Unavailable) |
| Thermal level | `IOPMGetThermalWarningLevel` → nominal/serious/critical (not published → Unavailable) | **Unsupported** | **Unsupported** |
| Thermal temperature | **Unsupported** | `/sys/class/thermal/thermal_zone*/temp` (max zone) — Measured | **Unsupported** |
| GPU memory | **Unsupported** | DRM `mem_info_vram_total/used` when present, else no entry | **Unsupported** |
| GPU utilization | **Unsupported** | DRM `gpu_busy_percent` when present (Approximate) | **Unsupported** |

No platform parity is fabricated. Where an API is absent or unverified the
adapter returns `Unavailable` or `Unsupported`; macOS/Windows GPU and macOS
Celsius temperature are explicitly `Unsupported`.

Runtime verification on the fixture host (macOS), observed during the test runs:
process CPU/RSS/thread count, host total/available memory and pressure via
`kern.memorystatus_vm_pressure_level` (measured value `1` = normal), power source
`AC` with battery present at 100%, and thermal level `nominal` were all measured
by the native adapter. GPU and Celsius temperature remained `Unsupported` as
designed.

## 5. Sampling architecture (Part D)

`ResourceMonitor` (`core/src/resource/ResourceMonitor.cpp`):

* **Opt-in.** Constructing does nothing; only `start()` creates the sampler
  thread.
* **Dedicated thread.** A single `std::thread` samples on an interval. It never
  runs on a model-execution thread and never starts or stops a model server.
* **No busy loop / no uncontrolled polling.** Execution is `sampleOnce()` then a
  `condition_variable::wait_for` with a stop predicate. The interval is clamped
  to `[250 ms, 60000 ms]`; non-positive requests fall back to the 2000 ms
  default.
* **Cancellation-safe.** `stop()` sets the stop flag, notifies, and joins;
  `~ResourceMonitor()` calls `stop()`. Repeated start/stop cycles are supported.
* **Bounded retention.** A `std::deque` history capped at `historyCapacity`
  (default 32) drops the oldest sample.
* **Thread-safe snapshots.** The whole sampling critical section and every
  `latestSnapshot()` / `history()` read serialize on one mutex; counters are
  atomic.
* **Configuration** via `ResourceMonitorConfig` (interval, metric max-age,
  history capacity). No adaptive threshold is defined.

CPU normalization: the adapter publishes a cumulative `cpuSecondsTotal`; the
monitor derives average fractions from the delta against the previous sample and
the monotonic interval, dividing by the host logical CPU count for the host
fraction. A non-positive interval, a missing baseline or a backwards counter
yields `Unknown` rather than a fabricated rate. Adapters that publish
pre-derived fractions leave `cpuSecondsTotal` unknown and are passed through.

Daemon integration is opt-in only: `DaemonService` creates and starts the
monitor when `SENTINEL_RESOURCE_TELEMETRY=1` (interval override
`SENTINEL_RESOURCE_TELEMETRY_INTERVAL_MS`). Default behavior is unchanged, so
there is no unintended daemon startup dependency. When enabled, the daemon logs
`Resource telemetry monitoring enabled (adapter=macos.native, interval=2000ms)`.

## 6. Security and ownership invariants (Part E)

* Telemetry is **read-only**: no code path terminates a process, unloads a model,
  changes a permission/approval/sandbox decision, or starts a server.
* `ToolExecutionGateway` authorization is untouched; the monitor is not part of,
  and cannot bypass, the execution boundary.
* Active Agent run ownership is unchanged; `sampleOnce()` cannot acquire, hold or
  release an `AgentExecutionScheduler::Budget` (test-pinned:
  `samplingDoesNotAlterExecutionBudget`).
* Managed versus Shared-Local process distinctions are untouched: the monitor
  owns no process handle and matches no process by name or port.
* Daemon IPC authority is unchanged: no IPC command or protocol change.
* Provider cancellation/deadline semantics are unchanged.
* `PD-D03`, `PD-D04`, `PD-D10` remain pending and are not pre-empted by any
  threshold in this task.

## 7. Deterministic test results (Part F)

Three new test files (test-only), registered in `tests/CMakeLists.txt`:

* `tests/core/test_native_resource_telemetry.cpp`
  (`test_native_resource_telemetry`, links `sentinel_core`)
* `tests/core/test_resource_monitor_sampling.cpp`
  (`test_resource_monitor_sampling`)
* `tests/benchmarks/bench_resource_telemetry.cpp` (`bench_resource_telemetry`)
* shared fixtures `tests/core/resource_telemetry_test_support.h`
  (`ScriptedResourceMonitorAdapter`, `ScriptedMonotonicClock`)

Required case coverage (all use injectable clock/sampler fixtures; no test reads
the host's actual battery or temperature):

| Required case | Test |
| --- | --- |
| Unknown metric values | `unknownMetricStaysUnknown` |
| Unsupported platform capabilities | `unsupportedPlatformCapabilityIsExplicit`, `nativeAdapterReportsSelfConsistentSnapshot` |
| Timestamp freshness | `freshnessKeepsRecentSampleMeasured` |
| Stale sample detection | `staleSampleIsDetectedAndValuePreserved`, `metricWithoutTimestampBecomesUnknown` |
| Correct CPU normalization | `cpuNormalizationIsCorrect`, `cpuNormalizationRejectsResetAndZeroInterval` |
| Resident memory units | `residentMemoryIsReportedInBytes` |
| Battery unavailable | `unavailableBatteryIsNotZero` |
| Thermal unavailable | `unsupportedStatePropagatesThroughMonitor`, `unsupportedPlatformCapabilityIsExplicit` |
| GPU metrics unavailable | `gpuVramIsNeverSubstitutedForSystemRam`, `unsupportedPlatformCapabilityIsExplicit` |
| Repeated start/stop | `repeatedStartStopIsSafe` |
| Cancellation | `cancellationStopsSamplerPromptly` |
| Sampling interval bounds | `samplingIntervalIsClamped`, `defaultConfigUsesSafeBounds` |
| Bounded history | `backgroundSamplingPopulatesBoundedHistory` |
| Concurrent snapshot reads | `concurrentSnapshotReadsAreSafe` |
| Safe shutdown | `destructorStopsRunningSampler` |
| No changes to Agent authorization | `samplingDoesNotAlterExecutionBudget` + existing `test_execution_budget_policy` |

Result: the four new suites plus the two P4-01 suites pass
(5/5 targeted: `test_resource_telemetry_contract`, `test_execution_budget_policy`,
`test_native_resource_telemetry`, `test_resource_monitor_sampling`,
`bench_resource_telemetry`). Full CTest: 141 of 142 pass; the single failure
`test_legacy_cli_exit_codes` is **pre-existing and unrelated** (see §9).

## 8. Performance validation (Part G)

Method mirrors P4-01: release `sentinel-daemon`, fresh portable profile, private
socket, AC power, no model loaded, 30 s settle then a 120 s window sampled at
1 Hz with `ps -o cputime=,rss=,vsz=,state=`; CPU seconds from the `cputime`
delta; normalized % divides by 8 logical cores. Four cases run in order
baseline → monitoring → baseline → monitoring. Raw evidence (samples.csv,
metrics.json, daemon.log per case, all_results.json) is preserved under
`<TEMP>/p4-02-measure/`.

| Case | Monitoring | Startup (s) | Threads | Idle CPU (% one core) | Normalized % | RSS median / p95 / max (MiB) | Shutdown (s) | Load avg 1m (start→end) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| baseline_1 | off | 0.149 | 2 | 0.277 | 0.0346 | 48.312 / 48.328 / 48.328 | 0.097 | 8.72 → 7.88 |
| baseline_2 | off | 0.187 | 2 | 0.335 | 0.0419 | 48.625 / 48.641 / 48.641 | 0.079 | 8.94 → 5.67 |
| monitoring_1 | on (2000 ms) | 0.196 | 3 | 0.353 | 0.0441 | 34.875 / 34.906 / 48.641¹ | 0.105 | 9.38 → 4.90 |
| monitoring_2 | on (2000 ms) | 0.190 | 4 | 0.369 | 0.0461 | 48.641 / 48.656 / 48.672 | 0.118 | 8.81 → 5.18 |

¹ monitoring_1's reported max is its pre-window first sample; during the window
RSS held at 34.8–34.9 MiB (host memory reclaim of an idle process under load),
not a footprint reduction. The paired monitoring_2 case matched baseline.

Derived overhead (monitoring minus baseline, paired):

| Metric | Baseline (mean) | Monitoring (mean) | Delta |
| --- | --- | --- | --- |
| Idle CPU (% of one core) | 0.306 | 0.361 | **+0.055 pp** |
| Idle CPU (normalized) | 0.0383 | 0.0451 | **+0.0068 pp of total capacity** |
| RSS median (MiB) | 48.47 | 48.64² | **no material change (≤ 0.3 MiB)** |
| Threads | 2 | 3–4 | **+1 sampler thread (+1 transient)** |
| Startup (s) | 0.168 | 0.193 | within noise |
| Shutdown (s) | 0.088 | 0.112 | +0.02 s (thread join) |

² using monitoring_2 only; monitoring_1 is the reclaim outlier.

Sampling cost (from `bench_resource_telemetry`, Debug build): native per-sample
median `151.38 µs`, p95 `190.21 µs`; at a 2000 ms interval the duty cycle is
≈ 0.0076%. Background interval/jitter with a 500 ms request: median `503 ms`,
max `506 ms` (no missed or bunched samples).

Comparison with P4-01 (reference observations, not acceptance thresholds): P4-01
reported ~48.9 MiB idle RSS and 0.23–0.32% single-core CPU with no model loaded.
This task's baseline is 48.3–48.6 MiB and 0.277–0.335% single-core, i.e. the
same magnitude on the same host class. The small upward shift is consistent with
a busier host (1-minute load 4.9–9.4 here vs 1.9–3.9 in the P4-01 windows) and
is not attributable to monitoring in the baseline cases.

Measurement uncertainty (mandatory reading):

* One Apple Silicon host, one power source (AC), one OS. No Linux/Windows
  equivalence claim; no low/high-memory or discrete-GPU profile was measured.
* macOS reports footprint-class RSS, not Linux PSS; shared pages are not
  separated, so RSS is an upper bound on daemon-exclusive memory and can be
  reclaimed downward (see monitoring_1).
* The host was **not idle**: WindowServer, Ghostty, WebKit and other desktop
  processes were active (1-minute load 4.9–9.4). Both cases were measured under
  similar load, bounding but not eliminating contamination.
* 1 Hz sampling cannot resolve wakeups, thread wakeups or per-sleep energy; no
  `energy` counters were collected. **No power-consumption claim is made or
  implied from CPU usage alone.**
* The CPU delta is ±0.01 s per 120 s window (≈ ±0.008 pp of one core), which is
  the same order as the measured monitoring delta; the direction is consistent
  in both pairs but the absolute value is small relative to resolution.

No model was loaded and no provider was reachable, so model-startup and
inference costs were not measured.

## 9. Validation log

| Check | Command | Result |
| --- | --- | --- |
| Configure | `cmake --preset tests` | passed |
| Full build | `cmake --build --preset tests` | passed (0 errors) |
| Full CTest | `ctest --preset tests --output-on-failure` | 141 / 142 passed in 229.28 s; 1 pre-existing failure (below) |
| Focused new + P4-01 tests | `ctest --test-dir build/tests -R 'test_resource_telemetry_contract\|test_execution_budget_policy\|test_native_resource_telemetry\|test_resource_monitor_sampling\|bench_resource_telemetry'` | 5 / 5 passed in 7.05 s |
| Release daemon build | `cmake --preset release && cmake --build --preset release --target sentinel-daemon` | passed; SHA-256 `f1ba1931…775764` |
| Formatting | `clang-format --dry-run -Werror` on all changed C++ files | 0 violations |
| Static analysis | `clang-tidy -p <db> core/src/resource/*.cpp` | 0 warnings on the three new production sources |
| Static analysis (daemon) | `clang-tidy -p <db> apps/sentinel-daemon/service/DaemonService.cpp` | 1 warning, on **pre-existing** unmodified line 208 (`openSessionProjectionStore`); all added lines clean |
| Generated IPC contracts | `python3 tools/ipc/generate.py --check` | exit 0 (unchanged) |
| Privacy scan | `python3 tools/analysis/check_personal_paths.py` | 0 files flagged (after path redaction in this document) |
| Git hygiene | `git diff --check` | clean |
| Rust format | `cargo fmt --check` (in `cli/`) | passed |
| Rust lint/tests | `cargo clippy --all-targets -- -D warnings`; `cargo test` (in `cli/`) | passed |

Pre-existing failure, not caused by this task: `test_legacy_cli_exit_codes`
(#118) fails in `initTestCase()` with
`settings file is not rooted at XDG_CONFIG_HOME`. The file is byte-identical to
`origin/main` (`git diff origin/main -- tests/integration/test_legacy_cli_exit_codes.cpp`
is empty); the failure is the known macOS CoreFoundation/XDG isolation gap that
the CI-04 (`integration/ci-04-main-stabilization`) / P1-05.1 remediation owns.
Per the task's coordination rule, that work was **not** duplicated or modified
here.

Rust gates result and evidence path are recorded in
`<TEMP>/p4-02-clippy.log` and `<TEMP>/p4-02-cargo-test.log`.

## 10. Platform coverage matrix (summary)

| Platform | Implemented here | Verified in this task | Missing / Unsupported |
| --- | --- | --- | --- |
| macOS (arm64) | process CPU/RSS/vm/threads, host total/available/pressure, power, thermal level | **Yes** (fixture host, executed) | GPU, Celsius temperature |
| Linux | process + host memory via `/proc`, PSI, power supply, thermal zones, DRM GPU where present | No (compiled only; no Linux host) | native thermal *level*; GPU on non-sysfs drivers |
| Windows | process CPU/RSS/vm/threads, host memory/pressure, power | No (compiled only; no Windows host) | thermal, GPU |

macOS is the only platform whose native adapter was executed in this task. Linux
and Windows native code paths are written but **not executed here** and require
the P4-02 native/manual verification step (PD-D03) before any policy consumes
them.

## 11. Pending product decisions

* **PD-D03** — concrete hardware profiles and environment versions (single Apple
  Silicon fixture host only; Linux/Windows and low/high/unknown-GPU profiles
  unconfirmed).
* **PD-D04** — calibrated numeric budgets per profile (all budgets remain Pending
  Calibration; the sampling interval bounds here are guards, not policy).
* **PD-D10** — provider ownership / hard-stop policy (unchanged; the monitor
  terminates nothing).
* `HardwareFacts.logicalCpuCount` unknown encoding: the P4-01 gap is not changed
  here; the new contracts carry an explicit `int hostLogicalCpuCount` where
  `<= 0` means unknown, but `HardwareCapabilityService` itself is untouched.

Remaining acceptance blockers for the P4 phase: no adaptive policy, no calibrated
thresholds, and no native verification of the Linux/Windows adapters.

## 12. Recommended P4-03 adaptive policy design

1. Consume `HostResourceSnapshot` only through a pure pressure model (inputs are
   snapshots, outputs are a level + bounded cooldown) with explicit handling for
   `Unknown`/`Unavailable`/`Unsupported`/`Stale` that degrades to today's fixed
   limits, never to a fabricated "safe" value.
2. Add hysteresis and a bounded cooldown before any behavior depends on pressure
   (escalate at threshold, de-escalate lower, damp oscillation).
3. Adjust only through the existing `AgentExecutionScheduler::Budget` (add a
   release notification and a foreground priority contract), never a second
   scheduler; cancellation keeps absolute priority.
4. Keep profiles as named multipliers over calibrated per-device budgets, clearly
   distinct from `RoutingMode::Balanced`.
5. Gate every escalation at the existing `ToolExecutionGateway` boundary; a
   pressure level must never weaken authorization, approval or sandbox policy.
6. Do not gate any model lifecycle change until PD-D10 is decided; then only the
   owned llama-server child via the existing owned-handle path.
7. Calibrate on declared hardware (PD-D03/PD-D04) before adopting thresholds;
   measure wakeups/energy and low/high-memory profiles.

Adaptive optimization is **not** complete: this task delivers truthful telemetry
and its measured cost, not a policy.
