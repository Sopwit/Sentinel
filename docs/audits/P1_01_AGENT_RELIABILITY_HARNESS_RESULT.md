# P1-01 — Agent reliability harness recovery result

Date: 2026-10-16 (Europe/Istanbul). Branch: `test/p1-01-agent-reliability`. Starting and current HEAD: `daef9b73f0ebbaf50c5eba35419a775a53ef4d22` (the existing WIP commit). This continuation is uncommitted. **The reported COR-12 failure and subsequent COR-13 crash were fixture defects. They are repaired without changing production logic. The harness reaches its later cases, but AC-P1-01 remains incomplete.**

## Scope and isolation

A dedicated managed worktree was created from the locally available `origin/test/p1-01-agent-reliability` ref and attached to its existing local branch. No fetch, commit, push, merge or PR was performed. Personal-Brain was unavailable on PATH; the supplied checkout/ref was verified rather than guessing another workspace. Root instructions, architecture/Agent Runtime/security/build/testing guidance, P0.1/P0.2 reports and acceptance matrix informed the inspection.

The original checkout and other worktrees were preserved. Exact continuation changes are [test_agent_reliability_harness.cpp](../../tests/core/test_agent_reliability_harness.cpp) and this new report. The test registration in [tests/CMakeLists.txt](../../tests/CMakeLists.txt) already existed in the WIP commit and was not changed here. Production AgentRuntime/AgentLoop/LlmAgentRuntime/gateway, Rust IPC/TUI, legacy CLI, dependencies, workflows and P1-04 documents were not edited. The branch inherits earlier P1-04 reports and a RealToolExecutor shadowing fix; this continuation neither changes nor claims authorship of those inherited changes.

## Reproduction and root cause

| Observation at starting HEAD | Evidence and diagnosis | Fixture correction |
| --- | --- | --- |
| COR-12 alone: read-file expected 1, actual 0; exit 1, no segfault | The handler remained sealed before the fresh task; src/calc.cpp was never seeded. The test could reach a context final without the intended read | Seed the file before baseline capture, reopen the handler only for the next run, reset planner index and emit the final representation expected by the existing evidence gate; retain exact read-count and unchanged-artifact assertions |
| COR-13 alone: EXC_BAD_ACCESS / null dereference | LLDB stack: pthread_mutex_lock → AgentRuntime::configureSession(`this = nullptr`) → RuntimeFixture::configure → cor13_cancelWhileAwaitingApproval. `configure` preceded `create` | Create/register runtime before configuration; guard helper preconditions. Use approval-required mode rather than autonomous auto-approval |
| Later cases were masked by the crash | Running COR-14–18 separately produced 6 passes and 6 failures (including setup/cleanup). COR-14 read/mutation, COR-15 grounded sync/async, COR-16 failing-receipt and COR-17 failed | Fix fixture prescriptions and canonical intent, preserve failing-tool/final gates, and stop presenting component checks as IPC coverage |
| Runtime lifetime debt in WIP fixtures | Test-wide holders retained AgentRuntime while stack-owned planner/handler/policy/executor dependencies had already died; this was a dangling-borrow risk, not the observed COR-13 stack | Remove standalone runtime/provider/planner holders. Local runtime destruction now fences workers before fixture dependencies die; assertion-return cleanup releases provider barriers |
| Host-specific COR-06 setup | “platform unsupported” marked Linux only and omitted required path args, so macOS tested argument rejection instead | Mark all platform entries Unsupported and provide valid path arguments, preserving the actual capability-negative oracle |
| COR-14/15/16 setup defects | Mutation/test commands were missing their exact fixture allowlists; canonical final was derived from a different/empty intent | Add the prescribed allowlist entries and use the same classified intent for canonical final recomputation; no production permission/final rule weakened |
| COR-17/18 coverage overstatement | Unsubscribe happened before execution and was not an IPC disconnect/attach. Terminal API rejection did not inject sequence/generation/foreign-run events | Explicitly skip these unimplemented corpus cases. Keep a separate, accurately named runtime terminal-command rejection test with valid reads and an independent unchanged-workspace check |

The initial crash is **not evidence of an AgentRuntime mutex defect**: the fixture called the runtime with a null pointer. No production defect was established by the repaired cases. Intermediate fixture-development failures (including child-release ownership and C++ field/capture errors) were corrected inside the test source; they are not discarded as production passes.

## COR-12 owned child and cancellation

The original semaphore-only simulated tool did not establish owned-process release. The corrected handler starts only this test executable's fixed `--cooperative-child` mode through existing ProcessExecutor, with a fixed artifact argument in a disposable workspace. It invokes no shell and accepts no model-supplied program/argv. The helper writes `build/partial.log`, signals readiness, then waits in its event loop. Cancellation terminates only that owned child and drains its terminal callback before the fixture cancellation callback returns. A bounded watchdog marks test failure; it is not a new latency threshold or a production hard-kill claim.

The oracle requires a positive child PID, confirmed child release, no drain timeout, one run-command invocation, exactly one cancelled terminal, no late approval, no queued write-file invocation, exact partial-artifact addition and a successful subsequent read on the same runtime. The queued write is explicitly authorized in the fixture yet must never dispatch after cancellation. Its target source and the partial artifact remain unchanged by the next run. Client cancellation and process release are treated as separate facts, rather than declaring a terminal event proof of release. The test logs the elapsed observation without scoring an uncalibrated timing budget.

## COR-01–COR-18 coverage

“Fixture pass” below describes executed assertions, not whole-criterion or real-model acceptance. **Partial** identifies a missing part of the matrix even when the existing fixture rows pass. Skipped means unimplemented test coverage, not an unsupported Sentinel product claim.

| Case | Executed rows | Verified behavior | Remaining scope / status |
| --- | --- | --- | --- |
| COR-01 | 5 | Sync/async scoped read final and insufficient evidence; hidden-scope negative; no workspace writes | Fixture pass for stated variants; broader truncation/absence variants remain |
| COR-02 | 4 | Sync/async planted single-file repair and fabricated receipt claim; independent exact content/hash/count oracle | Partial: build/test receipts are scripted; no independent compiler/test execution here |
| COR-03 | 1 | Header/source dependent edits, exact ordering and source contents, trusted fixture files untouched | Partial: no independent compilation/link/test oracle |
| COR-04 | 2 | Prescribed build/test receipt success versus deliberate failing receipt; bounded failure, no retry-until-green | Partial: no real subprocess exit verification for these scripted build/test commands |
| COR-05 | 4 | Unknown tool, missing field, wrong type, extra property; invalid invocation has no handler effect | Fixture pass; not a complete path-substitution/security campaign |
| COR-06 | 3 | Unsupported platform metadata, unavailable handler, disabled registry entry; no fallback effect | Partial: does not separately exercise every Git/browser/sandbox/vision/model failure |
| COR-07 | 1 | Required action denied, zero handler invocation, workspace unchanged | Partial: persistent-grant revoke variant not implemented |
| COR-08 | 2 | Loop and runtime allow-once, one invocation, no persistent grant, replay/foreign session rejection | Partial: daemon approval-ID contract is outside this fixture |
| COR-09 | 2 | Injected LLM provider fatal/transient failures, bounded recovery, no stale completed result/tool effect | Fixture pass for injected provider behavior; not HTTP/backend reliability |
| COR-10 | 1 | Classification barrier cancel, no planner/tool dispatch; a fresh loop succeeds | Partial: does not cover every planning-stage/same-runtime variant |
| COR-11 | 1 | Generation-entry barrier cancel, late returned provider result fenced; no side effects | Partial: not a streaming-after-first-delta case; frozen-binding/next-task variant not covered here |
| COR-12 | 1 | Actual owned helper PID/release, cancellation, no queued mutation, exact partial artifact and next task | Fixture pass for cooperative helper; no native timing or hostile-process guarantee |
| COR-13 | 1 | Approval wait then cancel, late allows rejected, no executor invocation or persistent grant | Fixture pass for runtime API; daemon approval-ID case remains separate |
| COR-14 | 2 | Repeated read recovery and deliberately stuck repeated mutation; bounded counts/effects | Fixture pass, synchronous variants only |
| COR-15 | 4 | Sync/async verified observation/operation/diff final versus missing-evidence final rejection | Partial: independent trusted compile/test oracle still absent |
| COR-16 | 2 | Fabricated test-success and no-observation claims fail without fabricated final | Partial: full mutated-resource-without-reread variant remains; hidden negative covered under COR-01 |
| COR-17 | 1 skipped | No IPC capability claimed from runtime unsubscribe | Unimplemented: real daemon disconnect/attach during an active Agent run with mutation-count/snapshot oracle |
| COR-18 | 1 skipped | No event-filtering capability claimed from command rejection | Unimplemented: delayed/duplicate/foreign-session/old-generation event injection through existing IPC/presentation fixtures |

Two additional fixture self/component checks verify the filesystem oracle rejects planted unauthorized effects and that a closed runtime rejects late/foreign commands while a subsequent real fixture read succeeds. These are not relabeled as COR-17/18.

## Independent verification and safety

WorkspaceVerifier reads fresh filesystem content/digests directly and compares exact changed/added/removed sets to a baseline captured before execution. Tool receipts cannot overwrite that oracle. Handler counters/order and exact prescribed write contents/command strings independently constrain dispatch. TerminalAudit counts typed completed/failed/cancelled events and requires one canonical terminal in runtime cases. Negative final/evidence and invalid-argument cases retain production gates and iteration limits.

All workspaces, files, provider responses and child output are synthetic/disposable. The only actual new tool process is the fixed owned helper, launched through ProcessExecutor; build/test receipt strings never become shell commands. No downloaded model, network provider, personal credential or destructive command was used. Tests deliberately use existing permissive fixture policies; they do not certify native sandbox enforcement, persistent-grant revocation, complete process-tree containment or real-provider reliability.

## Validation

Host: macOS 27.0.1 / Darwin arm64, AppleClang 21.0.0, Qt 6.12.0, CMake 4.4.4, clang-format/clang-tidy 23.1.3. This is one host's fixture evidence, not Linux/Windows/native model acceptance.

| Check | Actual result |
| --- | --- |
| Configure tests preset with ccache disabled | Passed, existing installed dependencies only |
| Full `CCACHE_DISABLE=1 cmake --build --preset tests -j6` | Passed |
| Direct final harness run with Qt text/XML outputs | **40 pass records, 0 failed, 2 skipped**, 1.047 seconds; pass records include setup/cleanup. Excluding those, **38 passing fixture cases**: 36 corpus rows plus 2 oracle/component checks; COR-17/18 are two separate skipped rows |
| `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | **128/128 targets passed**, 160.82 seconds. The harness target's passing exit does not erase its two skipped corpus rows |
| Added tests versus total targets | One additional runtime terminal-command component case; existing corpus fixtures repaired/strengthened. No new CTest target in this continuation; the harness target already existed in the WIP commit |
| COR-12 child release | Positive owned PID and terminal process receipt verified; queued write count 0; next read count 1. One direct-run observation was 51 ms from cancellation submission to observed released-child/cancelled state, with Qt polling granularity; not a calibrated latency gate |
| clang-format dry run / diff whitespace | Passed |
| Scoped clang-tidy | Passed with clang-analyzer-*, bugprone-use-after-move and bugprone-dangling-handle, warnings-as-errors; no whole-repository or ASan certification claimed |
| IPC and Desktop generated contract `--check` | Both passed; no protocol/Rust changes |
| Report links/fences and two-file change allowlist | Passed; original checkout clean, other worktrees preserved, nothing staged |
| Repository personal-path scan | **Failed: one inherited file**, docs/audits/P1_04_2_LINUX_CI_VALIDATION.md. Its bytes match HEAD (SHA-256 `f00a8bcd35c4ad5d47d2ef4db070f4f9f3e0b85d86f4bad7209db34f3d67809d`). It was not edited because P1-04 documents are excluded from this task |
| Same personal-path pattern on both changed files | Zero flagged files; no private path/credential content copied into this report |

Source SHA-256 for the final harness: `22bd30b09027359c90cbacee9c4cdf31895d9e5d2f29cc33ab702b93de46b0fa`. Harness executable SHA-256: `2067716571525dd1da0b1a67c60905a3fa9ba9810ae7e4f053a682220dec5529`. Raw LLDB/test outputs remain local disposable evidence because they contain generated machine paths; public-safe stack symbols, counts, candidate and hashes are recorded above. Original failures and skips are retained in this report instead of being relabeled as acceptance.

Relevant unchanged AgentRuntime, AgentLoop, LlmAgentRuntime, observation, gateway/process, daemon IPC and Desktop IPC targets passed within the full regression run. Their existing fixture coverage does not automatically close the new corpus entries. No live model, Cargo, native GUI or performance acceptance was executed.

## AC-P1-01 assessment and remaining work

**Not complete / not accepted.** Repaired assertions and a non-crashing test process do not satisfy “every deterministic case” while COR-17/18 are explicitly skipped and several corpus requirements remain partial. Acceptance matrix, iteration/security thresholds and AC-P0-02 adoption prerequisite are unchanged. No real-model P1 acceptance is claimed.

Recommended next task: finish the missing corpus oracles in this same test-only scope: connect COR-17 to existing daemon IPC fixtures, COR-18 to existing presentation event-identity fixtures, add independent trusted compile/test checks for COR-02/03/04/15, then complete the revoke, streaming cancellation and resource reread variants. Reuse existing interfaces and keep P1-04/P1-05 independent. Any later genuine production defect requires its own recorded diagnosis and separately scoped fix.


## P1-01.2 — Real IPC reliability fixtures (2026-10-16)

This section records a later test-only continuation at the same HEAD `daef9b73f0ebbaf50c5eba35419a775a53ef4d22`, on `test/p1-01-agent-reliability`. Earlier failed/skipped results and the earlier coverage table above are historical evidence and remain unchanged. The results below supersede only the two earlier unimplemented COR-17/18 rows; they do not retroactively convert those earlier skips into passes.

### Source, isolation and fixture architecture

The new [test_agent_reliability_ipc.cpp](../../tests/desktop/test_agent_reliability_ipc.cpp) target links the existing production DaemonIpcServer, ApplicationControllerBuilder, AgentRuntime/AgentLoop/gateway, DaemonClient and DesktopRuntimeClient libraries. It uses actual QLocalServer/QLocalSocket IPC, including normal handshakes, command correlation, explicit approval IDs, session attach snapshots, generation IDs and event sequences. The production daemon server is hosted in the fixture process; this is not an external daemon executable startup/restart test. No second daemon, runtime, provider registry or IPC contract is implemented.

Existing deterministic model/provider fixtures and an injected IOllamaRuntimeClient catalog keep the test offline. The planner pauses at a semaphore barrier after the prescribed write receipt, keeping the authoritative run active. A counting executor delegates the one initial workspace `glob` read to RealToolExecutor and permits exactly one prescribed synthetic write. All other dispatch is blocked and counted. The filesystem oracle independently hashes every workspace file and requires that only `allowed.txt` changes to the exact prescribed bytes; `unchanged.txt` must remain intact. Counts require one context read, one write and zero unauthorized invocations, including after terminal and approval replay.

A transparent socket relay in COR-18 forwards normal commands to the actual daemon and records its real frames. It withholds the real terminal frame and subsequent projection/session responses, preventing periodic snapshots from bypassing the deliberate event delay. Correlated daemon.status replies serve as wire-processing barriers. Injected events use the existing envelope/identity contract; no runtime-only subscription is substituted for client event handling. A separate observer connects directly to the daemon without the relay and independently checks authoritative snapshots and terminal events. Core eventHistory supplies a second terminal count oracle.

Each fixture uses a temporary socket/workspace, in-memory settings/credentials/memory and a unique test application profile. Cleanup releases the planner before controller worker teardown. No real provider, credential, model download/load/unload, shell command or unrelated workspace is used. The existing final-answer, permission, cancellation and iteration gates remain active.

### New executed corpus rows

| Case / row | Result | Independent evidence |
| --- | --- | --- |
| COR-17 disconnect / fresh-client reattach | Pass | Task starts via agent.start and explicit one-use approval; client disconnects after the write while planner is blocked. Direct observer sees the same running run. A fresh client attaches twice and recovers the same session/run/generation/model snapshot without replay. Releasing the barrier yields exactly one completed terminal on both subscribers and in runtime history; final attach remains completed; exact manifest and dispatch counts remain unchanged |
| COR-18 completed terminal | Pass | Real terminal delayed on the client wire; foreign session/run and old-generation high-sequence terminals rejected; duplicated old-sequence output ignored. Actual daemon terminal accepted, duplicate and conflicting terminal rejected, late run.started and approval.requested cannot reactivate the run. Client projection matches direct daemon snapshot; late IPC approval response returns invalid-approval; no persistent grant or extra dispatch |
| COR-18 cancelled terminal | Pass | Same injections against a run cancelled through actual daemon run.cancel after the authorized write; one cancelled terminal in direct wire observer and runtime history, stable cancelled presentation, no later completion overwrite, approval grant or extra mutation |

[tests/CMakeLists.txt](../../tests/CMakeLists.txt) registers one new CTest target, `test_agent_reliability_ipc`. The existing [Agent Reliability harness](../../tests/core/test_agent_reliability_harness.cpp) COR-17/18 slots invoke this exact fixture executable through ProcessExecutor with fixed test arguments; a nonzero child exit or child skip fails the parent case. COR-18 invokes both data rows. An explicit build-unavailable QSKIP remains for builds without the fixture definition, rather than manufacturing a pass. No skip occurred in the validated configuration. The separate terminal-command runtime component test retains its narrower scope.

### Development observations and production defects

Early fixture runs failed because the default temporary directory included a long application name, the controller's initial catalog refresh encountered a null runtime client, and the executor initially counted the legitimate context `glob` as a mutation. These fixture setup defects were corrected with a short temporary-directory template, an injected deterministic runtime catalog, and separate read/write counters. No production change was made to force a pass. The subsequent completed/cancelled wire fixtures passed without a production defect being established.

### Remaining coverage and acceptance

COR-17/18 now have the deterministic real-IPC evidence described above. They do not establish daemon process restart/crash recovery, cross-process launch behavior, native Windows named pipes, Linux/macOS native GUI behavior, hostile transport stress, or live model reliability. COR-18 tests completed/cancelled outcomes, not every failed-terminal variant.

The earlier COR-01–16 partial cases remain: broader truncation/absence coverage (COR-01), independent compiler/link/test subprocess oracles (COR-02/03/04/15), complete capability-failure variants (COR-06), persistent-grant revocation (COR-07), full planning-stage/same-runtime variants (COR-10), cancellation after a streaming delta and binding/next-task variants (COR-11), and mutated-resource-without-reread coverage (COR-16). COR-08/13 gain complementary real daemon late approval-ID rejection here, but their other corpus requirements are not reclassified automatically. Existing COR-12/13/14/15/16 repairs are preserved.

**AC-P1-01 remains incomplete / not accepted.** Zero skips in this run does not mean all required corpus variants or independent compile/test oracles exist. No threshold, roadmap decision or real-model phase acceptance changed. Recommended next task: add independent trusted compile/test subprocess oracles for COR-02/03/04/15 in a separately scoped test-only continuation, then complete revocation, streaming cancellation and reread variants.


### P1-01.2 validation and preservation evidence

The implementation under test is the existing [DaemonIpcServer](../../apps/sentinel-daemon/service/DaemonIpcServer.cpp), [ApplicationControllerBuilder](../../core/src/app/ApplicationControllerBuilder.cpp) composition and [DesktopRuntimeClient](../../apps/sentinel-desktop/src/DesktopRuntimeClient.cpp) event projection. No production source is changed.

| Check | Actual final result |
| --- | --- |
| CMake tests preset configure and full build | Passed on the macOS/Qt toolchain recorded above; no new dependencies installed |
| Focused real IPC target | **5 pass records, 0 failed, 0 skipped**, 2.050 seconds: setup/cleanup plus **3 scenario rows** (COR-17 and two COR-18 terminal variants) |
| Full Agent Reliability harness | **42 pass records, 0 failed, 0 skipped**, 3.603 seconds: setup/cleanup plus **40 actual test rows**, comprising 38 corpus rows and two oracle/component checks. The COR-18 parent row runs both child variants; child rows are not additional parent test counts |
| Full CTest regression after final assertions | **129/129 targets passed**, 164.23 seconds. Includes unchanged daemon IPC, Desktop IPC, runtime/loop/LLM, gateway/process and generated contract targets, plus the new real IPC target |
| Earlier full CTest before final event-delivery assertions | 129/129 passed, 127.90 seconds; retained separately from the final run |
| New coverage versus target totals | One new CTest target with three scenario rows; two existing skipped parent corpus slots now delegate to that target. No claim of 129 newly added tests |
| Event injection oracle | Every injected frame is independently counted on DaemonClient.eventReceived after transport schema validation. Duplicated frames must arrive exactly the injected number of times; malformed-frame rejection cannot manufacture a stable-presentation pass |
| Formatting and whitespace | clang-format dry run with warnings-as-errors and git diff --check passed |
| Scoped static analysis | clang-tidy clang-analyzer-*, bugprone-use-after-move and bugprone-dangling-handle passed on both changed C++ files; final IPC source rechecked after added delivery assertions |
| Generated IPC contracts | tools/ipc/generate.py --check and generate_desktop.py --check passed; generated files unchanged |
| Report validation | Relative links resolve, fenced blocks balanced, changed-file personal-path scan clean |
| Whole repository personal-path scanner | Failed only on the unchanged inherited P1_04_2_LINUX_CI_VALIDATION.md report, with the same SHA-256 recorded above. It was not edited |
| Workspace preservation | Original checkout remains clean at its original HEAD. COR-01–16 source prefix and the existing terminal component/source suffix match entry snapshots byte-for-byte; the entire prior report remains an unchanged byte prefix. Five other worktree file snapshots match. Concurrent changes were observed in the separate P2 provider worktree; this task did not write, reset or restore those files |
| Change allowlist / Git state | Exactly four documentation/test files changed: tests/CMakeLists.txt, tests/core/test_agent_reliability_harness.cpp, tests/desktop/test_agent_reliability_ipc.cpp and this report. Nothing staged, committed, pushed, merged, reset or stashed |

Final source SHA-256: harness `794f97a7810b47c1d88776ff88ea4824c1b6bb5757b5f7b0eb0d7406a5cd8c4d`; real IPC fixture `f21802ece8a0c75f0bd6e8e77f2da920536586f065de002457c951cd717fa40a`. Executable SHA-256: harness `8e2c76114f642cc0d91a71abdee9dda5ac2f4c24d7cb90a70326c22fa299198e`; real IPC fixture `5a639aba357bde9be7cf5f8005a4ce8fb1bff019dfbe9aed7b5f2b283792fe09`. Raw test/build logs remain disposable local evidence because they include generated host paths; only public-safe counts, identities and hashes are included here. No environmental compile/test failure remained; the inherited privacy-scan failure is reported separately.


## P1-01.3 — Independent compile and test oracles (2026-10-16)

This continuation uses the existing `test/p1-01-agent-reliability` worktree at HEAD `daef9b73f0ebbaf50c5eba35419a775a53ef4d22`. The entire preceding report, including historical failures/skips and P1-01.2 evidence, is preserved verbatim. The new results below supersede the previously missing independent compilation/link/test evidence only for the specified fixture variants. Personal-Brain remains unavailable on PATH; the user-supplied worktree, branch, HEAD, local instructions and existing dirty state were verified before editing.

### Implementation and authority

[IndependentCompileOracle.h](../../tests/support/IndependentCompileOracle.h) is a test-only helper using the existing [ProcessExecutor](../../core/src/runtime/ProcessExecutor.cpp). [tests/CMakeLists.txt](../../tests/CMakeLists.txt) supplies the configured compiler identity/path and Git executable. No provider registry, Agent Runtime, gateway, production final-answer rule or new runtime abstraction is introduced.

Every target fixture initializes a disposable Git repository with fixed `git init` arguments, an empty template and isolated global/system Git configuration. There are no commits, remotes or network operations. Source/header/test baselines are synthetic. The existing WorkspaceVerifier hashes all files, including trusted baseline files and repository metadata, before tool execution. Compiler objects, the trusted test driver and executables live in a separate disposable output directory; compilation/execution must not change the source repository manifest.

The independent command sequence is fixed: compile `src/calc.cpp`; separately compile a trusted driver; link those two objects; execute only that freshly linked test binary. Multi-file drivers include the prescribed `include/calc.h`, so header/definition mismatch is checked by the actual compiler/linker. Driver source is authored by the verifier outside Agent authority: it calls the repaired function, checks its numeric result and returns a real process exit code. The compiler path, argv, working directory and environment do not come from model/tool text. No shell parses model-generated command strings. Child environments contain only explicitly selected toolchain/system variables, locale, isolated Git configuration and `CCACHE_DISABLE=1`; credentials and arbitrary compiler/include override variables are not propagated. Ccache wrappers are bypassed for actual compiler execution in the final validation.

The final test verdict requires all of: an accepted nonempty AgentLoop final, exact authorized filesystem changes, unchanged source/header hashes bound to the verification result, successful drained compile/link/test processes with positive PIDs and no timeout, and the trusted driver's exact expected output. All four stages must exist in order. A missing test invocation, stale source after compilation, an altered trusted workspace file, misleading stdout or successful tool receipts cannot substitute for this evidence.

The original fixture `cmake --build build` / `ctest --test-dir build` tool receipts remain synthetic; these strings are never executed as shell commands. The independent oracle executes direct compiler/linker commands and a trusted binary instead. Negative fault-injection rows deliberately allow the existing loop's context final to report successful fixture receipts, then require the independent harness verdict to reject the result. This is a test acceptance oracle, not a new production final gate or a claim that production detects fabricated executor receipts. Existing grounded-final rejection and bounded failed-runtime variants are retained separately and cannot be overridden by a passing independent binary.

### Case coverage and exact added counts

| Corpus | Existing rows extended | New rows | Current focused rows | Evidence / negatives |
| --- | --- | --- | --- | --- |
| COR-02 | 4 sync/async repair and fabricated-final variants | 3 | 7 | Actual source compilation/link/run returns RESULT=5 for the prescribed repair. Wrong operator and fabricated-success receipts fail the trusted executable with exit 7; malformed repair fails compilation. Exactly src/calc.cpp may change; baseline tests and metadata remain unchanged |
| COR-03 | 1 dependent header/source repair | 3 | 4 | Actual driver includes the three-argument header and returns RESULT=6. A two-argument implementation compiles but fails linking to the declared three-argument function; malformed header fails compilation. A planted trusted-test change is rejected even when the binary passes |
| COR-04 | 2 successful/unfixable runtime variants | 6 | 8 | Real successful build/test, compilation failure, trusted-test failure, exit 23 despite ALL_TESTS_PASSED stdout, owned-child timeout, and misleading successful receipts. The original deliberately unfixable runtime remains Failed with real failing-test corroboration; independent fault-injection negatives remain separately identified |
| COR-15 | 4 sync/async grounded / receipt-only variants | 7 | 11 | Real compile/link/test evidence supplements existing evidence call IDs/scopes/final gates. Independent acceptance rejects wrong result, failed compilation/test, missing execution, trusted-file tampering and source changes after a successful compilation/test |
| Total | 11 | **19** | **30** | No new CTest target; existing harness extended |

The unauthorized-change negatives intentionally plant a test-owned fault after the allowed Agent mutations, rather than weakening tool authorization. The stale-source negative modifies the same authorized source after verification: the file allowlist still matches and the old executable passed, but the source hash binding rejects the stale evidence. The missing-test negative performs all three compiler/linker stages and deliberately omits execution; that cannot pass the final verdict.

### Real subprocess evidence and bounds

On this host the configured compiler is AppleClang `21.0.0.21000334`, reached through the existing CMake-configured c++ wrapper with ccache disabled. The focused 30 rows launch 30 isolated Git initializations plus **103 compiler/test processes**: 30 source compilations, 25 trusted-driver compilations, 25 links and 23 executable runs. These are subprocess counts, not Qt test or CTest target counts.

| Actual observed outcome | Count in focused run | Required interpretation |
| --- | --- | --- |
| Source compile exit 0 / exit 1 | 25 / 5 | Five expected malformed-source/header failures; not missing compilers |
| Trusted driver compile exit 0 | 25 | Verifier-owned driver compiled separately |
| Link exit 0 / exit 1 | 24 / 1 | One expected unresolved-signature failure |
| Executable exit 0 | 15 | Correct RESULT=5 or RESULT=6; some rows still reject runtime evidence, filesystem tampering or stale source |
| Executable exit 7 | 6 | Trusted arithmetic/assertion failures, including deliberately failing trusted-test expectations |
| Executable exit 23 | 1 | ALL_TESTS_PASSED stdout cannot override the nonzero status |
| Timed-out owned child | 1 | Positive PID, oracle-timeout-ready output, timedOut=true, Failed terminal and drained callback; observed exit 15 on this host is not a portable acceptance threshold |

Each compiler/link/test process has a 10-second execution bound plus a 5-second drain guard. The timeout case uses only this harness executable's fixed `--oracle-timeout-child` mode, which signals readiness and waits in its event loop; its execution deadline is one second. No arbitrary sleep or model-supplied executable is used. ProcessExecutor terminates/drains its owned child; fallback cleanup is scoped to that executor. Tests do not certify hostile descendant containment or native timing guarantees.

A missing configured compiler or unavailable disposable-repository Git executable produces an explicit environmental skip with a reason, never a passing verification result. Unsupported compiler-family argv likewise remains explicit. Compiler syntax errors, linker errors, assertion failures and nonzero/timeout outcomes are actual observed results, not silently converted into environment skips. No such environment skip occurred on this host; GNU/Clang/MSVC argv support is not cross-platform execution certification.

### Production findings and remaining acceptance

No production defect was established and no production code was modified. The fixture gap was that scripted receipts alone did not establish compilation, linking or test execution; this continuation adds an independent test verdict with actual subprocess evidence. Intermediate validation before the final ccache bypass also passed, but only the final cache-disabled results are used as current compiler-execution evidence.

**AC-P1-01 remains incomplete / not accepted.** The independent compile/test gap is closed for the 30 stated rows; it does not complete every required corpus variant or real-model P1 acceptance. Remaining gaps include broader COR-01 truncation/absence variants; complete COR-06 Git/browser/sandbox/vision/model capability failures; COR-07 persistent-grant revocation; COR-10 planning-stage/same-runtime variants; COR-11 cancellation after a streaming delta with binding/next-task verification; and COR-16 mutation-without-reread coverage. The P1-01.2 process-start/restart/native-platform limitations also remain. New independent fault-injection rejection must not be represented as new production runtime enforcement or live provider reliability.

Recommended next task: complete COR-07 persistent-grant revocation and COR-11 streaming cancellation/binding variants in a separate test-only continuation, retaining the existing runtime/approval/model-binding interfaces. Then complete COR-16 reread and the remaining capability/coverage variants before reassessing AC-P1-01.


### P1-01.3 final validation and preservation

| Check | Actual final result |
| --- | --- |
| Configure and complete CMake build | Passed with existing installed dependencies; final build uses CCACHE_DISABLE=1 |
| Focused COR-02/03/04/15 | **32 Qt pass records, 0 failed, 0 skipped**, 8.086 seconds; excludes two setup/cleanup records to give **30 actual rows** |
| Complete Agent Reliability harness | **61 Qt pass records, 0 failed, 0 skipped**, 10.789 seconds; **59 actual rows**, including 19 newly added rows and the preserved COR-12/13/17/18 cases |
| Existing real IPC target | **5 Qt pass records, 0 failed, 0 skipped**, 2.093 seconds; three scenario rows plus setup/cleanup |
| Complete CTest regression | **129/129 targets passed**, 128.64 seconds; no new CTest target in P1-01.3 |
| Earlier full regression before final child ccache bypass | 129/129 passed, 132.31 seconds; distinct earlier evidence, not the final compiler-execution run |
| Formatting / whitespace | clang-format dry run with warnings-as-errors and git diff --check passed |
| Scoped static analysis | clang-tidy clang-analyzer-*, bugprone-use-after-move and bugprone-dangling-handle passed on the harness translation unit including the new oracle helper; final cache-disable source rechecked |
| Generated contracts | Both IPC and Desktop generators --check passed; generated files unchanged |
| Changed-file privacy / report structure | Changed files contain no flagged personal paths; relative report links resolve and fenced blocks are balanced |
| Repository-wide privacy scan | One inherited failure: docs/audits/P1_04_2_LINUX_CI_VALIDATION.md, unchanged at the previously recorded SHA-256. No historical report was edited to conceal it |
| Preserved work | COR-05–14 source range (including COR-12/13 repairs), existing IPC delegation/terminal component and real IPC fixture match entry snapshots byte-for-byte. Entire report before P1-01.3 is an unchanged byte prefix |
| Other worktrees | Original checkout remains clean. Five other worktree file snapshots match; concurrent P2 provider worktree changes were observed and left alone. This task wrote only its existing isolated worktree and disposable local evidence |
| Git / change scope | Branch/HEAD unchanged; nothing staged, reset, stashed, committed, pushed or merged. This continuation changes tests/CMakeLists.txt, tests/core/test_agent_reliability_harness.cpp, the new tests/support/IndependentCompileOracle.h and this report. The earlier untracked real IPC fixture is preserved unchanged, so overall status retains five dirty/untracked paths |

Final source SHA-256: harness `e918ca1a88f287beac3b46f703e771367790c55c6489d9ded44d15786852e92d`; independent oracle `3b04ba4030e248e65167152d23f4bc99a8848b31b9d90c7be2536441bf050a5f`. Final harness executable SHA-256: `ad8d1c523830ad12f2cc198a26d966405e740c1fa01fbd260176601ddfb463d1`. Raw compiler diagnostics/build/test logs remain disposable local evidence because they include generated host paths. No compiler environment was missing on this host; all expected negative compiler/link/test/timeout results are assertions that passed, not hidden test failures or environmental skips.


## P1-01.4 — Grant Revocation & Streaming Cancellation

Date: 2026-10-16. Existing worktree/branch: `test/p1-01-agent-reliability`; starting and current HEAD: `daef9b73f0ebbaf50c5eba35419a775a53ef4d22`. This test-only continuation preserves all preceding report sections verbatim, including earlier failed/skipped results and P1-01.2/P1-01.3 evidence. HEAD, status and AGENTS.md were checked before editing; Personal-Brain remains unavailable on PATH.

**COR-07 persistent revocation is verified in the stated sync/async fixtures. COR-11 after-first-delta streaming remains blocked by the current production contract and is explicitly skipped.** Three additional passing COR-11 variants establish non-streaming cancellation, binding and subsequent-task behavior only; they are not relabeled as streaming evidence.

### Source architecture and test changes

Only [test_agent_reliability_harness.cpp](../../tests/core/test_agent_reliability_harness.cpp) and this report are changed in this continuation. RuntimeFixture accepts an optional existing IPermissionGrantStore dependency; its default construction remains unchanged for earlier cases. SQLitePermissionGrantStore is the actual production store, placed in disposable profile directories. No replacement permission service, grant cache, router, runtime or provider adapter is introduced.

Authoritative permission paths inspected: [AgentRuntime](../../core/src/agent/AgentRuntime.cpp) approve(session, true), revokePersistentPermission(id), clearSessionGrants and single-turn session lifecycle; [PermissionService](../../core/src/security/PermissionService.cpp) persistent grant matching/removal and transient denial precedence; [SQLitePermissionGrantStore](../../core/src/security/SQLitePermissionGrantStore.cpp) load/save/remove; and [ToolExecutionGateway](../../core/src/runtime/ToolExecutionGateway.cpp) authorization/resource/approval enforcement. Grant revocation removes the persistent permission and restores **Ask**, rather than inventing a permanent Deny. A subsequent protected operation is rejected through the existing explicit denial interface.

Generation fixtures use the actual AgentRuntime, LlmAgentRuntime and ModelService binding resolution with the existing deterministic catalog. A synthetic IChatProvider advertises streaming and exposes request/stream/delta/ack counters and deterministic barriers. Its streaming method is not manually called to manufacture coverage. All model selection changes occur on ModelService while the original planner binding remains frozen; rebinding for the subsequent task happens only after the original onFinished callback confirms the prior result has committed.

WorkspaceVerifier captures its independent manifest before execution. Exact handler counts, authorization queries, separately loaded SQLite rows, runtime state/history and provider token/request counters supplement that filesystem oracle. Profile databases remain outside the source workspace. No real user profile, credential, remote endpoint, model download or shell command is used.

### COR-07 — Persistent grant and revocation results

Two new data rows run the same sequence through synchronous submit/resume and asynchronous start/continueSession.

| Stage, per row | Actual evidence |
| --- | --- |
| Initial protected operation denied | AwaitingApproval then Failed; write count 0; workspace unchanged; stored grant count 0 |
| Explicit persistent authorization | Actual approve(session, true) creates exactly one SQLite grant with nonempty ID, canonical exact-file scope and FileSystem/Write authorization |
| Authorized operation | Accepted completed run; write count 1; only allowed.txt changes to exact prescribed bytes |
| Matching resource in another session of the same profile | Completes with the existing grant, write count 2, same single grant ID; profile-wide exact-resource persistence is intended behavior, not cross-session leakage |
| Different resource in the same profile | Requires approval, explicitly denied, Failed; write count remains 2; other.txt unchanged |
| Same target resource in a different disposable profile | Requires approval, explicitly denied; independent store/profile grant count 0 and executor write count 0 |
| Explicit revocation | revokePersistentPermission(grant.id) succeeds; runtime grant count 0; a separately opened production store reloads 0 rows |
| Previously authorized session caches | evaluateAuthorization returns Ask for both old authorized session IDs; late approve/continue cannot reopen the completed session or recreate a persistent grant |
| Subsequent protected operation after revocation | New session requires approval, explicitly denied and Failed; total write count remains exactly 2 |
| Independent final oracles | Exactly allowed.txt changed; other.txt unchanged; unauthorized handler count 0; fallback executor count 0. Every profile-A run and the profile-B denied run has one authoritative terminal |

This verifies one profile authority and separate profile stores. It does not establish invalidation across multiple concurrently loaded PermissionService owners of the same database, nor native profile migration behavior. No allow-once policy is presented as persistent revocation.

### COR-11 — Verified cancellation subset

Three new passing rows cover cooperative cancellation acknowledgment, a noncooperative late JSON final, and a noncooperative complete native write-file tool call returned after cancellation. In each row the production planner enters one actual **non-streaming** sendRequest with deadlineMs=120000 and remains blocked at a deterministic barrier. Cancellation sets the original token and leaves the runtime Cancelling with zero terminal events until the provider is released. The late reply then cannot produce a successful final, tool dispatch or mutation.

| Oracle, per passing row | Actual result |
| --- | --- |
| Original planner request / streaming requests / runtime deltas | 1 / 0 / 0; no first-delta claim |
| Provider cancellation acknowledgment | Cooperative row: 1 Cancelled lifecycle reply; noncooperative rows: 0 acknowledgments, late successful reply fenced |
| Cancelled run | One AgentCancelled terminal, zero AgentCompleted, empty finalAnswer, exact original session/turn identity |
| Tool and filesystem effects before the next task | read-file=0, write-file=0, unauthorized calls=0, fallback executor=0; baseline manifest unchanged |
| Frozen model identity | ModelService selection changes to next-model while planner/request/runtime events retain original-model and ollama provider identity |
| Next task on the same runtime | Different session/turn, new binding next-model, fresh token distinct from the cancelled token and not cancelled; exactly two planner requests, one authorized read, accepted completed final |
| Stale-result protection | Original history IDs/count and cancelled state remain frozen after the subsequent task; no late completion or write; workspace still unchanged |

Provider release here means the synthetic method returns and the runtime commits the fenced result. A client cancellation flag, a synthetic provider acknowledgment and real upstream GPU/server compute release are distinct facts. **No upstream server compute release is verified.** No callback is invoked after its synchronous provider-call lifetime merely to fabricate late-delta coverage.

### COR-11 streaming contract blocker — not fixed

Two new explicit skipped rows target JSON first-delta cancellation and native tool-call assembly. Before skipping, they independently verify the actual routing counters and complete cancellation/next-task cleanup. Both observe sendRequest=1, sendMessageStreaming=0, emitted delta count=0 and runtime ModelOutputDelta count=0 even with streaming capability enabled.

Source evidence in [LlmAgentRuntime::nextStep](../../core/src/agent/LlmAgentRuntime.cpp):

- The request sets options.deadlineMs=120000 unconditionally before dispatch.
- The streaming branch requires options.deadlineMs==0, so that branch cannot be selected for this planner request.
- The branch also requires !options.nativeToolCalling and !options.structuredOutput. Native tool-call assembly therefore has an additional routing limitation.
- Its callback sets outputEmitted but intentionally discards delta with Q_UNUSED(delta). The source comment protects private planner protocol; publishing raw planner JSON is not an acceptable fixture workaround.
- [IChatProvider](../../core/include/sentinel/core/interfaces/IChatProvider.h) exposes a text callback on sendMessageStreaming but supplies completed toolCalls on ChatProviderReply; it has no incremental tool-assembly callback contract. The bounded planner uses sendRequest instead.

The unreachable streaming dispatch is an observed production contract/reachability blocker for COR-11. Delta suppression is an intentional privacy boundary, not evidence that raw planner tokens should be exposed. No runtime/provider implementation, deadline, capability gate or final validation rule was changed to force a pass. These findings require a separately scoped contract decision/remediation before genuine after-first-delta and tool-assembly cancellation can be exercised. No grant leak or new permission/terminal production defect was established by the passing fixtures.

### Development failures retained

An initial test-source compile attempt used the wrong namespace qualification for the existing deterministic catalog fixture and was corrected. Another development run used tryAcquire inside QTRY, whose repeated condition evaluation consumed the semaphore; that owned test process was stopped, and the wait became an availability check followed by a separate acquire. Callback/semaphore lifetimes were ordered so runtime teardown fences workers before dependencies die.

A later focused development run recorded 4 Qt pass records and 5 failures: the next-task JSON used arguments instead of the existing planner protocol's args field, so no read handler ran. Exact invocation-count assertions caught that false completion. The fixture payload was corrected; no production parser or final gate was weakened. These development failures are separate from the final passing subset and the two genuine streaming skips.

### Remaining corpus gaps and assessment

COR-07's persistent grant/revoke variants now have real gateway/store evidence. COR-11's frozen binding, late full-result fencing and successful next-task gaps are covered for non-streaming requests, but its required **cancellation after an actual first streamed delta remains unverified/skipped**. Streaming tool-call assembly and upstream compute release are also unverified.

Other remaining cases retain their earlier limits: COR-01 broader truncation/absence variants; COR-06 complete Git/browser/sandbox/vision/model capability failures; COR-10 planning-stage/same-runtime cancellation variants; and COR-16 mutated-resource-without-reread final rejection. Existing real-IPC/native-platform and live-model limitations remain unchanged.

**AC-P1-01 is incomplete / not accepted. P1-01.4 is partial because COR-11 cannot be completed through the current contract within this test-only scope.** Recommended next task: separately diagnose/remediate bounded Agent streaming and safe public delta semantics while preserving request deadlines and private planner protocol; then rerun the explicit first-delta/assembly fixtures before closing COR-11. Continue COR-01/06/10/16 afterward.


### P1-01.4 final validation and preservation

| Check | Actual final result |
| --- | --- |
| Configure / complete CMake build | Tests preset configure and complete CCACHE_DISABLE=1 build passed with existing installed dependencies |
| Focused COR-07/11, including original cases | **9 Qt pass records, 0 failed, 2 skipped**, 1.486 seconds; excluding setup/cleanup: **7 passing test rows and 2 streaming skips** |
| Newly added rows | **7 data rows: 5 passing** (2 revocation, 3 non-streaming cancellation/binding) **and 2 explicitly skipped streaming/assembly rows**; no new CTest target |
| Complete Agent Reliability harness | **66 Qt pass records, 0 failed, 2 skipped**, 12.823 seconds; excluding setup/cleanup: **64 passing cases and 2 skipped cases** |
| Real daemon/Desktop IPC fixture target | **5 Qt pass records, 0 failed, 0 skipped**, 2.025 seconds; three real IPC scenario rows plus setup/cleanup |
| Full CTest regression | **129/129 targets passed**, 131.48 seconds. The harness target's passing exit does not erase its two required COR-11 skips |
| Formatting / whitespace | clang-format dry run with warnings-as-errors and git diff --check passed |
| Scoped static analysis | clang-tidy clang-analyzer-*, bugprone-use-after-move and bugprone-dangling-handle passed on the changed harness translation unit |
| Generated IPC/Desktop contracts | Both generator --check commands passed; generated contracts unchanged |
| Changed-file privacy and report checks | No flagged personal paths in changed files; report relative links resolve and fenced blocks are balanced |
| Full repository privacy scan | One inherited failure only: docs/audits/P1_04_2_LINUX_CI_VALIDATION.md, unchanged at SHA-256 f00a8bcd35c4ad5d47d2ef4db070f4f9f3e0b85d86f4bad7209db34f3d67809d. It was not edited |
| Preservation | Existing COR-12–16 source range, independent compiler helper/cases, IPC delegation/terminal component, real IPC source, test CMake registration and IndependentCompileOracle.h remain unchanged. Entire report preceding P1-01.4 is a byte-identical prefix |
| Other worktrees | Original checkout remains clean; five other worktree file snapshots match. Separate P2 provider worktree changes were observed concurrently and left alone |
| Git state / scope | Branch/HEAD unchanged, nothing staged/committed/pushed/merged/reset/stashed. This continuation edits exactly the harness and this report; five existing overall dirty/untracked paths are retained from earlier work |

Final harness source SHA-256: `89a44875f331d5e3a6d8776b467686dd0f8d7af445618c42a23b946c30053a85`. Executable SHA-256: `f7dc7222b9d8ef79e8d39641fb55c54b41ef74271e3fb8a6f31255b1f15f4848`. Raw local logs contain generated host paths and are retained as disposable evidence only. No environmental build/test failure remained; the inherited privacy finding and the source-backed streaming skips are distinct limitations. No live provider, upstream compute, native platform or full P1 acceptance is claimed.

## P1-01.5 — Bounded Planner Streaming Contract

Date: 2026-10-16 (Europe/Istanbul). Existing P1-01 worktree, branch `test/p1-01-agent-reliability`. The task brief expected HEAD `daef9b73f0ebbaf50c5eba35419a775a53ef4d22`; the worktree is at `ef7c18990ecc7139648fad64fe650478d522cf87`, whose direct parent is `daef9b73` (one additional committed P1-01 harness-extension commit, `test(agent): extend reliability harness with independent compile oracle and IPC fixture`). `daef9b73` is an ancestor, so the expected snapshot is intact. AGENTS.md, branch, HEAD, worktree list and dirty state were verified before editing. This continuation preserves every preceding report section verbatim (byte-identical prefix) and does not reset, stash, commit, push or merge.

**No production code was changed in P1-01.5.** The existing provider contract cannot carry the per-request deadline (or `maxOutputTokens`) into the streaming transport. Removing the contradictory routing guard would therefore drop the planner's `deadlineMs=120000` deadline onto the provider's own timeout, which is `0` (unbounded) by default, violating the "no unbounded requests" and "preserve actual deadline enforcement" constraints. Instead the exact missing contract and a narrowly scoped implementation proposal are recorded, the COR-11 after-first-delta case stays an explicit, asserted skip, and the streaming transport's real bounded/cancellation/terminal/privacy behaviour is now genuinely covered. This is deliberately not presented as end-to-end planner streaming.

### Confirmed source findings (reverified)

| Claim | Verified source evidence |
| --- | --- |
| Planner fixes a positive deadline | [LlmAgentRuntime::nextStep](../../core/src/agent/LlmAgentRuntime.cpp) sets `options.deadlineMs = 120000;` and `options.maxOutputTokens = 2048;` unconditionally before dispatch |
| Streaming branch requires a zero deadline | The same `send` lambda selects `sendMessageStreaming` only when `options.deadlineMs == 0 && attempt == 0 && options.images.isEmpty() && !options.structuredOutput && !options.nativeToolCalling && capabilities.streaming == Supported && streamObserver_` |
| Synthetic/real providers receive `sendRequest` | With `deadlineMs=120000` the guard is never satisfied; `sendRequest(request, options)` is used. The harness records `request=1, stream=0` even when the binding advertises `streaming=Supported` |
| Streaming method cannot carry options | [IChatProvider::sendMessageStreaming](../../core/include/sentinel/core/interfaces/IChatProvider.h) is `(const QString&, const std::function<void(const QString&)>&, const std::shared_ptr<std::atomic_bool>&)`; it has no `ChatRequestOptions` parameter, unlike `sendRequest(message, options)` |
| Streaming transport enforces a bound only when given a timeout | [OllamaChatProvider::sendMessageStreaming](../../core/src/chat/OllamaChatProvider.cpp) sets `request.options.timeoutMs = timeoutMs_` (the constructor value). The non-streaming `sendRequest` path instead passes `options.deadlineMs` through `sendMessageWithToken(..., deadlineMs, maxOutputTokens)`. [OllamaLocalInferenceStreamClient::startStream](../../core/src/runtime/LocalInference.cpp) resolves `request.options.timeoutMs > 0 ? … : config_.streamTimeoutMs` and calls [ProviderRequestRuntime::wait](../../core/src/runtime/ProviderRequestRuntime.cpp), which starts a deadline timer only `if (timeoutMs > 0)` |
| Default provider timeout is unbounded | [ModelService](../../core/src/model/ModelService.cpp) constructs `OllamaChatProvider(..., localInferenceTimeoutMs_)`; `localInferenceTimeoutMs_` defaults to `0` in [ModelService](../../core/include/sentinel/core/model/ModelService.h) and [ApplicationController](../../core/include/sentinel/core/app/ApplicationController.h). `wait(reply, 0, …)` therefore blocks until the reply finishes or the cancellation token flips |
| Streaming transport does support cancellation, terminal errors and accumulation | The stream client fences cancelled output, classifies `Cancelled`/`Timeout`/`MalformedResponse`, and accumulates `result.accumulatedText`; `ProviderRequestRuntime::wait` aborts on the 25 ms cancellation poll |
| Planner deltas are intentionally private | The inline streaming callback sets `outputEmitted = true` and `Q_UNUSED(delta)`. [AgentRuntime](../../core/src/agent/AgentRuntime.cpp) installs a `setStreamObserver` that would publish `ModelOutputDelta`, but the planner never invokes `streamObserver_`, so the public delta path is unreachable and no private planner protocol is published |
| Native tool calls lack an incremental-streaming contract | `sendMessageStreaming` exposes only a text callback; completed `ChatProviderReply::toolCalls` are returned at the end. `LlmAgentRuntime` routes native tool calls through `sendRequest` (the streaming branch excludes `nativeToolCalling`) |

### Reachability root cause

The unreachable path is a contract mismatch, not a missing feature in the transport:

1. The planner always requests a bounded generation (`deadlineMs=120000`).
2. The positive deadline is only consumable by `sendRequest`, because `ChatRequestOptions` reaches `OllamaChatProvider::sendRequest` and is forwarded to the non-streaming inference client as `timeoutMs`.
3. `sendMessageStreaming` accepts no `ChatRequestOptions`, so there is no way to pass `deadlineMs`; the streaming client falls back to the provider's constructor timeout, which is `0` (no timer) in the production wiring.
4. The historical guard `options.deadlineMs == 0` was therefore the only way to keep streaming from silently running unbounded. It is contradictory because the planner never sets a zero deadline, so the branch is dead.

A secondary, intentional boundary keeps raw planner JSON out of public events: even if streaming were selected, the planner callback discards `delta`, so nothing reaches `AgentRuntime`'s `ModelOutputDelta` publisher. That is a privacy requirement, not a defect.

### Whether safe production remediation was possible

**Not through the existing interfaces.** All ten requirements could only be met by extending the provider contract so the streaming call can carry the bounded request:

- Removing `options.deadlineMs == 0` alone (the literal "remove the contradictory requirement") would select `sendMessageStreaming` for a `120000` ms request while the transport enforces the provider's own `timeoutMs_`, which may be `0`. That drops the deadline and can produce an unbounded request — exactly what the constraints forbid.
- Enforcing the deadline from the runtime instead (a watchdog flipping the cancellation token) would conflate timeout with user cancellation, require a second token/composite polling loop, and duplicate `ProviderRequestRuntime` semantics. That is an unsafe workaround, so it was not implemented.
- Making the guard conditional on a real "can bound streaming" capability requires a provider-contract change owned by the separate P2 provider-contract track, which this task must not modify.

Because the transport itself (deadline timer, cancellation poll, terminal classification, accumulation) already supports the guarantees, the correction is small but belongs to the provider contract, not to P1-01.5 test/audit scope.

### Narrowly scoped implementation proposal

Preferred (reuses the existing deadline-honouring entry point):

1. Add `bool streaming = false;` to [ChatRequestOptions](../../core/include/sentinel/core/interfaces/IChatProvider.h), defaulting false so the struct stays backward compatible.
2. In `OllamaChatProvider::sendRequest` (and `SelectedEndpointChatProvider`), when `options.streaming` is set and the request is text-only (no `structuredOutput`, no `nativeToolCalling`, no images), invoke the existing stream client with `request.options.timeoutMs = options.deadlineMs > 0 ? options.deadlineMs : timeoutMs_` and `request.options.cancellationToken = options.cancellationToken`; keep deltas internal (no callback) so no private protocol can be published. The `IChatProvider::sendRequest` default must reject `streaming` it cannot honour.
3. In `LlmAgentRuntime::nextStep`, request `options.streaming = true` for text-only requests on a streaming-capable binding and delete the separate `sendMessageStreaming` branch and its dead `deadlineMs == 0` guard; keep `deadlineMs=120000`, the parser and the final-answer gates unchanged.
4. Require a focused provider-contract regression (bounded stream, cancel-after-first-delta, missing terminal, malformed chunk) before enabling the planner flag.

Alternative: an overload `sendMessageStreaming(message, onDelta, token, const ChatRequestOptions&)` plus a `supportsBoundedStreaming()` capability, routed only when the capability is true. This adds a second capability flag and a second dispatch path; the `streaming` option above is smaller and keeps one authoritative request contract.

### Native tool-call assembly limitation

`IChatProvider` exposes only `void(const QString&)` for streamed text and returns completed `ChatProviderReply::toolCalls` after the call. There is no incremental native tool-argument assembly event, and `LlmAgentRuntime` never streams native tool calls (`nativeToolCalling` excludes the stream branch). Retaining the explicit block/skip is correct: representing partial tool-argument fragments would require a new callback/event contract and risk executing a partially parsed call. No speculative incremental parser was added. The required contract decision is therefore: either keep native tool calls strictly non-streaming (current behaviour) or add a typed, fenced tool-assembly event to the provider contract and only dispatch a call after a validated terminal assembly.

### COR-11 regression evidence

Two cooperating fixtures are used. The transport row drives the **real** `OllamaChatProvider` against a disposable loopback `QTcpServer` that returns chunked NDJSON; every delta comes from HTTP bytes processed by the production provider callback (nothing is manufactured). The planner rows drive the production `LlmAgentRuntime`/`AgentRuntime` with the deterministic synthetic provider and assert the real routing decision and cancellation/binding semantics.

| Required coverage | Verifying case | Actual result |
| --- | --- | --- |
| First provider delta observed before cancellation | `cor11_boundedTransportAndPlannerPrivacy(cancel-after-first)` | The callback's first delta is recorded while `sendMessageStreaming` is still executing (`firstWhileActive`), then the token is set strictly after that delta |
| Cancellation while the stream remains active | same row | Token set inside the live callback; `lateChunkSent` proves a late chunk was written to the still-open stream after cancellation |
| Cancellation acknowledgment / fenced late result | same row | `actual.lifecycle == Cancelled`, `category == Cancelled`, `success == false`, `internalDeltas.size() == 1` (late chunk fenced by the provider callback) |
| No post-cancellation tool invocation | transport + replay | The cancelled transport result is replayed through the real parser/final gates; handler `read-file=0, write-file=0`, `executor.calls=0`, unauthorized calls 0, workspace untouched |
| No accepted successful final after cancellation | same replay | Run fails with empty `finalAnswer`; a late `write-file` JSON fragment never becomes a call |
| Exactly one cancelled terminal outcome | `cor11_cancelDuringGeneration` | `TerminalAudit` sees exactly one terminal, `cancelled == 1`; late approvals/duplicate cancels are rejected and the frozen history is unchanged |
| No stale content in the next task | `cor11_cancelledRequestBindingAndNextTask` | Next task completes with a fresh token; original session stays `Cancelled` with an empty final answer and an unchanged frozen event list |
| Original frozen model binding | same rows | Planner binding and every run event retain `ollama`/`original-*` even after `ModelService` selection changes; `originalDeadline == 120000`, `originalNative == native` |
| Successful subsequent task with a new binding | same rows | Next task binds `next-model`, completes with one authorized read, distinct non-cancelled token, one terminal |
| Bounded deadline expiration without cancellation | `…(deadline-after-first)` | Provider timeout expires after the first delta; `lifecycle == TimedOut`, `category == Timeout`, token never set |
| Malformed/incomplete streamed planner response | `…(malformed-ndjson)`, `…(missing-terminal)`, `…(incomplete-planner-json)` | Malformed chunk and missing terminal fail as `MalformedResponse`; an incomplete planner JSON that still completes is rejected by the real `decisionFromLlmOutput`/final gates (2 repair requests, `Failed`, empty final) |
| Provider callback genuinely invoked | all transport rows | `requests == 1`, the parsed request body has `model=synthetic-planner`, `stream=true`, `num_predict=2048`; `internalDeltas.first()` equals the exact sent bytes |
| No private planner tokens in public events/logs/diagnostics | all rows | Marker absent from `errorMessage`, `diagnostics`, every public event payload, `ModelOutputDelta` count 0, and captured `sentinel.agent.diagnostics` logs (which were demonstrably enabled) |
| Native tool-call assembly | `cor11_cancelAfterFirstStreamedDelta(native-tool-assembly)` | Explicit asserted skip; routing evidence recorded (native `sendRequest`, no stream, `originalNative == true`) |

Both `cor11_cancelAfterFirstStreamedDelta` rows still run their full assertion body — routing counters, frozen binding, one cancelled terminal, successful next task — and then `QSKIP` because the after-first-delta *streaming* portion is unreachable. The skip is evidence-backed, not a silent omission.

### Terminal, timeout and privacy results

- **Terminal**: the real transport returns exactly one lifecycle per call (`Completed`/`Cancelled`/`TimedOut`/`Failed`), and the runtime publishes exactly one `AgentCompleted`/`AgentCancelled`/`AgentFailed`. The `cor11_cancelDuringGeneration` wait was tightened to poll the authoritative terminal event rather than only the phase, because `commitResult()` commits the phase fractionally before `finishTurn()` publishes the terminal; this removed an observed intermittent harness race without changing production ordering.
- **Timeout**: bounded deadline expiration without cancellation is verified at the transport level (`deadline-after-first`). The planner-level deadline (`120000`) remains enforced through the reachable `sendRequest` path.
- **Privacy**: raw planner protocol never reaches public events, logs, user errors or diagnostics in any exercised path. This is not a live-model or blanket redaction certification; it covers the exercised planner/transport paths only.

### Validation

| Check | Actual result |
| --- | --- |
| Focused COR-11 (6 test functions) | **12 passing rows, 0 failed, 2 skipped**; the two skips are the unreachable streaming/assembly rows |
| Complete Agent Reliability harness | **72 passing records, 0 failed, 2 skipped** (~16.3–17.9 s); previously 66 passed/6 failed/2 skipped while the transport row was incomplete |
| Real daemon/Desktop IPC fixtures | **5 passing records, 0 failed, 0 skipped** (`QT_QPA_PLATFORM=offscreen`, as CTest sets). Direct runs without the offscreen platform intermittently show a client-projection timing flake in the unchanged fixture; not caused by this change |
| Complete CMake build + CTest | **129/129 targets passed**, 144.81 s (an earlier full run: 185.12 s) |
| Formatting | `clang-format --version 23.1.3`, `--dry-run --Werror` clean on the changed translation unit |
| Static analysis | `clang-tidy -p build/tests --checks='-*,clang-analyzer-*,bugprone-use-after-move,bugprone-dangling-handle'` clean, exit 0 |
| Generated IPC/Desktop contracts | `python3 tools/ipc/generate.py --check` and `tools/ipc/generate_desktop.py --check` both exit 0; generated contracts unchanged |
| Whitespace / privacy | `git diff --check` clean; `tools/analysis/check_personal_paths.py` flags only the inherited, unmodified `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md`. No external Desktop/IPC schema changed |

### Preservation and hashes

Only [test_agent_reliability_harness.cpp](../../tests/core/test_agent_reliability_harness.cpp) and this report are changed in this continuation. The COR-07 revocation rows, the independent compile oracles, the real IPC fixture, the COR-12–16 source range and `IndependentCompileOracle.h` are untouched. The separate P2 provider-contract worktree was inspected but left alone; no provider-contract, legacy-CLI, Rust-IPC or workflow file was modified. Branch/HEAD are unchanged; nothing was staged, committed, pushed, merged, reset or stashed.

- Harness source SHA-256: `3b7d23b7016eb841e61b0855781f96e1002b2264c8ee77d2d4dc2a409a8a9339`
- Harness executable SHA-256: `30d973abd5eab8e0949c42a8639aa218bdeefb3e1bc706031c8e9f32e64b600c`
- Inherited privacy finding SHA-256: `f00a8bcd35c4ad5d47d2ef4db070f4f9f3e0b85d86f4bad7209db34f3d67809d`

### Assessment

COR-11 is now split cleanly: the reachable non-streaming cancellation/binding/next-task subset is verified end-to-end, the streaming transport's bounded/cancellation/terminal/malformed/privacy behaviour is verified against the real provider over real HTTP bytes, and the end-to-end planner after-first-delta path remains an explicitly asserted, source-backed skip because the provider contract cannot carry the deadline. Nothing here weakens a cancellation, deadline or privacy gate.

**AC-P1-01 remains incomplete / not accepted.** Remaining gaps: COR-11 end-to-end planner streaming after the first delta and inline native tool-call assembly (both blocked on the narrow provider-contract change above); COR-01 broader truncation/absence variants; COR-06 complete Git/browser/sandbox/vision/model capability failures; COR-10 planning-stage/same-runtime variants; COR-16 mutated-resource-without-reread rejection; and the existing real-IPC/native-platform/live-model limits. No real model, cloud or upstream compute was contacted; no upstream compute release is claimed.

Recommended next task: implement the `ChatRequestOptions::streaming` provider-contract change in the dedicated provider-contract track with its own bounded/cancel/malformed regression, then re-enable the `cor11_cancelAfterFirstStreamedDelta` rows and remove the two skips; independently plan the native tool-assembly event contract before touching `native-tool-assembly`. Then continue COR-01/06/10/16.
